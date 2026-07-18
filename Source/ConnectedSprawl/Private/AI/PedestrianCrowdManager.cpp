// The Connected Sprawl - Pedestrian crowd density controller.

#include "AI/PedestrianCrowdManager.h"
#include "AI/SprawlPedestrian.h"
#include "World/SprawlCityGridSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

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
	Evaluate();
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

void APedestrianCrowdManager::Evaluate()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !PC->GetPawn()) return;

	UWorld* World = GetWorld();
	if (!World || !PedestrianClass) return;

	const FVector PlayerLoc = PC->GetPawn()->GetActorLocation();

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

	// Top up the crowd, a few per pass to spread the cost.
	int32 Budget = 4;
	while (ActivePeds.Num() < TargetCount && Budget-- > 0)
	{
		FVector SpawnPoint;
		if (!FindSidewalkSpawnPoint(PlayerLoc, SpawnPoint))
		{
			break;
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
		ASprawlPedestrian* Ped = World->SpawnActor<ASprawlPedestrian>(
			PedestrianClass, SpawnPoint,
			FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Params);
		if (!Ped)
		{
			continue;
		}
		ActivePeds.Add(Ped);
	}
}
