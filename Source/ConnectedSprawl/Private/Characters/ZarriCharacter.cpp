// The Connected Sprawl - Zarri on foot.

#include "Characters/ZarriCharacter.h"
#include "Vehicles/SprawlCar.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
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
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Characters/SprawlAvatarLibrary.h"
#include "UObject/ConstructorHelpers.h"

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
	Move->RotationRate              = FRotator(0.f, 900.f, 0.f);
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

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(SpringArm);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(84.f);

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

		static ConstructorHelpers::FObjectFinder<USkeletalMesh> ZarriMesh(
			TEXT("/Game/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin"));
		if (ZarriMesh.Succeeded())
		{
			MeshComp->SetSkeletalMeshAsset(ZarriMesh.Object);
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

	InitializeHeroAvatar();
	InitializeEquipment();

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
}

void AZarriCharacter::InitializeHeroAvatar()
{
	USkeletalMesh* Mesh = FSprawlAvatarLibrary::LoadAvatarMesh(HeroVariant);
	UAnimSequence* LoadedIdle = FSprawlAvatarLibrary::LoadAvatarAnim(HeroVariant, TEXT("Idle"));
	UAnimSequence* LoadedWalk = FSprawlAvatarLibrary::LoadAvatarAnim(HeroVariant, TEXT("Walk"));
	UAnimSequence* LoadedJog = FSprawlAvatarLibrary::LoadAvatarAnim(HeroVariant, TEXT("Jog"));
	if (!Mesh || !LoadedIdle || !LoadedWalk || !LoadedJog)
	{
		// Preserve the complete mannequin fallback when an avatar import is
		// missing any of the clips needed by the native locomotion driver.
		return;
	}

	if (!FSprawlAvatarLibrary::ApplyAvatar(GetMesh(), Mesh, HeroHeight,
		GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()))
	{
		return; // art not imported yet — keep the mannequin + ThirdPerson_AnimBP
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
	bHasHeroAvatar = true;
	FSprawlAvatarLibrary::PlayLoop(GetMesh(), HeroIdleAnim, HeroCurrentAnim);
}

void AZarriCharacter::UpdateHeroAnimation()
{
	if (!bHasHeroAvatar)
	{
		return;
	}

	const float Speed = GetVelocity().Size2D();
	if (Speed > 320.f)
	{
		FSprawlAvatarLibrary::PlayLoop(GetMesh(), HeroJogAnim, HeroCurrentAnim,
			FMath::Clamp(Speed / 330.f, 0.9f, 1.6f));
	}
	else if (Speed > 25.f)
	{
		FSprawlAvatarLibrary::PlayLoop(GetMesh(), HeroWalkAnim, HeroCurrentAnim,
			FMath::Clamp(Speed / 130.f, 0.7f, 2.0f));
	}
	else
	{
		FSprawlAvatarLibrary::PlayLoop(GetMesh(), HeroIdleAnim, HeroCurrentAnim);
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
		AddControllerYawInput(Axis.X * 0.82f);
		AddControllerPitchInput(Axis.Y * 0.68f);
	}
}

void AZarriCharacter::HandleSprint(const FInputActionValue&)
{
	GetCharacterMovement()->MaxWalkSpeed = 760.f;
}

void AZarriCharacter::HandleStopSprint(const FInputActionValue&)
{
	GetCharacterMovement()->MaxWalkSpeed = 520.f;
}

void AZarriCharacter::HandleInteract(const FInputActionValue&)
{
	UE_LOG(LogTemp, Warning, TEXT("[Zarri] Interact pressed"));
	EnterNearbyVehicle();
}

void AZarriCharacter::OnInteractKey()
{
	UE_LOG(LogTemp, Warning, TEXT("[Zarri] OnInteractKey (BindKey)"));
	EnterNearbyVehicle();
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
	if (!PC || !Vehicle->AssignDriver(this))
	{
		return false;
	}

	PC->Possess(Vehicle);
	UE_LOG(LogTemp, Warning, TEXT("[Zarri] possessed car"));
	// Hide Zarri but keep him in the world so we can teleport him to a door on exit.
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	GetCharacterMovement()->DisableMovement();
	return true;
}

void AZarriCharacter::OnExitedVehicle(ASprawlCar* FromVehicle)
{
	if (!FromVehicle) return;

	// Prefer the driver side, then try the passenger side and front/rear. This
	// keeps exiting reliable when a curb, building wall, or another car blocks
	// the nominal door position.
	const FVector CarLocation = FromVehicle->GetActorLocation();
	const FVector Side = FromVehicle->GetActorRightVector() * 190.f;
	const FVector End = FromVehicle->GetActorForwardVector() * 275.f;
	const FVector Candidates[] = {
		CarLocation - Side, CarLocation + Side,
		CarLocation - End, CarLocation + End,
	};
	FVector Exit = CarLocation - Side + FVector(0.f, 0.f, 30.f);
	if (UWorld* World = GetWorld())
	{
		FCollisionQueryParams Params(FName(TEXT("ZarriVehicleExit")), false, this);
		Params.AddIgnoredActor(FromVehicle);
		const FCollisionShape Capsule = FCollisionShape::MakeCapsule(
			GetCapsuleComponent()->GetScaledCapsuleRadius(),
			GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		for (const FVector& Candidate : Candidates)
		{
			const FVector TestLocation = Candidate + FVector(0.f, 0.f, 100.f);
			if (!World->OverlapBlockingTestByChannel(
				TestLocation, FQuat::Identity, ECC_Pawn, Capsule, Params))
			{
				Exit = TestLocation;
				break;
			}
		}
	}
	SetActorLocation(Exit, /*bSweep*/ false, nullptr, ETeleportType::TeleportPhysics);

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

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
		if (!Car || !Car->CanBeEntered())
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
