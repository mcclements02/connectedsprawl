// The Connected Sprawl - Procedural parking deck and traffic egress source.

#include "World/SprawlParkingGarage.h"

#include "AI/SprawlTrafficRoute.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/SprawlCar.h"

using Grid = USprawlCityGridSubsystem;

namespace
{
constexpr float GarageFloorZ = 26.f;
constexpr float LevelHeight = 360.f;
constexpr float SlabThickness = 24.f;
constexpr float GarageHeight = 1260.f;
constexpr float PortalWallHeight = 260.f;
constexpr float DrivewayClearance = 380.f;

const FLinearColor ConcreteTint(0.31f, 0.30f, 0.28f, 1.f);
const FLinearColor AsphaltTint(0.055f, 0.058f, 0.062f, 1.f);
const FLinearColor MarkingTint(0.82f, 0.82f, 0.76f, 1.f);
const FLinearColor VehicleTint(0.12f, 0.19f, 0.27f, 1.f);
const FLinearColor ParkingBlue(0.025f, 0.19f, 0.48f, 1.f);
const FLinearColor LampTint(1.f, 0.68f, 0.24f, 1.f);

void AddBox(TArray<FTransform>& Target, const FVector& Center,
	const FVector& Size, const FRotator& Rotation = FRotator::ZeroRotator)
{
	Target.Emplace(Rotation, Center, Size / 100.f);
}

bool HasPositiveTransforms(const TArray<FTransform>& Transforms)
{
	for (const FTransform& Transform : Transforms)
	{
		if (Transform.GetScale3D().GetMin() <= 0.f)
		{
			return false;
		}
	}
	return true;
}

bool FootprintIntersectsDriveway(const FBox2D& Footprint,
	const FSprawlParkingGarageLayout& Layout)
{
	const FVector2D Expansion(DrivewayClearance, DrivewayClearance);
	for (const FSprawlParkingGarageExit& Exit : Layout.Exits)
	{
		FVector Previous = Exit.SpawnTransform.GetLocation();
		for (const FVector& Point : Exit.DeparturePath)
		{
			const FVector2D A(Previous.X, Previous.Y);
			const FVector2D B(Point.X, Point.Y);
			const FBox2D Corridor(
				FVector2D(FMath::Min(A.X, B.X), FMath::Min(A.Y, B.Y)) - Expansion,
				FVector2D(FMath::Max(A.X, B.X), FMath::Max(A.Y, B.Y)) + Expansion);
			if (Footprint.Intersect(Corridor))
			{
				return true;
			}
			Previous = Point;
		}
	}
	return false;
}

bool PointTouchesDriveway(const FVector& Location,
	const FSprawlParkingGarageLayout& Layout)
{
	FVector FlatLocation = Location;
	FlatLocation.Z = 0.f;
	for (const FSprawlParkingGarageExit& Exit : Layout.Exits)
	{
		FVector Previous = Exit.SpawnTransform.GetLocation();
		Previous.Z = 0.f;
		for (FVector Point : Exit.DeparturePath)
		{
			Point.Z = 0.f;
			if (FMath::PointDistToSegmentSquared(
				FlatLocation, Previous, Point) <= FMath::Square(DrivewayClearance))
			{
				return true;
			}
			Previous = Point;
		}
	}
	return false;
}
}

bool FSprawlParkingGarageExit::IsValid() const
{
	if (Id.IsNone() || DeparturePath.Num() < 4
		|| RoadIndex < 0 || RoadIndex >= Grid::NumRoads)
	{
		return false;
	}

	const FVector Spawn = SpawnTransform.GetLocation();
	const FVector Merge = MergeTransform.GetLocation();
	if (!Grid::IsInsideCityBounds(Spawn.X, Spawn.Y)
		|| Grid::IsOnRoadSurface(Spawn.X, Spawn.Y)
		|| Grid::IsOverLake(Spawn.X, Spawn.Y, 50.f)
		|| !DeparturePath.Last().Equals(Merge, 1.f))
	{
		return false;
	}

	const float LaneCenter = Grid::LaneCenter(RoadIndex, MergeHeading);
	const float LaneError = Grid::IsNorthSouth(MergeHeading)
		? FMath::Abs(Merge.X - LaneCenter)
		: FMath::Abs(Merge.Y - LaneCenter);
	if (LaneError > 1.f
		|| !FSprawlTrafficRoute::IsSpawnRouteViable(
			Merge, MergeHeading, RoadIndex))
	{
		return false;
	}

	for (const FVector& Point : DeparturePath)
	{
		if (!Grid::IsInsideCityBounds(Point.X, Point.Y)
			|| Grid::IsOverLake(Point.X, Point.Y, 50.f))
		{
			return false;
		}
	}
	return true;
}

FVector2D FSprawlParkingGarageLayout::GarageCenter()
{
	return FVector2D(
		Grid::BlockCenter(GarageBlockX), Grid::BlockCenter(GarageBlockY));
}

FSprawlParkingGarageLayout FSprawlParkingGarageLayout::Build()
{
	FSprawlParkingGarageLayout Layout;
	const FVector2D C = GarageCenter();
	const FVector Base(C.X, C.Y, 0.f);
	Layout.SiteBounds = FBox2D(
		C - FVector2D(SiteHalfExtent, SiteHalfExtent),
		C + FVector2D(SiteHalfExtent, SiteHalfExtent));
	Layout.DeckCount = ExpectedDeckCount;

	// Ground pad plus three upper parking slabs. The existing block remains the
	// foundation, so this thin asphalt layer begins exactly at sidewalk top.
	for (int32 Level = 0; Level < ExpectedDeckCount; ++Level)
	{
		const float Z = GarageFloorZ + Level * LevelHeight;
		AddBox(Layout.Decks, Base + FVector(0.f, 0.f, Z),
			FVector(1800.f, 1800.f, Level == 0 ? 12.f : SlabThickness));

		// Continuous upper-deck parapets. Ground-level walls are split around
		// the two visible vehicle portals below.
		if (Level > 0)
		{
			const float RailZ = Z + 65.f;
			AddBox(Layout.Rails, Base + FVector(0.f, 870.f, RailZ),
				FVector(1720.f, 45.f, 110.f));
			AddBox(Layout.Rails, Base + FVector(0.f, -870.f, RailZ),
				FVector(1720.f, 45.f, 110.f));
			AddBox(Layout.Rails, Base + FVector(870.f, 0.f, RailZ),
				FVector(45.f, 1720.f, 110.f));
			AddBox(Layout.Rails, Base + FVector(-870.f, 0.f, RailZ),
				FVector(45.f, 1720.f, 110.f));
		}

		// Parking bay separators and compact parked-car silhouettes make each
		// storey read as occupied without adding ticking vehicle actors.
		for (const float X : { -520.f, -120.f, 280.f })
		{
			AddBox(Layout.Markings, Base + FVector(X, 610.f, Z + 15.f),
				FVector(9.f, 420.f, 3.f));
			AddBox(Layout.Markings, Base + FVector(-X, -610.f, Z + 15.f),
				FVector(9.f, 420.f, 3.f));
		}
		if (Level > 0)
		{
			for (int32 Bay = 0; Bay < 3; ++Bay)
			{
				const float X = -500.f + Bay * 400.f;
				const float Side = ((Level + Bay) & 1) ? 1.f : -1.f;
				AddBox(Layout.ParkedVehicles,
					Base + FVector(X, Side * 610.f, Z + 58.f),
					FVector(330.f, 150.f, 70.f));
				AddBox(Layout.ParkedVehicles,
					Base + FVector(X - 25.f, Side * 610.f, Z + 112.f),
					FVector(160.f, 132.f, 45.f));
			}
		}
	}

	// Eight slim perimeter columns keep the ground floor open and readable.
	for (const FVector2D P : {
		FVector2D(-820.f, -820.f), FVector2D(820.f, -820.f),
		FVector2D(-820.f, 820.f), FVector2D(820.f, 820.f),
		FVector2D(-820.f, 0.f), FVector2D(820.f, 0.f),
		FVector2D(0.f, -820.f), FVector2D(0.f, 820.f) })
	{
		AddBox(Layout.Structure, Base + FVector(P.X, P.Y, GarageHeight * 0.5f),
			FVector(75.f, 75.f, GarageHeight));
	}

	// North portal is offset east; south portal is offset west. Low walls and
	// overhead beams hide the birth points while leaving six-metre openings.
	AddBox(Layout.Structure, Base + FVector(-450.f, 870.f, PortalWallHeight * 0.5f),
		FVector(700.f, 65.f, PortalWallHeight));
	AddBox(Layout.Structure, Base + FVector(650.f, 870.f, PortalWallHeight * 0.5f),
		FVector(300.f, 65.f, PortalWallHeight));
	AddBox(Layout.Structure, Base + FVector(-650.f, -870.f, PortalWallHeight * 0.5f),
		FVector(300.f, 65.f, PortalWallHeight));
	AddBox(Layout.Structure, Base + FVector(450.f, -870.f, PortalWallHeight * 0.5f),
		FVector(700.f, 65.f, PortalWallHeight));
	AddBox(Layout.Structure, Base + FVector(0.f, 870.f, 315.f),
		FVector(1720.f, 75.f, 110.f));
	AddBox(Layout.Structure, Base + FVector(0.f, -870.f, 315.f),
		FVector(1720.f, 75.f, 110.f));
	AddBox(Layout.Structure, Base + FVector(870.f, 0.f, 130.f),
		FVector(65.f, 1720.f, 260.f));
	AddBox(Layout.Structure, Base + FVector(-870.f, 0.f, 130.f),
		FVector(65.f, 1720.f, 260.f));

	// Alternating internal ramps visually connect all four decks. They stay on
	// the east edge, clear of both north/south departure corridors.
	const float RampRun = 1000.f;
	const float RampLength = FMath::Sqrt(RampRun * RampRun + LevelHeight * LevelHeight);
	const float RampRoll = FMath::RadiansToDegrees(FMath::Atan2(LevelHeight, RampRun));
	for (int32 Level = 0; Level < ExpectedDeckCount - 1; ++Level)
	{
		const float MidZ = GarageFloorZ + Level * LevelHeight + LevelHeight * 0.5f;
		const float Roll = (Level & 1) ? RampRoll : -RampRoll;
		AddBox(Layout.Ramps, Base + FVector(650.f, 0.f, MidZ),
			FVector(270.f, RampLength, 20.f), FRotator(0.f, 0.f, Roll));
	}

	// Ventilation slats, warm ceiling lights, and a large blue parking sign.
	for (int32 Level = 1; Level < ExpectedDeckCount; ++Level)
	{
		const float BaseZ = GarageFloorZ + (Level - 1) * LevelHeight + 265.f;
		for (const float Y : { -620.f, -310.f, 0.f, 310.f, 620.f })
		{
			AddBox(Layout.Rails, Base + FVector(882.f, Y, BaseZ),
				FVector(30.f, 45.f, 210.f));
			AddBox(Layout.Rails, Base + FVector(-882.f, Y, BaseZ),
				FVector(30.f, 45.f, 210.f));
		}
	}
	for (int32 Level = 0; Level < ExpectedDeckCount - 1; ++Level)
	{
		const float Z = GarageFloorZ + Level * LevelHeight + 315.f;
		for (const float Y : { -420.f, 0.f, 420.f })
		{
			AddBox(Layout.Lights, Base + FVector(0.f, Y, Z),
				FVector(260.f, 22.f, 12.f));
		}
	}
	AddBox(Layout.Signs, Base + FVector(-560.f, 906.f, 900.f),
		FVector(300.f, 24.f, 360.f));
	// Block-built white P on the sign face (marking material).
	AddBox(Layout.Markings, Base + FVector(-625.f, 921.f, 900.f),
		FVector(38.f, 18.f, 220.f));
	AddBox(Layout.Markings, Base + FVector(-555.f, 921.f, 995.f),
		FVector(140.f, 18.f, 34.f));
	AddBox(Layout.Markings, Base + FVector(-500.f, 921.f, 945.f),
		FVector(34.f, 18.f, 110.f));
	AddBox(Layout.Markings, Base + FVector(-555.f, 921.f, 895.f),
		FVector(140.f, 18.f, 34.f));

	const int32 HorizontalRoad = Grid::NearestRoadIndex(C.Y + Grid::Step * 0.5f);
	const float NorthLane = Grid::LaneCenter(HorizontalRoad, ESprawlHeading::West);
	FSprawlParkingGarageExit North;
	North.Id = TEXT("NorthWestbound");
	North.SpawnTransform = FTransform(FRotator(0.f, 90.f, 0.f),
		FVector(C.X + 350.f, C.Y + 80.f, 180.f));
	North.DeparturePath = {
		FVector(C.X + 350.f, C.Y + 560.f, 180.f),
		FVector(C.X + 350.f, C.Y + 1010.f, 180.f),
		FVector(C.X + 300.f, C.Y + 1220.f, 180.f),
		FVector(C.X + 170.f, NorthLane - 35.f, 180.f),
		FVector(C.X - 40.f, NorthLane, 180.f),
		FVector(C.X - 450.f, NorthLane, 180.f),
	};
	North.MergeTransform = FTransform(
		FRotator(0.f, Grid::HeadingYaw(ESprawlHeading::West), 0.f),
		North.DeparturePath.Last());
	North.MergeHeading = ESprawlHeading::West;
	North.RoadIndex = HorizontalRoad;
	Layout.Exits.Add(MoveTemp(North));

	const int32 SouthRoad = Grid::NearestRoadIndex(C.Y - Grid::Step * 0.5f);
	const float SouthLane = Grid::LaneCenter(SouthRoad, ESprawlHeading::East);
	FSprawlParkingGarageExit South;
	South.Id = TEXT("SouthEastbound");
	South.SpawnTransform = FTransform(FRotator(0.f, -90.f, 0.f),
		FVector(C.X - 350.f, C.Y - 80.f, 180.f));
	South.DeparturePath = {
		FVector(C.X - 350.f, C.Y - 560.f, 180.f),
		FVector(C.X - 350.f, C.Y - 1010.f, 180.f),
		FVector(C.X - 300.f, C.Y - 1220.f, 180.f),
		FVector(C.X - 170.f, SouthLane + 35.f, 180.f),
		FVector(C.X + 40.f, SouthLane, 180.f),
		FVector(C.X + 450.f, SouthLane, 180.f),
	};
	South.MergeTransform = FTransform(
		FRotator(0.f, Grid::HeadingYaw(ESprawlHeading::East), 0.f),
		South.DeparturePath.Last());
	South.MergeHeading = ESprawlHeading::East;
	South.RoadIndex = SouthRoad;
	Layout.Exits.Add(MoveTemp(South));

	return Layout;
}

bool FSprawlParkingGarageLayout::IsValid() const
{
	if (DeckCount != ExpectedDeckCount || Decks.Num() != ExpectedDeckCount
		|| Ramps.Num() != ExpectedDeckCount - 1 || Structure.Num() < 14
		|| Rails.Num() < 20 || Markings.Num() < 20
		|| ParkedVehicles.Num() < 12 || Signs.IsEmpty() || Lights.IsEmpty()
		|| Exits.Num() != 2
		|| !SiteBounds.GetSize().Equals(
			FVector2D(SiteHalfExtent * 2.f), 1.f))
	{
		return false;
	}

	if (!HasPositiveTransforms(Structure) || !HasPositiveTransforms(Decks)
		|| !HasPositiveTransforms(Ramps) || !HasPositiveTransforms(Rails)
		|| !HasPositiveTransforms(Markings)
		|| !HasPositiveTransforms(ParkedVehicles)
		|| !HasPositiveTransforms(Signs) || !HasPositiveTransforms(Lights))
	{
		return false;
	}

	TSet<FName> ExitIds;
	for (const FSprawlParkingGarageExit& Exit : Exits)
	{
		if (!Exit.IsValid() || ExitIds.Contains(Exit.Id))
		{
			return false;
		}
		ExitIds.Add(Exit.Id);
	}
	return true;
}

ASprawlParkingGarage::ASprawlParkingGarage()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Cube(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> Concrete(
		TEXT("/Game/Import/CityKit/MI_ConcreteWorn.MI_ConcreteWorn"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> Asphalt(
		TEXT("/Game/Import/CityKit/MI_AsphaltWorn.MI_AsphaltWorn"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> Opaque(
		TEXT("/Game/Materials/Master/M_Simple_Opaque.M_Simple_Opaque"));
	CubeMesh = Cube.Get();
	ConcreteMaterial = Concrete.Get();
	AsphaltMaterial = Asphalt.Get();
	OpaqueBase = Opaque.Get();

	auto MakeInstances = [this](const TCHAR* Name, bool bCollision)
	{
		UHierarchicalInstancedStaticMeshComponent* Component =
			CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(Name);
		Component->SetupAttachment(RootComponent);
		Component->SetCollisionEnabled(bCollision
			? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		if (bCollision)
		{
			Component->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		}
		Component->SetGenerateOverlapEvents(false);
		Component->SetCanEverAffectNavigation(false);
		Component->SetCastShadow(false);
		Component->SetReceivesDecals(false);
		if (CubeMesh)
		{
			Component->SetStaticMesh(CubeMesh);
		}
		return Component;
	};

	Structure = MakeInstances(TEXT("GarageStructure"), true);
	Decks = MakeInstances(TEXT("GarageDecks"), true);
	Ramps = MakeInstances(TEXT("GarageRamps"), true);
	Rails = MakeInstances(TEXT("GarageRails"), true);
	Markings = MakeInstances(TEXT("GarageMarkings"), false);
	ParkedVehicles = MakeInstances(TEXT("GarageParkedVehicles"), false);
	Signs = MakeInstances(TEXT("GarageSigns"), false);
	Lights = MakeInstances(TEXT("GarageLights"), false);
}

int32 ASprawlParkingGarage::ClearAuthoredSite(
	const FSprawlParkingGarageLayout& Layout)
{
	int32 Cleared = 0;
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		FVector Origin;
		FVector Extent;
		It->GetActorBounds(false, Origin, Extent, false);
		// Preserve only ground-level city, sidewalk, and service surfaces. Thin
		// elevated awnings/signs must leave with the lot they were attached to.
		if (Origin.Z + Extent.Z < 60.f)
		{
			continue;
		}
		const FBox2D ActorFootprint(
			FVector2D(Origin.X - Extent.X, Origin.Y - Extent.Y),
			FVector2D(Origin.X + Extent.X, Origin.Y + Extent.Y));
		if (!Layout.SiteBounds.Intersect(ActorFootprint)
			&& !FootprintIntersectsDriveway(ActorFootprint, Layout))
		{
			continue;
		}
		It->SetActorHiddenInGame(true);
		It->SetActorEnableCollision(false);
		++Cleared;
	}
	return Cleared;
}

int32 ASprawlParkingGarage::RelocateDrivewayCars(
	const FSprawlParkingGarageLayout& Layout)
{
	const FVector2D C = FSprawlParkingGarageLayout::GarageCenter();
	TArray<FTransform> UpperDeckBays;
	for (int32 Level = 1; Level <= 2; ++Level)
	{
		const float FloorTop = GarageFloorZ + Level * LevelHeight
			+ SlabThickness * 0.5f;
		for (const float Side : { -1.f, 1.f })
		{
			for (const float X : { -650.f, -250.f, 150.f })
			{
				const float Yaw = Side < 0.f ? 0.f : 180.f;
				UpperDeckBays.Emplace(FRotator(0.f, Yaw, 0.f),
					FVector(C.X + X, C.Y + Side * 380.f, FloorTop + 125.f));
			}
		}
	}

	int32 Relocated = 0;
	for (TActorIterator<ASprawlCar> It(GetWorld()); It; ++It)
	{
		ASprawlCar* Car = *It;
		if (!Car || Car->bAutoDrive
			|| !PointTouchesDriveway(Car->GetActorLocation(), Layout))
		{
			continue;
		}
		if (!UpperDeckBays.IsValidIndex(Relocated))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[ParkingGarage] no upper-deck bay available for %s"),
				*Car->GetName());
			break;
		}

		Car->SetActorLocationAndRotation(
			UpperDeckBays[Relocated].GetLocation(),
			UpperDeckBays[Relocated].Rotator(), false, nullptr,
			ETeleportType::TeleportPhysics);
		if (UPrimitiveComponent* RootPrimitive =
			Cast<UPrimitiveComponent>(Car->GetRootComponent()))
		{
			RootPrimitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
			RootPrimitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}
		UE_LOG(LogTemp, Display,
			TEXT("[ParkingGarage] moved parked car %s into upper-deck bay=%d"),
			*Car->GetName(), Relocated);
		++Relocated;
	}
	return Relocated;
}

void ASprawlParkingGarage::BuildGarage(
	const FSprawlParkingGarageLayout& Layout)
{
	if (!Layout.IsValid())
	{
		return;
	}

	auto MakeTint = [this](const FLinearColor& Color, float Roughness)
		-> UMaterialInterface*
	{
		if (!OpaqueBase)
		{
			return nullptr;
		}
		UMaterialInstanceDynamic* Material =
			UMaterialInstanceDynamic::Create(OpaqueBase, this);
		Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
		Material->SetVectorParameterValue(TEXT("Color"), Color);
		Material->SetScalarParameterValue(TEXT("Roughness"), Roughness);
		RuntimeMaterials.Add(Material);
		return Material;
	};

	UMaterialInterface* Concrete = ConcreteMaterial
		? ConcreteMaterial.Get() : MakeTint(ConcreteTint, 0.9f);
	UMaterialInterface* Asphalt = AsphaltMaterial
		? AsphaltMaterial.Get() : MakeTint(AsphaltTint, 0.96f);
	UMaterialInterface* Paint = MakeTint(MarkingTint, 0.75f);
	UMaterialInterface* Vehicles = MakeTint(VehicleTint, 0.62f);
	UMaterialInterface* Blue = MakeTint(ParkingBlue, 0.56f);
	UMaterialInterface* Lamps = MakeTint(LampTint, 0.32f);

	struct FInstanceSet
	{
		UHierarchicalInstancedStaticMeshComponent* Component;
		const TArray<FTransform>* Transforms;
		UMaterialInterface* Material;
	};
	const FInstanceSet Sets[] = {
		{ Structure, &Layout.Structure, Concrete },
		{ Decks, &Layout.Decks, Asphalt },
		{ Ramps, &Layout.Ramps, Asphalt },
		{ Rails, &Layout.Rails, Concrete },
		{ Markings, &Layout.Markings, Paint },
		{ ParkedVehicles, &Layout.ParkedVehicles, Vehicles },
		{ Signs, &Layout.Signs, Blue },
		{ Lights, &Layout.Lights, Lamps },
	};
	for (const FInstanceSet& Set : Sets)
	{
		if (!Set.Component)
		{
			continue;
		}
		Set.Component->ClearInstances();
		if (Set.Material)
		{
			Set.Component->SetMaterial(0, Set.Material);
		}
		for (const FTransform& Transform : *Set.Transforms)
		{
			Set.Component->AddInstance(Transform);
		}
	}
	VehicleExits = Layout.Exits;
}

void ASprawlParkingGarage::BeginPlay()
{
	Super::BeginPlay();
	const FSprawlParkingGarageLayout Layout = FSprawlParkingGarageLayout::Build();
	const int32 ClearedActors = Layout.IsValid() ? ClearAuthoredSite(Layout) : 0;
	const int32 RelocatedCars = Layout.IsValid()
		? RelocateDrivewayCars(Layout) : 0;
	BuildGarage(Layout);
	UE_LOG(LogTemp, Display,
		TEXT("[ParkingGarage] valid=%d decks=%d exits=%d cleared_site_actors=%d relocated_driveway_cars=%d instances=%d"),
		Layout.IsValid(), Layout.DeckCount, VehicleExits.Num(), ClearedActors,
		RelocatedCars,
		Layout.Structure.Num() + Layout.Decks.Num() + Layout.Ramps.Num()
			+ Layout.Rails.Num() + Layout.Markings.Num()
			+ Layout.ParkedVehicles.Num() + Layout.Signs.Num() + Layout.Lights.Num());
}

ASprawlParkingGarage* ASprawlParkingGarage::EnsureForWorld(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	TActorIterator<ASprawlParkingGarage> Existing(World);
	if (Existing)
	{
		return *Existing;
	}
	FActorSpawnParameters Params;
	Params.Name = TEXT("SprawlParkingGarage");
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<ASprawlParkingGarage>(
		FVector::ZeroVector, FRotator::ZeroRotator, Params);
}
