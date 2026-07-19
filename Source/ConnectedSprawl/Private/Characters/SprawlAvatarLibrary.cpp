// The Connected Sprawl - Shared helpers for the imported human avatars.

#include "Characters/SprawlAvatarLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"

const TArray<FString>& FSprawlAvatarLibrary::PedestrianVariants()
{
	// Must match the folders created by Content/Python/import_artwork.py.
	// Zarri is intentionally omitted so the hero remains unique on the street.
	static const TArray<FString> Variants = {
		TEXT("Baldman"), TEXT("BizDude"), TEXT("Cappy"), TEXT("Chill"), TEXT("Erika"),
		TEXT("Jennifer"), TEXT("Jimmy"), TEXT("Kate"), TEXT("Kyle"),
		TEXT("Lydia"), TEXT("Mafiossini"), TEXT("OldMoustache"),
		TEXT("Olivia"), TEXT("Rose"), TEXT("Samuela"), TEXT("Shiro"),
		TEXT("Bruno"), TEXT("CursedAmy"), TEXT("Eugenia"),
		TEXT("Jenny"), TEXT("Juanita"),
	};
	return Variants;
}

bool FSprawlAvatarLibrary::UsesFormalWalk(const FString& VariantName)
{
	return VariantName == TEXT("BizDude") || VariantName == TEXT("Mafiossini");
}

USkeletalMesh* FSprawlAvatarLibrary::LoadAvatarMesh(const FString& VariantName)
{
	const FString Path = FString::Printf(
		TEXT("/Game/Pedestrians/%s/SK_%s.SK_%s"), *VariantName, *VariantName, *VariantName);
	return LoadObject<USkeletalMesh>(nullptr, *Path);
}

UAnimSequence* FSprawlAvatarLibrary::LoadAvatarAnim(const FString& VariantName, const TCHAR* AnimSuffix)
{
	const FString Path = FString::Printf(
		TEXT("/Game/Pedestrians/%s/%s_%s.%s_%s"),
		*VariantName, *VariantName, AnimSuffix, *VariantName, AnimSuffix);
	return LoadObject<UAnimSequence>(nullptr, *Path);
}

bool FSprawlAvatarLibrary::ApplyAvatar(USkeletalMeshComponent* MeshComp, USkeletalMesh* Mesh,
	float DesiredHeight, float CapsuleHalfHeight)
{
	if (!MeshComp || !Mesh)
	{
		return false;
	}

	MeshComp->SetAnimInstanceClass(nullptr);
	MeshComp->SetSkeletalMeshAsset(Mesh);
	MeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	// The avatars stand ~1.2m in source units; size them to the capsule.
	const FBoxSphereBounds Bounds = Mesh->GetImportedBounds();
	const float MeshHeight = FMath::Max(1.f, Bounds.BoxExtent.Z * 2.f);
	const float Scale = DesiredHeight / MeshHeight;
	MeshComp->SetRelativeScale3D(FVector(Scale));

	// Feet (mesh origin) on the capsule bottom; Mixamo-style imports face +Y,
	// so yaw them -90 onto the pawn's +X forward — same as the old mannequin.
	MeshComp->SetRelativeLocationAndRotation(
		FVector(0.f, 0.f, -CapsuleHalfHeight), FRotator(0.f, -90.f, 0.f));
	return true;
}

void FSprawlAvatarLibrary::PlayLoop(USkeletalMeshComponent* MeshComp, UAnimSequence* Anim,
	TObjectPtr<UAnimSequence>& CurrentAnim, float PlayRate)
{
	if (!MeshComp || !Anim)
	{
		return;
	}
	if (Anim != CurrentAnim)
	{
		MeshComp->PlayAnimation(Anim, /*bLooping=*/true);
		CurrentAnim = Anim;
	}
	if (UAnimSingleNodeInstance* Node = MeshComp->GetSingleNodeInstance())
	{
		Node->SetPlayRate(PlayRate);
	}
}
