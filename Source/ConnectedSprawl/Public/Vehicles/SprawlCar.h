// The Connected Sprawl - A drivable physics car.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "World/SprawlCityGridSubsystem.h"
#include "SprawlCar.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UInputMappingContext;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class AZarriCharacter;
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
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	virtual void PossessedBy(AController* NewController) override;

	/** Set by ZarriCharacter when he climbs in, so the car can hand control back. */
	void SetDriver(AZarriCharacter* InDriver) { Driver = InDriver; }

	/** Paint the body panels a given colour (called per-instance on spawn). */
	UFUNCTION(BlueprintCallable, Category="Car")
	void SetBodyMaterial(class UMaterialInterface* Mat);

	/** Replace the prototype kitbash with a marketplace/imported vehicle mesh. */
	UFUNCTION(BlueprintCallable, Category="Car")
	void SetExternalVehicleMesh(UStaticMesh* Mesh, FVector RelativeLocation,
		FRotator RelativeRotation, FVector RelativeScale);

	/** Replace the prototype kitbash with a marketplace/imported skeletal vehicle mesh. */
	UFUNCTION(BlueprintCallable, Category="Car")
	void SetExternalSkeletalVehicleMesh(USkeletalMesh* Mesh, FVector RelativeLocation,
		FRotator RelativeRotation, FVector RelativeScale);

	/** Return to the built-in lightweight prototype car pieces. */
	UFUNCTION(BlueprintCallable, Category="Car")
	void ClearExternalVehicleMesh();

	/** Step the driver back out onto the street. */
	void RequestExit();

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
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> BodyPaintMeshes;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> DetailMeshes;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> WheelMeshes;
	UPROPERTY() TObjectPtr<UMaterialInterface> BodyPaintMaterial;
	UPROPERTY(VisibleAnywhere, Category="Car") TObjectPtr<USpringArmComponent> SpringArm;
	UPROPERTY(VisibleAnywhere, Category="Car") TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY() TObjectPtr<UInputMappingContext> DefaultMappingContext;
	UPROPERTY() TObjectPtr<UInputAction> IA_Move;
	UPROPERTY() TObjectPtr<UInputAction> IA_Interact;

	/** Forward push, in centinewtons-ish; tuned by feel. */
	UPROPERTY(EditAnywhere, Category="Car") float EngineForce = 2400000.f;
	/** Yaw rate at speed, deg/s. */
	UPROPERTY(EditAnywhere, Category="Car") float TurnRate = 62.f;

	UPROPERTY() TObjectPtr<AZarriCharacter> Driver;

	float ThrottleInput = 0.f;
	float SteerInput = 0.f;

	// --- AI driving state (lane-following on the city grid) ---
	ESprawlHeading AIHeading = ESprawlHeading::North;
	int32 AIRoadIndex = 0;            // road we're travelling on (current axis)
	bool  bAIStateSeeded = false;
	int32 AIDecidedCrossing = -1;     // crossing-road index we've chosen a move for
	int32 AIPendingTurn = 0;          // -1 = left, 0 = straight, +1 = right
	float AISmoothedSpeed = 0.f;      // smoothed speed command (cm/s)
	FVector2D AILastIntersection = FVector2D(1.e9f, 1.e9f); // center of last crossing we executed

	/** Lane-follow the road grid: keep lane, obey signals, avoid traffic. */
	void RunAutoDrive(float DeltaSeconds);

	/** Pick straight/left/right at the upcoming crossing, avoiding lake/map edge. */
	void DecideIntersectionMove(int32 CrossingRoadIndex);

	/** Heading + road index after applying a turn choice at a crossing. */
	static void ResolveMove(ESprawlHeading InHeading, int32 InRoadIndex, int32 CrossingRoadIndex,
		int32 Turn, ESprawlHeading& OutHeading, int32& OutRoadIndex);

	/** Distance to the nearest blocking car/pedestrian ahead, or -1 if clear. */
	float SenseObstacleAhead(float SenseLength) const;

	void HandleMove(const FInputActionValue& Value);
	void HandleMoveEnd(const FInputActionValue& Value);
	void HandleExit(const FInputActionValue& Value);

	/** Legacy direct key handler — reliable exit regardless of Enhanced Input. */
	void OnExitKey();

	void SetKitbashVisible(bool bVisible);
};
