// The Connected Sprawl - Real-human visual roster for the ambient city crowd.

#include "Characters/SprawlCrowdMetaHuman.h"

#include "Animation/AnimSequence.h"
#include "Characters/SprawlCharacterRender.h"
#include "Characters/SprawlHumanCharacterModule.h"
#include "Components/ChildActorComponent.h"
#include "Components/LODSyncComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

namespace
{
constexpr TCHAR MetaHumanIdlePath[] =
	TEXT("/MetaHumanCharacter/Optional/Animation/UEFNAnimPreset/Locomotion/")
	TEXT("AS_MH_Neutral_Stand_Idle_Loop.AS_MH_Neutral_Stand_Idle_Loop");
constexpr TCHAR MetaHumanWalkPath[] =
	TEXT("/MetaHumanCharacter/Optional/Animation/UEFNAnimPreset/Locomotion/")
	TEXT("AS_MH_Neutral_Walk_Loop_F.AS_MH_Neutral_Walk_Loop_F");
constexpr TCHAR MetaHumanRunPath[] =
	TEXT("/MetaHumanCharacter/Optional/Animation/UEFNAnimPreset/Locomotion/")
	TEXT("AS_MH_Neutral_Run_Loop_F.AS_MH_Neutral_Run_Loop_F");

bool IsPresentationMatch(
	ESprawlHumanPresentation Requested, ESprawlHumanPresentation Candidate)
{
	return Requested == ESprawlHumanPresentation::Androgynous
		|| Candidate == Requested;
}

const FSprawlCrowdMetaHumanEntry* AddCandidate(
	TArray<const FSprawlCrowdMetaHumanEntry*>& Candidates,
	const FSprawlCrowdMetaHumanEntry* Entry)
{
	if (Entry && !Candidates.Contains(Entry))
	{
		Candidates.Add(Entry);
	}
	return Entry;
}
}

const TArray<FSprawlCrowdMetaHumanEntry>& FSprawlCrowdMetaHuman::Roster()
{
	static const TArray<FSprawlCrowdMetaHumanEntry> Entries = {
		{
			TEXT("Zarri"),
			FSoftObjectPath(TEXT("/Game/MetaHumans/Zarri/BP_Zarri.BP_Zarri_C")),
			FSoftObjectPath(TEXT(
				"/Game/MetaHumans/Zarri/Face/"
				"SKM_MHC_Zarri_FaceMesh.SKM_MHC_Zarri_FaceMesh")),
			ESprawlHumanPresentation::Masculine,
			178.f,
		},
		{
			TEXT("Amina"),
			FSoftObjectPath(
				TEXT("/Game/MetaHumans/Residents/Amina/BP_Amina.BP_Amina_C")),
			FSoftObjectPath(TEXT(
				"/Game/MetaHumans/Residents/Amina/Face/"
				"SKM_MHC_Amina_FaceMesh.SKM_MHC_Amina_FaceMesh")),
			ESprawlHumanPresentation::Feminine,
			168.f,
		},
		{
			TEXT("Andre"),
			FSoftObjectPath(
				TEXT("/Game/MetaHumans/Residents/Andre/BP_Andre.BP_Andre_C")),
			FSoftObjectPath(TEXT(
				"/Game/MetaHumans/Residents/Andre/Face/"
				"SKM_MHC_Andre_FaceMesh.SKM_MHC_Andre_FaceMesh")),
			ESprawlHumanPresentation::Masculine,
			183.f,
		},
	};
	return Entries;
}

const FSprawlCrowdMetaHumanEntry* FSprawlCrowdMetaHuman::FindEntry(
	FName AppearanceId)
{
	return Roster().FindByPredicate(
		[AppearanceId](const FSprawlCrowdMetaHumanEntry& Entry)
		{
			return Entry.AppearanceId == AppearanceId;
		});
}

FName FSprawlCrowdMetaHuman::ChooseAppearanceId(
	int32 Seed, ESprawlHumanPresentation Presentation)
{
	TArray<const FSprawlCrowdMetaHumanEntry*> Matches;
	for (const FSprawlCrowdMetaHumanEntry& Entry : Roster())
	{
		if (IsPresentationMatch(Presentation, Entry.Presentation))
		{
			Matches.Add(&Entry);
		}
	}
	if (Matches.IsEmpty())
	{
		return TEXT("Zarri");
	}
	FRandomStream Stream(Seed ^ 0x4D455441); // "META"
	return Matches[Stream.RandRange(0, Matches.Num() - 1)]->AppearanceId;
}

FName FSprawlCrowdMetaHuman::AppearanceIdForPopulationIndex(
	int32 PopulationIndex)
{
	const TArray<FSprawlCrowdMetaHumanEntry>& Entries = Roster();
	if (Entries.IsEmpty())
	{
		return NAME_None;
	}
	const int32 WrappedIndex =
		((PopulationIndex % Entries.Num()) + Entries.Num()) % Entries.Num();
	return Entries[WrappedIndex].AppearanceId;
}

FSprawlCrowdMetaHumanStyle FSprawlCrowdMetaHuman::BuildStyle(
	const FSprawlHumanCustomization& Customization, float AuthoredHeightCm)
{
	const float HeightScale = FMath::Clamp(
		Customization.HeightCm / FMath::Max(1.f, AuthoredHeightCm),
		0.88f, 1.14f);

	FSprawlCrowdMetaHumanStyle Result;
	// Uniform stature variation keeps each authored face and head proportion
	// intact. Body-build diversity comes from the distinct assembled residents.
	Result.VisualScale = FVector(HeightScale);
	Result.PrimaryWardrobe =
		Customization.Outfit.Outerwear == ESprawlWardrobeOuterwear::None
		? Customization.Outfit.TopColor
		: Customization.Outfit.OuterwearColor;
	Result.SecondaryWardrobe = Customization.Outfit.BottomColor;
	return Result;
}

bool FSprawlCrowdMetaHuman::Activate(
	UChildActorComponent* VisualComponent,
	const FSprawlHumanCustomization& Customization,
	USkeletalMeshComponent*& OutBodyMesh,
	FName& OutResolvedAppearanceId)
{
	OutBodyMesh = nullptr;
	OutResolvedAppearanceId = NAME_None;
	if (!VisualComponent)
	{
		return false;
	}

	TArray<const FSprawlCrowdMetaHumanEntry*> Candidates;
	AddCandidate(Candidates, FindEntry(Customization.AppearanceId));
	for (const FSprawlCrowdMetaHumanEntry& Entry : Roster())
	{
		if (IsPresentationMatch(Customization.Presentation, Entry.Presentation))
		{
			AddCandidate(Candidates, &Entry);
		}
	}
	for (const FSprawlCrowdMetaHumanEntry& Entry : Roster())
	{
		AddCandidate(Candidates, &Entry);
	}

	for (const FSprawlCrowdMetaHumanEntry* Entry : Candidates)
	{
		TSoftClassPtr<AActor> SoftClass(Entry->VisualClassPath);
		const TSubclassOf<AActor> VisualClass = SoftClass.LoadSynchronous();
		if (!VisualClass)
		{
			continue;
		}

		const FSprawlCrowdMetaHumanStyle Style =
			BuildStyle(Customization, Entry->AuthoredHeightCm);
		// Preserve the component's capsule-derived offset and any designer-authored
		// rotation. Copy styling changes only uniform stature.
		VisualComponent->SetRelativeScale3D(Style.VisualScale);
		VisualComponent->SetChildActorClass(VisualClass);
		AActor* VisualActor = VisualComponent->GetChildActor();
		if (!VisualActor)
		{
			VisualComponent->SetChildActorClass(nullptr);
			continue;
		}

		TInlineComponentArray<USkeletalMeshComponent*> SkeletalMeshes(VisualActor);
		for (USkeletalMeshComponent* SkeletalMesh : SkeletalMeshes)
		{
			if (!SkeletalMesh)
			{
				continue;
			}
			SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			SkeletalMesh->SetGenerateOverlapEvents(false);
			SkeletalMesh->VisibilityBasedAnimTickOption =
				EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
			if (SkeletalMesh->GetFName() == TEXT("Body")
				|| SkeletalMesh->ComponentHasTag(TEXT("MetaHumanBody")))
			{
				OutBodyMesh = SkeletalMesh;
			}
		}
		TInlineComponentArray<UPrimitiveComponent*> Primitives(VisualActor);
		for (UPrimitiveComponent* Primitive : Primitives)
		{
			if (Primitive)
			{
				Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Primitive->SetGenerateOverlapEvents(false);
			}
		}
		VisualActor->SetActorEnableCollision(false);
		FSprawlCharacterRender::DisableDecalProjection(VisualActor);

		if (!OutBodyMesh || !OutBodyMesh->GetSkeletalMeshAsset())
		{
			VisualComponent->SetChildActorClass(nullptr);
			OutBodyMesh = nullptr;
			continue;
		}

		OutBodyMesh->SetAnimInstanceClass(nullptr);
		OutBodyMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		// Start cheap. Each rendering client promotes only its nearest residents.
		SetHighDetail(VisualComponent, false);
		OutResolvedAppearanceId = Entry->AppearanceId;
		return true;
	}

	VisualComponent->SetChildActorClass(nullptr);
	return false;
}

bool FSprawlCrowdMetaHuman::LoadLocomotion(
	UAnimSequence*& OutIdle, UAnimSequence*& OutWalk, UAnimSequence*& OutRun)
{
	static TSoftObjectPtr<UAnimSequence> Idle{FSoftObjectPath(MetaHumanIdlePath)};
	static TSoftObjectPtr<UAnimSequence> Walk{FSoftObjectPath(MetaHumanWalkPath)};
	static TSoftObjectPtr<UAnimSequence> Run{FSoftObjectPath(MetaHumanRunPath)};
	OutIdle = Idle.LoadSynchronous();
	OutWalk = Walk.LoadSynchronous();
	OutRun = Run.LoadSynchronous();
	return OutIdle && OutWalk && OutRun;
}

void FSprawlCrowdMetaHuman::SetHighDetail(
	UChildActorComponent* VisualComponent, bool bHighDetail)
{
	AActor* VisualActor = VisualComponent ? VisualComponent->GetChildActor() : nullptr;
	if (!VisualActor)
	{
		return;
	}
	if (ULODSyncComponent* LODSync =
		VisualActor->FindComponentByClass<ULODSyncComponent>())
	{
		// LODSync is zero-based and controls the body, face, hair, and grooms as
		// one MetaHuman. Direct per-mesh forcing would be overwritten next tick.
		// Optimized Low exports may contain only two LODs. Pick relative to the
		// assembled asset so near and far never clamp to the same lowest level.
		const int32 DesiredLOD = bHighDetail || LODSync->NumLODs <= 0
			? 0
			: LODSync->NumLODs - 1;
		LODSync->ForcedLOD = DesiredLOD;
		LODSync->UpdateLOD();
		return;
	}
	TInlineComponentArray<USkeletalMeshComponent*> SkeletalMeshes(VisualActor);
	for (USkeletalMeshComponent* SkeletalMesh : SkeletalMeshes)
	{
		if (SkeletalMesh)
		{
			// Optimized/Low MetaHumans supply the crowd LOD range.  Nearby
			// residents retain facial definition; distant copies stay cheap.
			// SetForcedLOD is one-based (1 selects authored LOD 0).
			SkeletalMesh->SetForcedLOD(bHighDetail
				? 1
				: FMath::Max(1, SkeletalMesh->GetNumLODs()));
			SkeletalMesh->VisibilityBasedAnimTickOption =
				EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		}
	}
}

int32 FSprawlCrowdMetaHuman::PopulationCap(bool bIOS)
{
	return bIOS ? 3 : 8;
}

int32 FSprawlCrowdMetaHuman::HighDetailBudget(bool bIOS)
{
	return bIOS ? 1 : 3;
}

TArray<int32> FSprawlCrowdMetaHuman::SelectHighDetailIndices(
	const TArray<float>& DistanceSquared, int32 Budget)
{
	TArray<int32> Indices;
	Indices.Reserve(DistanceSquared.Num());
	for (int32 Index = 0; Index < DistanceSquared.Num(); ++Index)
	{
		Indices.Add(Index);
	}
	Indices.Sort([&DistanceSquared](int32 A, int32 B)
	{
		if (FMath::IsNearlyEqual(DistanceSquared[A], DistanceSquared[B]))
		{
			return A < B;
		}
		return DistanceSquared[A] < DistanceSquared[B];
	});
	Indices.SetNum(FMath::Clamp(Budget, 0, Indices.Num()));
	return Indices;
}
