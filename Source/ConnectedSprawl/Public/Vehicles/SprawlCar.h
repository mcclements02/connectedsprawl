// The Connected Sprawl - A drivable physics car.

#pragma once

#include "CoreMinimal.h"
#include "AI/SprawlTrafficRoute.h"
#include "GameFramework/Pawn.h"
#include "World/SprawlCityGridSubsystem.h"
#include "SprawlCar.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UInputMappingContext;
class USkeletalMesh;
class USkeletalMeshComponent;
class UAnimSequence;
class UStaticMesh;
class UMaterialInterface;
class UPrimitiveComponent;
class AZarriCharacter;
class ASprawlPedestrian;
struct FInputActionValue;

/**
 * ASprawlCar
 * ----------
 * A drivable car built on rigid-body physics (no Chaos skeletal rig needed):
 * a box physics body with bolt-on visual panels, arcade-style throttle/steer.
 * Zarri enters via his Interact key; Interact again steps back out.
 */
UCLASS()
class CONNECTEDSPRAWL_API ASprawlCar : public APawn
{
	GENERATED_BODY()

public:
	ASprawlCar();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	virtual void PossessedBy(AController* NewController) override;

	/** A stopped, unoccupied parked car can be entered directly. */
	UFUNCTION(BlueprintPure, Category="Car")
	bool CanBeEntered() const;

	/** A slow occupied traffic car can be taken after it has safely stopped. */
	UFUNCTION(BlueprintPure, Category="Car")
	bool CanBeCarjacked() const;

	/** Claim the driver seat and suspend AI until the player exits. */
	bool AssignDriver(AZarriCharacter* InDriver);

	/** Eject the logical AI driver, retire the traffic car, and seat Zarri. */
	bool CarjackDriver(AZarriCharacter* InDriver);

	/** Finalize side effects only after the player controller owns this car. */
	void ConfirmDriverEntry();

	UFUNCTION(BlueprintPure, Category="Car|Occupant")
	bool HasAIDriver() const { return bHasAIDriver; }

	UFUNCTION(BlueprintPure, Category="Car|Occupant")
	bool HasVisibleDriver() const;

	/** True when a logical occupant is seated and the mounted mesh is suppressed. */
	UFUNCTION(BlueprintPure, Category="Car|Occupant")
	bool HasHiddenSeatedDriver() const;

	/** Legacy containment diagnostic; hidden mounted drivers return false. */
	bool HasContainedDriverVisual() const;

	const FString& GetAIDriverVariant() const { return AIDriverVariant; }
	ASprawlPedestrian* GetLastEjectedDriver() const
	{
		return LastEjectedDriver.Get();
	}

	bool HasActiveIntersectionReservation() const
	{
		return AIReservedIntersectionX >= 0 || AIReservedIntersectionY >= 0
			|| bAIClearingIntersection;
	}

	/** Planned directed lane, used by traffic diagnostics instead of actor yaw. */
	bool GetAITravelState(ESprawlHeading& OutHeading, int32& OutRoadIndex) const
	{
		if (!bAIStateSeeded)
		{
			return false;
		}
		OutHeading = AIHeading;
		OutRoadIndex = AIRoadIndex;
		return true;
	}

	/** Invalid edge/topology states stop and ask the manager for a safe recycle. */
	bool NeedsTrafficRecycle() const { return bNeedsTrafficRecycle; }
	bool IsGarageEgressActive() const { return bGarageEgressActive; }
	int32 GetAICompletedTurnCount() const { return AICompletedTurnCount; }
	int32 GetAICompletedUTurnCount() const { return AICompletedUTurnCount; }

	/**
	 * Start behind a parking-deck facade and follow a slow driveway path to a
	 * legal directed lane. Normal traffic routing begins only after the merge.
	 */
	bool BeginGarageEgress(FName ExitId, const TArray<FVector>& WorldPath,
		ESprawlHeading MergeHeading, int32 MergeRoadIndex);

	/** Shared obstruction-tested side-door exit used by Zarri and ejected drivers. */
	bool FindClearSideExit(float CapsuleRadius, float CapsuleHalfHeight,
		FVector& OutLocation, const AActor* ActorToIgnore = nullptr,
		bool bPreferDriverSide = true) const;

	UFUNCTION(BlueprintPure, Category="Car|Safety")
	bool IsCrashRecoveryActive() const { return CrashUprightGraceRemaining > 0.f; }

	UFUNCTION(BlueprintPure, Category="Car|Safety")
	bool IsWithinCityBounds() const;

	/** Paint the body panels a given colour (called per-instance on spawn). */
	UFUNCTION(BlueprintCallable, Category="Car")
	void SetBodyMaterial(class UMaterialInterface* Mat);

	/** Replace the prototype kitbash with a marketplace/imported vehicle mesh. */
	UFUNCTION(BlueprintCallable, Category="Car")
	void SetExternalVehicleMesh(UStaticMesh* Mesh, FVector RelativeLocation,
		FRotator RelativeRotation, FVector RelativeScale);

	/**
	 * Install separately imported body/detail meshes and four wheel meshes.
	 * WheelMeshes and WheelCenters use FL, FR, RL, RR order; all geometry keeps
	 * the FBX's local coordinates. This retains lights, glass, mirrors and
	 * grilles while allowing true wheel spin and front-wheel steering.
	 */
	UFUNCTION(BlueprintCallable, Category="Car")
	void SetExternalVehicleParts(const TArray<UStaticMesh*>& ExternalBodyMeshes,
		const TArray<UStaticMesh*>& ExternalWheelMeshes,
		const TArray<FVector>& ExternalWheelCenters,
		FVector RelativeLocation, FRotator RelativeRotation, FVector RelativeScale);

	/** Replace the prototype kitbash with a marketplace/imported skeletal vehicle mesh. */
	UFUNCTION(BlueprintCallable, Category="Car")
	void SetExternalSkeletalVehicleMesh(USkeletalMesh* Mesh, FVector RelativeLocation,
		FRotator RelativeRotation, FVector RelativeScale);

	/** Return to the built-in lightweight prototype car pieces. */
	UFUNCTION(BlueprintCallable, Category="Car")
	void ClearExternalVehicleMesh();

	UFUNCTION(BlueprintPure, Category="Car")
	bool HasAnimatedWheelParts() const { return bUsingExternalWheelParts; }
	float GetVisualWheelRotationDegrees() const { return WheelRotationDegrees; }

	/** Unconditionally step the driver out; input-facing phone handling lives elsewhere. */
	void RequestExit();

	/** Touch-HUD pedals: +1 gas, -1 brake/reverse, 0 released. Overrides stick Y. */
	UFUNCTION(BlueprintCallable, Category="Car|Input")
	void SetTouchThrottle(float InThrottle);

	/** If true and no player is in the seat, the car drives itself around the grid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Car|AI") bool bAutoDrive = false;
	/** Cruise speed (cm/s) for AI-driven cars on open road. */
	UPROPERTY(EditAnywhere, Category="Car|AI") float AICruiseSpeed = 850.f;
	/** Speed (cm/s) while turning through an intersection. */
	UPROPERTY(EditAnywhere, Category="Car|AI") float AITurnSpeed = 340.f;

protected:
	UPROPERTY(VisibleAnywhere, Category="Car") TObjectPtr<UBoxComponent> Hull;
	UPROPERTY(VisibleAnywhere, Category="Car") TObjectPtr<UStaticMeshComponent> FullBodyMesh;
	UPROPERTY(VisibleAnywhere, Category="Car") TObjectPtr<USkeletalMeshComponent> FullSkeletalBodyMesh;
	UPROPERTY(VisibleAnywhere, Category="Car") TObjectPtr<UStaticMeshComponent> BodyMesh;
	UPROPERTY(VisibleAnywhere, Category="Car") TObjectPtr<UStaticMeshComponent> CabinMesh;
	/** Legacy mounted occupant component; policy keeps it hidden and unticked. */
	UPROPERTY(VisibleAnywhere, Category="Car|Occupant") TObjectPtr<USkeletalMeshComponent> DriverMesh;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> BodyPaintMeshes;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> DetailMeshes;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> ExternalBodyDetailMeshes;
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> WheelPivots;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> WheelMeshes;
	UPROPERTY() TObjectPtr<UStaticMesh> PrototypeWheelMesh;
	UPROPERTY() TObjectPtr<UMaterialInterface> BodyPaintMaterial;
	UPROPERTY(VisibleAnywhere, Category="Car") TObjectPtr<USpringArmComponent> SpringArm;
	UPROPERTY(VisibleAnywhere, Category="Car") TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY() TObjectPtr<UInputMappingContext> DefaultMappingContext;
	UPROPERTY() TObjectPtr<UInputAction> IA_Move;
	UPROPERTY() TObjectPtr<UInputAction> IA_Interact;

	/** Forward push, in centinewtons-ish; tuned by feel. */
	UPROPERTY(EditAnywhere, Category="Car") float EngineForce = 3200000.f;
	/** Yaw rate at speed, deg/s. */
	UPROPERTY(EditAnywhere, Category="Car") float TurnRate = 78.f;
	/** How quickly sideways velocity is gripped away (higher = more planted). */
	UPROPERTY(EditAnywhere, Category="Car") float LateralGripResponse = 7.5f;
	/** Extra engine-force multiplier while braking against current travel. */
	UPROPERTY(EditAnywhere, Category="Car") float BrakeForceMultiplier = 1.8f;
	/** How quickly throttle follows stick/buttons; higher feels snappier. */
	UPROPERTY(EditAnywhere, Category="Car|Input") float ThrottleResponse = 8.f;
	/** How quickly steering follows stick/buttons; higher feels snappier. */
	UPROPERTY(EditAnywhere, Category="Car|Input") float SteeringResponse = 11.f;
	/** Small analog drift filter for touch/gamepad steering. */
	UPROPERTY(EditAnywhere, Category="Car|Input") float InputDeadZone = 0.08f;
	/** Visual tyre radius used to match spin rate to ground speed. */
	UPROPERTY(EditAnywhere, Category="Car|Wheels") float VisualWheelRadius = 41.f;
	/** Maximum visual steering angle of the front wheels. */
	UPROPERTY(EditAnywhere, Category="Car|Wheels") float VisualSteeringAngle = 30.f;
	/** Maximum speed at which a pedestrian may safely take the driver seat. */
	UPROPERTY(EditAnywhere, Category="Car|Safety") float MaxEntrySpeed = 220.f;
	/** Maximum speed for pulling an AI driver from a traffic car. */
	UPROPERTY(EditAnywhere, Category="Car|Safety") float MaxCarjackSpeed = 260.f;
	UPROPERTY(EditAnywhere, Category="Car|Occupant") FVector DriverSeatLocation = FVector(10.f, -45.f, -20.f);
	UPROPERTY(EditAnywhere, Category="Car|Occupant") FRotator DriverSeatRotation = FRotator(0.f, -90.f, 0.f);
	UPROPERTY(EditAnywhere, Category="Car|Occupant") float DriverVisualHeight = 168.f;
	/** Minimum impact impulse and speed that count as a crash. */
	UPROPERTY(EditAnywhere, Category="Car|Safety") float CrashImpulseThreshold = 180000.f;
	UPROPERTY(EditAnywhere, Category="Car|Safety") float CrashSpeedThreshold = 700.f;
	/** Let a severe crash roll naturally for this long before self-righting. */
	UPROPERTY(EditAnywhere, Category="Car|Safety") float CrashUprightGraceSeconds = 2.25f;

	UPROPERTY() TObjectPtr<AZarriCharacter> Driver;
	UPROPERTY() TObjectPtr<UAnimSequence> DriverCurrentAnim;
	UPROPERTY() FString AIDriverVariant;
	UPROPERTY() bool bHasAIDriver = false;
	UPROPERTY() TWeakObjectPtr<ASprawlPedestrian> LastEjectedDriver;
	bool bDriverVisualInitializationAttempted = false;
	bool bPendingCarjack = false;
	FString PendingCarjackVariant;

	float ThrottleInput = 0.f;
	float SteerInput = 0.f;
	float TargetThrottleInput = 0.f;
	float TargetSteerInput = 0.f;
	float TouchThrottleInput = 0.f;
	float WheelRotationDegrees = 0.f;
	UPROPERTY() bool bUsingExternalWheelParts = false;
	bool bResumeAutoDriveAfterExit = false;
	float CrashUprightGraceRemaining = 0.f;
	FTransform LastSafeTransform = FTransform::Identity;
	bool bHasLastSafeTransform = false;

	// --- AI driving state (lane-following on the city grid) ---
	ESprawlHeading AIHeading = ESprawlHeading::North;
	int32 AIRoadIndex = 0;            // road we're travelling on (current axis)
	bool  bAIStateSeeded = false;
	int32 AIDecidedCrossing = -1;     // crossing-road index we've chosen a move for
	ESprawlTrafficManeuver AIPendingManeuver = ESprawlTrafficManeuver::Straight;
	float AISmoothedSpeed = 0.f;      // smoothed speed command (cm/s)
	FVector2D AILastIntersection = FVector2D(1.e9f, 1.e9f); // center of last crossing we executed
	int32 AIReservedIntersectionX = -1;
	int32 AIReservedIntersectionY = -1;
	bool bAIClearingIntersection = false;
	bool bNeedsTrafficRecycle = false;
	int32 AICompletedTurnCount = 0;
	int32 AICompletedUTurnCount = 0;
	bool bGarageEgressActive = false;
	FName AIGarageExitId = NAME_None;
	TArray<FVector> AIGarageEgressPath;
	int32 AIGarageEgressPoint = INDEX_NONE;
	ESprawlHeading AIGarageMergeHeading = ESprawlHeading::East;
	int32 AIGarageMergeRoadIndex = INDEX_NONE;

	/** Lane-follow the road grid: keep lane, obey signals, avoid traffic. */
	void RunAutoDrive(float DeltaSeconds);
	/** Low-speed covered-driveway phase before lane following is seeded. */
	void RunGarageEgress(float DeltaSeconds);
	void CancelGarageEgress(bool bRequestTrafficRecycle);

	/** Lazily assign deterministic logical occupancy to an active traffic car. */
	void InitializeAIDriverVisual();
	void ApplyDriverVisibilityPolicy();
	void HideDriverVisual();
	bool AssignDriverInternal(AZarriCharacter* InDriver, bool bResumeAIOnExit);
	void CompletePendingCarjack();
	void RestorePendingCarjack();
	void ExitDriver();

	/** Keep normal driving level, but allow a short physical reaction after a crash. */
	void MaintainUpright(float DeltaSeconds);

	/** Physical walls are primary; this restores a car if tunnelling escapes them. */
	void EnforceCityBoundary();
	void UpdateSafeRecoveryTransform();

	UFUNCTION()
	void HandleHullHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, FVector NormalImpulse,
		const FHitResult& Hit);

	/** Pick a legal move at the upcoming crossing, including a reserved U-turn. */
	bool DecideIntersectionMove(int32 CrossingRoadIndex);

	/** Distance to the nearest blocking car/pedestrian ahead, or -1 if clear. */
	float SenseObstacleAhead(float SenseLength) const;

	/** True when no vehicle occupies the intersection box or planned exit lane. */
	bool IsIntersectionPathClear(const FVector2D& Center, int32 CrossingRoadIndex) const;

	/** Drop the shared intersection lease, if this car owns one. */
	void ReleaseIntersectionReservation();

	/** Spin all four wheels from signed speed and steer the front pair. */
	void UpdateWheelVisuals(float DeltaSeconds);

	/** Use cooked split vehicle assets for runtime-spawned traffic when present. */
	void TryApplyRuntimeVehicleParts();
	/** Align imported visual geometry to the physics hull's +X forward axis. */
	void NormalizeExternalVisualForward();
	/** Rotate direct visual children around the hull without affecting camera or physics. */
	void ApplyExternalVisualYawCorrection(float CorrectionYawDegrees);

	/**
	 * Lift or drop the whole visual so its lowest point meets the hull's
	 * contact plane — the plane the car physically rests on. Catches bodies
	 * whose offsets were baked into the level by an authoring pass, which a
	 * constructor default can never reach.
	 */
	void SeatVisualOnContactPlane();

	void HandleMove(const FInputActionValue& Value);
	void HandleMoveEnd(const FInputActionValue& Value);
	void HandleExit(const FInputActionValue& Value);

	/** Legacy direct key handler — reliable exit regardless of Enhanced Input. */
	void OnExitKey();

	void SetKitbashVisible(bool bVisible);
	void ClearExternalBodyDetails();
	void ResetPrototypeWheels();
};
