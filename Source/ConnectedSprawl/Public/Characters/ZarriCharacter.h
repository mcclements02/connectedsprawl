// The Connected Sprawl - Zarri on foot.
// When he enters his car, possession transfers to the MobileOfficeVehicle.

#pragma once

#include "Characters/SprawlMeleeModule.h"
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
class USprawlHumanCharacterModule;
class USprawlLocomotionComponent;
class USprawlStreetwearModule;
class USprawlWardrobeModule;
class ASprawlCar;
struct FSprawlMeleeInput;
struct FInputActionValue;

/**
 * AZarriCharacter
 * ---------------
 * Third-person on-foot character. Key interactions:
 *   - Move, look, sprint, jump
 *   - "Interact" to enter buildings or the Mobile Office when near them
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

	/** Phone, building doorway, then nearby vehicle in deterministic priority. */
	UFUNCTION(BlueprintCallable, Category="Zarri")
	bool TryContextInteraction();

	/** Building-only context label used by the native desktop HUD. */
	bool GetBuildingInteractionPrompt(FText& OutPrompt) const;

	/** True when an enterable vehicle is in reach (drives the HUD hint). */
	UFUNCTION(BlueprintCallable, Category="Zarri")
	bool HasNearbyEnterableVehicle() const { return FindNearbyVehicle() != nullptr; }

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

	/** Tight, stable full-body framing used only by -SprawlAuditWardrobe. */
	void PrepareWardrobeVisualAudit();

	/** Shared identity/action state used by Zarri and every city resident. */
	UFUNCTION(BlueprintPure, Category="Zarri|Appearance")
	USprawlHumanCharacterModule* GetHumanCharacterModule() const
	{
		return HumanCharacter;
	}

	/** Real clothing physics simulation and fabric solver profile module. */
	UFUNCTION(BlueprintPure, Category="Zarri|Cloth Physics")
	class USprawlClothPhysicsModule* GetClothPhysicsModule() const { return ClothPhysics; }

	/** Authored hoodie, bomber, cargo, and beanie presentation. */
	UFUNCTION(BlueprintPure, Category="Zarri|Streetwear")
	USprawlStreetwearModule* GetStreetwearModule() const { return Streetwear; }

	/** Asset-independent punch/kick gameplay and replicated attack state. */
	UFUNCTION(BlueprintPure, Category="Zarri|Melee")
	USprawlMeleeModule* GetMeleeModule() const { return Melee; }

	UFUNCTION(BlueprintCallable, Category="Zarri|Melee")
	bool TryPunch();

	UFUNCTION(BlueprintCallable, Category="Zarri|Melee")
	bool TryKick();

	/** Shared desktop/gamepad/touch action: alternate punch then kick. */
	UFUNCTION(BlueprintCallable, Category="Zarri|Melee")
	bool TryMeleeAttack();

	/** Sprint toggle shared by keyboard, gamepad, and the touch HUD. */
	UFUNCTION(BlueprintCallable, Category="Zarri")
	void SetSprinting(bool bSprinting);

protected:
	friend struct FSprawlMeleeInput;

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

	/**
	 * Owns gait selection and body facing for whichever visual is active, so
	 * neither the MetaHuman nor the fallback avatar can drift out of line with
	 * the direction Zarri is actually travelling.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USprawlLocomotionComponent> Locomotion;

	/** Replicated stand/walk/run/talk/sit/drive intent and customization. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USprawlHumanCharacterModule> HumanCharacter;

	/** Complete fitted clothing, shoes/socks, and optional accessory layers. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USprawlWardrobeModule> Wardrobe;

	/** Fits project-owned static streetwear pieces to the live MetaHuman. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USprawlStreetwearModule> Streetwear;

	/** Real clothing physics simulation and fabric solver profile module. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<class USprawlClothPhysicsModule> ClothPhysics;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USprawlMeleeModule> Melee;

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

	// --- GTA-style follow camera ---
	/** Swing the camera behind the movement direction when look input is idle. */
	UPROPERTY(EditAnywhere, Category="Zarri|Camera") bool bAutoFollowCamera = true;
	/** Seconds after the last look input before auto-follow resumes. */
	UPROPERTY(EditAnywhere, Category="Zarri|Camera") float FollowCameraDelay = 1.1f;
	/** How assertively the camera eases behind the travel direction. */
	UPROPERTY(EditAnywhere, Category="Zarri|Camera") float FollowCameraTurnSpeed = 2.6f;
	/** Resting pitch the follow camera settles toward while moving. */
	UPROPERTY(EditAnywhere, Category="Zarri|Camera") float FollowCameraRestPitch = -12.f;

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
	UPROPERTY(EditAnywhere, Category="Zarri|Appearance|MetaHuman|Animation")
	TSoftObjectPtr<UAnimSequence> MetaHumanTalkAnim;
	UPROPERTY(EditAnywhere, Category="Zarri|Appearance|MetaHuman|Animation")
	TSoftObjectPtr<UAnimSequence> MetaHumanSitAnim;
	UPROPERTY(EditAnywhere, Category="Zarri|Appearance|MetaHuman|Animation")
	TSoftObjectPtr<UAnimSequence> MetaHumanDriveAnim;
	/** Optional skeleton-compatible one-shots; gameplay remains active when unset. */
	UPROPERTY(EditAnywhere, Category="Zarri|Appearance|MetaHuman|Animation")
	TSoftObjectPtr<UAnimSequence> MetaHumanPunchAnim;
	UPROPERTY(EditAnywhere, Category="Zarri|Appearance|MetaHuman|Animation")
	TSoftObjectPtr<UAnimSequence> MetaHumanKickAnim;

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
	UPROPERTY() TObjectPtr<UAnimSequence> HeroTalkAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> HeroSitAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> HeroPunchAnim;
	UPROPERTY() TObjectPtr<UAnimSequence> HeroKickAnim;
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
	TObjectPtr<UAnimSequence> LoadedMetaHumanTalkAnim;
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> LoadedMetaHumanSitAnim;
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> LoadedMetaHumanDriveAnim;
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> LoadedMetaHumanPunchAnim;
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> LoadedMetaHumanKickAnim;

	float LastLookInputTime = -100.f;

	// --- On-foot boundary rescue (cars have their own EnforceCityBoundary) ---
	FVector LastSafeFootLocation = FVector::ZeroVector;
	bool bHasSafeFootLocation = false;
	float SafeFootLocationTimer = 0.f;

	/** Ease control yaw behind the travel direction after look input goes idle. */
	void UpdateFollowCamera(float DeltaSeconds);

	/** Teleport back to the last safe sidewalk spot after a fall or escape. */
	void UpdateBoundaryRescue(float DeltaSeconds);

	/** Swap in the imported hero avatar if the art has been imported. */
	void InitializeHeroAvatar();

	/** Point the locomotion component back at the imported avatar body. */
	void SetupFallbackLocomotion();

	/** Feed the locomotion component this frame's ground speed. */
	void UpdateHeroAnimation();

	// --- Facing audit (-SprawlLocomotionAudit) ---
	/** Sweep known yaws and assert the body faces the pawn's forward. */
	void RunLocomotionAudit();
	bool bLocomotionAuditEnabled = false;
	/** -SprawlAuditRun: hold forward input so captures show him running. */
	bool bAuditRunForward = false;

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

	UFUNCTION()
	void HandleMeleeAttackStarted(
		ESprawlMeleeAttack Attack, float RecoverySeconds, bool bHitCharacter);

	/** Legacy direct key handler — reliable interact regardless of Enhanced Input. */
	void OnInteractKey();

	/** Pawn-layer melee route used by keyboard X and the gamepad X button. */
	void OnMeleeKey();

	/** -SprawlMeleeInputAudit: verify live pawn bindings and first attack. */
	void RunMeleeInputAudit();

	ASprawlCar* FindNearbyVehicle() const;
};
