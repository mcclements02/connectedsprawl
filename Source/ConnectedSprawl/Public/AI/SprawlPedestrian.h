// The Connected Sprawl - Pedestrian NPC. Wanders the streets on its own.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SprawlPedestrian.generated.h"

/**
 * ASprawlPedestrian
 * -----------------
 * A street NPC: engine mannequin mesh that wanders the city under its own
 * power — no nav mesh required. It walks in a direction, repicks one every
 * few seconds, and turns away when it bumps something.
 */
UCLASS()
class CONNECTEDSPRAWL_API ASprawlPedestrian : public ACharacter
{
	GENERATED_BODY()

public:
	ASprawlPedestrian();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	FVector WanderDir = FVector::ForwardVector;
	float RepickTimer = 0.f;
	float StuckTimer = 0.f;

	void PickNewDirection();
};
