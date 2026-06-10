// The Connected Sprawl - Procedural traffic.

#include "AI/ProceduralTrafficManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Vehicles/SprawlCar.h"
#include "World/SprawlCityGridSubsystem.h"

AProceduralTrafficManager::AProceduralTrafficManager()
{
	PrimaryActorTick.bCanEverTick     = true;
	PrimaryActorTick.TickInterval     = 0.5f;
}

void AProceduralTrafficManager::BeginPlay()
{
	Super::BeginPlay();
}

void AProceduralTrafficManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TimeSinceEval += DeltaSeconds;
	if (TimeSinceEval < EvaluateInterval) return;
	TimeSinceEval = 0.f;
	Evaluate();
}

void AProceduralTrafficManager::Evaluate()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !PC->GetPawn()) return;

	const FVector PlayerLoc = PC->GetPawn()->GetActorLocation();
	CullDistant(PlayerLoc);
	SpawnNeeded(PlayerLoc);
}

void AProceduralTrafficManager::CullDistant(const FVector& PlayerLoc)
{
	for (int32 i = ActiveTraffic.Num() - 1; i >= 0; --i)
	{
		APawn* P = ActiveTraffic[i];
		if (!IsValid(P))
		{
			ActiveTraffic.RemoveAtSwap(i);
			continue;
		}

		const float D = FVector::Dist(P->GetActorLocation(), PlayerLoc);
		if (D > SpawnRadius)
		{
			P->Destroy();
			ActiveTraffic.RemoveAtSwap(i);
		}
	}
}

void AProceduralTrafficManager::SpawnNeeded(const FVector& PlayerLoc)
{
	using Grid = USprawlCityGridSubsystem;

	UWorld* World = GetWorld();
	if (!World) return;

	int32 Attempts = 0;
	while (ActiveTraffic.Num() < TargetActiveCount && Attempts++ < 40)
	{
		// Random point in the outer annulus (InterestRadius..SpawnRadius),
		// snapped onto a road lane so the car starts mid-traffic-flow.
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const float Dist  = FMath::FRandRange(InterestRadius, SpawnRadius);
		FVector SpawnLoc = PlayerLoc + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.f);

		// Snap to the nearest road on a random axis and pick a travel direction.
		const bool bVerticalRoad = FMath::RandBool();
		ESprawlHeading Heading;
		if (bVerticalRoad)
		{
			const int32 Road = Grid::NearestRoadIndex(SpawnLoc.X);
			Heading = FMath::RandBool() ? ESprawlHeading::North : ESprawlHeading::South;
			SpawnLoc.X = Grid::LaneCenter(Road, Heading);
		}
		else
		{
			const int32 Road = Grid::NearestRoadIndex(SpawnLoc.Y);
			Heading = FMath::RandBool() ? ESprawlHeading::East : ESprawlHeading::West;
			SpawnLoc.Y = Grid::LaneCenter(Road, Heading);
		}
		SpawnLoc.Z = 180.f;

		if (Grid::IsOverLake(SpawnLoc.X, SpawnLoc.Y) ||
			!Grid::IsInsideRoadGrid(SpawnLoc.X, SpawnLoc.Y))
		{
			continue;
		}

		TSubclassOf<APawn> Cls = ASprawlCar::StaticClass();
		if (TrafficPawnClasses.Num() > 0)
		{
			Cls = TrafficPawnClasses[FMath::RandRange(0, TrafficPawnClasses.Num() - 1)];
		}
		if (!Cls) break;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		APawn* Spawned = World->SpawnActor<APawn>(
			Cls, SpawnLoc, FRotator(0.f, Grid::HeadingYaw(Heading), 0.f), Params);
		if (!Spawned) break;

		if (ASprawlCar* Car = Cast<ASprawlCar>(Spawned))
		{
			Car->bAutoDrive = true;
		}
		ActiveTraffic.Add(Spawned);
	}
}
