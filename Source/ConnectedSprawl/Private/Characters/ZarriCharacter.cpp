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
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Characters/SprawlAvatarLibrary.h"
#include "Phone/PhoneSubsystem.h"
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

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(SpringArm);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(84.f);

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
	TryInitializeMetaHumanVisual();

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

bool AZarriCharacter::TryInitializeMetaHumanVisual()
{
	bUsingMetaHumanVisual = false;
	MetaHumanBodyComponent = nullptr;
	LoadedMetaHumanIdleAnim = nullptr;
	LoadedMetaHumanWalkAnim = nullptr;
	LoadedMetaHumanRunAnim = nullptr;
	MetaHumanCurrentAnim = nullptr;

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
	FSprawlAvatarLibrary::PlayLoop(MetaHumanBodyComponent,
		LoadedMetaHumanIdleAnim, MetaHumanCurrentAnim);
	const UAnimSingleNodeInstance* SingleNode =
		MetaHumanBodyComponent->GetSingleNodeInstance();
	if (!SingleNode || SingleNode->GetAnimationAsset() != LoadedMetaHumanIdleAnim)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Zarri] MetaHuman Body rejected its locomotion clips; using %s fallback"),
			*ActiveHeroVariant);
		MetaHumanVisualComponent->SetChildActorClass(nullptr);
		MetaHumanBodyComponent = nullptr;
		MetaHumanCurrentAnim = nullptr;
		return false;
	}

	bUsingMetaHumanVisual = true;
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
	bHasHeroAvatar = true;
	FSprawlAvatarLibrary::PlayLoop(GetMesh(), HeroIdleAnim, HeroCurrentAnim);
}

void AZarriCharacter::UpdateHeroAnimation()
{
	if (bUsingMetaHumanVisual && MetaHumanBodyComponent)
	{
		const float Speed = GetVelocity().Size2D();
		if (Speed > 320.f)
		{
			FSprawlAvatarLibrary::PlayLoop(MetaHumanBodyComponent,
				LoadedMetaHumanRunAnim, MetaHumanCurrentAnim,
				FMath::Clamp(Speed / 520.f, 0.8f, 1.35f));
		}
		else if (Speed > 25.f)
		{
			FSprawlAvatarLibrary::PlayLoop(MetaHumanBodyComponent,
				LoadedMetaHumanWalkAnim, MetaHumanCurrentAnim,
				FMath::Clamp(Speed / 150.f, 0.7f, 1.75f));
		}
		else
		{
			FSprawlAvatarLibrary::PlayLoop(MetaHumanBodyComponent,
				LoadedMetaHumanIdleAnim, MetaHumanCurrentAnim);
		}
		return;
	}

	if (!bHasHeroAvatar)
	{
		return;
	}

	const float Speed = GetVelocity().Size2D();
	if (Speed > 560.f)
	{
		UAnimSequence* Sprint = HeroSprintAnim ? HeroSprintAnim : HeroJogAnim;
		const float ReferenceSpeed = HeroSprintAnim ? 610.f : 330.f;
		FSprawlAvatarLibrary::PlayLoop(GetMesh(), Sprint, HeroCurrentAnim,
			FMath::Clamp(Speed / ReferenceSpeed, 0.85f, HeroSprintAnim ? 1.35f : 1.8f));
	}
	else if (Speed > 320.f)
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
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPhoneSubsystem* Phone = GI->GetSubsystem<UPhoneSubsystem>();
			Phone && Phone->TryAnswerRingingCall())
		{
			return;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("[Zarri] Interact pressed"));
	EnterNearbyVehicle();
}

void AZarriCharacter::OnInteractKey()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPhoneSubsystem* Phone = GI->GetSubsystem<UPhoneSubsystem>();
			Phone && Phone->TryAnswerRingingCall())
		{
			return;
		}
	}
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
