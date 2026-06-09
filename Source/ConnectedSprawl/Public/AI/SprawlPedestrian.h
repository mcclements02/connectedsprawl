// The Connected Sprawl - Pedestrian NPC. Walks the sidewalks like a local.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SprawlPedestrian.generated.h"

/**
 * ASprawlPedestrian
 * -----------------
 * A street NPC that actually uses the city: it walks the sidewalk ring of
 * its current block, corner to corner, occasionally crossing the road to an
 * adjacent block — waiting at the curb if traffic is coming. If a fast car
 * gets too close it breaks into a run away from it. No nav mesh required;
 * the city grid subsystem supplies all the geometry.
 */
UCLASS()
class CONNECTEDSPRAWL_API ASprawlPedestrian : public ACharacter
{
	GENERATED_BODY()

public:
	ASprawlPedestrian();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Casual walking speed range (cm/s); each pedestrian rolls one value. */
	UPROPERTY(EditAnywhere, Category="Pedestrian") float MinWalkSpeed = 115.f;
	UPROPERTY(EditAnywhere, Category="Pedestrian") float MaxWalkSpeed = 175.f;
	/** Speed when fleeing a dangerous car. */
	UPROPERTY(EditAnywhere, Category="Pedestrian") float RunSpeed = 470.f;
	/** Sidewalk walking line, inset from the block edge (cm from block center). */
	UPROPERTY(EditAnywhere, Category="Pedestrian") float SidewalkInset = 120.f;
	/** Chance of crossing the road on reaching a corner. */
	UPROPERTY(EditAnywhere, Category="Pedestrian") float CrossChance = 0.30f;

private:
	enum class EPedState : uint8 { WalkEdge, WaitToCross, Cross, Flee };

	EPedState State = EPedState::WalkEdge;
	int32 BlockX = 0;
	int32 BlockY = 0;
	int32 CornerIndex = 0;   // corner we're heading to (0..3)
	int32 WalkDir = 1;       // +1 / -1: direction around the block perimeter
	FVector TargetPoint = FVector::ZeroVector;
	FVector FleeDir = FVector::ForwardVector;
	float BaseSpeed = 140.f;
	float StateTimer = 0.f;
	float DangerScanTimer = 0.f;
	float CurbScanTimer = 0.f;
	float StuckTimer = 0.f;

	/** Corner sign offsets for index 0..3. */
	static FIntPoint CornerSigns(int32 Index);

	/** World position of a corner of a block's sidewalk walking line. */
	FVector CornerPoint(int32 Gx, int32 Gy, int32 Index) const;

	/** True if (Gx, Gy) is a walkable block: on the map and not in the lake. */
	static bool IsWalkableBlock(int32 Gx, int32 Gy);

	/** Re-anchor to the nearest block/corner (after spawn or a flee). */
	void AnchorToNearestCorner();

	void OnReachedCorner();
	bool TryStartCrossing();
	bool IsTrafficApproaching() const;
	void CheckForDanger();
};
