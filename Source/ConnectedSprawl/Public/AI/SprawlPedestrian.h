// The Connected Sprawl - Pedestrian NPC. Walks the sidewalks like a local.

#pragma once

#include "Characters/SprawlHumanCharacterModule.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SprawlPedestrian.generated.h"

class UAnimSequence;
class AActor;
class UChildActorComponent;
class USkeletalMeshComponent;
class USprawlLocomotionComponent;
class USprawlWardrobeModule;

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
	/** True only when an assembled MetaHuman and compatible locomotion are active. */
	bool HasRealAvatar() const { return bUsingMetaHumanVisual; }
	bool IsUsingMetaHumanVisual() const { return bUsingMetaHumanVisual; }
	FName GetAppearanceId() const { return ActiveAppearanceId; }
	FName GetRequestedAppearanceId() const { return RequestedAppearanceId; }
	void SetMetaHumanHighDetail(bool bHighDetail);

	/** Select a stable roster identity for a deferred-spawned logical driver. */
	void SetForcedVariant(const FString& VariantName) { ForcedVariant = VariantName; }

	/** Must be called before BeginPlay so gait and appearance use the authored identity. */
	void SetDevelopedProfile(const FSprawlCharacterProfile& InProfile);

	UFUNCTION(BlueprintPure, Category="Pedestrian|Character")
	FSprawlCharacterProfile GetCharacterProfile() const { return CharacterProfile; }

	/** Shared replicated customization/action state used by every human tier. */
	UFUNCTION(BlueprintPure, Category="Pedestrian|Character")
	USprawlHumanCharacterModule* GetHumanCharacterModule() const
	{
		return HumanCharacter;
	}

	/** Script a stand/talk/sit/drive pose; held stationary poses pause navigation. */
	UFUNCTION(BlueprintCallable, Category="Pedestrian|Character")
	bool SetHumanAction(ESprawlHumanAction Action, bool bHoldAction = true);

	/** Release a scripted pose and rejoin ordinary sidewalk activity. */
	UFUNCTION(BlueprintCallable, Category="Pedestrian|Character")
	void ResumeCityActivity();

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

	// --- Strict assembled-MetaHuman visual tier ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pedestrian|Character",
		meta=(AllowPrivateAccess="true"))
	TObjectPtr<USprawlHumanCharacterModule> HumanCharacter;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pedestrian|Character",
		meta=(AllowPrivateAccess="true"))
	TObjectPtr<USprawlWardrobeModule> Wardrobe;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pedestrian|Character",
		meta=(AllowPrivateAccess="true"))
	TObjectPtr<USprawlLocomotionComponent> Locomotion;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pedestrian|Character",
		meta=(AllowPrivateAccess="true"))
	TObjectPtr<UChildActorComponent> MetaHumanVisualComponent;
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> MetaHumanBodyMesh;
	UPROPERTY() TObjectPtr<UAnimSequence> IdleAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> WalkAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> JogAnim;
	UPROPERTY() FString ForcedVariant;
	UPROPERTY() FString ActiveVariant;
	UPROPERTY() FName RequestedAppearanceId;
	UPROPERTY() FName ActiveAppearanceId;
	UPROPERTY(Transient) FSprawlHumanCustomization ActiveVisualCustomization;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Pedestrian|Character",
		meta=(AllowPrivateAccess="true"))
	FSprawlCharacterProfile CharacterProfile;
	bool bHasDevelopedProfile = false;
	bool bHasActiveVisualCustomization = false;
	bool bUsingMetaHumanVisual = false;
	bool bIdleTalkPose = false;

	/** Resolve replicated roster identity and atomically activate a real human. */
	void InitializeAppearance();

	/** Drive common MetaHuman stand/walk/run clips from semantic action state. */
	void UpdateAnimation();

	UFUNCTION()
	void HandleHumanRuntimeStateChanged(FSprawlHumanRuntimeState RuntimeState);

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
