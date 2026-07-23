// The Connected Sprawl - Zarri on foot.

#include "Characters/ZarriCharacter.h"
#include "Vehicles/SprawlCar.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Founder/FounderSubsystem.h"
#include "Phone/PhoneSubsystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Characters/SprawlAvatarLibrary.h"
#include "Characters/SprawlCharacterRender.h"
#include "Characters/SprawlClothPhysicsModule.h"
#include "Characters/SprawlHumanCharacterModule.h"
#include "Characters/SprawlLocomotionComponent.h"
#include "Characters/SprawlMeleeInput.h"
#include "Characters/SprawlMeleeModule.h"
#include "Characters/SprawlPanelClothModule.h"
#include "Characters/SprawlReferenceClothingModule.h"
#include "Characters/SprawlStreetwearModule.h"
#include "Characters/SprawlWardrobeModule.h"
#include "Engine/Engine.h"
#include "Phone/PhoneSubsystem.h"
#include "World/SprawlCityGridSubsystem.h"
#include "World/SprawlEnterableInteriors.h"
#include "UObject/ConstructorHelpers.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

// File-scope alias (matches the other Sprawl translation units so unity
// builds don't see a local declaration shadowing the global one).
using Grid = USprawlCityGridSubsystem;

AZarriCharacter::AZarriCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(40.f, 92.f);

	// Rotate toward movement, not controller.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate              = FRotator(0.f, 720.f, 0.f);
	Move->JumpZVelocity             = 620.f;
	Move->MaxWalkSpeed              = 520.f;
	Move->MinAnalogWalkSpeed        = 35.f;
	Move->MaxAcceleration           = 3600.f;
	Move->BrakingDecelerationWalking= 2600.f;
	Move->GroundFriction            = 9.f;
	Move->AirControl                = 0.35f;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength       = 430.f;
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bEnableCameraLag       = true;
	SpringArm->CameraLagSpeed         = 11.f;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraRotationLagSpeed = 14.f;
	// Slight over-the-shoulder framing: Zarri sits just left of center with
	// the lens riding above his shoulder line instead of his lower back.
	SpringArm->SocketOffset = FVector(0.f, 46.f, 64.f);

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(SpringArm);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(84.f);

	Locomotion = CreateDefaultSubobject<USprawlLocomotionComponent>(TEXT("Locomotion"));
	HumanCharacter = CreateDefaultSubobject<USprawlHumanCharacterModule>(
		TEXT("HumanCharacter"));
	Wardrobe = CreateDefaultSubobject<USprawlWardrobeModule>(TEXT("Wardrobe"));
	Streetwear = CreateDefaultSubobject<USprawlStreetwearModule>(TEXT("Streetwear"));
	ReferenceClothing = CreateDefaultSubobject<USprawlReferenceClothingModule>(
		TEXT("ReferenceClothing"));
	PanelCloth = CreateDefaultSubobject<USprawlPanelClothModule>(TEXT("PanelCloth"));
	ClothPhysics = CreateDefaultSubobject<USprawlClothPhysicsModule>(TEXT("ClothPhysics"));
	Melee = CreateDefaultSubobject<USprawlMeleeModule>(TEXT("Melee"));

	MetaHumanVisualComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("MetaHumanVisual"));
	MetaHumanVisualComponent->SetupAttachment(RootComponent);
	MetaHumanVisualComponent->SetRelativeTransform(MetaHumanRelativeTransform);
	MetaHumanVisualComponent->SetChildActorOwnerOnCreation(true);

	// MetaHuman Creator assembles this actor into a stable path. Creator Core
	// Data supplies locomotion authored for the same body skeleton. Soft
	// references preserve a zero-cost fallback when those assets are absent.
	MetaHumanVisualClass = TSoftClassPtr<AActor>(FSoftObjectPath(
		TEXT("/Game/MetaHumans/Zarri/BP_Zarri.BP_Zarri_C")));
	MetaHumanIdleAnim = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(
		TEXT("/MetaHumanCharacter/Optional/Animation/UEFNAnimPreset/Locomotion/AS_MH_Neutral_Stand_Idle_Loop.AS_MH_Neutral_Stand_Idle_Loop")));
	MetaHumanWalkAnim = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(
		TEXT("/MetaHumanCharacter/Optional/Animation/UEFNAnimPreset/Locomotion/AS_MH_Neutral_Walk_Loop_F.AS_MH_Neutral_Walk_Loop_F")));
	MetaHumanRunAnim = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(
		TEXT("/MetaHumanCharacter/Optional/Animation/UEFNAnimPreset/Locomotion/AS_MH_Neutral_Run_Loop_F.AS_MH_Neutral_Run_Loop_F")));

	MobileOfficeRigComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MobileOfficeRigComponent"));
	MobileOfficeRigComponent->SetupAttachment(GetMesh());

	RunwayTrackerComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RunwayTrackerComponent"));
	RunwayTrackerComponent->SetupAttachment(GetMesh());

	CommDeviceComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CommDeviceComponent"));
	CommDeviceComponent->SetupAttachment(GetMesh());

	// --- Base character mesh (placeholder: engine UE4 mannequin) ---
	// Authoritative defaults set in C++ so they can't be lost by flaky
	// Blueprint inherited-component overrides.
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetRelativeLocationAndRotation(
			FVector(0.f, 0.f, -92.f),
			FRotator(0.f, -90.f, 0.f));   // FRotator(Pitch, Yaw, Roll)

		static ConstructorHelpers::FObjectFinder<USkeletalMesh> HeroMesh(
			TEXT("/Game/Pedestrians/Zarri/SK_Zarri.SK_Zarri"));
		if (HeroMesh.Succeeded())
		{
			MeshComp->SetSkeletalMeshAsset(HeroMesh.Object);
		}
		else
		{
			static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannequinMesh(
				TEXT("/Game/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin"));
			if (MannequinMesh.Succeeded())
			{
				MeshComp->SetSkeletalMeshAsset(MannequinMesh.Object);
			}
		}

		static ConstructorHelpers::FClassFinder<UAnimInstance> ZarriAnim(
			TEXT("/Game/Mannequin/Animations/ThirdPerson_AnimBP"));
		if (ZarriAnim.Succeeded())
		{
			MeshComp->SetAnimInstanceClass(ZarriAnim.Class);
		}
	}

	// --- Enhanced Input assets (loaded in C++ so the pawn is self-contained) ---
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMCFinder(
		TEXT("/Game/Input/IMC_Default.IMC_Default"));
	if (IMCFinder.Succeeded()) { DefaultMappingContext = IMCFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> MoveFinder(
		TEXT("/Game/Input/IA_Move.IA_Move"));
	if (MoveFinder.Succeeded()) { IA_Move = MoveFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> LookFinder(
		TEXT("/Game/Input/IA_Look.IA_Look"));
	if (LookFinder.Succeeded()) { IA_Look = LookFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> JumpFinder(
		TEXT("/Game/Input/IA_Jump.IA_Jump"));
	if (JumpFinder.Succeeded()) { IA_Jump = JumpFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> SprintFinder(
		TEXT("/Game/Input/IA_Sprint.IA_Sprint"));
	if (SprintFinder.Succeeded()) { IA_Sprint = SprintFinder.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> InteractFinder(
		TEXT("/Game/Input/IA_Interact.IA_Interact"));
	if (InteractFinder.Succeeded()) { IA_Interact = InteractFinder.Object; }
}

void AZarriCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Covers the fallback body and every piece of carried equipment; the
	// assembled MetaHuman is a separate actor and is handled on activation.
	FSprawlCharacterRender::DisableDecalProjection(this);
	if (HumanCharacter)
	{
		HumanCharacter->ConfigureZarri();
	}
	if (Melee)
	{
		Melee->OnAttackStarted.AddDynamic(
			this, &AZarriCharacter::HandleMeleeAttackStarted);
	}

	InitializeHeroAvatar();
	InitializeEquipment();
	TryInitializeMetaHumanVisual();

	bLocomotionAuditEnabled = FParse::Param(
		FCommandLine::Get(), TEXT("SprawlLocomotionAudit"));
	bAuditRunForward = FParse::Param(FCommandLine::Get(), TEXT("SprawlAuditRun"));

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

void AZarriCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateHeroAnimation();
	UpdateEquipmentVisuals(DeltaSeconds);
	UpdateFollowCamera(DeltaSeconds);
	UpdateBoundaryRescue(DeltaSeconds);

	if (bAuditRunForward && Controller)
	{
		// Hold forward so headless captures show a running pose rather than idle.
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), 1.f);
	}
	if (bLocomotionAuditEnabled)
	{
		RunLocomotionAudit();
	}
}

void AZarriCharacter::PrepareWardrobeVisualAudit()
{
	// -SprawlAuditRun turns the same close wardrobe capture into a live gait
	// check, proving shoe followers stay on the animated feet while moving.
	bAuditRunForward = FParse::Param(
		FCommandLine::Get(), TEXT("SprawlAuditRun"));
	if (!bAuditRunForward)
	{
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			Move->StopMovementImmediately();
		}
	}
	if (HumanCharacter)
	{
		HumanCharacter->SetAction(
			bAuditRunForward
				? ESprawlHumanAction::Run : ESprawlHumanAction::Stand,
			false);
	}
	if (SpringArm)
	{
		SpringArm->TargetArmLength = 335.f;
		SpringArm->SocketOffset = FVector(0.f, 24.f, 18.f);
		SpringArm->bEnableCameraLag = false;
		SpringArm->bEnableCameraRotationLag = false;
	}
	if (FollowCamera)
	{
		FollowCamera->SetFieldOfView(70.f);
	}
}

void AZarriCharacter::RunLocomotionAudit()
{
	bLocomotionAuditEnabled = false;

	if (!Locomotion || !Locomotion->HasVisual())
	{
		UE_LOG(LogTemp, Error,
			TEXT("[LocomotionAudit] FAIL: no locomotion visual is active"));
		FPlatformMisc::RequestExitWithStatus(true, 1, TEXT("SprawlLocomotionAudit"));
		return;
	}

	// Facing is what "runs straight" reduces to: the body must point wherever
	// the pawn points, at any heading. Movement itself is engine-owned
	// (bOrientRotationToMovement turns the capsule toward velocity), so sweeping
	// yaws measures the part this project actually controls — and does it
	// deterministically, with no traffic or collision noise.
	const FRotator OriginalRotation = GetActorRotation();
	const float TestYaws[] = { 0.f, 45.f, 90.f, 180.f, 270.f, -135.f };
	float WorstErrorDegrees = 0.f;
	float WorstYaw = 0.f;
	for (const float TestYaw : TestYaws)
	{
		SetActorRotation(FRotator(0.f, TestYaw, 0.f), ETeleportType::TeleportPhysics);
		const FVector VisualForward = Locomotion->GetVisualForward();
		const float ErrorDegrees = FMath::Abs(FMath::FindDeltaAngleDegrees(
			VisualForward.Rotation().Yaw, TestYaw));
		if (ErrorDegrees > WorstErrorDegrees)
		{
			WorstErrorDegrees = ErrorDegrees;
			WorstYaw = TestYaw;
		}
	}
	SetActorRotation(OriginalRotation, ETeleportType::TeleportPhysics);

	const bool bPassed = WorstErrorDegrees <= 1.f;
	const TCHAR* VisualKind = bUsingMetaHumanVisual
		? TEXT("metahuman") : TEXT("fallback");
	if (bPassed)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[LocomotionAudit] PASS visual=%s worst_facing_error=%.2f deg "
				"at_yaw=%.0f applied_correction=%.1f deg"),
			VisualKind, WorstErrorDegrees, WorstYaw,
			Locomotion->GetAppliedYawCorrection());
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("[LocomotionAudit] FAIL visual=%s worst_facing_error=%.2f deg "
				"at_yaw=%.0f applied_correction=%.1f deg gate=<=1.0"),
			VisualKind, WorstErrorDegrees, WorstYaw,
			Locomotion->GetAppliedYawCorrection());
	}
	FPlatformMisc::RequestExitWithStatus(
		true, bPassed ? 0 : 1, TEXT("SprawlLocomotionAudit"));
}

void AZarriCharacter::UpdateFollowCamera(float DeltaSeconds)
{
	if (!bAutoFollowCamera)
	{
		return;
	}
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}
	if (GetWorld()->GetTimeSeconds() - LastLookInputTime < FollowCameraDelay)
	{
		return;
	}

	const FVector Velocity2D(GetVelocity().X, GetVelocity().Y, 0.f);
	const float Speed = Velocity2D.Size();
	if (Speed < 90.f)
	{
		// Standing still: leave the camera wherever the player parked it.
		return;
	}

	// Ease behind the direction of travel. The correction is proportional to
	// how far off-axis the camera is: near-centered errors barely move (no
	// side-to-side wobble while running straight), big errors swing harder.
	FRotator Control = PC->GetControlRotation();
	const float TargetYaw = Velocity2D.Rotation().Yaw;
	const float DeltaYaw = FMath::FindDeltaAngleDegrees(Control.Yaw, TargetYaw);
	const float ErrorScale = FMath::Clamp(FMath::Abs(DeltaYaw) / 25.f, 0.f, 1.f);
	const float FollowRate = FollowCameraTurnSpeed
		* FMath::Clamp(Speed / 520.f, 0.45f, 1.f) * ErrorScale;
	const float YawAlpha = FMath::Clamp(DeltaSeconds * FollowRate, 0.f, 1.f);
	Control.Yaw += DeltaYaw * YawAlpha;

	const float CurrentPitch = FRotator::NormalizeAxis(Control.Pitch);
	const float PitchAlpha = FMath::Clamp(DeltaSeconds * 0.9f, 0.f, 1.f);
	Control.Pitch = CurrentPitch
		+ (FollowCameraRestPitch - CurrentPitch) * PitchAlpha;

	PC->SetControlRotation(Control);
}

void AZarriCharacter::UpdateBoundaryRescue(float DeltaSeconds)
{
	// While driving, Zarri is hidden with collision and movement off; the car
	// runs its own boundary recovery.
	if (IsHidden())
	{
		return;
	}

	const FVector Location = GetActorLocation();

	SafeFootLocationTimer -= DeltaSeconds;
	const UCharacterMovementComponent* Move = GetCharacterMovement();
	const float SafeInset = Grid::CityBoundaryHalfExtent - 220.f;
	const bool bWellInside = FMath::Abs(Location.X) <= SafeInset
		&& FMath::Abs(Location.Y) <= SafeInset;
	if (SafeFootLocationTimer <= 0.f && Move && Move->IsMovingOnGround()
		&& bWellInside && !Grid::IsOverLake(Location.X, Location.Y, 120.f)
		&& Location.Z > -120.f && Location.Z < 1500.f)
	{
		LastSafeFootLocation = Location;
		bHasSafeFootLocation = true;
		SafeFootLocationTimer = 0.75f;
	}

	const bool bOutOfBounds = !Grid::IsInsideCityBounds(Location.X, Location.Y, 60.f);
	const bool bFellThrough = Location.Z < -350.f;
	if (!bOutOfBounds && !bFellThrough)
	{
		return;
	}

	FVector Rescue = LastSafeFootLocation;
	if (!bHasSafeFootLocation)
	{
		// No history yet (e.g. spawned outside): center-block sidewalk anchor.
		const float CenterBlock = Grid::BlockCenter(Grid::NumBlocks / 2);
		Rescue = FVector(CenterBlock, CenterBlock, 120.f);
	}
	Rescue.Z += 40.f;
	SetActorLocation(Rescue, false, nullptr, ETeleportType::TeleportPhysics);
	if (UCharacterMovementComponent* MutableMove = GetCharacterMovement())
	{
		MutableMove->StopMovementImmediately();
		MutableMove->SetMovementMode(MOVE_Falling);
	}
	UE_LOG(LogTemp, Warning, TEXT("[Zarri] Rescued to safety (%s)"),
		bFellThrough ? TEXT("fell below world") : TEXT("outside city bounds"));
}

bool AZarriCharacter::TryInitializeMetaHumanVisual()
{
	if (ReferenceClothing)
	{
		ReferenceClothing->ClearClothing();
	}
	if (PanelCloth)
	{
		PanelCloth->ClearPanelCloth();
	}
	if (Wardrobe)
	{
		Wardrobe->ClearWardrobe();
	}
	bUsingMetaHumanVisual = false;
	MetaHumanBodyComponent = nullptr;
	LoadedMetaHumanIdleAnim = nullptr;
	LoadedMetaHumanWalkAnim = nullptr;
	LoadedMetaHumanRunAnim = nullptr;
	LoadedMetaHumanTalkAnim = nullptr;
	LoadedMetaHumanSitAnim = nullptr;
	LoadedMetaHumanDriveAnim = nullptr;
	LoadedMetaHumanPunchAnim = nullptr;
	LoadedMetaHumanKickAnim = nullptr;

	if (!MetaHumanVisualComponent || MetaHumanVisualClass.IsNull()
		|| MetaHumanIdleAnim.IsNull() || MetaHumanWalkAnim.IsNull()
		|| MetaHumanRunAnim.IsNull())
	{
		return false;
	}

	const TSubclassOf<AActor> VisualClass = MetaHumanVisualClass.LoadSynchronous();
	LoadedMetaHumanIdleAnim = MetaHumanIdleAnim.LoadSynchronous();
	LoadedMetaHumanWalkAnim = MetaHumanWalkAnim.LoadSynchronous();
	LoadedMetaHumanRunAnim = MetaHumanRunAnim.LoadSynchronous();
	if (!MetaHumanTalkAnim.IsNull())
	{
		LoadedMetaHumanTalkAnim = MetaHumanTalkAnim.LoadSynchronous();
	}
	if (!MetaHumanSitAnim.IsNull())
	{
		LoadedMetaHumanSitAnim = MetaHumanSitAnim.LoadSynchronous();
	}
	if (!MetaHumanDriveAnim.IsNull())
	{
		LoadedMetaHumanDriveAnim = MetaHumanDriveAnim.LoadSynchronous();
	}
	if (!MetaHumanPunchAnim.IsNull())
	{
		LoadedMetaHumanPunchAnim = MetaHumanPunchAnim.LoadSynchronous();
	}
	if (!MetaHumanKickAnim.IsNull())
	{
		LoadedMetaHumanKickAnim = MetaHumanKickAnim.LoadSynchronous();
	}
	if (!VisualClass || !LoadedMetaHumanIdleAnim || !LoadedMetaHumanWalkAnim
		|| !LoadedMetaHumanRunAnim)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[Zarri] MetaHuman actor or locomotion is unavailable; using %s fallback"),
			*ActiveHeroVariant);
		return false;
	}

	MetaHumanVisualComponent->SetRelativeTransform(MetaHumanRelativeTransform);
	MetaHumanVisualComponent->SetChildActorClass(VisualClass);
	AActor* VisualActor = MetaHumanVisualComponent->GetChildActor();
	if (!VisualActor)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Zarri] Failed to spawn MetaHuman visual; using %s fallback"),
			*ActiveHeroVariant);
		MetaHumanVisualComponent->SetChildActorClass(nullptr);
		return false;
	}

	TInlineComponentArray<USkeletalMeshComponent*> SkeletalMeshes(VisualActor);
	for (USkeletalMeshComponent* SkeletalMesh : SkeletalMeshes)
	{
		if (!SkeletalMesh)
		{
			continue;
		}
		SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SkeletalMesh->SetGenerateOverlapEvents(false);
		if (SkeletalMesh->GetFName() == TEXT("Body")
			|| SkeletalMesh->ComponentHasTag(TEXT("MetaHumanBody")))
		{
			MetaHumanBodyComponent = SkeletalMesh;
		}
	}

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(VisualActor);
	for (UPrimitiveComponent* Primitive : PrimitiveComponents)
	{
		if (Primitive)
		{
			Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Primitive->SetGenerateOverlapEvents(false);
		}
	}
	VisualActor->SetActorEnableCollision(false);
	// Body, face, hair and clothing are separate components; none of them
	// should catch the road's paint decals.
	FSprawlCharacterRender::DisableDecalProjection(VisualActor);

	if (!MetaHumanBodyComponent || !MetaHumanBodyComponent->GetSkeletalMeshAsset())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Zarri] MetaHuman visual has no assembled Body mesh; using %s fallback"),
			*ActiveHeroVariant);
		MetaHumanVisualComponent->SetChildActorClass(nullptr);
		MetaHumanBodyComponent = nullptr;
		return false;
	}

	MetaHumanBodyComponent->SetAnimInstanceClass(nullptr);
	MetaHumanBodyComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Locomotion->SetVisual(MetaHumanBodyComponent, MetaHumanVisualComponent);
	Locomotion->SetGaits({
		FSprawlGait(LoadedMetaHumanRunAnim, 320.f, 520.f, 0.8f, 1.5f),
		FSprawlGait(LoadedMetaHumanWalkAnim, 25.f, 150.f, 0.7f, 1.75f),
		FSprawlGait(LoadedMetaHumanIdleAnim, 0.f, 0.f, 1.f, 1.f),
	});
	Locomotion->UpdateLocomotion(0.f);
	const UAnimSingleNodeInstance* SingleNode =
		MetaHumanBodyComponent->GetSingleNodeInstance();
	if (!SingleNode || SingleNode->GetAnimationAsset() != LoadedMetaHumanIdleAnim)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Zarri] MetaHuman Body rejected its locomotion clips; using %s fallback"),
			*ActiveHeroVariant);
		MetaHumanVisualComponent->SetChildActorClass(nullptr);
		MetaHumanBodyComponent = nullptr;
		SetupFallbackLocomotion();
		return false;
	}

	bUsingMetaHumanVisual = true;
	// The assembled body is authored down its own axis, so square it up with
	// the capsule before it is ever seen — otherwise Zarri travels sideways.
	Locomotion->AlignVisualToOwnerForward();
	if (Wardrobe && HumanCharacter && HumanCharacter->IsConfigured())
	{
		const FSprawlWardrobeOutfit& HeroOutfit =
			HumanCharacter->GetRuntimeState().Customization.Outfit;
		const bool bUseAuthoredStreetwear = Streetwear
			&& USprawlStreetwearModule::SupportsOutfit(HeroOutfit);
		const bool bAuthoredStreetwearApplied = bUseAuthoredStreetwear
			&& Streetwear->ApplyToMetaHuman(
				VisualActor, MetaHumanBodyComponent, HeroOutfit);
		const bool bReferenceClothingApplied = bAuthoredStreetwearApplied
			&& ReferenceClothing
			&& ReferenceClothing->ApplyToMetaHuman(
				VisualActor, MetaHumanBodyComponent);
		if (bReferenceClothingApplied)
		{
			// Keep the proven authored shoes/beanie, but replace the bulky rigid
			// torso, sleeves, and cargo shells with the fitted reference outfit.
			Streetwear->SetLayerVisibility(
				ESprawlStreetwearLayer::Hoodie, false);
			Streetwear->SetLayerVisibility(
				ESprawlStreetwearLayer::Bomber, false);
			Streetwear->SetLayerVisibility(
				ESprawlStreetwearLayer::Cargo, false);
		}
		const bool bPanelClothApplied = bAuthoredStreetwearApplied
			&& PanelCloth
			&& PanelCloth->ApplyToMetaHuman(
				VisualActor, MetaHumanBodyComponent);
		if (bPanelClothApplied)
		{
			// Chaos owns the visible hoodie. Hide every rigid torso/sleeve shell
			// so none of the original segmented placeholders can occlude it.
			Streetwear->SetLayerVisibility(
				ESprawlStreetwearLayer::Hoodie, false);
			Streetwear->SetLayerVisibility(
				ESprawlStreetwearLayer::Bomber, false);
			if (bReferenceClothingApplied)
			{
				ReferenceClothing->SetLayerVisibility(
					ESprawlReferenceClothingLayer::Shirt, false);
			}
		}
		if (!bAuthoredStreetwearApplied
			&& !Wardrobe->ApplyToMetaHuman(
				VisualActor, MetaHumanBodyComponent, HeroOutfit))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[Wardrobe] Zarri kept his fitted base garment; an accessory layer was unavailable"));
		}
		if (ClothPhysics)
		{
			ClothPhysics->ConfigureOutfitPhysics(
				VisualActor, HeroOutfit);
		}
	}
	AttachEquipmentToVisual(MetaHumanBodyComponent, true);
	SetCharacterVisualHidden(false);

	UE_LOG(LogTemp, Display, TEXT("[Zarri] Activated assembled MetaHuman visual %s"),
		*VisualClass->GetName());
	return true;
}

void AZarriCharacter::SetCharacterVisualHidden(bool bHidden)
{
	if (USkeletalMeshComponent* FallbackMesh = GetMesh())
	{
		FallbackMesh->SetHiddenInGame(bHidden || bUsingMetaHumanVisual, true);
	}

	if (MetaHumanVisualComponent)
	{
		if (AActor* VisualActor = MetaHumanVisualComponent->GetChildActor())
		{
			VisualActor->SetActorHiddenInGame(bHidden || !bUsingMetaHumanVisual);
		}
	}

	if (MobileOfficeRigComponent) { MobileOfficeRigComponent->SetHiddenInGame(bHidden); }
	if (RunwayTrackerComponent) { RunwayTrackerComponent->SetHiddenInGame(bHidden); }
	if (CommDeviceComponent) { CommDeviceComponent->SetHiddenInGame(bHidden); }
}

void AZarriCharacter::InitializeHeroAvatar()
{
	TArray<FString> Candidates = { HeroVariant };
	if (HeroVariant != TEXT("Cappy"))
	{
		Candidates.Add(TEXT("Cappy"));
	}

	USkeletalMesh* Mesh = nullptr;
	UAnimSequence* LoadedIdle = nullptr;
	UAnimSequence* LoadedWalk = nullptr;
	UAnimSequence* LoadedJog = nullptr;
	for (const FString& Candidate : Candidates)
	{
		Mesh = FSprawlAvatarLibrary::LoadAvatarMesh(Candidate);
		LoadedIdle = FSprawlAvatarLibrary::LoadAvatarAnim(Candidate, TEXT("Idle"));
		LoadedWalk = FSprawlAvatarLibrary::LoadAvatarAnim(Candidate, TEXT("Walk"));
		LoadedJog = FSprawlAvatarLibrary::LoadAvatarAnim(Candidate, TEXT("Jog"));
		if (!Mesh || !LoadedIdle || !LoadedWalk || !LoadedJog)
		{
			continue;
		}
		if (!FSprawlAvatarLibrary::ApplyAvatar(GetMesh(), Mesh, HeroHeight,
			GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()))
		{
			continue;
		}
		ActiveHeroVariant = Candidate;
		break;
	}
	if (!Mesh || !LoadedIdle || !LoadedWalk || !LoadedJog
		|| GetMesh()->GetSkeletalMeshAsset() != Mesh)
	{
		return; // complete mannequin fallback
	}

	// The equipment sockets were named for the mannequin skeleton; retarget
	// them onto the Mixamo-style bones of the imported avatar.
	auto FindBone = [Mesh](const TCHAR* Suffix) -> FName
	{
		const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
		for (int32 i = 0; i < Ref.GetNum(); ++i)
		{
			if (Ref.GetBoneName(i).ToString().EndsWith(Suffix))
			{
				return Ref.GetBoneName(i);
			}
		}
		return NAME_None;
	};
	if (const FName Spine = FindBone(TEXT("Spine2")); !Spine.IsNone()) { MobileOfficeRigSocket = Spine; }
	if (const FName Wrist = FindBone(TEXT("LeftHand")); !Wrist.IsNone()) { RunwayTrackerSocket = Wrist; }
	if (const FName Head = FindBone(TEXT("Head")); !Head.IsNone()) { CommDeviceSocket = Head; }

	HeroIdleAnim = LoadedIdle;
	HeroWalkAnim = LoadedWalk;
	HeroJogAnim = LoadedJog;
	HeroSprintAnim = FSprawlAvatarLibrary::LoadAvatarAnim(
		ActiveHeroVariant, TEXT("Sprint"));
	HeroTalkAnim = FSprawlAvatarLibrary::LoadAvatarAnim(
		ActiveHeroVariant, TEXT("Talk"));
	HeroSitAnim = FSprawlAvatarLibrary::LoadAvatarAnim(
		ActiveHeroVariant, TEXT("Sit"));
	// Optional future art: these suffixes are deliberately not part of the
	// completeness gate, so an absent combat clip never disables Zarri's body.
	HeroPunchAnim = FSprawlAvatarLibrary::LoadAvatarAnim(
		ActiveHeroVariant, TEXT("Punch"));
	HeroKickAnim = FSprawlAvatarLibrary::LoadAvatarAnim(
		ActiveHeroVariant, TEXT("Kick"));
	bHasHeroAvatar = true;
	SetupFallbackLocomotion();
}

void AZarriCharacter::SetupFallbackLocomotion()
{
	// The engine mannequin drives itself from a real AnimBP, so the component
	// only takes over once an imported avatar is actually in place.
	if (!bHasHeroAvatar || !Locomotion)
	{
		return;
	}
	Locomotion->SetVisual(GetMesh(), GetMesh());
	Locomotion->SetStandardGaits(
		HeroIdleAnim, HeroWalkAnim, HeroJogAnim, HeroSprintAnim);
	Locomotion->AlignVisualToOwnerForward();
	Locomotion->UpdateLocomotion(0.f);
}

void AZarriCharacter::UpdateHeroAnimation()
{
	if (!Locomotion)
	{
		return;
	}

	const float GroundSpeed = GetVelocity().Size2D();
	bool bTalking = false;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (const UPhoneSubsystem* Phone = GI->GetSubsystem<UPhoneSubsystem>())
		{
			bTalking = Phone->IsOnCall();
		}
	}
	const ESprawlHumanAction Action = HumanCharacter
		? HumanCharacter->UpdateActionFromMovement(
			GroundSpeed, bTalking, false, false, false)
		: USprawlHumanCharacterModule::ResolveAction(
			GroundSpeed, bTalking, false, false, false);

	UAnimSequence* ActionAnimation = nullptr;
	if (bUsingMetaHumanVisual)
	{
		switch (Action)
		{
		case ESprawlHumanAction::Talk: ActionAnimation = LoadedMetaHumanTalkAnim; break;
		case ESprawlHumanAction::Sit: ActionAnimation = LoadedMetaHumanSitAnim; break;
		case ESprawlHumanAction::Drive:
			ActionAnimation = LoadedMetaHumanDriveAnim
				? LoadedMetaHumanDriveAnim : LoadedMetaHumanSitAnim;
			break;
		default: break;
		}
	}
	else
	{
		switch (Action)
		{
		case ESprawlHumanAction::Talk: ActionAnimation = HeroTalkAnim; break;
		case ESprawlHumanAction::Sit:
		case ESprawlHumanAction::Drive: ActionAnimation = HeroSitAnim; break;
		default: break;
		}
	}

	if (ActionAnimation)
	{
		Locomotion->SetActionAnimation(ActionAnimation);
	}
	else
	{
		Locomotion->ClearActionAnimation();
	}
	Locomotion->UpdateLocomotion(GroundSpeed);
}

bool AZarriCharacter::TryPunch()
{
	return Melee && Melee->TryPunch();
}

bool AZarriCharacter::TryKick()
{
	return Melee && Melee->TryKick();
}

bool AZarriCharacter::TryMeleeAttack()
{
	if (!Melee)
	{
		return false;
	}

	const ESprawlMeleeAttack Attack = Melee->GetNextAlternatingAttack();
	const bool bStarted = Melee->TryAlternatingAttack();
	if (bStarted)
	{
		const TCHAR* AttackName = Attack == ESprawlMeleeAttack::Punch
			? TEXT("PUNCH") : TEXT("KICK");
		UE_LOG(LogTemp, Display, TEXT("[MeleeInput] X/action started %s"),
			AttackName);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				static_cast<uint64>(0x4D454C45), 0.45f, FColor::White,
				AttackName);
		}
	}
	return bStarted;
}

void AZarriCharacter::HandleMeleeAttackStarted(
	ESprawlMeleeAttack Attack, float RecoverySeconds, bool bHitCharacter)
{
	(void)bHitCharacter;
	if (!Locomotion)
	{
		return;
	}
	UAnimSequence* AttackAnimation = nullptr;
	if (bUsingMetaHumanVisual)
	{
		AttackAnimation = Attack == ESprawlMeleeAttack::Punch
			? LoadedMetaHumanPunchAnim : LoadedMetaHumanKickAnim;
	}
	else
	{
		AttackAnimation = Attack == ESprawlMeleeAttack::Punch
			? HeroPunchAnim : HeroKickAnim;
	}
	if (AttackAnimation)
	{
		Locomotion->PlayOneShotAnimation(AttackAnimation, RecoverySeconds);
	}
}

void AZarriCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Zarri] SetupInput Move=%d Look=%d Jump=%d Sprint=%d Interact=%d"),
			IA_Move != nullptr, IA_Look != nullptr, IA_Jump != nullptr,
			IA_Sprint != nullptr, IA_Interact != nullptr);
		if (IA_Move)     EIC->BindAction(IA_Move,     ETriggerEvent::Triggered, this, &AZarriCharacter::HandleMove);
		if (IA_Look)     EIC->BindAction(IA_Look,     ETriggerEvent::Triggered, this, &AZarriCharacter::HandleLook);
		if (IA_Jump)
		{
			EIC->BindAction(IA_Jump,  ETriggerEvent::Started,   this, &ACharacter::Jump);
			EIC->BindAction(IA_Jump,  ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
		if (IA_Sprint)
		{
			EIC->BindAction(IA_Sprint, ETriggerEvent::Started,   this, &AZarriCharacter::HandleSprint);
			EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &AZarriCharacter::HandleStopSprint);
		}
		if (IA_Interact)
		{
			EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &AZarriCharacter::HandleInteract);
		}
	}

	// Legacy key binding — reliable interact even if Enhanced Input misbehaves.
	PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &AZarriCharacter::OnInteractKey);
	PlayerInputComponent->BindKey(EKeys::F, IE_Pressed, this, &AZarriCharacter::OnInteractKey);

	// X belongs on the possessed pawn's input component. Controller bindings
	// are lower priority and can be shadowed by this Enhanced Input stack.
	FSprawlMeleeInput::BindOnFoot(PlayerInputComponent, this);

	if (FParse::Param(FCommandLine::Get(), TEXT("SprawlMeleeInputAudit")))
	{
		RunMeleeInputAudit();
	}
}

void AZarriCharacter::OnMeleeKey()
{
	TryMeleeAttack();
}

void AZarriCharacter::RunMeleeInputAudit()
{
	int32 KeyboardXBindings = 0;
	int32 GamepadXBindings = 0;
	if (InputComponent)
	{
		for (const FInputKeyBinding& Binding : InputComponent->KeyBindings)
		{
			if (Binding.KeyEvent != IE_Pressed)
			{
				continue;
			}
			KeyboardXBindings += Binding.Chord.Key == EKeys::X ? 1 : 0;
			GamepadXBindings += Binding.Chord.Key
				== EKeys::Gamepad_FaceButton_Left ? 1 : 0;
		}
	}

	const int32 RevisionBefore = Melee ? Melee->GetRuntimeState().Revision : -1;
	OnMeleeKey();
	const FSprawlMeleeRuntimeState State = Melee
		? Melee->GetRuntimeState() : FSprawlMeleeRuntimeState();
	const bool bPassed = InputComponent && KeyboardXBindings == 1
		&& GamepadXBindings == 1 && Melee
		&& State.Revision == RevisionBefore + 1
		&& State.LastAttack == ESprawlMeleeAttack::Punch
		&& State.bAttackActive
		&& Melee->GetNextAlternatingAttack() == ESprawlMeleeAttack::Kick;

	const TCHAR* AttackName = State.LastAttack == ESprawlMeleeAttack::Punch
		? TEXT("Punch") : TEXT("Kick");
	const TCHAR* NextName = Melee
		&& Melee->GetNextAlternatingAttack() == ESprawlMeleeAttack::Kick
		? TEXT("Kick") : TEXT("Punch");
	if (bPassed)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[MeleeInputAudit] PASS keyboard_x=%d gamepad_x=%d "
				"revision=%d attack=%s next=%s"),
			KeyboardXBindings, GamepadXBindings, State.Revision,
			AttackName, NextName);
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("[MeleeInputAudit] FAIL keyboard_x=%d gamepad_x=%d "
				"revision=%d attack=%s next=%s"),
			KeyboardXBindings, GamepadXBindings, State.Revision,
			AttackName, NextName);
	}
	FPlatformMisc::RequestExitWithStatus(
		true, bPassed ? 0 : 1, TEXT("SprawlMeleeInputAudit"));
}

void AZarriCharacter::HandleMove(const FInputActionValue& Value)
{
	FVector2D Axis = Value.Get<FVector2D>();
	if (Axis.SizeSquared() < FMath::Square(0.08f))
	{
		return;
	}
	Axis = Axis.GetClampedToMaxSize(1.f);
	if (Controller && !Axis.IsNearlyZero())
	{
		const FRotator YawRot(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Fwd   = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
		AddMovementInput(Fwd, Axis.Y);
		AddMovementInput(Right, Axis.X);
	}
}

void AZarriCharacter::HandleLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>().GetClampedToMaxSize(1.f);
	if (Controller)
	{
		if (Axis.SizeSquared() > FMath::Square(0.05f))
		{
			// Manual look pauses the auto-follow so the player wins.
			LastLookInputTime = GetWorld()->GetTimeSeconds();
		}
		AddControllerYawInput(Axis.X * 0.82f);
		AddControllerPitchInput(Axis.Y * 0.68f);
	}
}

void AZarriCharacter::SetSprinting(bool bSprinting)
{
	GetCharacterMovement()->MaxWalkSpeed = bSprinting ? 760.f : 520.f;
}

void AZarriCharacter::HandleSprint(const FInputActionValue&)
{
	SetSprinting(true);
}

void AZarriCharacter::HandleStopSprint(const FInputActionValue&)
{
	SetSprinting(false);
}

void AZarriCharacter::HandleInteract(const FInputActionValue&)
{
	UE_LOG(LogTemp, Warning, TEXT("[Zarri] Interact pressed"));
	TryContextInteraction();
}

void AZarriCharacter::OnInteractKey()
{
	UE_LOG(LogTemp, Warning, TEXT("[Zarri] OnInteractKey (BindKey)"));
	TryContextInteraction();
}

bool AZarriCharacter::TryContextInteraction()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPhoneSubsystem* Phone = GI->GetSubsystem<UPhoneSubsystem>();
			Phone && Phone->TryAnswerRingingCall())
		{
			return true;
		}
	}
	if (ASprawlEnterableInteriors* Interiors =
		ASprawlEnterableInteriors::FindForWorld(GetWorld());
		Interiors && Interiors->TryInteract(this))
	{
		return true;
	}
	ASprawlCar* Vehicle = FindNearbyVehicle();
	UE_LOG(LogTemp, Warning, TEXT("[Zarri] Context vehicle found=%s"),
		Vehicle ? *Vehicle->GetName() : TEXT("NULL"));
	return EnterVehicle(Vehicle);
}

bool AZarriCharacter::GetBuildingInteractionPrompt(FText& OutPrompt) const
{
	if (const ASprawlEnterableInteriors* Interiors =
		ASprawlEnterableInteriors::FindForWorld(GetWorld()))
	{
		return Interiors->GetInteractionPrompt(this, OutPrompt);
	}
	OutPrompt = FText::GetEmpty();
	return false;
}

void AZarriCharacter::EnterNearbyVehicle()
{
	ASprawlCar* Vehicle = FindNearbyVehicle();
	UE_LOG(LogTemp, Warning, TEXT("[Zarri] EnterNearbyVehicle found=%s"),
		Vehicle ? *Vehicle->GetName() : TEXT("NULL"));
	EnterVehicle(Vehicle);
}

bool AZarriCharacter::EnterVehicle(ASprawlCar* Vehicle)
{
	if (!Vehicle)
	{
		return false;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return false;
	}

	const bool bCarjacking = Vehicle->CanBeCarjacked();
	const bool bAssigned = bCarjacking
		? Vehicle->CarjackDriver(this)
		: Vehicle->AssignDriver(this);
	if (!bAssigned)
	{
		return false;
	}

	PC->Possess(Vehicle);
	if (PC->GetPawn() != Vehicle)
	{
		Vehicle->RequestExit();
		if (PC->GetPawn() != this)
		{
			PC->Possess(this);
		}
		return false;
	}
	if (HumanCharacter)
	{
		// Logical state still says Drive, while the existing vehicle policy keeps
		// every seated driver visual hidden behind the cabin glass.
		HumanCharacter->SetAction(ESprawlHumanAction::Drive, true);
	}
	// Hide Zarri but keep him in the world so we can teleport him to a door on exit.
	SetCharacterVisualHidden(true);
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	GetCharacterMovement()->DisableMovement();
	Vehicle->ConfirmDriverEntry();
	UE_LOG(LogTemp, Warning, TEXT("[Zarri] possessed car"));
	return true;
}

void AZarriCharacter::OnExitedVehicle(ASprawlCar* FromVehicle)
{
	if (!FromVehicle) return;

	FVector Exit = FromVehicle->GetActorLocation()
		- FromVehicle->GetActorRightVector() * 190.f + FVector(0.f, 0.f, 100.f);
	if (!FromVehicle->FindClearSideExit(
		GetCapsuleComponent()->GetScaledCapsuleRadius(),
		GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), Exit, this))
	{
		// Exhaustive sidewalk candidates should make this unreachable. Keep a
		// visible, collision-safe last resort instead of stranding a hidden pawn.
		Exit = FromVehicle->GetActorLocation() + FVector(0.f, 0.f, 350.f);
		UE_LOG(LogTemp, Error,
			TEXT("[Zarri] No clear sidewalk exit for %s; using roof fallback"),
			*FromVehicle->GetName());
	}
	SetActorLocation(Exit, /*bSweep*/ false, nullptr, ETeleportType::TeleportPhysics);

	SetActorHiddenInGame(false);
	SetCharacterVisualHidden(false);
	SetActorEnableCollision(true);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	if (HumanCharacter)
	{
		HumanCharacter->ClearHeldAction();
		HumanCharacter->SetAction(ESprawlHumanAction::Stand, false);
	}

	if (APlayerController* PC = Cast<APlayerController>(FromVehicle->GetController()))
	{
		PC->Possess(this);
	}
}

ASprawlCar* AZarriCharacter::FindNearbyVehicle() const
{
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASprawlCar::StaticClass(), Found);

	ASprawlCar* Best = nullptr;
	float BestDistSq = InteractRadius * InteractRadius;

	const FVector MyLoc = GetActorLocation();
	for (AActor* Actor : Found)
	{
		ASprawlCar* Car = Cast<ASprawlCar>(Actor);
		if (!Car || (!Car->CanBeEntered() && !Car->CanBeCarjacked()))
		{
			continue;
		}
		const float D = FVector::DistSquared(MyLoc, Actor->GetActorLocation());
		if (D <= BestDistSq)
		{
			BestDistSq = D;
			Best = Car;
		}
	}
	return Best;
}

void AZarriCharacter::InitializeEquipment()
{
	// 1. Mobile-Office Rig (Skeletal Mesh)
	if (MobileOfficeRigAsset)
	{
		MobileOfficeRigComponent->SetSkeletalMesh(MobileOfficeRigAsset);
		MobileOfficeRigComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, MobileOfficeRigSocket);
	}

	// 2. Runway Tracker (Static Mesh Watch)
	if (RunwayTrackerAsset)
	{
		RunwayTrackerComponent->SetStaticMesh(RunwayTrackerAsset);
		RunwayTrackerComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, RunwayTrackerSocket);

		UMaterialInterface* Material = RunwayTrackerComponent->GetMaterial(RunwayTrackerMaterialIndex);
		if (Material)
		{
			RunwayTrackerMID = RunwayTrackerComponent->CreateDynamicMaterialInstance(RunwayTrackerMaterialIndex, Material);
		}
	}

	// 3. Comm Device (Static Mesh Earpiece)
	if (CommDeviceAsset)
	{
		CommDeviceComponent->SetStaticMesh(CommDeviceAsset);
		CommDeviceComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, CommDeviceSocket);

		UMaterialInterface* Material = CommDeviceComponent->GetMaterial(CommDeviceMaterialIndex);
		if (Material)
		{
			CommDeviceMID = CommDeviceComponent->CreateDynamicMaterialInstance(CommDeviceMaterialIndex, Material);
		}
	}
}

void AZarriCharacter::AttachEquipmentToVisual(
	USkeletalMeshComponent* TargetMesh, bool bMetaHumanSockets)
{
	if (!TargetMesh)
	{
		return;
	}

	const FName RigSocket = bMetaHumanSockets ? MetaHumanRigSocket : MobileOfficeRigSocket;
	const FName TrackerSocket = bMetaHumanSockets ? MetaHumanTrackerSocket : RunwayTrackerSocket;
	const FName DeviceSocket = bMetaHumanSockets ? MetaHumanCommSocket : CommDeviceSocket;

	if (MobileOfficeRigComponent && MobileOfficeRigAsset)
	{
		MobileOfficeRigComponent->AttachToComponent(
			TargetMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, RigSocket);
	}
	if (RunwayTrackerComponent && RunwayTrackerAsset)
	{
		RunwayTrackerComponent->AttachToComponent(
			TargetMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, TrackerSocket);
	}
	if (CommDeviceComponent && CommDeviceAsset)
	{
		CommDeviceComponent->AttachToComponent(
			TargetMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, DeviceSocket);
	}
}

void AZarriCharacter::UpdateEquipmentVisuals(float DeltaSeconds)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	// 1. Update Runway Tracker dynamic watch face parameters
	if (RunwayTrackerMID)
	{
		if (UFounderSubsystem* Founder = GI->GetSubsystem<UFounderSubsystem>())
		{
			const float Cash = Founder->GetCash();
			const float RunwayDays = Founder->GetRunwayDays();

			RunwayTrackerMID->SetScalarParameterValue(TEXT("CashOnHand"), Cash);
			RunwayTrackerMID->SetScalarParameterValue(TEXT("RunwayDays"), RunwayDays);

			// Pulsing glow for the smartwatch face
			const float Pulse = FMath::Sin(GetWorld()->GetTimeSeconds() * 2.0f) * 0.2f + 0.8f;
			RunwayTrackerMID->SetScalarParameterValue(TEXT("PulseBrightness"), Pulse);
		}
	}

	// 2. Update Comm Device LED indicator based on Phone call state
	if (CommDeviceMID)
	{
		if (UPhoneSubsystem* Comms = GI->GetSubsystem<UPhoneSubsystem>())
		{
			float CallState = 0.0f; // 0 = Idle, 1 = Ringing, 2 = Active Call
			FLinearColor LEDColor = FLinearColor(0.0f, 0.2f, 1.0f, 1.0f); // Default cool blue
			float FlashSpeed = 0.0f;

			if (Comms->IsRinging())
			{
				CallState = 1.0f;
				// Rapid flashing orange-red when ringing
				const float Sine = FMath::Sin(GetWorld()->GetTimeSeconds() * 10.0f);
				LEDColor = (Sine > 0.0f) ? FLinearColor(1.0f, 0.15f, 0.0f, 1.0f) : FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
				FlashSpeed = 10.0f;
			}
			else if (Comms->IsOnCall())
			{
				CallState = 2.0f;
				LEDColor = FLinearColor(0.0f, 1.0f, 0.3f, 1.0f); // Solid active green
				FlashSpeed = 0.0f;
			}

			CommDeviceMID->SetScalarParameterValue(TEXT("CallState"), CallState);
			CommDeviceMID->SetVectorParameterValue(TEXT("LEDColor"), LEDColor);
			CommDeviceMID->SetScalarParameterValue(TEXT("FlashSpeed"), FlashSpeed);
		}
	}
}
