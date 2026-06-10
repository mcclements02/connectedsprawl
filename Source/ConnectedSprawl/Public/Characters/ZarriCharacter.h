// The Connected Sprawl - Zarri on foot.
// When he enters his car, possession transfers to the MobileOfficeVehicle.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZarriCharacter.generated.h"

class UAnimSequence;
class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UInputMappingContext;
class ASprawlCar;
struct FInputActionValue;

/**
 * AZarriCharacter
 * ---------------
 * Third-person on-foot character. Key interactions:
 *   - Move, look, sprint, jump
 *   - "Interact" to enter the Mobile Office when near it
 *   - Answer a phone call (scripted, same interact key)
 */
UCLASS()
class CONNECTEDSPRAWL_API AZarriCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AZarriCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Look for a nearby vehicle, possess it, leave Zarri behind as an actor. */
	UFUNCTION(BlueprintCallable, Category="Zarri")
	void EnterNearbyVehicle();

	/** Exit the vehicle back onto the sidewalk. Called by the vehicle side. */
	UFUNCTION(BlueprintCallable, Category="Zarri")
	void OnExitedVehicle(ASprawlCar* FromVehicle);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCameraComponent> FollowCamera;

	// --- Enhanced Input assets (assigned in BP) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input") TObjectPtr<UInputAction> IA_Move;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input") TObjectPtr<UInputAction> IA_Look;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input") TObjectPtr<UInputAction> IA_Jump;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input") TObjectPtr<UInputAction> IA_Sprint;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input") TObjectPtr<UInputAction> IA_Interact;

	/** Radius within which Zarri can enter a vehicle. */
	UPROPERTY(EditAnywhere, Category="Zarri") float InteractRadius = 550.f;

	// --- Visual Equipment Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Equipment")
	TObjectPtr<USkeletalMeshComponent> MobileOfficeRigComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Equipment")
	TObjectPtr<UStaticMeshComponent> RunwayTrackerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Equipment")
	TObjectPtr<UStaticMeshComponent> CommDeviceComponent;

	// --- Equipment Assets and Sockets ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zarri|Equipment")
	TObjectPtr<USkeletalMesh> MobileOfficeRigAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zarri|Equipment")
	FName MobileOfficeRigSocket = TEXT("spine_03");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zarri|Equipment")
	TObjectPtr<UStaticMesh> RunwayTrackerAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zarri|Equipment")
	FName RunwayTrackerSocket = TEXT("wrist_l");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zarri|Equipment")
	TObjectPtr<UStaticMesh> CommDeviceAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zarri|Equipment")
	FName CommDeviceSocket = TEXT("head");

	// --- Dynamic Material Properties ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zarri|Equipment")
	int32 RunwayTrackerMaterialIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zarri|Equipment")
	int32 CommDeviceMaterialIndex = 0;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RunwayTrackerMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CommDeviceMID;

	// --- Hero avatar (imported "Cappy" look; mannequin fallback) ---
	/** Avatar variant folder under /Game/Pedestrians/ used for Zarri's look. */
	UPROPERTY(EditAnywhere, Category="Zarri|Appearance") FString HeroVariant = TEXT("Cappy");
	/** Standing height (cm) the hero mesh is scaled to. */
	UPROPERTY(EditAnywhere, Category="Zarri|Appearance") float HeroHeight = 178.f;

	UPROPERTY() TObjectPtr<UAnimSequence> HeroIdleAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> HeroWalkAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> HeroJogAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> HeroCurrentAnim;
	bool bHasHeroAvatar = false;

	/** Swap in the imported hero avatar if the art has been imported. */
	void InitializeHeroAvatar();

	/** Single-node Idle/Walk/Jog switching for the hero avatar. */
	void UpdateHeroAnimation();

	void InitializeEquipment();
	void UpdateEquipmentVisuals(float DeltaSeconds);

	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleSprint(const FInputActionValue& Value);
	void HandleStopSprint(const FInputActionValue& Value);
	void HandleInteract(const FInputActionValue& Value);

	/** Legacy direct key handler — reliable interact regardless of Enhanced Input. */
	void OnInteractKey();

	ASprawlCar* FindNearbyVehicle() const;
};
