// The Connected Sprawl - A drivable physics car.

#include "Vehicles/SprawlCar.h"
#include "Characters/ZarriCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/MeshComponent.h"
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
#include "Engine/World.h"
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

	const bool bPlayerDriving = Cast<APlayerController>(GetController()) != nullptr;
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

void ASprawlCar::ResolveMove(ESprawlHeading InHeading, int32 InRoadIndex, int32 CrossingRoadIndex,
	int32 Turn, ESprawlHeading& OutHeading, int32& OutRoadIndex)
{
	if (Turn == 0)
	{
		OutHeading = InHeading;
		OutRoadIndex = InRoadIndex;
		return;
	}

	// Headings follow increasing yaw (East, North, West, South). In UE,
	// +90 yaw is a right turn, so right = +1 step and left = +3 (== -1 mod 4).
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

	FCollisionQueryParams Params(FName(TEXT("SprawlCarSense")), false, this);

	FHitResult Hit;
	const bool bHit = World->SweepSingleByObjectType(
		Hit, Start, End, GetActorQuat(),
		ObjectParams, FCollisionShape::MakeBox(FVector(20.f, 110.f, 55.f)), Params);

	return bHit ? FMath::Max(0.f, Hit.Distance) : -1.f;
}

void ASprawlCar::RunAutoDrive(float DeltaSeconds)
{
	USprawlCityGridSubsystem* GridSub = GetWorld()
		? GetWorld()->GetSubsystem<USprawlCityGridSubsystem>() : nullptr;

	const FVector Loc = GetActorLocation();

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

	// Traffic signal: brake to the stop line on red, or on amber if we can
	// still stop comfortably. Once we're at the line, hold at zero.
	// 750 puts the front bumper just behind the painted crosswalk.
	constexpr float StopLine = 750.f;
	if (GridSub && DistAhead > 300.f && DistAhead < DecisionDistance)
	{
		const int32 Ix = bNS ? AIRoadIndex : NextCrossing;
		const int32 Iy = bNS ? NextCrossing : AIRoadIndex;
		const ESprawlSignal Signal = GridSub->GetSignal(Ix, Iy, bNS);
		const bool bMustStop = (Signal == ESprawlSignal::Red)
			|| (Signal == ESprawlSignal::Amber && DistAhead > StopLine + 150.f);
		if (bMustStop)
		{
			const float RoomToLine = FMath::Max(0.f, DistAhead - StopLine);
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
	const float CurSpeed = Hull ? Hull->GetPhysicsLinearVelocity().Size2D() : 0.f;
	const float SenseLength = FMath::Max(450.f, CurSpeed * 1.1f);
	const float ObstacleDist = SenseObstacleAhead(SenseLength);
	if (ObstacleDist >= 0.f)
	{
		TargetSpeed = FMath::Min(TargetSpeed, FMath::Max(0.f, (ObstacleDist - 160.f) * 2.2f));
	}

	// --- Execute the turn at the intersection center ---
	if (AIDecidedCrossing == NextCrossing && DistAhead < 90.f)
	{
		ESprawlHeading NewHeading;
		int32 NewRoad;
		ResolveMove(AIHeading, AIRoadIndex, NextCrossing, AIPendingTurn, NewHeading, NewRoad);
		AIHeading = NewHeading;
		AIRoadIndex = NewRoad;
		AIDecidedCrossing = -1;
		AIPendingTurn = 0;
		AILastIntersection = CrossingCenter;
	}

	// --- Lane keeping: steer toward a lookahead point on the lane center ---
	const float LaneCoord = Grid::LaneCenter(AIRoadIndex, AIHeading);
	FVector LookAhead = Loc + Grid::HeadingVector(AIHeading) * 600.f;
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
		const FVector AngVel = Hull->GetPhysicsAngularVelocityInDegrees();
		Hull->SetPhysicsAngularVelocityInDegrees(
			FVector(AngVel.X, AngVel.Y, SteerInput * TurnRate));
	}
}
