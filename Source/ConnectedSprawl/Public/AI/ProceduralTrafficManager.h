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
	UPROPERTY(EditAnywhere, Category="Traffic") int32 TargetActiveCount = 12;

	/** Interest ring radius (cm). Vehicles inside this are kept; outside are despawned. */
	UPROPERTY(EditAnywhere, Category="Traffic") float InterestRadius = 40000.f; // ~400m

	/** Spawn annulus outer bound (cm). New vehicles spawn in [InterestRadius, SpawnRadius]. */
	UPROPERTY(EditAnywhere, Category="Traffic") float SpawnRadius = 55000.f;

	/** Throttle in seconds between spawn/cull passes. */
	UPROPERTY(EditAnywhere, Category="Traffic") float EvaluateInterval = 1.0f;

protected:
	float TimeSinceEval = 0.f;
	UPROPERTY() TArray<APawn*> ActiveTraffic;

	void Evaluate();
	void CullDistant(const FVector& PlayerLoc);
	void SpawnNeeded(const FVector& PlayerLoc);
};
