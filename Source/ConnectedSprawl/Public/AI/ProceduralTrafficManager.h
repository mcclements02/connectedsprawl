// The Connected Sprawl - Procedural traffic.
// Spawns NPC-vehicle "clumps" around Zarri so the city feels populated without
// simulating 15 miles of AI. Outside the active ring, vehicles are despawned.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralTrafficManager.generated.h"

class APawn;

/**
 * AProceduralTrafficManager
 * -------------------------
 * Lightweight traffic density controller.
 *
 * Strategy:
 *   - Keep N active AI vehicles within a 400m "interest ring" around the player.
 *   - When the player drives out of a vehicle's SLEEP_RADIUS, despawn it.
 *   - Periodically spawn new vehicles on off-screen spline paths within SPAWN_RADIUS.
 *
 * This is a placeholder ahead of a full MassEntity implementation. With Mass we
 * can scale to thousands of silhouette vehicles on the Arteries at near-zero
 * cost. For gray-box / vertical slice this actor-based version is enough.
 */
UCLASS()
class CONNECTEDSPRAWL_API AProceduralTrafficManager : public AActor
{
	GENERATED_BODY()

public:
	AProceduralTrafficManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Vehicle classes to spawn (cars, vans, delivery). Assigned in BP. */
	UPROPERTY(EditAnywhere, Category="Traffic")
	TArray<TSubclassOf<APawn>> TrafficPawnClasses;

	/** Target count of active AI vehicles within the interest ring. */
	UPROPERTY(EditAnywhere, Category="Traffic") int32 TargetActiveCount = 14;

	/** Interest ring radius (cm). Vehicles inside this are kept; outside are despawned.
	 *  Sized for the current prototype grid (~180m across), not the full 15-mile map. */
	UPROPERTY(EditAnywhere, Category="Traffic") float InterestRadius = 7000.f;

	/** Spawn annulus outer bound (cm). New vehicles spawn in [InterestRadius, SpawnRadius]. */
	UPROPERTY(EditAnywhere, Category="Traffic") float SpawnRadius = 13000.f;

	/** Throttle in seconds between spawn/cull passes. */
	UPROPERTY(EditAnywhere, Category="Traffic") float EvaluateInterval = 1.0f;

protected:
	float TimeSinceEval = 0.f;
	UPROPERTY() TArray<APawn*> ActiveTraffic;
	bool bTrafficAuditEnabled = false;
	bool bTrafficAuditComplete = false;
	float TrafficAuditElapsed = 0.f;
	float TrafficAuditMinSpacing = TNumericLimits<float>::Max();
	int32 TrafficAuditMaxIntersectionOccupancy = 0;
	int32 TrafficAuditMaxPedestrians = 0;
	int32 TrafficAuditMaxRealAvatars = 0;
	int32 TrafficAuditPeakCars = 0;
	int32 TrafficAuditPeakTotalCars = 0;
	int32 TrafficAuditMaxEnterableCars = 0;
	int32 TrafficAuditMaxBoundaryViolations = 0;
	int32 TrafficAuditMaxUprightViolations = 0;
	int32 TrafficAuditUnauthorizedEntries = 0;
	TMap<TWeakObjectPtr<APawn>, FVector> TrafficAuditStarts;
	TMap<TWeakObjectPtr<APawn>, float> TrafficAuditLaneViolationDurations;
	TSet<TWeakObjectPtr<APawn>> TrafficAuditMovedCars;
	TSet<TWeakObjectPtr<APawn>> TrafficAuditWheelMotionCars;
	TSet<TWeakObjectPtr<APawn>> TrafficAuditSignalStops;
	TSet<TWeakObjectPtr<APawn>> TrafficAuditLaneViolators;
	TSet<TWeakObjectPtr<APawn>> TrafficAuditCarsInIntersections;

	void Evaluate();
	void CullDistant(const FVector& PlayerLoc);
	void SpawnNeeded(const FVector& PlayerLoc);
	void RunTrafficAudit(float DeltaSeconds);
};
