// The Connected Sprawl - shared city-map data and coordinate projection.

#include "World/SprawlCityMapSubsystem.h"

#include "World/SprawlCityGridSubsystem.h"
#include "World/SprawlEnterableInteriors.h"
#include "World/SprawlParkingGarage.h"

using Grid = USprawlCityGridSubsystem;

namespace
{
ESprawlMapLandmarkType MapTypeForBuilding(ESprawlBuildingKind Kind)
{
	switch (Kind)
	{
	case ESprawlBuildingKind::Store: return ESprawlMapLandmarkType::Store;
	case ESprawlBuildingKind::Office: return ESprawlMapLandmarkType::Office;
	default: return ESprawlMapLandmarkType::Condo;
	}
}
}

void USprawlCityMapSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Landmarks = BuildDefaultLandmarks();
}

TArray<FSprawlMapLandmark> USprawlCityMapSubsystem::BuildDefaultLandmarks()
{
	TArray<FSprawlMapLandmark> Result;
	const FSprawlEnterableInteriorsLayout Interiors =
		FSprawlEnterableInteriorsLayout::Build();
	for (const FSprawlEnterableBuildingSpec& Building : Interiors.Buildings)
	{
		FSprawlMapLandmark Marker;
		Marker.Id = Building.Id;
		Marker.Label = Building.DisplayName;
		Marker.Type = MapTypeForBuilding(Building.Kind);
		Marker.WorldLocation = Building.ExteriorReturnLocation;
		Marker.Color = Building.AccentColor;
		Marker.bEnterable = true;
		Result.Add(MoveTemp(Marker));
	}

	FSprawlMapLandmark Garage;
	Garage.Id = TEXT("ParkingGarage");
	Garage.Label = NSLOCTEXT("Sprawl", "ParkingGarageMap", "Central Parking Deck");
	Garage.Type = ESprawlMapLandmarkType::Parking;
	const FVector2D GarageCenter = FSprawlParkingGarageLayout::GarageCenter();
	Garage.WorldLocation = FVector(GarageCenter.X, GarageCenter.Y, 0.f);
	Garage.Color = FLinearColor(0.22f, 0.50f, 0.96f, 1.f);
	Result.Add(MoveTemp(Garage));

	FSprawlMapLandmark Waterfront;
	Waterfront.Id = TEXT("SouthWaterfront");
	Waterfront.Label = NSLOCTEXT("Sprawl", "WaterfrontMap", "South Waterfront");
	Waterfront.Type = ESprawlMapLandmarkType::Waterfront;
	Waterfront.WorldLocation = FVector(
		Grid::BlockCenter(5),
		(Grid::BlockCenter(0) + Grid::BlockCenter(1)) * 0.5f, 0.f);
	Waterfront.Color = FLinearColor(0.20f, 0.72f, 0.92f, 1.f);
	Result.Add(MoveTemp(Waterfront));

	return Result;
}

FVector USprawlCityMapSubsystem::GetDisplayLocationForActor(const AActor* Actor) const
{
	if (!Actor)
	{
		return FVector::ZeroVector;
	}
	const FSprawlEnterableInteriorsLayout Interiors =
		FSprawlEnterableInteriorsLayout::Build();
	return Interiors.ResolveMapLocation(Actor->GetActorLocation());
}

FVector2D USprawlCityMapSubsystem::WorldToMapNormalized(
	const FVector& WorldLocation)
{
	const float Diameter = Grid::CityBoundaryHalfExtent * 2.f;
	return FVector2D(
		FMath::Clamp(
			(WorldLocation.X + Grid::CityBoundaryHalfExtent) / Diameter, 0.f, 1.f),
		FMath::Clamp(
			(WorldLocation.Y + Grid::CityBoundaryHalfExtent) / Diameter, 0.f, 1.f));
}
