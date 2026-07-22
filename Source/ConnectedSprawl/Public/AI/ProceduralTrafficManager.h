// The Connected Sprawl - Procedural traffic.
// Maintains a small NPC-vehicle population without simulating 15 miles of AI.
// Replacement cars now leave a real parking deck instead of appearing in lanes.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralTrafficManager.generated.h"

class APawn;
class ASprawlPedestrian;
class ASprawlCar;

/**
 * AProceduralTrafficManager
 * -------------------------
 * Lightweight traffic density controller.
 *
 * Strategy:
 *   - Keep N active AI vehicles while culling cars well outside the player ring.
 *   - Replenish at most one car per pass from an obstruction-tested garage exit.
 *   - Hand each car to normal directed-lane AI only after its driveway merge.
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

	/** Legacy interest-ring tune retained for saved Blueprint compatibility. */
	UPROPERTY(EditAnywhere, Category="Traffic") float InterestRadius = 7000.f;

	/** Distance at which managed traffic may be culled from the player. */
	UPROPERTY(EditAnywhere, Category="Traffic") float SpawnRadius = 13000.f;

	/** Throttle in seconds between spawn/cull passes. */
	UPROPERTY(EditAnywhere, Category="Traffic") float EvaluateInterval = 1.0f;

	/** Bounded pool of abandoned stolen cars retained near the player. */
	UPROPERTY(EditAnywhere, Category="Traffic") int32 MaxRetiredTraffic = 8;

protected:
	float TimeSinceEval = 0.f;
	UPROPERTY() TArray<APawn*> ActiveTraffic;
	UPROPERTY() TArray<APawn*> RetiredTraffic;
	bool bTrafficAuditEnabled = false;
	bool bTrafficAuditComplete = false;
	bool bTrafficAuditPassed = false;
	bool bCarjackAuditEnabled = false;
	bool bCarjackAuditComplete = false;
	bool bCarjackAuditPassed = false;
	bool bCarjackAuditTriggered = false;
	float CarjackAuditElapsed = 0.f;
	float CarjackAuditTriggerTime = 0.f;
	TWeakObjectPtr<ASprawlCar> CarjackAuditCar;
	FString CarjackAuditDriverVariant;
	int32 CarjackAuditPedestrianCountBefore = 0;
	bool bCarjackAuditSawFleeingDriver = false;
	float TrafficAuditElapsed = 0.f;
	float TrafficAuditMinSpacing = TNumericLimits<float>::Max();
	int32 TrafficAuditMaxIntersectionOccupancy = 0;
	int32 TrafficAuditMaxPedestrians = 0;
	int32 TrafficAuditMaxRealAvatars = 0;
	int32 TrafficAuditMaxDistinctMetaHumans = 0;
	int32 TrafficAuditMaxNonMetaHumansAfterWarmup = 0;
	int32 TrafficAuditPeakCars = 0;
	int32 TrafficAuditPeakTotalCars = 0;
	int32 TrafficAuditMaxEnterableCars = 0;
	int32 TrafficAuditMaxBoundaryViolations = 0;
	int32 TrafficAuditMaxUprightViolations = 0;
	int32 TrafficAuditMaxHiddenDrivers = 0;
	int32 TrafficAuditMaxVisibleDriverViolations = 0;
	int32 TrafficAuditMaxMissingHiddenDriversAfterWarmup = 0;
	int32 TrafficAuditUnauthorizedEntries = 0;
	int32 TrafficAuditMaxCompletedTurns = 0;
	int32 TrafficAuditMaxCompletedUTurns = 0;
	int32 TrafficRouteRecycles = 0;
	int32 TrafficSpawnRouteRejects = 0;
	int32 TrafficGarageSpawnAttempts = 0;
	int32 TrafficGarageSpawnSuccesses = 0;
	int32 TrafficGarageBlockedExits = 0;
	int32 NextGarageExitIndex = 0;
	TMap<TWeakObjectPtr<APawn>, FVector> TrafficAuditStarts;
	TMap<TWeakObjectPtr<APawn>, float> TrafficAuditLaneViolationDurations;
	TMap<TWeakObjectPtr<APawn>, float> TrafficAuditWrongWayDurations;
	TSet<TWeakObjectPtr<APawn>> TrafficAuditMovedCars;
	TSet<TWeakObjectPtr<APawn>> TrafficAuditWheelMotionCars;
	TSet<TWeakObjectPtr<APawn>> TrafficAuditSignalStops;
	TSet<TWeakObjectPtr<APawn>> TrafficAuditLaneViolators;
	TSet<TWeakObjectPtr<APawn>> TrafficAuditWrongWayViolators;
	TSet<TWeakObjectPtr<APawn>> TrafficAuditCarsInIntersections;
	TMap<TWeakObjectPtr<ASprawlPedestrian>, float> TrafficAuditPedOffRoadDurations;
	TSet<TWeakObjectPtr<ASprawlPedestrian>> TrafficAuditPedOffsideViolators;

	void Evaluate();
	void CullDistant(const FVector& PlayerLoc);
	void SpawnNeeded(const FVector& PlayerLoc);
	void RunTrafficAudit(float DeltaSeconds);
	void RunCarjackAudit(float DeltaSeconds);
	void RequestAuditExitIfComplete();
};
