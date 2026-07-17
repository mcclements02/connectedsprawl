// The Connected Sprawl - Intersection traffic signal.

#include "World/SprawlTrafficLight.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ASprawlTrafficLight::ASprawlTrafficLight()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f; // signals only need ~10Hz

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	// Prefer the project's emissive signal material (optional: authored by the
	// materials pass); fall back to the engine basic-shape material, which at
	// least exposes a "Color" vector parameter.
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> GlowMat(
		TEXT("/Game/Materials/M_SignalGlow.M_SignalGlow"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMat(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	UStaticMesh* Cube = CubeMesh.Succeeded() ? CubeMesh.Object.Get() : nullptr;
	UStaticMesh* Cylinder = CylinderMesh.Succeeded() ? CylinderMesh.Object.Get() : Cube;
	UMaterialInterface* LampBase = GlowMat.Get()
		? GlowMat.Get()
		: (BasicMat.Succeeded() ? BasicMat.Object.Get() : nullptr);

	auto MakePart = [&](const TCHAR* Name, UStaticMesh* Mesh, FVector RelLoc,
	                    FVector Scale) -> UStaticMeshComponent*
	{
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		if (RootComponent) { C->SetupAttachment(RootComponent); }
		else { RootComponent = C; }
		if (Mesh) { C->SetStaticMesh(Mesh); }
		C->SetRelativeLocation(RelLoc);
		C->SetRelativeScale3D(Scale);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return C;
	};

	// Pole: 460cm tall cylinder. Heads hang near the top, one per axis.
	Pole = MakePart(TEXT("Pole"), Cylinder, FVector(0, 0, 230), FVector(0.14f, 0.14f, 4.6f));
	Pole->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Pole->SetCollisionProfileName(TEXT("BlockAll"));

	HeadNS = MakePart(TEXT("HeadNS"), Cube, FVector(0, 38, 430), FVector(0.30f, 0.34f, 0.78f));
	HeadEW = MakePart(TEXT("HeadEW"), Cube, FVector(38, 0, 430), FVector(0.34f, 0.30f, 0.78f));
	LampNS = MakePart(TEXT("LampNS"), Cube, FVector(0, 58, 430), FVector(0.22f, 0.10f, 0.60f));
	LampEW = MakePart(TEXT("LampEW"), Cube, FVector(58, 0, 430), FVector(0.10f, 0.22f, 0.60f));

	if (LampBase)
	{
		LampNS->SetMaterial(0, LampBase);
		LampEW->SetMaterial(0, LampBase);
	}
}

void ASprawlTrafficLight::BeginPlay()
{
	Super::BeginPlay();

	if (IntersectionX < 0 || IntersectionY < 0)
	{
		const FVector Loc = GetActorLocation();
		IntersectionX = USprawlCityGridSubsystem::NearestRoadIndex(Loc.X);
		IntersectionY = USprawlCityGridSubsystem::NearestRoadIndex(Loc.Y);
	}

	if (LampNS) { LampMatNS = LampNS->CreateAndSetMaterialInstanceDynamic(0); }
	if (LampEW) { LampMatEW = LampEW->CreateAndSetMaterialInstanceDynamic(0); }
}

void ASprawlTrafficLight::ApplySignalColor(UMaterialInstanceDynamic* Mat, ESprawlSignal Signal) const
{
	if (!Mat)
	{
		return;
	}

	FLinearColor Color;
	switch (Signal)
	{
	case ESprawlSignal::Green: Color = FLinearColor(0.05f, 1.6f, 0.25f); break;
	case ESprawlSignal::Amber: Color = FLinearColor(1.8f, 0.9f, 0.05f); break;
	default:                   Color = FLinearColor(2.0f, 0.06f, 0.04f); break;
	}

	// Set both parameter names so either the project glow material
	// ("GlowColor" -> emissive) or the engine fallback ("Color") responds.
	Mat->SetVectorParameterValue(TEXT("GlowColor"), Color);
	Mat->SetVectorParameterValue(TEXT("Color"), Color);
}

void ASprawlTrafficLight::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const USprawlCityGridSubsystem* Grid =
		GetWorld() ? GetWorld()->GetSubsystem<USprawlCityGridSubsystem>() : nullptr;
	if (!Grid)
	{
		return;
	}

	const ESprawlSignal NS = Grid->GetSignal(IntersectionX, IntersectionY, true);
	const ESprawlSignal EW = Grid->GetSignal(IntersectionX, IntersectionY, false);

	if (bFirstUpdate || NS != LastNS)
	{
		ApplySignalColor(LampMatNS, NS);
		LastNS = NS;
	}
	if (bFirstUpdate || EW != LastEW)
	{
		ApplySignalColor(LampMatEW, EW);
		LastEW = EW;
	}
	bFirstUpdate = false;
}
