// The Connected Sprawl - A drivable physics car.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
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

protected:
	UPROPERTY(VisibleAnywhere, Category="Car") TObjectPtr<UBoxComponent> Hull;
	UPROPERTY(VisibleAnywhere, Category="Car") TObjectPtr<UStaticMeshComponent> FullBodyMesh;
	UPROPERTY(VisibleAnywhere, Category="Car") TObjectPtr<USkeletalMeshComponent> FullSkeletalBodyMesh;
	UPROPERTY(VisibleAnywhere, Category="Car") TObjectPtr<UStaticMeshComponent> BodyMesh;
	UPROPERTY(VisibleAnywhere, Category="Car") TObjectPtr<UStaticMeshComponent> CabinMesh;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> BodyPaintMeshes;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> DetailMeshes;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> WheelMeshes;
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

	void HandleMove(const FInputActionValue& Value);
	void HandleMoveEnd(const FInputActionValue& Value);
	void HandleExit(const FInputActionValue& Value);

	/** Legacy direct key handler — reliable exit regardless of Enhanced Input. */
	void OnExitKey();

	void SetKitbashVisible(bool bVisible);
};
