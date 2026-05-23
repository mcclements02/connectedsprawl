// The Connected Sprawl - Procedural traffic.

#include "AI/ProceduralTrafficManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

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
	if (TrafficPawnClasses.Num() == 0) return;

	UWorld* World = GetWorld();
	if (!World) return;

	while (ActiveTraffic.Num() < TargetActiveCount)
	{
		// Random point in the outer annulus (InterestRadius..SpawnRadius).
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const float Dist  = FMath::FRandRange(InterestRadius, SpawnRadius);
		const FVector Offset(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.f);
		const FVector SpawnLoc = PlayerLoc + Offset;

		// TODO: snap to navmesh / spline road once road network is defined.

		const int32 Idx = FMath::RandRange(0, TrafficPawnClasses.Num() - 1);
		TSubclassOf<APawn> Cls = TrafficPawnClasses[Idx];
		if (!Cls) break;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		APawn* Spawned = World->SpawnActor<APawn>(Cls, SpawnLoc, FRotator::ZeroRotator, Params);
		if (!Spawned) break;

		ActiveTraffic.Add(Spawned);
	}
}
