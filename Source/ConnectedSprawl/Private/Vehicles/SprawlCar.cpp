// The Connected Sprawl - A drivable physics car.

#include "Vehicles/SprawlCar.h"
#include "Characters/ZarriCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ASprawlCar::ASprawlCar()
{
	PrimaryActorTick.bCanEverTick = true;

	// --- Physics hull (root) ---
	Hull = CreateDefaultSubobject<UBoxComponent>(TEXT("Hull"));
	RootComponent = Hull;
	Hull->SetBoxExtent(FVector(235.f, 100.f, 62.f));
	Hull->SetCollisionProfileName(TEXT("PhysicsActor"));
	// Keep the car flat — only let it yaw. (Safe to set on the CDO.)
	Hull->BodyInstance.bLockXRotation = true;
	Hull->BodyInstance.bLockYRotation = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BodyMat(
		TEXT("/Game/CityArt/MI_Car_Blue.MI_Car_Blue"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlassMat(
		TEXT("/Game/CityArt/MI_CarGlass.MI_CarGlass"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TireMat(
		TEXT("/Game/CityArt/MI_Tire.MI_Tire"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ChromeMat(
		TEXT("/Game/CityArt/MI_Metal.MI_Metal"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> LampMat(
		TEXT("/Game/CityArt/M_LampGlow.M_LampGlow"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TailMat(
		TEXT("/Game/CityArt/MI_Car_Red.MI_Car_Red"));

	auto MakePanel = [&](const TCHAR* Name, UStaticMesh* Mesh, FVector RelLoc,
	                     FVector PanelScale, UMaterialInterface* Mat,
	                     FRotator RelRot = FRotator::ZeroRotator) -> UStaticMeshComponent*
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(Hull);
		if (Mesh)
		{
			C->SetStaticMesh(Mesh);
		}
		C->SetRelativeLocation(RelLoc);
		C->SetRelativeRotation(RelRot);
		C->SetRelativeScale3D(PanelScale);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (Mat) { C->SetMaterial(0, Mat); }
		return C;
	};

	// Cube mesh is 100u; panels are scaled to centimetres / 100.
	UStaticMesh* Cube = CubeMesh.Succeeded() ? CubeMesh.Object.Get() : nullptr;
	UStaticMesh* Cylinder = CylinderMesh.Succeeded() ? CylinderMesh.Object.Get() : Cube;
	UMaterialInterface* BodyMaterial = BodyMat.Succeeded() ? BodyMat.Object.Get() : nullptr;
	UMaterialInterface* GlassMaterial = GlassMat.Succeeded() ? GlassMat.Object.Get() : nullptr;
	UMaterialInterface* TireMaterial = TireMat.Succeeded() ? TireMat.Object.Get() : nullptr;
	UMaterialInterface* ChromeMaterial = ChromeMat.Succeeded() ? ChromeMat.Object.Get() : nullptr;
	UMaterialInterface* LampMaterial = LampMat.Succeeded() ? LampMat.Object.Get() : nullptr;
	UMaterialInterface* TailMaterial = TailMat.Succeeded() ? TailMat.Object.Get() : BodyMaterial;

	FullBodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ExternalVehicleMesh"));
	FullBodyMesh->SetupAttachment(Hull);
	FullBodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FullBodyMesh->SetVisibility(false, true);
	FullBodyMesh->SetHiddenInGame(true);

	FullSkeletalBodyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ExternalSkeletalVehicleMesh"));
	FullSkeletalBodyMesh->SetupAttachment(Hull);
	FullSkeletalBodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FullSkeletalBodyMesh->SetVisibility(false, true);
	FullSkeletalBodyMesh->SetHiddenInGame(true);

	BodyMesh = MakePanel(TEXT("BodyMesh"), Cube, FVector(0.f, 0.f, -8.f),
	                     FVector(4.8f, 1.96f, 0.70f), BodyMaterial);
	BodyPaintMeshes.Add(BodyMesh);
	BodyPaintMeshes.Add(MakePanel(TEXT("FrontHood"), Cube, FVector(132.f, 0.f, 31.f),
	                              FVector(1.74f, 1.84f, 0.36f), BodyMaterial));
	BodyPaintMeshes.Add(MakePanel(TEXT("RearDeck"), Cube, FVector(-150.f, 0.f, 34.f),
	                              FVector(1.36f, 1.82f, 0.38f), BodyMaterial));
	BodyPaintMeshes.Add(MakePanel(TEXT("FrontFender"), Cube, FVector(142.f, 0.f, -7.f),
	                              FVector(1.55f, 2.08f, 0.62f), BodyMaterial));
	BodyPaintMeshes.Add(MakePanel(TEXT("RearFender"), Cube, FVector(-151.f, 0.f, -6.f),
	                              FVector(1.48f, 2.08f, 0.62f), BodyMaterial));

	CabinMesh = MakePanel(TEXT("CabinMesh"), Cube, FVector(-24.f, 0.f, 82.f),
	                      FVector(2.18f, 1.62f, 0.72f), GlassMaterial);
	DetailMeshes.Add(CabinMesh);
	DetailMeshes.Add(MakePanel(TEXT("RoofPanel"), Cube, FVector(-34.f, 0.f, 124.f),
	                           FVector(1.82f, 1.48f, 0.16f), BodyMaterial));
	BodyPaintMeshes.Add(DetailMeshes.Last());

	DetailMeshes.Add(MakePanel(TEXT("FrontBumper"), Cube, FVector(254.f, 0.f, -6.f),
	                           FVector(0.22f, 1.88f, 0.30f), ChromeMaterial));
	DetailMeshes.Add(MakePanel(TEXT("RearBumper"), Cube, FVector(-254.f, 0.f, -6.f),
	                           FVector(0.22f, 1.88f, 0.30f), ChromeMaterial));
	DetailMeshes.Add(MakePanel(TEXT("LeftMirror"), Cube, FVector(28.f, -117.f, 72.f),
	                           FVector(0.26f, 0.10f, 0.18f), ChromeMaterial));
	DetailMeshes.Add(MakePanel(TEXT("RightMirror"), Cube, FVector(28.f, 117.f, 72.f),
	                           FVector(0.26f, 0.10f, 0.18f), ChromeMaterial));
	DetailMeshes.Add(MakePanel(TEXT("LeftHeadlight"), Cube, FVector(259.f, -54.f, 19.f),
	                           FVector(0.08f, 0.38f, 0.18f), LampMaterial));
	DetailMeshes.Add(MakePanel(TEXT("RightHeadlight"), Cube, FVector(259.f, 54.f, 19.f),
	                           FVector(0.08f, 0.38f, 0.18f), LampMaterial));
	DetailMeshes.Add(MakePanel(TEXT("LeftTailLight"), Cube, FVector(-259.f, -55.f, 20.f),
	                           FVector(0.08f, 0.32f, 0.18f), TailMaterial));
	DetailMeshes.Add(MakePanel(TEXT("RightTailLight"), Cube, FVector(-259.f, 55.f, 20.f),
	                           FVector(0.08f, 0.32f, 0.18f), TailMaterial));

	const FVector WheelScale(0.82f, 0.82f, 0.34f);
	const FRotator WheelRot(0.f, 0.f, 90.f);
	const float WX = 150.f, WY = 104.f, WZ = -54.f;
	const FVector WheelPos[4] = {
		FVector(WX, WY, WZ), FVector(WX, -WY, WZ),
		FVector(-WX, WY, WZ), FVector(-WX, -WY, WZ)
	};
	for (int32 i = 0; i < 4; ++i)
	{
		const FString WName = FString::Printf(TEXT("Wheel%d"), i);
		UStaticMeshComponent* W = MakePanel(*WName, Cylinder, WheelPos[i], WheelScale,
		                                    TireMaterial, WheelRot);
		WheelMeshes.Add(W);
		DetailMeshes.Add(W);
	}

	// --- Camera ---
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(Hull);
	SpringArm->TargetArmLength = 720.f;
	SpringArm->SetRelativeLocation(FVector(0.f, 0.f, 140.f));
	SpringArm->SetRelativeRotation(FRotator(-14.f, 0.f, 0.f));
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = true;
	SpringArm->bInheritRoll = false;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 5.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(SpringArm);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(90.f);

	// --- Input assets ---
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMC(
		TEXT("/Game/Input/IMC_Default.IMC_Default"));
	if (IMC.Succeeded()) { DefaultMappingContext = IMC.Object; }
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveAct(
		TEXT("/Game/Input/IA_Move.IA_Move"));
	if (MoveAct.Succeeded()) { IA_Move = MoveAct.Object; }
	static ConstructorHelpers::FObjectFinder<UInputAction> InteractAct(
		TEXT("/Game/Input/IA_Interact.IA_Interact"));
	if (InteractAct.Succeeded()) { IA_Interact = InteractAct.Object; }
}

void ASprawlCar::BeginPlay()
{
	Super::BeginPlay();

	// Physics setup deferred out of the constructor (GEngine isn't ready there).
	if (Hull)
	{
		Hull->SetSimulatePhysics(true);
		Hull->SetMassOverrideInKg(NAME_None, 1500.f, true);
		Hull->SetLinearDamping(0.8f);
		Hull->SetAngularDamping(3.5f);
	}
}

void ASprawlCar::SetBodyMaterial(UMaterialInterface* Mat)
{
	if (Mat)
	{
		for (UStaticMeshComponent* BodyPart : BodyPaintMeshes)
		{
			if (BodyPart)
			{
				BodyPart->SetMaterial(0, Mat);
			}
		}
	}
}

void ASprawlCar::SetExternalVehicleMesh(UStaticMesh* Mesh, FVector RelativeLocation,
	FRotator RelativeRotation, FVector RelativeScale)
{
	if (!FullBodyMesh || !Mesh)
	{
		return;
	}

	FullBodyMesh->SetStaticMesh(Mesh);
	FullBodyMesh->SetRelativeLocation(RelativeLocation);
	FullBodyMesh->SetRelativeRotation(RelativeRotation);
	FullBodyMesh->SetRelativeScale3D(RelativeScale);
	FullBodyMesh->SetVisibility(true, true);
	FullBodyMesh->SetHiddenInGame(false);
	if (FullSkeletalBodyMesh)
	{
		FullSkeletalBodyMesh->SetVisibility(false, true);
		FullSkeletalBodyMesh->SetHiddenInGame(true);
	}
	SetKitbashVisible(false);
}

void ASprawlCar::SetExternalSkeletalVehicleMesh(USkeletalMesh* Mesh, FVector RelativeLocation,
	FRotator RelativeRotation, FVector RelativeScale)
{
	if (!FullSkeletalBodyMesh || !Mesh)
	{
		return;
	}

	FullSkeletalBodyMesh->SetSkeletalMesh(Mesh);
	FullSkeletalBodyMesh->SetRelativeLocation(RelativeLocation);
	FullSkeletalBodyMesh->SetRelativeRotation(RelativeRotation);
	FullSkeletalBodyMesh->SetRelativeScale3D(RelativeScale);
	FullSkeletalBodyMesh->SetVisibility(true, true);
	FullSkeletalBodyMesh->SetHiddenInGame(false);
	if (FullBodyMesh)
	{
		FullBodyMesh->SetVisibility(false, true);
		FullBodyMesh->SetHiddenInGame(true);
	}
	SetKitbashVisible(false);
}

void ASprawlCar::ClearExternalVehicleMesh()
{
	if (FullBodyMesh)
	{
		FullBodyMesh->SetStaticMesh(nullptr);
		FullBodyMesh->SetVisibility(false, true);
		FullBodyMesh->SetHiddenInGame(true);
	}
	if (FullSkeletalBodyMesh)
	{
		FullSkeletalBodyMesh->SetSkeletalMesh(nullptr);
		FullSkeletalBodyMesh->SetVisibility(false, true);
		FullSkeletalBodyMesh->SetHiddenInGame(true);
	}
	SetKitbashVisible(true);
}

void ASprawlCar::SetKitbashVisible(bool bVisible)
{
	TArray<UStaticMeshComponent*> Parts;
	for (UStaticMeshComponent* Part : BodyPaintMeshes)
	{
		Parts.Add(Part);
	}
	for (UStaticMeshComponent* Part : DetailMeshes)
	{
		Parts.Add(Part);
	}
	for (UStaticMeshComponent* Part : WheelMeshes)
	{
		Parts.Add(Part);
	}

	for (UStaticMeshComponent* Part : Parts)
	{
		if (Part)
		{
			Part->SetVisibility(bVisible, true);
			Part->SetHiddenInGame(!bVisible);
		}
	}
}

void ASprawlCar::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (APlayerController* PC = Cast<APlayerController>(NewController))
	{
		if (auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
				PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

void ASprawlCar::SetupPlayerInputComponent(UInputComponent* InputComponent)
{
	Super::SetupPlayerInputComponent(InputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (IA_Move)
		{
			EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ASprawlCar::HandleMove);
			EIC->BindAction(IA_Move, ETriggerEvent::Completed, this, &ASprawlCar::HandleMoveEnd);
		}
		if (IA_Interact)
		{
			EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &ASprawlCar::HandleExit);
		}
	}

	// Legacy key binding — reliable exit even if Enhanced Input misbehaves.
	InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ASprawlCar::OnExitKey);
}

void ASprawlCar::OnExitKey()
{
	HandleExit(FInputActionValue());
}

void ASprawlCar::RequestExit()
{
	HandleExit(FInputActionValue());
}

void ASprawlCar::HandleMove(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	SteerInput    = Axis.X;
	ThrottleInput = Axis.Y;
}

void ASprawlCar::HandleMoveEnd(const FInputActionValue& /*Value*/)
{
	SteerInput = 0.f;
	ThrottleInput = 0.f;
}

void ASprawlCar::HandleExit(const FInputActionValue& /*Value*/)
{
	if (Driver)
	{
		ThrottleInput = 0.f;
		SteerInput = 0.f;
		Driver->OnExitedVehicle(this);
		Driver = nullptr;
	}
}

void ASprawlCar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Hull || !Hull->IsSimulatingPhysics())
	{
		return;
	}

	// Only the player drives; parked cars just sit.
	if (!Cast<APlayerController>(GetController()))
	{
		return;
	}

	const FVector Fwd = GetActorForwardVector();
	const FVector Vel = Hull->GetPhysicsLinearVelocity();
	const float ForwardSpeed = FVector::DotProduct(Vel, Fwd);

	// Throttle / reverse
	if (FMath::Abs(ThrottleInput) > 0.05f)
	{
		Hull->AddForce(Fwd * ThrottleInput * EngineForce);
	}

	// Steering — yaw rate scales with speed, flips sign in reverse.
	if (FMath::Abs(SteerInput) > 0.05f && FMath::Abs(ForwardSpeed) > 25.f)
	{
		const float SpeedFactor = FMath::Clamp(FMath::Abs(ForwardSpeed) / 600.f, 0.25f, 1.f);
		const float Dir = FMath::Sign(ForwardSpeed);
		const FVector AngVel = Hull->GetPhysicsAngularVelocityInDegrees();
		const float TargetYaw = SteerInput * Dir * TurnRate * SpeedFactor;
		Hull->SetPhysicsAngularVelocityInDegrees(
			FVector(AngVel.X, AngVel.Y, TargetYaw));
	}
}
