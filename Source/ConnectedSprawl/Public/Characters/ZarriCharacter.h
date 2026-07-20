// The Connected Sprawl - Zarri on foot.
// When he enters his car, possession transfers to the MobileOfficeVehicle.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZarriCharacter.generated.h"

class UAnimSequence;
class UCameraComponent;
class UChildActorComponent;
class USpringArmComponent;
class UInputAction;
class UInputMappingContext;
class USkeletalMeshComponent;
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

	/** Possess a specific vehicle; used by interaction and save restoration. */
	UFUNCTION(BlueprintCallable, Category="Zarri")
	bool EnterVehicle(ASprawlCar* Vehicle);

	/** Exit the vehicle back onto the sidewalk. Called by the vehicle side. */
	UFUNCTION(BlueprintCallable, Category="Zarri")
	void OnExitedVehicle(ASprawlCar* FromVehicle);

	/** Imported avatar currently used by Zarri and his in-car seat visual. */
	const FString& GetActiveHeroVariant() const { return ActiveHeroVariant; }

	/** True when the assembled MetaHuman is driving the on-foot visual. */
	UFUNCTION(BlueprintPure, Category="Zarri|Appearance")
	bool IsUsingMetaHumanVisual() const { return bUsingMetaHumanVisual; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCameraComponent> FollowCamera;

	/**
	 * Optional assembled MetaHuman actor. The native character remains the
	 * movement/collision/camera authority, so a missing or invalid asset falls
	 * back to the lightweight Zarri avatar without affecting playability.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UChildActorComponent> MetaHumanVisualComponent;

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

	// --- Hero avatar (dedicated Zarri look; Cappy/mannequin fallbacks) ---
	/** Avatar variant folder under /Game/Pedestrians/ used for Zarri's look. */
	UPROPERTY(EditAnywhere, Category="Zarri|Appearance") FString HeroVariant = TEXT("Zarri");
	/** Standing height (cm) the hero mesh is scaled to. */
	UPROPERTY(EditAnywhere, Category="Zarri|Appearance") float HeroHeight = 178.f;

	/** UE 5.8 assembled MetaHuman actor generated by MetaHuman Creator. */
	UPROPERTY(EditAnywhere, Category="Zarri|Appearance|MetaHuman")
	TSoftClassPtr<AActor> MetaHumanVisualClass;

	/** Native MetaHuman locomotion clips supplied by Creator Core Data. */
	UPROPERTY(EditAnywhere, Category="Zarri|Appearance|MetaHuman|Animation")
	TSoftObjectPtr<UAnimSequence> MetaHumanIdleAnim;
	UPROPERTY(EditAnywhere, Category="Zarri|Appearance|MetaHuman|Animation")
	TSoftObjectPtr<UAnimSequence> MetaHumanWalkAnim;
	UPROPERTY(EditAnywhere, Category="Zarri|Appearance|MetaHuman|Animation")
	TSoftObjectPtr<UAnimSequence> MetaHumanRunAnim;

	/** Capsule-root-relative transform for the assembled actor (feet at Z=0). */
	UPROPERTY(EditAnywhere, Category="Zarri|Appearance|MetaHuman")
	FTransform MetaHumanRelativeTransform = FTransform(
		FRotator::ZeroRotator, FVector(0.f, 0.f, -92.f), FVector::OneVector);

	/** Standard MetaHuman body sockets used by Zarri's equipment. */
	UPROPERTY(EditAnywhere, Category="Zarri|Appearance|MetaHuman")
	FName MetaHumanRigSocket = TEXT("spine_05");
	UPROPERTY(EditAnywhere, Category="Zarri|Appearance|MetaHuman")
	FName MetaHumanTrackerSocket = TEXT("hand_l");
	UPROPERTY(EditAnywhere, Category="Zarri|Appearance|MetaHuman")
	FName MetaHumanCommSocket = TEXT("head");

	UPROPERTY() TObjectPtr<UAnimSequence> HeroIdleAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> HeroWalkAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> HeroJogAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> HeroSprintAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> HeroCurrentAnim;
	FString ActiveHeroVariant = TEXT("Zarri");
	bool bHasHeroAvatar = false;
	bool bUsingMetaHumanVisual = false;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> MetaHumanBodyComponent;
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> LoadedMetaHumanIdleAnim;
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> LoadedMetaHumanWalkAnim;
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> LoadedMetaHumanRunAnim;
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> MetaHumanCurrentAnim;

	/** Swap in the imported hero avatar if the art has been imported. */
	void InitializeHeroAvatar();

	/** Single-node locomotion switching for either visual implementation. */
	void UpdateHeroAnimation();

	/** Activate the assembled visual only after its body and animation validate. */
	bool TryInitializeMetaHumanVisual();

	/** Keep fallback and MetaHuman visibility coherent during vehicle transfer. */
	void SetCharacterVisualHidden(bool bHidden);

	/** Reattach Zarri's equipment to either the fallback or MetaHuman skeleton. */
	void AttachEquipmentToVisual(USkeletalMeshComponent* TargetMesh, bool bMetaHumanSockets);

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
