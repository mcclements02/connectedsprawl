// The Connected Sprawl - Pedestrian AI controller. Wanders the nav mesh.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "SprawlPedestrianAI.generated.h"

/**
 * ASprawlPedestrianAI
 * -------------------
 * Drives an ASprawlPedestrian: repeatedly picks a random reachable point on
 * the city nav mesh and walks there, pausing briefly between trips.
 */
UCLASS()
class CONNECTEDSPRAWL_API ASprawlPedestrianAI : public AAIController
{
	GENERATED_BODY()

public:
	ASprawlPedestrianAI();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnMoveCompleted(FAIRequestID RequestID,
	                             const FPathFollowingResult& Result) override;

	/** Roam radius around the pedestrian's current spot. */
	UPROPERTY(EditDefaultsOnly, Category="Pedestrian")
	float WanderRadius = 3500.f;

	void PickNewDestination();

private:
	FTimerHandle MoveTimer;
};
