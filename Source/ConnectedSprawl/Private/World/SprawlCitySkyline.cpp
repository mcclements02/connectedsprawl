// The Connected Sprawl - Low-poly skyline ring beyond the playable city.

#include "World/SprawlCitySkyline.h"

#include "World/SprawlCityGridSubsystem.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

using Grid = USprawlCityGridSubsystem;

namespace
{
// The ring's two rows start beyond the boundary walls and fence — far enough
// out that from the perimeter sidewalk they read as a skyline across open
// ground rather than a wall at the fence line.
constexpr float InnerOffset = 3000.f;
constexpr float RowSpacing = 3200.f;
constexpr float RoofThickness = 60.f;

// The lake meets the south boundary on this X range; the skyline leaves it
// open so the bay still reads as water running to the mountain horizon.
constexpr float BayGapMinX = 600.f;
constexpr float BayGapMaxX = 11800.f;

const FLinearColor RoofColor(0.028f, 0.028f, 0.031f, 1.f);

// The same authored facade instances the real city buildings wear, so the
// proxies match the streetscape's palette exactly.
const TCHAR* FacadePaths[FSprawlSkylineLayout::NumFacadeStyles] = {
	TEXT("/Game/CityArt/MI_Facade_Slate.MI_Facade_Slate"),
	TEXT("/Game/CityArt/MI_Facade_Tan.MI_Facade_Tan"),
	TEXT("/Game/CityArt/MI_Facade_Warm.MI_Facade_Warm"),
	TEXT("/Game/CityArt/MI_Facade_Pale.MI_Facade_Pale"),
};
}

FSprawlSkylineLayout FSprawlSkylineLayout::Build()
{
	FSprawlSkylineLayout Layout;
	FRandomStream Rand(20260721);
	const float Boundary = Grid::CityBoundaryHalfExtent;

	auto PlaceRun = [&](int32 Side, int32 Row)
	{
		// Sides: 0=+Y north, 1=-Y south, 2=+X east, 3=-X west. North/south
		// runs span the corners; east/west runs stop short so corners are
		// placed exactly once.
		const bool bAlongX = Side < 2;
		const float R = Boundary + InnerOffset + Row * RowSpacing
			+ Rand.FRandRange(0.f, 300.f);
		const float Extent = bAlongX ? R + 2000.f : Boundary - 200.f;
		const float Sign = (Side == 0 || Side == 2) ? 1.f : -1.f;

		float Cursor = -Extent;
		while (Cursor < Extent)
		{
			const float Width = Rand.FRandRange(900.f, 1700.f);
			const float Depth = Rand.FRandRange(900.f, 1700.f);
			const float Gap = Rand.FRandRange(250.f, 900.f);
			const float Along = Cursor + Width * 0.5f;
			Cursor += Width + Gap;
			if (Along + Width * 0.5f > Extent)
			{
				break;
			}
			// Height: near row human-scale mid-rise, far row towers, so the
			// silhouette layers instead of forming a wall.
			const float Height = (Row == 0)
				? Rand.FRandRange(2400.f, 4400.f)
				: Rand.FRandRange(4200.f, 7800.f);
			const int32 Facade = Rand.RandRange(0, NumFacadeStyles - 1);

			const float X = bAlongX ? Along : Sign * R;
			const float Y = bAlongX ? Sign * R : Along;
			// Keep the bay open: no proxies on the south run across the
			// lake's outward arc.
			if (Side == 1 && X > BayGapMinX && X < BayGapMaxX)
			{
				continue;
			}
			const FVector Size(Width, Depth, Height);
			Layout.BuildingTransforms.Emplace(
				FRotator::ZeroRotator,
				FVector(X, Y, Height * 0.5f - 80.f),
				Size / 100.f);
			Layout.FacadeIndices.Add(Facade);
			Layout.RoofTransforms.Emplace(
				FRotator::ZeroRotator,
				FVector(X, Y, Height - 80.f + RoofThickness * 0.5f),
				FVector(Width + 30.f, Depth + 30.f, RoofThickness) / 100.f);
			// Tall towers carry a dark rooftop-mechanical crown (appended
			// after the parallel building/roof pairs), breaking the flat-top
			// silhouette the way real high-rises do.
			if (Height > 6000.f)
			{
				const float CrownH = Rand.FRandRange(300.f, 520.f);
				Layout.CrownTransforms.Emplace(
					FRotator::ZeroRotator,
					FVector(X + Rand.FRandRange(-Width * 0.12f, Width * 0.12f),
						Y + Rand.FRandRange(-Depth * 0.12f, Depth * 0.12f),
						Height - 80.f + RoofThickness + CrownH * 0.5f),
					FVector(Width * 0.5f, Depth * 0.5f, CrownH) / 100.f);
			}
		}
	};

	for (int32 Side = 0; Side < 4; ++Side)
	{
		for (int32 Row = 0; Row < 2; ++Row)
		{
			PlaceRun(Side, Row);
		}
	}
	return Layout;
}

bool FSprawlSkylineLayout::IsValid() const
{
	if (BuildingTransforms.Num() < 40
		|| BuildingTransforms.Num() != FacadeIndices.Num()
		|| BuildingTransforms.Num() != RoofTransforms.Num())
	{
		return false;
	}
	for (int32 i = 0; i < BuildingTransforms.Num(); ++i)
	{
		const FVector Loc = BuildingTransforms[i].GetLocation();
		const FVector Scale = BuildingTransforms[i].GetScale3D();
		if (Grid::IsInsideCityBounds(Loc.X, Loc.Y)
			|| Scale.GetMin() <= 0.f
			|| FacadeIndices[i] < 0 || FacadeIndices[i] >= NumFacadeStyles)
		{
			return false;
		}
	}
	return true;
}

ASprawlCitySkyline::ASprawlCitySkyline()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorEnableCollision(false);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Cube(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> Opaque(
		TEXT("/Game/Materials/Master/M_Simple_Opaque.M_Simple_Opaque"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> Asphalt(
		TEXT("/Game/Import/CityKit/MI_AsphaltWorn.MI_AsphaltWorn"));
	CubeMesh = Cube.Get();
	OpaqueBase = Opaque.Get();
	ApronAsphalt = Asphalt.Get();

	auto ConfigureBackground = [](UHierarchicalInstancedStaticMeshComponent* Comp)
	{
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetGenerateOverlapEvents(false);
		Comp->SetCanEverAffectNavigation(false);
		Comp->SetCastShadow(false);
		Comp->SetReceivesDecals(false);
		Comp->SetRenderCustomDepth(false);
	};

	for (int32 i = 0; i < FSprawlSkylineLayout::NumFacadeStyles; ++i)
	{
		const FString Name = FString::Printf(TEXT("SkylineFacade%d"), i);
		UHierarchicalInstancedStaticMeshComponent* Comp =
			CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(*Name);
		Comp->SetupAttachment(RootComponent);
		ConfigureBackground(Comp);
		if (CubeMesh)
		{
			Comp->SetStaticMesh(CubeMesh);
		}
		ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> Facade(
			FacadePaths[i]);
		if (Facade.Get())
		{
			Comp->SetMaterial(0, Facade.Get());
		}
		FacadeMeshes.Add(Comp);
	}

	Roofs = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("SkylineRoofs"));
	Roofs->SetupAttachment(RootComponent);
	ConfigureBackground(Roofs);
	if (CubeMesh)
	{
		Roofs->SetStaticMesh(CubeMesh);
	}
}

void ASprawlCitySkyline::BuildRing()
{
	const FSprawlSkylineLayout Layout = FSprawlSkylineLayout::Build();
	if (!Layout.IsValid())
	{
		return;
	}
	for (UHierarchicalInstancedStaticMeshComponent* Comp : FacadeMeshes)
	{
		Comp->ClearInstances();
	}
	Roofs->ClearInstances();

	if (OpaqueBase && !RoofMaterial)
	{
		RoofMaterial = UMaterialInstanceDynamic::Create(OpaqueBase, this);
		RoofMaterial->SetVectorParameterValue(TEXT("BaseColor"), RoofColor);
		RoofMaterial->SetVectorParameterValue(TEXT("Color"), RoofColor);
		RoofMaterial->SetScalarParameterValue(TEXT("Roughness"), 0.9f);
		Roofs->SetMaterial(0, RoofMaterial);
	}

	for (int32 i = 0; i < Layout.BuildingTransforms.Num(); ++i)
	{
		FacadeMeshes[Layout.FacadeIndices[i]]->AddInstance(
			Layout.BuildingTransforms[i]);
		Roofs->AddInstance(Layout.RoofTransforms[i]);
	}
	for (const FTransform& Crown : Layout.CrownTransforms)
	{
		Roofs->AddInstance(Crown);
	}

	// Belt and braces: if any facade slot lost its authored material (the
	// proxies then render flat default grey), rebind it here and say so.
	for (int32 i = 0; i < FacadeMeshes.Num(); ++i)
	{
		UMaterialInterface* Bound = FacadeMeshes[i]->GetMaterial(0);
		if (!Bound || Bound->GetMaterial() == UMaterial::GetDefaultMaterial(MD_Surface))
		{
			if (UMaterialInterface* Loaded = LoadObject<UMaterialInterface>(
				nullptr, FacadePaths[i]))
			{
				FacadeMeshes[i]->SetMaterial(0, Loaded);
				Bound = Loaded;
			}
		}
		UE_LOG(LogTemp, Display, TEXT("[Skyline] facade[%d] material=%s"),
			i, Bound ? *Bound->GetName() : TEXT("<null>"));
	}
}

void ASprawlCitySkyline::BuildApron()
{
	if (!CubeMesh)
	{
		return;
	}
	// From just inside the city floor's outer edge (the lake-basin ring ends
	// at 11446) out past the far proxy row, flush with the ring's top so the
	// ground runs continuously to the skyline's feet.
	const float Inner = 11400.f;
	const float Outer = Grid::CityBoundaryHalfExtent + 9200.f;
	constexpr float Thickness = 40.f;

	UMaterialInterface* Surface = ApronAsphalt;
	if (!Surface && OpaqueBase)
	{
		if (!ApronMaterial)
		{
			ApronMaterial = UMaterialInstanceDynamic::Create(OpaqueBase, this);
			const FLinearColor Ground(0.048f, 0.048f, 0.050f, 1.f);
			ApronMaterial->SetVectorParameterValue(TEXT("BaseColor"), Ground);
			ApronMaterial->SetVectorParameterValue(TEXT("Color"), Ground);
			ApronMaterial->SetScalarParameterValue(TEXT("Roughness"), 0.95f);
		}
		Surface = ApronMaterial;
	}

	auto Slab = [&](const TCHAR* Name, float X0, float X1, float Y0, float Y1)
	{
		UStaticMeshComponent* Box = NewObject<UStaticMeshComponent>(this, Name);
		Box->SetupAttachment(RootComponent);
		Box->RegisterComponent();
		Box->SetStaticMesh(CubeMesh);
		Box->SetWorldScale3D(
			FVector(X1 - X0, Y1 - Y0, Thickness) / 100.f);
		Box->SetWorldLocation(FVector(
			(X0 + X1) * 0.5f, (Y0 + Y1) * 0.5f, -Thickness * 0.5f));
		Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Box->SetGenerateOverlapEvents(false);
		Box->SetCanEverAffectNavigation(false);
		Box->SetCastShadow(false);
		Box->SetReceivesDecals(false);
		if (Surface)
		{
			Box->SetMaterial(0, Surface);
		}
		ApronSlabs.Add(Box);
	};
	Slab(TEXT("Apron_N"), -Outer, Outer, Inner, Outer);
	Slab(TEXT("Apron_S"), -Outer, Outer, -Outer, -Inner);
	Slab(TEXT("Apron_W"), -Outer, -Inner, -Inner, Inner);
	Slab(TEXT("Apron_E"), Inner, Outer, -Inner, Inner);
}

void ASprawlCitySkyline::BeginPlay()
{
	Super::BeginPlay();
	BuildApron();
	BuildRing();
	const FSprawlSkylineLayout Layout = FSprawlSkylineLayout::Build();
	UE_LOG(LogTemp, Display,
		TEXT("[Skyline] ring of %d proxy buildings (+%d roofs) beyond the boundary"),
		Layout.BuildingTransforms.Num(), Layout.RoofTransforms.Num());
}

ASprawlCitySkyline* ASprawlCitySkyline::EnsureForWorld(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	TActorIterator<ASprawlCitySkyline> Existing(World);
	if (Existing)
	{
		return *Existing;
	}
	FActorSpawnParameters Params;
	Params.Name = TEXT("SprawlCitySkyline");
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<ASprawlCitySkyline>(
		FVector::ZeroVector, FRotator::ZeroRotator, Params);
}
