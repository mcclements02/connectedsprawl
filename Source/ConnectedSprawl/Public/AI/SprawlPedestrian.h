// The Connected Sprawl - Pedestrian NPC. Walks the sidewalks like a local.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SprawlPedestrian.generated.h"

class UAnimSequence;
class AActor;

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

	enum class EPedState : uint8 { WalkEdge, WaitToCross, Cross, Flee };

public:
	ASprawlPedestrian();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Casual walking speed range (cm/s); each pedestrian rolls one value. */
	UPROPERTY(EditAnywhere, Category="Pedestrian") float MinWalkSpeed = 115.f;
	UPROPERTY(EditAnywhere, Category="Pedestrian") float MaxWalkSpeed = 175.f;
	/** Speed when fleeing a dangerous car. */
	UPROPERTY(EditAnywhere, Category="Pedestrian") float RunSpeed = 470.f;
	/** Sidewalk walking line inset from the block edge, behind recessed curb parking. */
	UPROPERTY(EditAnywhere, Category="Pedestrian") float SidewalkInset = 360.f;
	/** Chance of crossing the road on reaching a corner. */
	UPROPERTY(EditAnywhere, Category="Pedestrian") float CrossChance = 0.30f;
	/** Standing height (cm) the avatar mesh is scaled to (small per-ped variance added). */
	UPROPERTY(EditAnywhere, Category="Pedestrian") float DesiredHeight = 165.f;
	/** True after a complete imported human mesh + locomotion set is active. */
	bool HasRealAvatar() const { return bHasAvatar; }

	/** Must be called before BeginPlay (deferred spawn) to preserve an ejected driver's look. */
	void SetForcedVariant(const FString& VariantName) { ForcedVariant = VariantName; }

	/** Immediately leave normal navigation and flee from a danger source. */
	void StartFleeFrom(const AActor* DangerActor, float DurationSeconds = 4.f);

	bool IsCrossingRoad() const { return State == EPedState::Cross; }
	bool IsFleeing() const { return State == EPedState::Flee; }
	bool IsWaitingToCross() const { return State == EPedState::WaitToCross; }
	float GetContinuousOffRoadTime() const { return ContinuousOffRoadTime; }
	const FString& GetActiveVariant() const { return ActiveVariant; }

private:
	EPedState State = EPedState::WalkEdge;
	int32 BlockX = 0;
	int32 BlockY = 0;
	int32 CornerIndex = 0;   // corner we're heading to (0..3)
	int32 PendingBlockX = 0;
	int32 PendingBlockY = 0;
	int32 PendingCornerIndex = 0;
	int32 WalkDir = 1;       // +1 / -1: direction around the block perimeter
	FVector TargetPoint = FVector::ZeroVector;
	FVector FleeDir = FVector::ForwardVector;
	float BaseSpeed = 140.f;
	float StateTimer = 0.f;
	float DangerScanTimer = 0.f;
	float CurbScanTimer = 0.f;
	float StuckTimer = 0.f;
	float ContinuousOffRoadTime = 0.f;
	bool bCrossingAlongX = false;
	int32 CrossingIntersectionX = 0;
	int32 CrossingIntersectionY = 0;

	// --- Imported human avatar (mesh + looping clips, single-node mode) ---
	UPROPERTY() TObjectPtr<UAnimSequence> IdleAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> WalkAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> JogAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> CurrentAnim;
	UPROPERTY() FString ForcedVariant;
	UPROPERTY() FString ActiveVariant;
	bool bHasAvatar = false;

	/** Pick a random avatar variant and apply its mesh + animation set. */
	void InitializeAppearance();

	/** Swap Idle/Walk/Jog by current speed and scale the walk play rate. */
	void UpdateAnimation();

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
	bool HasPedestrianRightOfWay() const;
	void EnforceSidewalkBoundary(float DeltaSeconds);
	void ContainFleeDirection();
	void CheckForDanger();
};
