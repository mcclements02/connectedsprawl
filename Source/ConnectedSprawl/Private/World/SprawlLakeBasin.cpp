// The Connected Sprawl - Sinks the lake into a basin below the ground.

#include "World/SprawlLakeBasin.h"

#include "World/SprawlWaterfrontScenery.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
// Half-extent of the city floor (City_Ground) this ring replaces.
constexpr float RingExtent = 11446.f;
constexpr float RingThickness = 40.f;
// The basin bed sits this far below the water; the quay curb this far above Z0.
constexpr float BedDropBelowWater = 90.f;
constexpr float QuayTopZ = 15.f;
constexpr float QuayThickness = 40.f;

const FLinearColor AsphaltColor(0.050f, 0.050f, 0.055f, 1.f);
const FLinearColor QuayColor(0.115f, 0.115f, 0.125f, 1.f);
const FLinearColor BedColor(0.050f, 0.045f, 0.038f, 1.f);
}

ASprawlLakeBasin::ASprawlLakeBasin()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Cube(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> Opaque(
		TEXT("/Game/Materials/Master/M_Simple_Opaque.M_Simple_Opaque"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> Asphalt(
		TEXT("/Game/Import/CityKit/MI_AsphaltWorn.MI_AsphaltWorn"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> Concrete(
		TEXT("/Game/Import/CityKit/MI_ConcreteWorn.MI_ConcreteWorn"));
	CubeMesh = Cube.Get();
	OpaqueBase = Opaque.Get();
	AsphaltMaterial = Asphalt.Get();
	ConcreteMaterial = Concrete.Get();
}

UStaticMeshComponent* ASprawlLakeBasin::AddBox(const FString& Name,
	const FVector& Center, const FVector& SizeCm, const FLinearColor& Color,
	float Roughness, bool bCollide, UMaterialInterface* Textured)
{
	if (!CubeMesh)
	{
		return nullptr;
	}
	UStaticMeshComponent* Box =
		NewObject<UStaticMeshComponent>(this, *Name);
	Box->SetupAttachment(RootComponent);
	Box->RegisterComponent();
	Box->SetStaticMesh(CubeMesh);
	// The engine cube is 100 cm, so a metric size maps to scale directly.
	Box->SetWorldScale3D(SizeCm / 100.f);
	Box->SetWorldLocation(Center);
	Box->SetCastShadow(false);
	Box->SetReceivesDecals(false);
	if (bCollide)
	{
		Box->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	}
	else
	{
		Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (Textured)
	{
		Box->SetMaterial(0, Textured);
	}
	else if (OpaqueBase)
	{
		UMaterialInstanceDynamic* Mat =
			UMaterialInstanceDynamic::Create(OpaqueBase, this);
		Mat->SetVectorParameterValue(TEXT("BaseColor"), Color);
		Mat->SetVectorParameterValue(TEXT("Color"), Color);
		Mat->SetScalarParameterValue(TEXT("Metallic"), 0.f);
		Mat->SetScalarParameterValue(TEXT("Roughness"), Roughness);
		Box->SetMaterial(0, Mat);
		Materials.Add(Mat);
	}
	return Box;
}

int32 ASprawlLakeBasin::HideCityGroundPlane()
{
	int32 Hidden = 0;
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		UStaticMeshComponent* Mesh = It->GetStaticMeshComponent();
		if (!Mesh)
		{
			continue;
		}
		const FBoxSphereBounds B = Mesh->Bounds;
		// City_Ground is a wide, thin, near-origin plate. Match it by shape so
		// this works without editor-only labels; the perimeter edge planes sit
		// far from origin and are left alone.
		const bool bWideFlatCentral =
			B.BoxExtent.X > 8000.f && B.BoxExtent.Y > 8000.f
			&& B.BoxExtent.Z < 400.f
			&& FMath::Abs(B.Origin.X) < 3000.f
			&& FMath::Abs(B.Origin.Y) < 3000.f;
		if (bWideFlatCentral)
		{
			// Hide the visual only; collision stays so nothing falls through.
			Mesh->SetVisibility(false, true);
			++Hidden;
		}
	}
	return Hidden;
}

void ASprawlLakeBasin::BuildBasin()
{
	const FSprawlWaterfrontSceneryLayout Layout =
		FSprawlWaterfrontSceneryLayout::Build();
	if (!Layout.IsValid())
	{
		return;
	}
	const float Cx = Layout.WaterCenter.X;
	const float Cy = Layout.WaterCenter.Y;
	const float Hx = Layout.WaterSize.X * 0.5f;
	const float Hy = Layout.WaterSize.Y * 0.5f;
	const float X0 = Cx - Hx, X1 = Cx + Hx;
	const float Y0 = Cy - Hy, Y1 = Cy + Hy;
	const float R = RingExtent;

	const float WaterZ = Layout.WaterCenter.Z;
	const float BedTopZ = WaterZ - BedDropBelowWater;

	// --- Ground ring: the visible city floor everywhere except the lake hole,
	//     replacing the hidden City_Ground. Four rectangles tile map-minus-lake.
	auto RingRect = [&](const TCHAR* Name, float Rx0, float Rx1,
		float Ry0, float Ry1)
	{
		AddBox(Name,
			FVector((Rx0 + Rx1) * 0.5f, (Ry0 + Ry1) * 0.5f,
				-RingThickness * 0.5f),
			FVector(Rx1 - Rx0, Ry1 - Ry0, RingThickness),
			AsphaltColor, 0.9f, /*bCollide*/ false, AsphaltMaterial);
	};
	RingRect(TEXT("Ring_N"), -R, R, Y1, R);
	RingRect(TEXT("Ring_S"), -R, R, -R, Y0);
	RingRect(TEXT("Ring_W"), -R, X0, Y0, Y1);
	RingRect(TEXT("Ring_E"), X1, R, Y0, Y1);

	// --- Basin bed under the translucent water.
	const float BedThickness = 40.f;
	AddBox(TEXT("Bed"),
		FVector(Cx, Cy, BedTopZ - BedThickness * 0.5f),
		FVector(X1 - X0, Y1 - Y0, BedThickness), BedColor, 0.85f, false);

	// --- Quay walls: the vertical basin sides, doubling as a low curb barrier
	//     (collision) so the player stops at the water's edge instead of walking
	//     onto the invisible ground-collision floor over the pit.
	const float WallHeight = QuayTopZ - BedTopZ;
	const float WallCz = (QuayTopZ + BedTopZ) * 0.5f;
	AddBox(TEXT("Quay_N"), FVector(Cx, Y1, WallCz),
		FVector(X1 - X0 + QuayThickness, QuayThickness, WallHeight),
		QuayColor, 0.8f, /*bCollide*/ true, ConcreteMaterial);
	AddBox(TEXT("Quay_S"), FVector(Cx, Y0, WallCz),
		FVector(X1 - X0 + QuayThickness, QuayThickness, WallHeight),
		QuayColor, 0.8f, true, ConcreteMaterial);
	AddBox(TEXT("Quay_E"), FVector(X1, Cy, WallCz),
		FVector(QuayThickness, Y1 - Y0, WallHeight), QuayColor, 0.8f, true,
		ConcreteMaterial);
	AddBox(TEXT("Quay_W"), FVector(X0, Cy, WallCz),
		FVector(QuayThickness, Y1 - Y0, WallHeight), QuayColor, 0.8f, true,
		ConcreteMaterial);
}

int32 ASprawlLakeBasin::HideFloatingLakeProps()
{
	const FSprawlWaterfrontSceneryLayout Layout =
		FSprawlWaterfrontSceneryLayout::Build();
	if (!Layout.IsValid())
	{
		return 0;
	}
	const float Cx = Layout.WaterCenter.X;
	const float Cy = Layout.WaterCenter.Y;
	// Shrink the test area so props sitting right on the shore rim are kept;
	// only clearly-inside-the-lake dressing is hidden.
	const float Hx = Layout.WaterSize.X * 0.5f - 500.f;
	const float Hy = Layout.WaterSize.Y * 0.5f - 500.f;
	const float WaterZ = Layout.WaterCenter.Z;

	int32 Hidden = 0;
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		UStaticMeshComponent* Mesh = It->GetStaticMeshComponent();
		if (!Mesh)
		{
			continue;
		}
		const FBoxSphereBounds B = Mesh->Bounds;
		// Never touch large structures (buildings, the ground plane).
		if (B.BoxExtent.X > 4000.f || B.BoxExtent.Y > 4000.f)
		{
			continue;
		}
		const bool bInsideLake =
			FMath::Abs(B.Origin.X - Cx) < Hx && FMath::Abs(B.Origin.Y - Cy) < Hy;
		const float BottomZ = B.Origin.Z - B.BoxExtent.Z;
		if (bInsideLake && BottomZ > WaterZ + 40.f)
		{
			Mesh->SetVisibility(false, true);
			++Hidden;
		}
	}
	return Hidden;
}

void ASprawlLakeBasin::BeginPlay()
{
	Super::BeginPlay();
	const int32 Hidden = HideCityGroundPlane();
	BuildBasin();
	const int32 Floated = HideFloatingLakeProps();
	UE_LOG(LogTemp, Display,
		TEXT("[LakeBasin] hid %d city-ground plane(s), %d floating prop(s); basin waterZ=%.0f"),
		Hidden, Floated, FSprawlWaterfrontSceneryLayout::Build().WaterCenter.Z);
}

ASprawlLakeBasin* ASprawlLakeBasin::EnsureForWorld(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	TActorIterator<ASprawlLakeBasin> Existing(World);
	if (Existing)
	{
		return *Existing;
	}
	FActorSpawnParameters Params;
	Params.Name = TEXT("SprawlLakeBasin");
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<ASprawlLakeBasin>(
		FVector::ZeroVector, FRotator::ZeroRotator, Params);
}
