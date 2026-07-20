// The Connected Sprawl - Stable, mobile-lightweight lake and horizon scenery.

#include "World/SprawlWaterfrontScenery.h"

#include "World/SprawlCityGridSubsystem.h"
#include "World/SprawlOceanSurface.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

using Grid = USprawlCityGridSubsystem;

namespace
{
constexpr float WaterSurfaceZ = 18.f;
constexpr float ShapeMeshSize = 100.f;

FTransform NormalizeToMeshBounds(const FTransform& DesiredTransform,
	const UStaticMesh* Mesh)
{
	if (!Mesh)
	{
		return DesiredTransform;
	}
	const FBoxSphereBounds Bounds = Mesh->GetBounds();
	const FVector MeshSize = Bounds.BoxExtent * 2.f;
	if (MeshSize.GetMin() <= UE_SMALL_NUMBER)
	{
		return DesiredTransform;
	}
	// Layout scales express the desired silhouette dimensions in metres. Asset
	// normalization lets authored rocks replace primitive cones without baking
	// knowledge of their import scale into the city layout.
	const FVector Scale = DesiredTransform.GetScale3D() * ShapeMeshSize / MeshSize;
	FVector Location = DesiredTransform.GetLocation();
	const float ScaledMeshBottom =
		(Bounds.Origin.Z - Bounds.BoxExtent.Z) * Scale.Z;
	Location.Z -= ScaledMeshBottom;
	return FTransform(DesiredTransform.GetRotation(), Location, Scale);
}

void ConfigureSceneryMesh(UPrimitiveComponent* Component)
{
	if (!Component)
	{
		return;
	}
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetGenerateOverlapEvents(false);
	Component->SetCanEverAffectNavigation(false);
	Component->SetCastShadow(false);
	Component->SetReceivesDecals(false);
	Component->SetRenderCustomDepth(false);
}

void TintMaterial(UMaterialInstanceDynamic* Material, const FLinearColor& Color,
	float Roughness)
{
	if (!Material)
	{
		return;
	}
	// The authored city material exposes BaseColor; the engine shape material
	// exposes Color. Setting both keeps the lightweight fallback deterministic.
	Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
	Material->SetVectorParameterValue(TEXT("Color"), Color);
	Material->SetScalarParameterValue(TEXT("Metallic"), 0.f);
	Material->SetScalarParameterValue(TEXT("Roughness"), Roughness);
}
}

FSprawlWaterfrontSceneryLayout FSprawlWaterfrontSceneryLayout::Build()
{
	FSprawlWaterfrontSceneryLayout Layout;
	const float LakeMinX = Grid::BlockCenter(4) - Grid::Step * 0.5f;
	const float LakeMaxX = Grid::BlockCenter(6) + Grid::Step * 0.5f;
	const float LakeMinY = Grid::BlockCenter(0) - Grid::Step * 0.5f;
	const float LakeMaxY = Grid::BlockCenter(1) + Grid::Step * 0.5f;
	Layout.WaterCenter = FVector(
		(LakeMinX + LakeMaxX) * 0.5f,
		(LakeMinY + LakeMaxY) * 0.5f,
		WaterSurfaceZ);
	Layout.WaterSize = FVector2D(LakeMaxX - LakeMinX, LakeMaxY - LakeMinY);

	// Low, non-colliding far banks prevent the water from meeting the sky as a
	// white void. Both sit beyond the playable perimeter.
	Layout.ShoreTransforms.Emplace(FRotator::ZeroRotator,
		FVector(Layout.WaterCenter.X, LakeMinY - 260.f, 35.f),
		FVector(Layout.WaterSize.X / ShapeMeshSize, 5.2f, 1.4f));
	Layout.ShoreTransforms.Emplace(FRotator::ZeroRotator,
		FVector(LakeMaxX + 260.f, Layout.WaterCenter.Y, 35.f),
		FVector(5.2f, Layout.WaterSize.Y / ShapeMeshSize, 1.4f));

	auto AddPeak = [&](float X, float Y, float Yaw, float Width,
		float Depth, float Height)
	{
		Layout.MountainTransforms.Emplace(FRotator(0.f, Yaw, 0.f),
			FVector(X, Y, -180.f),
			FVector(Width, Depth, Height));
	};
	const float FarHorizon = Grid::CityBoundaryHalfExtent + 9000.f;
	const float FoothillHorizon = Grid::CityBoundaryHalfExtent + 7000.f;
	// The authored asset is a complete ridge, so four broad instances create a
	// continuous south/east horizon with much less repetition than loose peaks.
	AddPeak(-5200.f, -FarHorizon, 4.f, 190.f, 48.f, 35.f);
	AddPeak(11200.f, -FarHorizon - 500.f, -7.f, 170.f, 44.f, 31.f);
	AddPeak(FarHorizon, -7600.f, 94.f, 175.f, 45.f, 33.f);
	AddPeak(FarHorizon + 300.f, 7600.f, 86.f, 165.f, 43.f, 29.f);

	auto AddFoothill = [&](float X, float Y, float Yaw, float Width, float Depth)
	{
		Layout.FoothillTransforms.Emplace(FRotator(0.f, Yaw, 0.f),
			FVector(X, Y, -140.f), FVector(Width, Depth, 5.2f));
	};
	AddFoothill(-7500.f, -FoothillHorizon, 8.f, 48.f, 18.f);
	AddFoothill(-1500.f, -FoothillHorizon - 300.f, -6.f, 56.f, 20.f);
	AddFoothill(5000.f, -FoothillHorizon + 200.f, 7.f, 52.f, 18.f);
	AddFoothill(11200.f, -FoothillHorizon - 200.f, -5.f, 50.f, 18.f);
	AddFoothill(FoothillHorizon, -9500.f, 88.f, 48.f, 17.f);
	AddFoothill(FoothillHorizon + 200.f, -3500.f, 94.f, 54.f, 19.f);
	AddFoothill(FoothillHorizon, 2500.f, 86.f, 46.f, 17.f);
	return Layout;
}

bool FSprawlWaterfrontSceneryLayout::IsValid() const
{
	if (WaterSize.X <= 0.f || WaterSize.Y <= 0.f
		|| ShoreTransforms.Num() != 2
		|| MountainTransforms.IsEmpty() || FoothillTransforms.IsEmpty())
	{
		return false;
	}
	for (const FTransform& Transform : MountainTransforms)
	{
		const FVector Scale = Transform.GetScale3D();
		const FVector Location = Transform.GetLocation();
		if (Scale.GetMin() <= 0.f
			|| Grid::IsInsideCityBounds(Location.X, Location.Y))
		{
			return false;
		}
	}
	return true;
}

ASprawlWaterfrontScenery::ASprawlWaterfrontScenery()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorEnableCollision(false);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	WaterSurface = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LakeSurface"));
	WaterSurface->SetupAttachment(RootComponent);
	ConfigureSceneryMesh(WaterSurface);

	Shoreline = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("FarShore"));
	Shoreline->SetupAttachment(RootComponent);
	ConfigureSceneryMesh(Shoreline);

	Mountains = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("Mountains"));
	Mountains->SetupAttachment(RootComponent);
	ConfigureSceneryMesh(Mountains);
	Mountains->SetCullDistances(45000, 70000);

	Foothills = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("Foothills"));
	Foothills->SetupAttachment(RootComponent);
	ConfigureSceneryMesh(Foothills);
	Foothills->SetCullDistances(45000, 70000);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> OceanMesh(
		TEXT("/Game/Geometry/SM_Water.SM_Water"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(
		TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MountainRock(
		TEXT("/Game/Downtown_West/Assets/props/prop_rocks/SM_rock_large_a_low.SM_rock_large_a_low"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> FoothillRock(
		TEXT("/Game/Downtown_West/Assets/props/prop_rocks/SM_rock_medium_b_low.SM_rock_medium_b_low"));
	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> LegacyMountains(
		TEXT("/Game/Downtown_West/Assets/background_mountain/SM_background_mountains.SM_background_mountains"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> OceanWater(
		TEXT("/Game/Materials/MI_WaterPond.MI_WaterPond"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> WaterFallback(
		TEXT("/Game/CityArt/MI_Water.MI_Water"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	if (CubeMesh.Succeeded())
	{
		Shoreline->SetStaticMesh(CubeMesh.Object);
	}
	if (OceanMesh.Get())
	{
		WaterSurface->SetStaticMesh(OceanMesh.Get());
	}
	else if (PlaneMesh.Succeeded())
	{
		WaterSurface->SetStaticMesh(PlaneMesh.Object);
	}
	LegacyMountainMesh = LegacyMountains.Get();
	if (LegacyMountainMesh)
	{
		Mountains->SetStaticMesh(LegacyMountainMesh);
	}
	else if (MountainRock.Succeeded())
	{
		Mountains->SetStaticMesh(MountainRock.Object);
	}
	if (FoothillRock.Succeeded())
	{
		Foothills->SetStaticMesh(FoothillRock.Object);
	}
	if (OceanWater.Get())
	{
		WaterSurface->SetMaterial(0, OceanWater.Get());
	}
	else if (WaterFallback.Get())
	{
		WaterSurface->SetMaterial(0, WaterFallback.Get());
	}
	if (ShapeMaterial.Succeeded())
	{
		Shoreline->SetMaterial(0, ShapeMaterial.Object);
	}
}

void ASprawlWaterfrontScenery::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildGeometry();
}

void ASprawlWaterfrontScenery::BeginPlay()
{
	Super::BeginPlay();
	RebuildGeometry();
	BuildRuntimeMaterials();
	const int32 HiddenVistas = HideLegacyMountainVistas();
	const FSprawlWaterfrontSceneryLayout Layout =
		FSprawlWaterfrontSceneryLayout::Build();
	UE_LOG(LogTemp, Display,
		TEXT("[Waterfront] ocean=%.0fx%.0f flow=%.2f wave=%.0f strength=%.1f ranges=%d foothills=%d legacy_hidden=%d"),
		Layout.WaterSize.X, Layout.WaterSize.Y,
		FSprawlOceanWaveProfile::CoastalOcean().FlowSpeed,
		FSprawlOceanWaveProfile::CoastalOcean().WaveSizeCm,
		FSprawlOceanWaveProfile::CoastalOcean().WaveStrength,
		Layout.MountainTransforms.Num(), Layout.FoothillTransforms.Num(),
		HiddenVistas);
}

ASprawlWaterfrontScenery* ASprawlWaterfrontScenery::EnsureForWorld(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	TActorIterator<ASprawlWaterfrontScenery> Existing(World);
	if (Existing)
	{
		return *Existing;
	}
	FActorSpawnParameters Params;
	Params.Name = TEXT("SprawlWaterfrontScenery");
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<ASprawlWaterfrontScenery>(
		FVector::ZeroVector, FRotator::ZeroRotator, Params);
}

void ASprawlWaterfrontScenery::RebuildGeometry()
{
	const FSprawlWaterfrontSceneryLayout Layout =
		FSprawlWaterfrontSceneryLayout::Build();
	if (!Layout.IsValid())
	{
		return;
	}
	FSprawlOceanSurface::FitMesh(*WaterSurface,
		Layout.WaterSize, Layout.WaterCenter);

	Shoreline->ClearInstances();
	Mountains->ClearInstances();
	Foothills->ClearInstances();
	for (const FTransform& Transform : Layout.ShoreTransforms)
	{
		Shoreline->AddInstance(Transform);
	}
	for (const FTransform& Transform : Layout.MountainTransforms)
	{
		Mountains->AddInstance(NormalizeToMeshBounds(
			Transform, Mountains->GetStaticMesh()));
	}
	for (const FTransform& Transform : Layout.FoothillTransforms)
	{
		Foothills->AddInstance(NormalizeToMeshBounds(
			Transform, Foothills->GetStaticMesh()));
	}
}

int32 ASprawlWaterfrontScenery::HideLegacyMountainVistas()
{
	if (!LegacyMountainMesh)
	{
		return 0;
	}
	int32 HiddenCount = 0;
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		UStaticMeshComponent* Mesh = It->GetStaticMeshComponent();
		if (Mesh && Mesh->GetStaticMesh() == LegacyMountainMesh)
		{
			It->SetActorHiddenInGame(true);
			It->SetActorEnableCollision(false);
			Mesh->SetVisibility(false, true);
			++HiddenCount;
		}
	}
	return HiddenCount;
}

void ASprawlWaterfrontScenery::BuildRuntimeMaterials()
{
	auto MakeMaterial = [&](UPrimitiveComponent* Component,
		TObjectPtr<UMaterialInstanceDynamic>& Target,
		const FLinearColor& Color, float Roughness)
	{
		UMaterialInterface* Base = Component ? Component->GetMaterial(0) : nullptr;
		if (!Base)
		{
			return;
		}
		Target = UMaterialInstanceDynamic::Create(Base, this);
		TintMaterial(Target, Color, Roughness);
		Component->SetMaterial(0, Target);
	};

	if (UMaterialInterface* Base = WaterSurface->GetMaterial(0))
	{
		WaterMaterial = FSprawlOceanSurface::BuildWaveMaterial(
			*WaterSurface, *this, *Base,
			FSprawlOceanWaveProfile::CoastalOcean());
	}
	MakeMaterial(Shoreline, ShoreMaterial,
		FLinearColor(0.18f, 0.23f, 0.19f, 1.f), 0.92f);
}
