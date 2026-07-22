// The Connected Sprawl - Pedestrian crowd density controller.

#include "AI/PedestrianCrowdManager.h"
#include "AI/SprawlPedestrian.h"
#include "Characters/SprawlCharacterDeveloper.h"
#include "Characters/SprawlCrowdMetaHuman.h"
#include "World/SprawlCityGridSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "EngineUtils.h"

using Grid = USprawlCityGridSubsystem;

APedestrianCrowdManager::APedestrianCrowdManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.5f;
	PedestrianClass = ASprawlPedestrian::StaticClass();
}

void APedestrianCrowdManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TimeSinceEval += DeltaSeconds;
	if (TimeSinceEval < EvaluateInterval) return;
	TimeSinceEval = 0.f;
	// This manager is a non-replicated level actor, so HasAuthority() is also
	// true for its client-local copy. Net mode is the reliable population-owner
	// check; clients only select presentation detail for replicated pedestrians.
	if (GetNetMode() != NM_Client)
	{
		EvaluatePopulation();
	}
	if (GetNetMode() != NM_DedicatedServer)
	{
		UpdateLocalDetail();
	}
}

void APedestrianCrowdManager::AdoptExternalPedestrian(
	ASprawlPedestrian* Pedestrian)
{
	if (GetNetMode() == NM_Client || !IsValid(Pedestrian)
		|| ActivePeds.Contains(Pedestrian))
	{
		return;
	}

	// Keep the iPhone population cap stable. The ejected driver is beside the
	// player, so recycle the farthest ordinary pedestrian when the ring is full.
	const int32 EffectiveTarget = FMath::Min(
		TargetCount, FSprawlCrowdMetaHuman::PopulationCap(PLATFORM_IOS != 0));
	if (EffectiveTarget <= 0)
	{
		Pedestrian->SetLifeSpan(30.f);
		return;
	}
	if (ActivePeds.Num() >= EffectiveTarget)
	{
		const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		const FVector Focus = PC && PC->GetPawn()
			? PC->GetPawn()->GetActorLocation() : Pedestrian->GetActorLocation();
		int32 FarthestIndex = INDEX_NONE;
		float FarthestDistanceSq = -1.f;
		for (int32 Index = 0; Index < ActivePeds.Num(); ++Index)
		{
			if (!IsValid(ActivePeds[Index]))
			{
				FarthestIndex = Index;
				break;
			}
			const float DistanceSq = FVector::DistSquared2D(
				ActivePeds[Index]->GetActorLocation(), Focus);
			if (DistanceSq > FarthestDistanceSq)
			{
				FarthestDistanceSq = DistanceSq;
				FarthestIndex = Index;
			}
		}
		if (ActivePeds.IsValidIndex(FarthestIndex))
		{
			if (IsValid(ActivePeds[FarthestIndex]))
			{
				ActivePeds[FarthestIndex]->Destroy();
			}
			ActivePeds.RemoveAtSwap(FarthestIndex);
		}
	}
	ActivePeds.Add(Pedestrian);
}

bool APedestrianCrowdManager::FindSidewalkSpawnPoint(const FVector& PlayerLoc, FVector& OutPoint) const
{
	for (int32 Attempt = 0; Attempt < 12; ++Attempt)
	{
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const float Dist  = FMath::FRandRange(SpawnMinRadius, SpawnMaxRadius);
		const float Px = PlayerLoc.X + FMath::Cos(Angle) * Dist;
		const float Py = PlayerLoc.Y + FMath::Sin(Angle) * Dist;

		const int32 Gx = Grid::NearestBlockIndex(Px);
		const int32 Gy = Grid::NearestBlockIndex(Py);
		const float Bx = Grid::BlockCenter(Gx);
		const float By = Grid::BlockCenter(Gy);
		if (Grid::IsOverLake(Bx, By, 0.f))
		{
			continue;
		}

		// A random point on the block's sidewalk ring (the outer band of the
		// raised block surface), so the pedestrian starts mid-stroll.
		// Keep the walking line behind recessed curb-parking bays instead of
		// spawning people through car bodies at the block edge.
		const float Reach = Grid::BlockSize * 0.5f - 360.f;
		const bool bXEdge = FMath::RandBool();
		const float Along = FMath::FRandRange(-Reach, Reach);
		const float Side  = FMath::RandBool() ? Reach : -Reach;
		OutPoint = bXEdge
			? FVector(Bx + Along, By + Side, 130.f)
			: FVector(Bx + Side, By + Along, 130.f);

		FCollisionObjectQueryParams Objects;
		Objects.AddObjectTypesToQuery(ECC_WorldStatic);
		Objects.AddObjectTypesToQuery(ECC_Pawn);
		Objects.AddObjectTypesToQuery(ECC_PhysicsBody);
		FCollisionQueryParams Params(
			FName(TEXT("SprawlPedestrianSpawnClearance")), false, this);
		const FVector ClearanceCenter = OutPoint + FVector(0.f, 0.f, 20.f);
		if (GetWorld()->OverlapAnyTestByObjectType(ClearanceCenter, FQuat::Identity,
			Objects, FCollisionShape::MakeBox(FVector(45.f, 45.f, 70.f)), Params))
		{
			continue;
		}
		return true;
	}
	return false;
}

void APedestrianCrowdManager::EvaluatePopulation()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !PC->GetPawn()) return;

	UWorld* World = GetWorld();
	if (!World || !PedestrianClass) return;

	const FVector PlayerLoc = PC->GetPawn()->GetActorLocation();
	const float CityHour = USprawlCharacterDeveloper::ResolveCityHour(World);
	const ESprawlCharacterDistrict District =
		USprawlCharacterDeveloper::DistrictForLocation(PlayerLoc);
	const int32 EffectiveTarget = FMath::Min(
		TargetCount, FSprawlCrowdMetaHuman::PopulationCap(PLATFORM_IOS != 0));
	const int32 DesiredCount = EffectiveTarget > 0
		? FMath::Clamp(FMath::RoundToInt(EffectiveTarget
			* USprawlCharacterDeveloper::PopulationMultiplier(District, CityHour)),
			1, EffectiveTarget)
		: 0;

	// Recycle pedestrians the player has left behind.
	for (int32 i = ActivePeds.Num() - 1; i >= 0; --i)
	{
		ASprawlPedestrian* Ped = ActivePeds[i];
		if (!IsValid(Ped))
		{
			ActivePeds.RemoveAtSwap(i);
			continue;
		}
		if (FVector::Dist2D(Ped->GetActorLocation(), PlayerLoc) > RecycleRadius)
		{
			Ped->Destroy();
			ActivePeds.RemoveAtSwap(i);
		}
	}

	// Ease density down when the player enters a quiet district or late hour.
	// Removing at most two per pass avoids a visible crowd pop.
	int32 RetireBudget = 2;
	while (ActivePeds.Num() > DesiredCount && RetireBudget-- > 0)
	{
		int32 FarthestIndex = INDEX_NONE;
		float FarthestDistanceSq = -1.f;
		for (int32 Index = 0; Index < ActivePeds.Num(); ++Index)
		{
			if (!IsValid(ActivePeds[Index]))
			{
				FarthestIndex = Index;
				break;
			}
			const float DistanceSq = FVector::DistSquared2D(
				ActivePeds[Index]->GetActorLocation(), PlayerLoc);
			if (DistanceSq > FarthestDistanceSq)
			{
				FarthestDistanceSq = DistanceSq;
				FarthestIndex = Index;
			}
		}
		if (!ActivePeds.IsValidIndex(FarthestIndex))
		{
			break;
		}
		if (IsValid(ActivePeds[FarthestIndex]))
		{
			ActivePeds[FarthestIndex]->Destroy();
		}
		ActivePeds.RemoveAtSwap(FarthestIndex);
	}

	// Load at most one uncached assembled resident per pass. This deliberately
	// spreads first-use MetaHuman class/material work across frames on iPhone.
	int32 Budget = 1;
	while (ActivePeds.Num() < DesiredCount && Budget-- > 0)
	{
		FVector SpawnPoint;
		if (!FindSidewalkSpawnPoint(PlayerLoc, SpawnPoint))
		{
			break;
		}

		const FTransform SpawnTransform(
			FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), SpawnPoint);
		ASprawlPedestrian* Ped = World->SpawnActorDeferred<ASprawlPedestrian>(
			PedestrianClass, SpawnTransform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
		if (!Ped)
		{
			continue;
		}

		const int32 PopulationIndex = NextCharacterSeed++;
		const int32 Seed = static_cast<int32>(HashCombineFast(
			HashCombineFast(GetTypeHash(FMath::RoundToInt(SpawnPoint.X)),
				GetTypeHash(FMath::RoundToInt(SpawnPoint.Y))),
			GetTypeHash(PopulationIndex)));
		Ped->SetDevelopedProfile(USprawlCharacterDeveloper::DevelopCharacter(
			Seed, SpawnPoint, CityHour));
		if (USprawlHumanCharacterModule* Human = Ped->GetHumanCharacterModule())
		{
			FSprawlHumanCustomization Customization =
				Human->GetRuntimeState().Customization;
			Customization.AppearanceId =
				FSprawlCrowdMetaHuman::AppearanceIdForPopulationIndex(
					PopulationIndex - 1);
			if (const FSprawlCrowdMetaHumanEntry* Entry =
				FSprawlCrowdMetaHuman::FindEntry(Customization.AppearanceId))
			{
				Customization.Presentation = Entry->Presentation;
			}
			FString CustomizationError;
			if (!Human->SetCustomization(Customization, CustomizationError))
			{
				UE_LOG(LogTemp, Error,
					TEXT("[CrowdMetaHuman] balanced appearance rejected: %s"),
					*CustomizationError);
			}
		}
		AActor* FinishedActor = UGameplayStatics::FinishSpawningActor(Ped, SpawnTransform);
		if (ASprawlPedestrian* FinishedPed = Cast<ASprawlPedestrian>(FinishedActor))
		{
			ActivePeds.Add(FinishedPed);
		}
	}

	// Presentation detail is selected independently on every rendering client.
}

void APedestrianCrowdManager::UpdateLocalDetail()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !PC->GetPawn())
	{
		return;
	}

	const FVector PlayerLoc = PC->GetPawn()->GetActorLocation();
	TArray<ASprawlPedestrian*> LocalPedestrians;
	TArray<float> DistanceSquared;
	for (TActorIterator<ASprawlPedestrian> It(GetWorld()); It; ++It)
	{
		ASprawlPedestrian* Pedestrian = *It;
		if (!IsValid(Pedestrian))
		{
			continue;
		}
		LocalPedestrians.Add(Pedestrian);
		DistanceSquared.Add(FVector::DistSquared2D(
			Pedestrian->GetActorLocation(), PlayerLoc));
	}
	const TArray<int32> HighDetailIndices =
		FSprawlCrowdMetaHuman::SelectHighDetailIndices(
			DistanceSquared,
			FSprawlCrowdMetaHuman::HighDetailBudget(PLATFORM_IOS != 0));
	for (int32 Index = 0; Index < LocalPedestrians.Num(); ++Index)
	{
		LocalPedestrians[Index]->SetMetaHumanHighDetail(
			HighDetailIndices.Contains(Index));
	}
}
