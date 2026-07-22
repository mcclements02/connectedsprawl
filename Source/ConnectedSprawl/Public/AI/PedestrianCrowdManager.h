// The Connected Sprawl - Pedestrian crowd density controller.
// Keeps the sidewalks around the player populated without simulating the
// whole city; the same interest-ring strategy the traffic manager uses.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PedestrianCrowdManager.generated.h"

class ASprawlPedestrian;

/**
 * APedestrianCrowdManager
 * -----------------------
 * Maintains a target number of pedestrians inside an interest ring around
 * the player. New pedestrians spawn on sidewalk edges of nearby blocks
 * (never in the road, never over the lake) just outside the player's view
 * distance; pedestrians that fall far behind are recycled.
 */
UCLASS()
class CONNECTEDSPRAWL_API APedestrianCrowdManager : public AActor
{
	GENERATED_BODY()

public:
	APedestrianCrowdManager();

	virtual void Tick(float DeltaSeconds) override;

	/** Pedestrian class to spawn; defaults to ASprawlPedestrian. */
	UPROPERTY(EditAnywhere, Category="Crowd")
	TSubclassOf<ASprawlPedestrian> PedestrianClass;

	/** Desired live count before the platform's full-MetaHuman safety cap. */
	UPROPERTY(EditAnywhere, Category="Crowd") int32 TargetCount = 8;

	/** Despawn beyond this distance from the player (cm). */
	UPROPERTY(EditAnywhere, Category="Crowd") float RecycleRadius = 16000.f;

	/** New spawns appear between these distances from the player (cm). */
	UPROPERTY(EditAnywhere, Category="Crowd") float SpawnMinRadius = 4500.f;
	UPROPERTY(EditAnywhere, Category="Crowd") float SpawnMaxRadius = 11000.f;

	/** Seconds between population passes. */
	UPROPERTY(EditAnywhere, Category="Crowd") float EvaluateInterval = 1.5f;

	/** Bring a deferred-spawned ejected driver into the normal recycle ring. */
	void AdoptExternalPedestrian(ASprawlPedestrian* Pedestrian);

	int32 GetActivePedestrianCount() const { return ActivePeds.Num(); }

protected:
	float TimeSinceEval = 0.f;
	int32 NextCharacterSeed = 1;
	UPROPERTY() TArray<TObjectPtr<ASprawlPedestrian>> ActivePeds;

	/** Authority-only population ownership. */
	void EvaluatePopulation();

	/** Rendering-client LOD selection over the replicated pedestrian set. */
	void UpdateLocalDetail();
	bool FindSidewalkSpawnPoint(const FVector& PlayerLoc, FVector& OutPoint) const;
};
