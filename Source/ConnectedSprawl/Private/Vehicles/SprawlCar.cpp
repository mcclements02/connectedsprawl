// The Connected Sprawl - A drivable physics car.

#include "Vehicles/SprawlCar.h"
#include "AI/PedestrianCrowdManager.h"
#include "AI/SprawlPedestrian.h"
#include "Characters/SprawlAvatarLibrary.h"
#include "Characters/ZarriCharacter.h"
#include "Phone/PhoneSubsystem.h"
#include "Animation/AnimSequence.h"
#include "Components/BoxComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
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
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

#include <initializer_list>

// File-scope alias (matches the other Sprawl translation units so unity
// builds don't see a local declaration shadowing the global one).
using Grid = USprawlCityGridSubsystem;

namespace
{
bool SlotNameContainsAny(const FString& LowerSlotName, std::initializer_list<const TCHAR*> Terms)
{
	for (const TCHAR* Term : Terms)
	{
		if (LowerSlotName.Contains(Term))
		{
			return true;
		}
	}
	return false;
}

bool IsVehiclePaintSlotName(const FName SlotName)
{
	const FString LowerSlotName = SlotName.ToString().ToLower();
	if (SlotNameContainsAny(LowerSlotName, {
		TEXT("glass"), TEXT("screen"), TEXT("wheel"), TEXT("tyre"),
		TEXT("tire"), TEXT("rim"), TEXT("brake"), TEXT("light"),
		TEXT("lamp"), TEXT("grill"), TEXT("exhaust"), TEXT("interior"),
		TEXT("plate")
	}))
	{
		return false;
	}

	return SlotNameContainsAny(LowerSlotName, { TEXT("body"), TEXT("paint"), TEXT("carbon") });
}

void ApplyVehiclePaintMaterial(UMeshComponent* MeshComponent, UMaterialInterface* PaintMaterial)
{
	if (!MeshComponent || !PaintMaterial)
	{
		return;
	}

	bool bAppliedPaint = false;
	const TArray<FName> SlotNames = MeshComponent->GetMaterialSlotNames();
	for (int32 SlotIndex = 0; SlotIndex < SlotNames.Num(); ++SlotIndex)
	{
		if (IsVehiclePaintSlotName(SlotNames[SlotIndex]))
		{
			MeshComponent->SetMaterial(SlotIndex, PaintMaterial);
			bAppliedPaint = true;
		}
	}

	if (!bAppliedPaint && MeshComponent->GetNumMaterials() > 0)
	{
		MeshComponent->SetMaterial(0, PaintMaterial);
	}
}
}

ASprawlCar::ASprawlCar()
{
	PrimaryActorTick.bCanEverTick = true;

	// --- Physics hull (root) ---
	Hull = CreateDefaultSubobject<UBoxComponent>(TEXT("Hull"));
	RootComponent = Hull;
	Hull->SetBoxExtent(FVector(235.f, 100.f, 62.f));
	Hull->SetCollisionProfileName(TEXT("PhysicsActor"));
	// Normal driving is levelled in Tick. Rotations stay physically available
	// so a severe crash can roll before the timed recovery takes over.
	Hull->BodyInstance.bLockXRotation = false;
	Hull->BodyInstance.bLockYRotation = false;
	Hull->SetNotifyRigidBodyCollision(true);
	Hull->OnComponentHit.AddDynamic(this, &ASprawlCar::HandleHullHit);

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
	PrototypeWheelMesh = Cylinder;
	UMaterialInterface* BodyMaterial = BodyMat.Succeeded() ? BodyMat.Object.Get() : nullptr;
	UMaterialInterface* GlassMaterial = GlassMat.Succeeded() ? GlassMat.Object.Get() : nullptr;
	UMaterialInterface* TireMaterial = TireMat.Succeeded() ? TireMat.Object.Get() : nullptr;
	UMaterialInterface* ChromeMaterial = ChromeMat.Succeeded() ? ChromeMat.Object.Get() : nullptr;
	UMaterialInterface* LampMaterial = LampMat.Succeeded() ? LampMat.Object.Get() : nullptr;
	UMaterialInterface* TailMaterial = TailMat.Succeeded() ? TailMat.Object.Get() : BodyMaterial;
	BodyPaintMaterial = BodyMaterial;

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

	DriverMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("DriverMesh"));
	DriverMesh->SetupAttachment(Hull);
	DriverMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DriverMesh->SetGenerateOverlapEvents(false);
	DriverMesh->SetCastShadow(false);
	DriverMesh->bCastDynamicShadow = false;
	DriverMesh->bCastStaticShadow = false;
	DriverMesh->SetReceivesDecals(false);
	DriverMesh->VisibilityBasedAnimTickOption =
		EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	DriverMesh->bEnableUpdateRateOptimizations = true;
	DriverMesh->SetCullDistance(5000.f);
	DriverMesh->SetVisibility(false, true);
	DriverMesh->SetHiddenInGame(true);

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
		const FString PivotName = FString::Printf(TEXT("WheelPivot%d"), i);
		USceneComponent* Pivot = CreateDefaultSubobject<USceneComponent>(*PivotName);
		Pivot->SetupAttachment(Hull);
		Pivot->SetRelativeLocation(WheelPos[i]);
		WheelPivots.Add(Pivot);

		const FString WName = FString::Printf(TEXT("Wheel%d"), i);
		UStaticMeshComponent* W = CreateDefaultSubobject<UStaticMeshComponent>(*WName);
		W->SetupAttachment(Pivot);
		W->SetStaticMesh(Cylinder);
		W->SetRelativeRotation(WheelRot);
		W->SetRelativeScale3D(WheelScale);
		W->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (TireMaterial)
		{
			W->SetMaterial(0, TireMaterial);
		}
		WheelMeshes.Add(W);
		DetailMeshes.Add(W);
	}

	// --- Camera ---
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(Hull);
	SpringArm->TargetArmLength = 780.f;
	SpringArm->SetRelativeLocation(FVector(0.f, 0.f, 165.f));
	SpringArm->SetRelativeRotation(FRotator(-12.f, 0.f, 0.f));
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = true;
	SpringArm->bInheritRoll = false;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 9.f;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraRotationLagSpeed = 10.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(SpringArm);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(86.f);

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
	TryApplyRuntimeVehicleParts();

	// Physics setup deferred out of the constructor (GEngine isn't ready there).
	if (Hull)
	{
		const FRotator SpawnRotation = GetActorRotation();
		SetActorRotation(FRotator(0.f, SpawnRotation.Yaw, 0.f),
			ETeleportType::TeleportPhysics);
		// Reapply current crash-aware settings so serialized map instances cannot
		// retain stale rotation locks from older versions of the class.
		Hull->BodyInstance.bLockXRotation = false;
		Hull->BodyInstance.bLockYRotation = false;
		Hull->SetSimulatePhysics(true);
		Hull->SetMassOverrideInKg(NAME_None, 1500.f, true);
		Hull->SetLinearDamping(0.8f);
		Hull->SetAngularDamping(3.5f);
		MaintainUpright(0.f);
	}
	if (Grid::IsInsideCityBounds(GetActorLocation().X, GetActorLocation().Y))
	{
		LastSafeTransform = GetActorTransform();
		bHasLastSafeTransform = true;
	}
}

void ASprawlCar::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleaseIntersectionReservation();
	Super::EndPlay(EndPlayReason);
}

void ASprawlCar::SetBodyMaterial(UMaterialInterface* Mat)
{
	if (Mat)
	{
		BodyPaintMaterial = Mat;
		for (UStaticMeshComponent* BodyPart : BodyPaintMeshes)
		{
			if (BodyPart)
			{
				BodyPart->SetMaterial(0, Mat);
			}
		}
		ApplyVehiclePaintMaterial(FullBodyMesh, Mat);
		ApplyVehiclePaintMaterial(FullSkeletalBodyMesh, Mat);
	}
}

void ASprawlCar::SetExternalVehicleMesh(UStaticMesh* Mesh, FVector RelativeLocation,
	FRotator RelativeRotation, FVector RelativeScale)
{
	if (!FullBodyMesh || !Mesh)
	{
		return;
	}
	ClearExternalBodyDetails();

	FullBodyMesh->SetStaticMesh(Mesh);
	FullBodyMesh->EmptyOverrideMaterials();
	FullBodyMesh->SetRelativeLocation(RelativeLocation);
	FullBodyMesh->SetRelativeRotation(RelativeRotation);
	FullBodyMesh->SetRelativeScale3D(RelativeScale);
	ApplyVehiclePaintMaterial(FullBodyMesh, BodyPaintMaterial);
	FullBodyMesh->SetVisibility(true, true);
	FullBodyMesh->SetHiddenInGame(false);
	if (FullSkeletalBodyMesh)
	{
		FullSkeletalBodyMesh->SetVisibility(false, true);
		FullSkeletalBodyMesh->SetHiddenInGame(true);
	}
	bUsingExternalWheelParts = false;
	SetKitbashVisible(false);
}

void ASprawlCar::SetExternalVehicleParts(const TArray<UStaticMesh*>& ExternalBodyMeshes,
	const TArray<UStaticMesh*>& ExternalWheelMeshes,
	const TArray<FVector>& ExternalWheelCenters,
	FVector RelativeLocation, FRotator RelativeRotation, FVector RelativeScale)
{
	if (!FullBodyMesh || ExternalBodyMeshes.IsEmpty() || !ExternalBodyMeshes[0]
		|| ExternalWheelMeshes.Num() != 4
		|| ExternalWheelCenters.Num() != 4 || WheelMeshes.Num() != 4
		|| WheelPivots.Num() != 4)
	{
		return;
	}
	for (UStaticMesh* BodyPart : ExternalBodyMeshes)
	{
		if (!BodyPart)
		{
			return;
		}
	}

	for (UStaticMesh* WheelMesh : ExternalWheelMeshes)
	{
		if (!WheelMesh)
		{
			return;
		}
	}

	ClearExternalBodyDetails();
	FullBodyMesh->SetStaticMesh(ExternalBodyMeshes[0]);
	FullBodyMesh->EmptyOverrideMaterials();
	FullBodyMesh->SetRelativeLocation(RelativeLocation);
	FullBodyMesh->SetRelativeRotation(RelativeRotation);
	FullBodyMesh->SetRelativeScale3D(RelativeScale);
	ApplyVehiclePaintMaterial(FullBodyMesh, BodyPaintMaterial);
	FullBodyMesh->SetVisibility(true, true);
	FullBodyMesh->SetHiddenInGame(false);

	for (int32 Index = 1; Index < ExternalBodyMeshes.Num(); ++Index)
	{
		const FName ComponentName(*FString::Printf(TEXT("ExternalVehicleDetail_%02d"), Index - 1));
		UStaticMeshComponent* Detail = NewObject<UStaticMeshComponent>(this, ComponentName);
		if (!Detail)
		{
			continue;
		}
		Detail->SetupAttachment(Hull);
		Detail->SetStaticMesh(ExternalBodyMeshes[Index]);
		Detail->SetRelativeLocation(RelativeLocation);
		Detail->SetRelativeRotation(RelativeRotation);
		Detail->SetRelativeScale3D(RelativeScale);
		Detail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		AddInstanceComponent(Detail);
		Detail->RegisterComponent();
		ExternalBodyDetailMeshes.Add(Detail);
	}

	if (FullSkeletalBodyMesh)
	{
		FullSkeletalBodyMesh->SetVisibility(false, true);
		FullSkeletalBodyMesh->SetHiddenInGame(true);
	}

	SetKitbashVisible(false);
	const FQuat BodyRotation = RelativeRotation.Quaternion();
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FVector ScaledCenter = ExternalWheelCenters[Index] * RelativeScale;
		WheelPivots[Index]->SetRelativeLocation(
			RelativeLocation + BodyRotation.RotateVector(ScaledCenter));
		WheelPivots[Index]->SetRelativeRotation(FRotator::ZeroRotator);
		WheelPivots[Index]->SetRelativeScale3D(FVector::OneVector);

		UStaticMeshComponent* Wheel = WheelMeshes[Index];
		Wheel->SetStaticMesh(ExternalWheelMeshes[Index]);
		Wheel->EmptyOverrideMaterials();
		Wheel->SetRelativeLocation(BodyRotation.RotateVector(-ScaledCenter));
		Wheel->SetRelativeRotation(RelativeRotation);
		Wheel->SetRelativeScale3D(RelativeScale);
		Wheel->SetVisibility(true, true);
		Wheel->SetHiddenInGame(false);
	}

	bUsingExternalWheelParts = true;
}

void ASprawlCar::SetExternalSkeletalVehicleMesh(USkeletalMesh* Mesh, FVector RelativeLocation,
	FRotator RelativeRotation, FVector RelativeScale)
{
	if (!FullSkeletalBodyMesh || !Mesh)
	{
		return;
	}
	ClearExternalBodyDetails();

	FullSkeletalBodyMesh->SetSkeletalMesh(Mesh);
	FullSkeletalBodyMesh->EmptyOverrideMaterials();
	FullSkeletalBodyMesh->SetRelativeLocation(RelativeLocation);
	FullSkeletalBodyMesh->SetRelativeRotation(RelativeRotation);
	FullSkeletalBodyMesh->SetRelativeScale3D(RelativeScale);
	ApplyVehiclePaintMaterial(FullSkeletalBodyMesh, BodyPaintMaterial);
	FullSkeletalBodyMesh->SetVisibility(true, true);
	FullSkeletalBodyMesh->SetHiddenInGame(false);
	if (FullBodyMesh)
	{
		FullBodyMesh->SetVisibility(false, true);
		FullBodyMesh->SetHiddenInGame(true);
	}
	bUsingExternalWheelParts = false;
	SetKitbashVisible(false);
}

void ASprawlCar::ClearExternalVehicleMesh()
{
	ClearExternalBodyDetails();
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
	ResetPrototypeWheels();
	SetKitbashVisible(true);
}

void ASprawlCar::ClearExternalBodyDetails()
{
	for (UStaticMeshComponent* Detail : ExternalBodyDetailMeshes)
	{
		if (Detail)
		{
			RemoveInstanceComponent(Detail);
			Detail->DestroyComponent();
		}
	}
	ExternalBodyDetailMeshes.Reset();
}

void ASprawlCar::ResetPrototypeWheels()
{
	const FVector WheelScale(0.82f, 0.82f, 0.34f);
	const FRotator WheelRotation(0.f, 0.f, 90.f);
	const float WheelX = 150.f;
	const float WheelY = 104.f;
	const float WheelZ = -54.f;
	const FVector Positions[4] = {
		FVector(WheelX, WheelY, WheelZ), FVector(WheelX, -WheelY, WheelZ),
		FVector(-WheelX, WheelY, WheelZ), FVector(-WheelX, -WheelY, WheelZ)
	};

	for (int32 Index = 0; Index < WheelMeshes.Num() && Index < WheelPivots.Num(); ++Index)
	{
		WheelPivots[Index]->SetRelativeLocation(Positions[Index]);
		WheelPivots[Index]->SetRelativeRotation(FRotator::ZeroRotator);
		WheelPivots[Index]->SetRelativeScale3D(FVector::OneVector);
		WheelMeshes[Index]->SetStaticMesh(PrototypeWheelMesh);
		WheelMeshes[Index]->SetRelativeLocation(FVector::ZeroVector);
		WheelMeshes[Index]->SetRelativeRotation(WheelRotation);
		WheelMeshes[Index]->SetRelativeScale3D(WheelScale);
	}
	bUsingExternalWheelParts = false;
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

bool ASprawlCar::CanBeEntered() const
{
	if (Driver || bAutoDrive || bHasAIDriver || Cast<APlayerController>(GetController()))
	{
		return false;
	}
	const float Speed = Hull ? Hull->GetPhysicsLinearVelocity().Size2D() : GetVelocity().Size2D();
	return Speed <= MaxEntrySpeed && IsWithinCityBounds();
}

bool ASprawlCar::CanBeCarjacked() const
{
	if (!bHasAIDriver || !bAutoDrive || Driver
		|| Cast<APlayerController>(GetController()) || HasActiveIntersectionReservation())
	{
		return false;
	}
	const float Speed = Hull ? Hull->GetPhysicsLinearVelocity().Size2D() : GetVelocity().Size2D();
	return Speed <= MaxCarjackSpeed && IsWithinCityBounds();
}

bool ASprawlCar::AssignDriver(AZarriCharacter* InDriver)
{
	if (!InDriver || !CanBeEntered())
	{
		return false;
	}
	return AssignDriverInternal(InDriver, bAutoDrive);
}

bool ASprawlCar::AssignDriverInternal(AZarriCharacter* InDriver, bool bResumeAIOnExit)
{
	if (!InDriver || Driver || Cast<APlayerController>(GetController()))
	{
		return false;
	}

	bResumeAutoDriveAfterExit = bResumeAIOnExit;
	bAutoDrive = false;
	ReleaseIntersectionReservation();
	Driver = InDriver;
	ShowSeatedDriver(InDriver->GetActiveHeroVariant());
	if (Hull)
	{
		Hull->WakeAllRigidBodies();
	}
	return true;
}

bool ASprawlCar::CarjackDriver(AZarriCharacter* InDriver)
{
	if (!InDriver || !CanBeCarjacked())
	{
		return false;
	}

	if (Hull)
	{
		Hull->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Hull->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}

	bPendingCarjack = true;
	PendingCarjackVariant = AIDriverVariant;
	LastEjectedDriver.Reset();
	bHasAIDriver = false;
	AIDriverVariant.Reset();
	HideDriverVisual();
	ReleaseIntersectionReservation();
	bAutoDrive = false;
	bAIStateSeeded = false;
	if (!AssignDriverInternal(InDriver, false))
	{
		RestorePendingCarjack();
		return false;
	}
	return true;
}

void ASprawlCar::ConfirmDriverEntry()
{
	if (bPendingCarjack && Driver && Cast<APlayerController>(GetController()))
	{
		CompletePendingCarjack();
	}
}

void ASprawlCar::CompletePendingCarjack()
{
	if (!bPendingCarjack)
	{
		return;
	}

	const FString EjectedVariant = PendingCarjackVariant;

	FVector EjectLocation = GetActorLocation()
		- GetActorRightVector() * 190.f + FVector(0.f, 0.f, 100.f);
	if (!FindClearSideExit(40.f, 92.f, EjectLocation, Driver, true))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[Carjack] Could not find a safe sidewalk exit for %s"), *GetName());
		return;
	}

	if (UWorld* World = GetWorld())
	{
		const FTransform SpawnTransform(
			FRotator(0.f, GetActorRotation().Yaw, 0.f), EjectLocation, FVector::OneVector);
		ASprawlPedestrian* EjectedPedestrian = World->SpawnActorDeferred<ASprawlPedestrian>(
			ASprawlPedestrian::StaticClass(), SpawnTransform, this, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (EjectedPedestrian)
		{
			EjectedPedestrian->SetForcedVariant(EjectedVariant);
			UGameplayStatics::FinishSpawningActor(EjectedPedestrian, SpawnTransform);
			if (!IsValid(EjectedPedestrian))
			{
				UE_LOG(LogTemp, Error,
					TEXT("[Carjack] Failed to finish spawning the driver from %s"), *GetName());
				return;
			}
			EjectedPedestrian->StartFleeFrom(this, 4.f);
			LastEjectedDriver = EjectedPedestrian;
			APedestrianCrowdManager* CrowdManager = Cast<APedestrianCrowdManager>(
				UGameplayStatics::GetActorOfClass(
					World, APedestrianCrowdManager::StaticClass()));
			if (CrowdManager)
			{
				CrowdManager->AdoptExternalPedestrian(EjectedPedestrian);
			}
			else
			{
				EjectedPedestrian->SetLifeSpan(45.f);
			}
			bPendingCarjack = false;
			PendingCarjackVariant.Reset();
		}
		else
		{
			UE_LOG(LogTemp, Error,
				TEXT("[Carjack] Failed to spawn the driver from %s"), *GetName());
		}
	}
}

void ASprawlCar::RestorePendingCarjack()
{
	if (!bPendingCarjack)
	{
		return;
	}

	bHasAIDriver = true;
	AIDriverVariant = PendingCarjackVariant;
	bAutoDrive = true;
	bAIStateSeeded = false;
	bDriverVisualInitializationAttempted = false;
	bPendingCarjack = false;
	PendingCarjackVariant.Reset();
}

bool ASprawlCar::FindClearSideExit(float CapsuleRadius, float CapsuleHalfHeight,
	FVector& OutLocation, const AActor* ActorToIgnore, bool bPreferDriverSide) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector CarLocation = GetActorLocation();
	const FVector DriverSide = -GetActorRightVector() * 190.f;
	const FVector PassengerSide = GetActorRightVector() * 190.f;
	const FVector FarDriverSide = -GetActorRightVector() * 360.f;
	const FVector FarPassengerSide = GetActorRightVector() * 360.f;
	const FVector Rear = -GetActorForwardVector() * 300.f;
	const FVector Front = GetActorForwardVector() * 300.f;
	const FVector PreferredSide = bPreferDriverSide ? DriverSide : PassengerSide;
	const FVector OtherSide = bPreferDriverSide ? PassengerSide : DriverSide;
	const FVector PreferredFarSide = bPreferDriverSide ? FarDriverSide : FarPassengerSide;
	const FVector OtherFarSide = bPreferDriverSide ? FarPassengerSide : FarDriverSide;
	TArray<FVector> Candidates = {
		CarLocation + PreferredSide, CarLocation + OtherSide,
		CarLocation + PreferredFarSide, CarLocation + OtherFarSide,
		CarLocation + Rear, CarLocation + Front,
	};

	// If nearby props or parked cars block both doors, search every authored
	// sidewalk corner rather than falling back into a road, lake, or map edge.
	TArray<FVector> SidewalkCorners;
	const float Reach = Grid::BlockSize * 0.5f - 360.f;
	for (int32 BlockX = 0; BlockX < Grid::NumBlocks; ++BlockX)
	{
		for (int32 BlockY = 0; BlockY < Grid::NumBlocks; ++BlockY)
		{
			if (Grid::IsOverLake(Grid::BlockCenter(BlockX), Grid::BlockCenter(BlockY), 0.f))
			{
				continue;
			}
			for (float SignX : { -1.f, 1.f })
			{
				for (float SignY : { -1.f, 1.f })
				{
					SidewalkCorners.Add(FVector(
						Grid::BlockCenter(BlockX) + SignX * Reach,
						Grid::BlockCenter(BlockY) + SignY * Reach,
						CarLocation.Z));
				}
			}
		}
	}
	SidewalkCorners.Sort([&](const FVector& A, const FVector& B)
	{
		return FVector::DistSquared2D(A, CarLocation) < FVector::DistSquared2D(B, CarLocation);
	});
	Candidates.Append(SidewalkCorners);

	FCollisionQueryParams Params(FName(TEXT("SprawlVehicleExit")), false, ActorToIgnore);
	Params.AddIgnoredActor(this);
	if (ActorToIgnore)
	{
		Params.AddIgnoredActor(ActorToIgnore);
	}
	const FCollisionShape Capsule = FCollisionShape::MakeCapsule(
		CapsuleRadius, CapsuleHalfHeight);

	for (FVector TestLocation : Candidates)
	{
		if (!Grid::IsInsideCityBounds(TestLocation.X, TestLocation.Y)
			|| Grid::IsOverLake(TestLocation.X, TestLocation.Y, 60.f)
			|| Grid::IsOnRoadSurface(TestLocation.X, TestLocation.Y, -40.f))
		{
			continue;
		}
		TestLocation.Z = CarLocation.Z + CapsuleHalfHeight + 8.f;
		if (!World->OverlapBlockingTestByChannel(
			TestLocation, FQuat::Identity, ECC_Pawn, Capsule, Params))
		{
			OutLocation = TestLocation;
			return true;
		}
	}
	return false;
}

bool ASprawlCar::HasVisibleDriver() const
{
	return DriverMesh && DriverMesh->IsVisible()
		&& DriverMesh->GetSkeletalMeshAsset() != nullptr;
}

void ASprawlCar::InitializeAIDriverVisual()
{
	if (bDriverVisualInitializationAttempted || !bAutoDrive || Driver
		|| Cast<APlayerController>(GetController()))
	{
		return;
	}
	bDriverVisualInitializationAttempted = true;

	const TArray<FString>& Variants = FSprawlAvatarLibrary::PedestrianVariants();
	if (Variants.IsEmpty())
	{
		return;
	}
	const int32 StartIndex = static_cast<int32>(GetTypeHash(GetFName()) % Variants.Num());
	bHasAIDriver = true; // logical occupancy survives a partial/missing art import
	for (int32 Offset = 0; Offset < Variants.Num(); ++Offset)
	{
		const FString& Candidate = Variants[(StartIndex + Offset) % Variants.Num()];
		if (ApplySeatedDriverVariant(Candidate))
		{
			AIDriverVariant = Candidate;
			return;
		}
	}
	AIDriverVariant = Variants[StartIndex];
	HideDriverVisual();
}

void ASprawlCar::ShowSeatedDriver(const FString& VariantName)
{
	if (ApplySeatedDriverVariant(VariantName))
	{
		return;
	}
	if (VariantName != TEXT("Cappy")
		&& ApplySeatedDriverVariant(TEXT("Cappy")))
	{
		return;
	}
	HideDriverVisual();
}

bool ASprawlCar::ApplySeatedDriverVariant(const FString& VariantName)
{
	USkeletalMesh* Mesh = FSprawlAvatarLibrary::LoadAvatarMesh(VariantName);
	UAnimSequence* Sit = FSprawlAvatarLibrary::LoadAvatarAnim(VariantName, TEXT("Sit"));
	UAnimSequence* Idle = FSprawlAvatarLibrary::LoadAvatarAnim(VariantName, TEXT("Idle"));
	UAnimSequence* Walk = FSprawlAvatarLibrary::LoadAvatarAnim(VariantName,
		FSprawlAvatarLibrary::UsesFormalWalk(VariantName) ? TEXT("WalkFormal") : TEXT("Walk"));
	UAnimSequence* Jog = FSprawlAvatarLibrary::LoadAvatarAnim(VariantName, TEXT("Jog"));
	if (!Mesh || !Sit || !Idle || !Walk || !Jog || !DriverMesh
		|| !FSprawlAvatarLibrary::ApplyAvatar(
			DriverMesh, Mesh, DriverVisualHeight, 0.f))
	{
		return false;
	}
	DriverMesh->SetRelativeLocationAndRotation(DriverSeatLocation, DriverSeatRotation);
	DriverCurrentAnim = nullptr;
	FSprawlAvatarLibrary::PlayLoop(DriverMesh, Sit, DriverCurrentAnim);
	DriverMesh->SetVisibility(true, true);
	DriverMesh->SetHiddenInGame(false);
	return true;
}

void ASprawlCar::HideDriverVisual()
{
	if (DriverMesh)
	{
		DriverMesh->Stop();
		DriverMesh->SetVisibility(false, true);
		DriverMesh->SetHiddenInGame(true);
	}
	DriverCurrentAnim = nullptr;
}

bool ASprawlCar::IsWithinCityBounds() const
{
	const FVector Location = GetActorLocation();
	return Grid::IsInsideCityBounds(Location.X, Location.Y, 300.f)
		&& Location.Z > -250.f;
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
	InputComponent->BindKey(EKeys::F, IE_Pressed, this, &ASprawlCar::OnExitKey);
}

void ASprawlCar::OnExitKey()
{
	HandleExit(FInputActionValue());
}

void ASprawlCar::RequestExit()
{
	ExitDriver();
}

void ASprawlCar::HandleMove(const FInputActionValue& Value)
{
	FVector2D Axis = Value.Get<FVector2D>();
	if (Axis.SizeSquared() < FMath::Square(InputDeadZone))
	{
		Axis = FVector2D::ZeroVector;
	}
	Axis = Axis.GetClampedToMaxSize(1.f);
	TargetSteerInput    = Axis.X;
	TargetThrottleInput = Axis.Y;
}

void ASprawlCar::HandleMoveEnd(const FInputActionValue& /*Value*/)
{
	TargetSteerInput = 0.f;
	TargetThrottleInput = 0.f;
}

void ASprawlCar::HandleExit(const FInputActionValue& /*Value*/)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPhoneSubsystem* Phone = GI->GetSubsystem<UPhoneSubsystem>();
			Phone && Phone->TryAnswerRingingCall())
		{
			return;
		}
	}

	RequestExit();
}

void ASprawlCar::ExitDriver()
{
	if (Driver)
	{
		AZarriCharacter* ExitingDriver = Driver;
		ThrottleInput = 0.f;
		SteerInput = 0.f;
		TargetThrottleInput = 0.f;
		TargetSteerInput = 0.f;
		Driver = nullptr;
		HideDriverVisual();
		if (bPendingCarjack)
		{
			bResumeAutoDriveAfterExit = false;
			RestorePendingCarjack();
		}
		else
		{
			bAutoDrive = bResumeAutoDriveAfterExit;
			bResumeAutoDriveAfterExit = false;
			bAIStateSeeded = false;
			if (bAutoDrive)
			{
				bDriverVisualInitializationAttempted = false;
			}
		}
		ExitingDriver->OnExitedVehicle(this);
	}
}

void ASprawlCar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Hull || !Hull->IsSimulatingPhysics())
	{
		return;
	}
	CrashUprightGraceRemaining = FMath::Max(
		0.f, CrashUprightGraceRemaining - DeltaSeconds);
	EnforceCityBoundary();
	MaintainUpright(DeltaSeconds);
	UpdateSafeRecoveryTransform();
	UpdateWheelVisuals(DeltaSeconds);

	const bool bPlayerDriving = Cast<APlayerController>(GetController()) != nullptr;
	if (bAutoDrive && !bPlayerDriving)
	{
		InitializeAIDriverVisual();
	}
	if (!bPlayerDriving && !bAutoDrive)
	{
		// Parked, unpossessed cars just sit.
		return;
	}
	if (bAutoDrive && !bPlayerDriving)
	{
		RunAutoDrive(DeltaSeconds);
		// AI drives physics directly inside RunAutoDrive — skip the
		// AddForce-based player path so the two paths don't fight.
		return;
	}

	const FVector Fwd = GetActorForwardVector();
	const FVector Vel = Hull->GetPhysicsLinearVelocity();
	const float ForwardSpeed = FVector::DotProduct(Vel, Fwd);
	ThrottleInput = FMath::FInterpTo(
		ThrottleInput, TargetThrottleInput, DeltaSeconds, ThrottleResponse);
	SteerInput = FMath::FInterpTo(
		SteerInput, TargetSteerInput, DeltaSeconds, SteeringResponse);

	// Throttle / reverse
	if (FMath::Abs(ThrottleInput) > 0.05f)
	{
		Hull->AddForce(Fwd * ThrottleInput * EngineForce);
	}

	// Steering — yaw rate scales with speed, flips sign in reverse.
	if (FMath::Abs(SteerInput) > 0.05f && FMath::Abs(ForwardSpeed) > 25.f)
	{
		const float SpeedFactor = FMath::Clamp(FMath::Abs(ForwardSpeed) / 520.f, 0.35f, 1.f);
		const float Dir = FMath::Sign(ForwardSpeed);
		const float TargetYaw = SteerInput * Dir * TurnRate * SpeedFactor;
		Hull->SetPhysicsAngularVelocityInDegrees(
			FVector(0.f, 0.f, TargetYaw));
	}
}

void ASprawlCar::UpdateWheelVisuals(float DeltaSeconds)
{
	if (!Hull || WheelPivots.Num() != 4)
	{
		return;
	}

	const float SignedSpeed = FVector::DotProduct(
		Hull->GetPhysicsLinearVelocity(), GetActorForwardVector());
	const float Radius = FMath::Max(1.f, VisualWheelRadius);
	WheelRotationDegrees = FMath::Fmod(
		WheelRotationDegrees - FMath::RadiansToDegrees(SignedSpeed * DeltaSeconds / Radius),
		360.f);

	const float SteeringDegrees = SteerInput * VisualSteeringAngle;
	for (int32 Index = 0; Index < WheelPivots.Num(); ++Index)
	{
		const float WheelSteering = Index < 2 ? SteeringDegrees : 0.f;
		const FQuat SteeringQuat(FVector::UpVector, FMath::DegreesToRadians(WheelSteering));
		const FQuat SpinQuat(FVector::RightVector, FMath::DegreesToRadians(WheelRotationDegrees));
		WheelPivots[Index]->SetRelativeRotation(SteeringQuat * SpinQuat);
	}
}

void ASprawlCar::TryApplyRuntimeVehicleParts()
{
	if (!FullBodyMesh || FullBodyMesh->GetStaticMesh()
		|| (FullSkeletalBodyMesh && FullSkeletalBodyMesh->GetSkeletalMeshAsset()))
	{
		return;
	}

	const int32 Variant = static_cast<int32>(GetTypeHash(GetFName()) % 10u) + 1;
	const FString BasePath = FString::Printf(TEXT("/Game/Vehicles/Animated/Car_%d"), Variant);
	auto LoadPart = [&](const TCHAR* Suffix) -> UStaticMesh*
	{
		const FString AssetName = FString::Printf(TEXT("SM_Car_%d_%s"), Variant, Suffix);
		const FString ObjectPath = FString::Printf(TEXT("%s/%s.%s"),
			*BasePath, *AssetName, *AssetName);
		return LoadObject<UStaticMesh>(nullptr, *ObjectPath);
	};

	TArray<UStaticMesh*> Bodies = { LoadPart(TEXT("Body")) };
	for (int32 DetailIndex = 0; DetailIndex < 32; ++DetailIndex)
	{
		const FString Suffix = FString::Printf(TEXT("Detail_%02d"), DetailIndex);
		UStaticMesh* Detail = LoadPart(*Suffix);
		if (!Detail)
		{
			break;
		}
		Bodies.Add(Detail);
	}
	TArray<UStaticMesh*> Wheels = {
		LoadPart(TEXT("Wheel_FL")), LoadPart(TEXT("Wheel_FR")),
		LoadPart(TEXT("Wheel_RL")), LoadPart(TEXT("Wheel_RR"))
	};
	if (!Bodies[0] || Wheels.Contains(nullptr))
	{
		return;
	}

	const FBox BodyBounds = Bodies[0]->GetBoundingBox();
	const FVector BodySize = BodyBounds.GetSize();
	const double NaturalLength = FMath::Max3<double>(BodySize.X, BodySize.Y, 1.0);
	const float Scale = static_cast<float>(
		FMath::Clamp(480.0 / NaturalLength, 0.45, 1.4));
	const FVector RelativeScale(Scale);
	const FVector RelativeLocation(0.f, 0.f, -62.f - BodyBounds.Min.Z * Scale);
	const FRotator RelativeRotation(0.f, 90.f, 0.f);

	TArray<FVector> Centers;
	for (UStaticMesh* Wheel : Wheels)
	{
		Centers.Add(Wheel->GetBoundingBox().GetCenter());
	}
	SetExternalVehicleParts(Bodies, Wheels, Centers,
		RelativeLocation, RelativeRotation, RelativeScale);
}

void ASprawlCar::MaintainUpright(float DeltaSeconds)
{
	(void)DeltaSeconds;
	if (!Hull)
	{
		return;
	}
	if (CrashUprightGraceRemaining > 0.f)
	{
		return;
	}

	const FVector AngularVelocity = Hull->GetPhysicsAngularVelocityInDegrees();
	if (!FMath::IsNearlyZero(AngularVelocity.X) || !FMath::IsNearlyZero(AngularVelocity.Y))
	{
		Hull->SetPhysicsAngularVelocityInDegrees(
			FVector(0.f, 0.f, AngularVelocity.Z));
	}

	constexpr float MinimumUprightDot = 0.995f;
	if (FVector::DotProduct(Hull->GetUpVector(), FVector::UpVector) >= MinimumUprightDot)
	{
		return;
	}

	FVector FlatForward = Hull->GetForwardVector();
	FlatForward.Z = 0.f;
	const float UprightYaw = FlatForward.Normalize()
		? FlatForward.Rotation().Yaw
		: Hull->GetComponentRotation().Yaw;
	const FVector LinearVelocity = Hull->GetPhysicsLinearVelocity();
	Hull->SetWorldRotation(FRotator(0.f, UprightYaw, 0.f), false, nullptr,
		ETeleportType::TeleportPhysics);
	Hull->SetPhysicsLinearVelocity(LinearVelocity);
}

void ASprawlCar::HandleHullHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	(void)HitComponent;
	(void)OtherActor;
	(void)OtherComponent;
	if (!Hull || Hit.ImpactNormal.Z > 0.75f)
	{
		return;
	}

	const float Speed = Hull->GetPhysicsLinearVelocity().Size();
	if (Speed >= CrashSpeedThreshold && NormalImpulse.Size() >= CrashImpulseThreshold)
	{
		CrashUprightGraceRemaining = FMath::Max(
			CrashUprightGraceRemaining, CrashUprightGraceSeconds);
		UE_LOG(LogTemp, Log, TEXT("[Car] Crash response active for %s (speed=%.0f impulse=%.0f)"),
			*GetName(), Speed, NormalImpulse.Size());
	}
}

void ASprawlCar::UpdateSafeRecoveryTransform()
{
	if (!Hull || CrashUprightGraceRemaining > 0.f)
	{
		return;
	}
	const FVector Location = GetActorLocation();
	const float UprightDot = FVector::DotProduct(GetActorUpVector(), FVector::UpVector);
	if (Location.Z >= 20.f && Location.Z <= 400.f && UprightDot >= 0.98f
		&& Grid::IsNearDrivableRoad(Location.X, Location.Y))
	{
		LastSafeTransform = GetActorTransform();
		bHasLastSafeTransform = true;
	}
}

void ASprawlCar::EnforceCityBoundary()
{
	if (IsWithinCityBounds())
	{
		return;
	}

	ReleaseIntersectionReservation();
	FTransform Recovery = LastSafeTransform;
	if (!bHasLastSafeTransform)
	{
		const FVector Location = GetActorLocation();
		const ESprawlHeading Heading = Grid::HeadingFromYaw(GetActorRotation().Yaw);
		const bool bNorthSouth = Grid::IsNorthSouth(Heading);
		const int32 RoadIndex = Grid::NearestRoadIndex(bNorthSouth ? Location.X : Location.Y);
		FVector SafeLocation = Location;
		if (bNorthSouth)
		{
			SafeLocation.X = Grid::LaneCenter(RoadIndex, Heading);
			SafeLocation.Y = FMath::Clamp(SafeLocation.Y,
				-Grid::CityBoundaryHalfExtent + 500.f, Grid::CityBoundaryHalfExtent - 500.f);
		}
		else
		{
			SafeLocation.Y = Grid::LaneCenter(RoadIndex, Heading);
			SafeLocation.X = FMath::Clamp(SafeLocation.X,
				-Grid::CityBoundaryHalfExtent + 500.f, Grid::CityBoundaryHalfExtent - 500.f);
		}
		SafeLocation.Z = 130.f;
		Recovery = FTransform(FRotator(0.f, Grid::HeadingYaw(Heading), 0.f), SafeLocation);
	}

	FVector Forward = Recovery.GetRotation().GetForwardVector();
	Forward.Z = 0.f;
	const float Yaw = Forward.Normalize() ? Forward.Rotation().Yaw : GetActorRotation().Yaw;
	Recovery.SetRotation(FRotator(0.f, Yaw, 0.f).Quaternion());
	Recovery.SetScale3D(GetActorScale3D());
	SetActorTransform(Recovery, false, nullptr, ETeleportType::TeleportPhysics);
	if (Hull)
	{
		Hull->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Hull->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}
	CrashUprightGraceRemaining = 0.f;
	bAIStateSeeded = false;
	AIDecidedCrossing = -1;
	UE_LOG(LogTemp, Warning, TEXT("[Car] Restored %s inside the city boundary"), *GetName());
}

void ASprawlCar::ResolveMove(ESprawlHeading InHeading, int32 InRoadIndex, int32 CrossingRoadIndex,
	int32 Turn, ESprawlHeading& OutHeading, int32& OutRoadIndex)
{
	if (Turn == 0)
	{
		OutHeading = InHeading;
		OutRoadIndex = InRoadIndex;
		return;
	}

	// Headings follow Unreal's increasing yaw (East, North, West, South).
	// At yaw zero the actor's local right is world +Y, so a right turn steps
	// +1 and remains consistent with LaneCenter's right-hand-lane convention.
	const int32 Steps = (Turn > 0) ? 1 : 3;
	OutHeading = static_cast<ESprawlHeading>((static_cast<int32>(InHeading) + Steps) % 4);
	// After turning we travel along what was the crossing road.
	OutRoadIndex = CrossingRoadIndex;
}

void ASprawlCar::DecideIntersectionMove(int32 CrossingRoadIndex)
{
	const FVector2D Center = Grid::IsNorthSouth(AIHeading)
		? FVector2D(Grid::RoadCenter(AIRoadIndex), Grid::RoadCenter(CrossingRoadIndex))
		: FVector2D(Grid::RoadCenter(CrossingRoadIndex), Grid::RoadCenter(AIRoadIndex));

	// A move is legal if one block past the intersection is still on the
	// grid and not over the lake.
	auto IsLegal = [&](int32 Turn) -> bool
	{
		ESprawlHeading NewHeading;
		int32 NewRoad;
		ResolveMove(AIHeading, AIRoadIndex, CrossingRoadIndex, Turn, NewHeading, NewRoad);
		const FVector Dir = Grid::HeadingVector(NewHeading);
		const FVector2D Probe = Center + FVector2D(Dir.X, Dir.Y) * Grid::Step;
		return Grid::IsInsideRoadGrid(Probe.X, Probe.Y) && !Grid::IsOverLake(Probe.X, Probe.Y);
	};

	// Weighted choice: mostly straight, sometimes a turn; fall back to any
	// legal move, and finally a U-turn if the car is boxed in.
	const float Roll = FMath::FRand();
	int32 Preferred = (Roll < 0.55f) ? 0 : (Roll < 0.80f) ? 1 : -1;

	if (!IsLegal(Preferred))
	{
		Preferred = 99; // sentinel: none chosen yet
		for (int32 Turn : { 0, 1, -1 })
		{
			if (IsLegal(Turn))
			{
				Preferred = Turn;
				break;
			}
		}
		if (Preferred == 99)
		{
			// Dead end: U-turn in place at the intersection.
			AIHeading = static_cast<ESprawlHeading>((static_cast<int32>(AIHeading) + 2) % 4);
			AIDecidedCrossing = -1;
			return;
		}
	}

	AIPendingTurn = Preferred;
	AIDecidedCrossing = CrossingRoadIndex;
}

float ASprawlCar::SenseObstacleAhead(float SenseLength) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return -1.f;
	}

	const FVector Fwd = GetActorForwardVector();
	const FVector Start = GetActorLocation() + Fwd * 280.f + FVector(0, 0, 30.f);
	const FVector End = Start + Fwd * SenseLength;

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);        // pedestrians, characters
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody); // other cars' hulls
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic); // parked cars / road obstructions

	FCollisionQueryParams Params(FName(TEXT("SprawlCarSense")), false, this);

	FHitResult Hit;
	const bool bHit = World->SweepSingleByObjectType(
		Hit, Start, End, GetActorQuat(),
		ObjectParams, FCollisionShape::MakeBox(FVector(20.f, 110.f, 55.f)), Params);

	return bHit ? FMath::Max(0.f, Hit.Distance) : -1.f;
}

bool ASprawlCar::IsIntersectionPathClear(
	const FVector2D& Center, int32 CrossingRoadIndex) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	FCollisionQueryParams Params(FName(TEXT("SprawlIntersectionClearance")), false, this);

	auto HasOtherVehicle = [&](const FVector& TestLocation, const FQuat& TestRotation,
		const FVector& HalfExtent) -> bool
	{
		TArray<FOverlapResult> Overlaps;
		if (!World->OverlapMultiByObjectType(Overlaps, TestLocation, TestRotation,
			ObjectParams, FCollisionShape::MakeBox(HalfExtent), Params))
		{
			return false;
		}
		for (const FOverlapResult& Overlap : Overlaps)
		{
			if (Overlap.GetActor() != this && Cast<ASprawlCar>(Overlap.GetActor()))
			{
				return true;
			}
		}
		return false;
	};

	const FVector BoxCenter(Center.X, Center.Y, GetActorLocation().Z);
	if (HasOtherVehicle(BoxCenter, FQuat::Identity, FVector(330.f, 330.f, 100.f)))
	{
		return false;
	}

	ESprawlHeading ExitHeading;
	int32 ExitRoadIndex = 0;
	ResolveMove(AIHeading, AIRoadIndex, CrossingRoadIndex, AIPendingTurn,
		ExitHeading, ExitRoadIndex);
	const FVector ExitDirection = Grid::HeadingVector(ExitHeading);
	FVector ExitCenter = BoxCenter + ExitDirection * 900.f;
	const float ExitLaneCenter = Grid::LaneCenter(ExitRoadIndex, ExitHeading);
	if (Grid::IsNorthSouth(ExitHeading))
	{
		ExitCenter.X = ExitLaneCenter;
	}
	else
	{
		ExitCenter.Y = ExitLaneCenter;
	}
	const FQuat ExitRotation = FRotator(0.f, Grid::HeadingYaw(ExitHeading), 0.f).Quaternion();
	return !HasOtherVehicle(ExitCenter, ExitRotation, FVector(360.f, 115.f, 100.f));
}

void ASprawlCar::ReleaseIntersectionReservation()
{
	if (AIReservedIntersectionX < 0 || AIReservedIntersectionY < 0)
	{
		return;
	}
	if (USprawlCityGridSubsystem* GridSub = GetWorld()
		? GetWorld()->GetSubsystem<USprawlCityGridSubsystem>() : nullptr)
	{
		GridSub->ReleaseIntersection(
			AIReservedIntersectionX, AIReservedIntersectionY, this);
	}
	AIReservedIntersectionX = -1;
	AIReservedIntersectionY = -1;
	bAIClearingIntersection = false;
}

void ASprawlCar::RunAutoDrive(float DeltaSeconds)
{
	USprawlCityGridSubsystem* GridSub = GetWorld()
		? GetWorld()->GetSubsystem<USprawlCityGridSubsystem>() : nullptr;

	const FVector Loc = GetActorLocation();
	if (bAIClearingIntersection
		&& FVector2D::Distance(FVector2D(Loc.X, Loc.Y), AILastIntersection) > 700.f)
	{
		ReleaseIntersectionReservation();
	}

	// Seed driving state from the spawn transform: snap heading to the road
	// grid and latch onto the nearest road for that axis.
	if (!bAIStateSeeded)
	{
		AIHeading = Grid::HeadingFromYaw(GetActorRotation().Yaw);
		AIRoadIndex = Grid::IsNorthSouth(AIHeading)
			? Grid::NearestRoadIndex(Loc.X)
			: Grid::NearestRoadIndex(Loc.Y);
		bAIStateSeeded = true;
	}

	const bool bNS = Grid::IsNorthSouth(AIHeading);
	const float TravelDir = (AIHeading == ESprawlHeading::North || AIHeading == ESprawlHeading::East)
		? 1.f : -1.f;
	const float TravelCoord = bNS ? Loc.Y : Loc.X;

	// --- Find the next crossing road ahead of us ---
	int32 NextCrossing = -1;
	float DistAhead = TNumericLimits<float>::Max();
	for (int32 R = 0; R < Grid::NumRoads; ++R)
	{
		const float D = (Grid::RoadCenter(R) - TravelCoord) * TravelDir;
		if (D > -80.f && D < DistAhead)
		{
			DistAhead = D;
			NextCrossing = R;
		}
	}

	if (NextCrossing < 0)
	{
		// Ran out of road (past the last crossing): turn back toward the city.
		ReleaseIntersectionReservation();
		AIHeading = static_cast<ESprawlHeading>((static_cast<int32>(AIHeading) + 2) % 4);
		AIDecidedCrossing = -1;
		return;
	}

	const FVector2D CrossingCenter = bNS
		? FVector2D(Grid::RoadCenter(AIRoadIndex), Grid::RoadCenter(NextCrossing))
		: FVector2D(Grid::RoadCenter(NextCrossing), Grid::RoadCenter(AIRoadIndex));

	// --- Decide our move for the upcoming intersection, once ---
	// The distance-from-last-intersection latch stops us re-deciding while
	// still rolling through the crossing we just turned at.
	constexpr float DecisionDistance = 1100.f;
	if (DistAhead < DecisionDistance && AIDecidedCrossing != NextCrossing &&
		FVector2D::Distance(FVector2D(Loc.X, Loc.Y), AILastIntersection) > 700.f)
	{
		DecideIntersectionMove(NextCrossing);
	}

	// --- Speed planning ---
	float TargetSpeed = AICruiseSpeed;
	const float CurSpeed = Hull ? Hull->GetPhysicsLinearVelocity().Size2D() : 0.f;

	// Traffic signal + intersection lease: a car may enter only when its
	// approach is served, the box is unoccupied, and its exit lane has room.
	// Cars denied a lease queue at the same painted stop line as red traffic.
	constexpr float StopLine = Grid::VehicleStopDistance;
	constexpr float ReservationRequestDistance = 930.f;
	if (GridSub && DistAhead > 300.f && DistAhead < DecisionDistance)
	{
		const int32 Ix = bNS ? AIRoadIndex : NextCrossing;
		const int32 Iy = bNS ? NextCrossing : AIRoadIndex;
		const ESprawlSignal Signal = GridSub->GetSignal(Ix, Iy, bNS);
		bool bHasReservation = GridSub->HasIntersectionReservation(Ix, Iy, this);
		const float RoomToLine = FMath::Max(0.f, DistAhead - StopLine);
		constexpr float ComfortableBrakingDecel = 700.f;
		constexpr float AmberReactionTime = 0.25f;
		const float SafeStoppingDistance =
			FMath::Square(CurSpeed) / (2.f * ComfortableBrakingDecel)
			+ CurSpeed * AmberReactionTime;
		const bool bLateAmber = Signal == ESprawlSignal::Amber
			&& CurSpeed > 80.f && RoomToLine <= SafeStoppingDistance;
		const bool bSignalAllowsEntry = Signal == ESprawlSignal::Green || bLateAmber;

		if (bHasReservation)
		{
			// Refresh the short lease while approaching or clearing the box.
			GridSub->TryReserveIntersection(Ix, Iy, this);
		}
		else if (bSignalAllowsEntry && DistAhead <= ReservationRequestDistance
			&& AIDecidedCrossing == NextCrossing
			&& IsIntersectionPathClear(CrossingCenter, NextCrossing)
			&& GridSub->TryReserveIntersection(Ix, Iy, this))
		{
			AIReservedIntersectionX = Ix;
			AIReservedIntersectionY = Iy;
			bHasReservation = true;
		}

		// Once admitted, the lease remains authoritative through amber/all-red:
		// braking again after the phase changes would strand a car in the box and
		// block the next approach. Cars without a lease still obey the live head.
		const bool bMustStop = !bHasReservation && (!bSignalAllowsEntry
			|| DistAhead <= ReservationRequestDistance);
		if (bMustStop)
		{
			// Comfortable braking curve: v = sqrt(2 * a * d), a ~ 700 cm/s^2.
			TargetSpeed = FMath::Min(TargetSpeed, FMath::Sqrt(2.f * 700.f * RoomToLine));
			if (RoomToLine < 25.f)
			{
				TargetSpeed = 0.f;
			}
		}
	}

	// Slow for turns through the intersection.
	if (AIPendingTurn != 0 && DistAhead < 700.f)
	{
		TargetSpeed = FMath::Min(TargetSpeed, AITurnSpeed);
	}

	// Car / pedestrian ahead: match a safe gap, hard-stop when close.
	const float SenseLength = FMath::Max(450.f, CurSpeed * 1.1f);
	const float ObstacleDist = SenseObstacleAhead(SenseLength);
	if (ObstacleDist >= 0.f)
	{
		TargetSpeed = FMath::Min(TargetSpeed, FMath::Max(0.f, (ObstacleDist - 160.f) * 2.2f));
	}

	// --- Execute the turn on the near-side lane center ---
	// Beginning a turn only at the road center makes a 4.7m car overshoot the
	// receiving lane and brush curbside parking. The lane-offset point gives
	// the visual steering arc room to arrive centered without cutting the box.
	const float CommitDistance = AIPendingTurn == 0 ? 90.f : Grid::LaneOffset;
	if (AIDecidedCrossing == NextCrossing && DistAhead < CommitDistance)
	{
		ESprawlHeading NewHeading;
		int32 NewRoad;
		ResolveMove(AIHeading, AIRoadIndex, NextCrossing, AIPendingTurn, NewHeading, NewRoad);
		AIHeading = NewHeading;
		AIRoadIndex = NewRoad;
		AIDecidedCrossing = -1;
		AIPendingTurn = 0;
		AILastIntersection = CrossingCenter;
		bAIClearingIntersection = true;
	}

	// --- Lane keeping: steer toward a lookahead point on the lane center ---
	const float LaneCoord = Grid::LaneCenter(AIRoadIndex, AIHeading);
	// A compact lookahead recenters the car promptly after a turn without the
	// long, shallow diagonal that can otherwise cross the center line.
	FVector LookAhead = Loc + Grid::HeadingVector(AIHeading) * 360.f;
	if (Grid::IsNorthSouth(AIHeading)) { LookAhead.X = LaneCoord; }
	else                               { LookAhead.Y = LaneCoord; }

	const float DesiredYaw = FMath::RadiansToDegrees(
		FMath::Atan2(LookAhead.Y - Loc.Y, LookAhead.X - Loc.X));
	const float YawError = FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, DesiredYaw);
	SteerInput = FMath::Clamp(YawError / 30.f, -1.f, 1.f);

	// Big heading error (mid-turn / U-turn): crawl until we're pointed right.
	if (FMath::Abs(YawError) > 55.f)
	{
		TargetSpeed = FMath::Min(TargetSpeed, AITurnSpeed * 0.8f);
	}

	// --- Drive the physics body ---
	AISmoothedSpeed = FMath::FInterpConstantTo(AISmoothedSpeed, TargetSpeed, DeltaSeconds,
		TargetSpeed < AISmoothedSpeed ? 1400.f : 600.f); // brake harder than we accelerate
	ThrottleInput = (AICruiseSpeed > 1.f) ? AISmoothedSpeed / AICruiseSpeed : 0.f;

	if (Hull)
	{
		const FVector Fwd = GetActorForwardVector();
		const FVector CurVel = Hull->GetPhysicsLinearVelocity();
		const FVector Drive = Fwd * AISmoothedSpeed;
		// Keep gravity acting on Z; drive the horizontal velocity directly so
		// damping / micro-collisions can't stall the AI car.
		Hull->SetPhysicsLinearVelocity(FVector(Drive.X, Drive.Y, CurVel.Z));
		Hull->SetPhysicsAngularVelocityInDegrees(
			FVector(0.f, 0.f, SteerInput * TurnRate));
	}
}
