// The Connected Sprawl - Directed traffic routing rules.

#include "AI/SprawlTrafficRoute.h"

using Grid = USprawlCityGridSubsystem;

ESprawlHeading FSprawlTrafficRoute::OppositeHeading(ESprawlHeading Heading)
{
	return static_cast<ESprawlHeading>(
		(static_cast<int32>(Heading) + 2) % 4);
}

bool FSprawlTrafficRoute::TryGetNextApproach(const FVector& Location,
	ESprawlHeading Heading, int32 RoadIndex,
	FSprawlTrafficApproach& OutApproach, float MinimumDistance)
{
	OutApproach = FSprawlTrafficApproach();
	if (RoadIndex < 0 || RoadIndex >= Grid::NumRoads)
	{
		return false;
	}

	const bool bNorthSouth = Grid::IsNorthSouth(Heading);
	const float TravelCoordinate = bNorthSouth ? Location.Y : Location.X;
	const float TravelDirection =
		(Heading == ESprawlHeading::North || Heading == ESprawlHeading::East)
		? 1.f : -1.f;

	for (int32 CrossingRoad = 0; CrossingRoad < Grid::NumRoads; ++CrossingRoad)
	{
		const float Distance =
			(Grid::RoadCenter(CrossingRoad) - TravelCoordinate) * TravelDirection;
		if (Distance >= MinimumDistance && Distance < OutApproach.DistanceAhead)
		{
			OutApproach.CrossingRoadIndex = CrossingRoad;
			OutApproach.DistanceAhead = Distance;
		}
	}

	if (OutApproach.CrossingRoadIndex == INDEX_NONE)
	{
		return false;
	}

	if (bNorthSouth)
	{
		OutApproach.IntersectionX = RoadIndex;
		OutApproach.IntersectionY = OutApproach.CrossingRoadIndex;
	}
	else
	{
		OutApproach.IntersectionX = OutApproach.CrossingRoadIndex;
		OutApproach.IntersectionY = RoadIndex;
	}
	OutApproach.Center = Grid::IntersectionCenter(
		OutApproach.IntersectionX, OutApproach.IntersectionY);
	return true;
}

void FSprawlTrafficRoute::ResolveManeuver(ESprawlHeading InHeading,
	int32 InRoadIndex, int32 CrossingRoadIndex,
	ESprawlTrafficManeuver Maneuver, ESprawlHeading& OutHeading,
	int32& OutRoadIndex)
{
	OutHeading = InHeading;
	OutRoadIndex = InRoadIndex;

	switch (Maneuver)
	{
	case ESprawlTrafficManeuver::Straight:
	case ESprawlTrafficManeuver::None:
		return;
	case ESprawlTrafficManeuver::UTurn:
		OutHeading = OppositeHeading(InHeading);
		return;
	case ESprawlTrafficManeuver::Right:
		OutHeading = static_cast<ESprawlHeading>(
			(static_cast<int32>(InHeading) + 1) % 4);
		OutRoadIndex = CrossingRoadIndex;
		return;
	case ESprawlTrafficManeuver::Left:
		OutHeading = static_cast<ESprawlHeading>(
			(static_cast<int32>(InHeading) + 3) % 4);
		OutRoadIndex = CrossingRoadIndex;
		return;
	}
}

bool FSprawlTrafficRoute::IsManeuverLegal(ESprawlHeading Heading,
	int32 RoadIndex, int32 CrossingRoadIndex,
	ESprawlTrafficManeuver Maneuver)
{
	if (RoadIndex < 0 || RoadIndex >= Grid::NumRoads
		|| CrossingRoadIndex < 0 || CrossingRoadIndex >= Grid::NumRoads
		|| Maneuver == ESprawlTrafficManeuver::None)
	{
		return false;
	}

	ESprawlHeading ExitHeading;
	int32 ExitRoadIndex = INDEX_NONE;
	ResolveManeuver(Heading, RoadIndex, CrossingRoadIndex, Maneuver,
		ExitHeading, ExitRoadIndex);
	const FVector2D Center = Grid::IsNorthSouth(Heading)
		? FVector2D(Grid::RoadCenter(RoadIndex), Grid::RoadCenter(CrossingRoadIndex))
		: FVector2D(Grid::RoadCenter(CrossingRoadIndex), Grid::RoadCenter(RoadIndex));
	const FVector ExitDirection = Grid::HeadingVector(ExitHeading);
	const FVector2D Probe = Center
		+ FVector2D(ExitDirection.X, ExitDirection.Y) * Grid::Step;
	return Grid::IsInsideRoadGrid(Probe.X, Probe.Y)
		&& !Grid::IsOverLake(Probe.X, Probe.Y);
}

ESprawlTrafficManeuver FSprawlTrafficRoute::ChooseManeuver(
	ESprawlHeading Heading, int32 RoadIndex, int32 CrossingRoadIndex,
	float RandomUnit)
{
	const float Roll = FMath::Clamp(RandomUnit, 0.f, 1.f);
	const ESprawlTrafficManeuver Preferred = Roll < 0.55f
		? ESprawlTrafficManeuver::Straight
		: Roll < 0.80f
			? ESprawlTrafficManeuver::Right
			: ESprawlTrafficManeuver::Left;
	if (IsManeuverLegal(Heading, RoadIndex, CrossingRoadIndex, Preferred))
	{
		return Preferred;
	}

	for (const ESprawlTrafficManeuver Candidate : {
		ESprawlTrafficManeuver::Straight,
		ESprawlTrafficManeuver::Right,
		ESprawlTrafficManeuver::Left })
	{
		if (IsManeuverLegal(Heading, RoadIndex, CrossingRoadIndex, Candidate))
		{
			return Candidate;
		}
	}

	return IsManeuverLegal(Heading, RoadIndex, CrossingRoadIndex,
		ESprawlTrafficManeuver::UTurn)
		? ESprawlTrafficManeuver::UTurn
		: ESprawlTrafficManeuver::None;
}

bool FSprawlTrafficRoute::HasForwardExit(ESprawlHeading Heading,
	int32 RoadIndex, int32 CrossingRoadIndex)
{
	for (const ESprawlTrafficManeuver Candidate : {
		ESprawlTrafficManeuver::Straight,
		ESprawlTrafficManeuver::Right,
		ESprawlTrafficManeuver::Left })
	{
		if (IsManeuverLegal(Heading, RoadIndex, CrossingRoadIndex, Candidate))
		{
			return true;
		}
	}
	return false;
}

bool FSprawlTrafficRoute::IsSpawnRouteViable(const FVector& Location,
	ESprawlHeading Heading, int32 RoadIndex)
{
	if (!Grid::IsInsideRoadGrid(Location.X, Location.Y)
		|| Grid::IsOverLake(Location.X, Location.Y)
		|| !IsClearOfIntersection(Location, Heading))
	{
		return false;
	}

	FSprawlTrafficApproach Approach;
	return TryGetNextApproach(Location, Heading, RoadIndex, Approach, 0.f)
		&& HasForwardExit(Heading, RoadIndex, Approach.CrossingRoadIndex);
}

bool FSprawlTrafficRoute::IsClearOfIntersection(const FVector& Location,
	ESprawlHeading Heading, float ExtraClearance)
{
	const float AlongCoordinate = Grid::IsNorthSouth(Heading)
		? Location.Y : Location.X;
	const float Clearance = Grid::RoadWidth * 0.5f
		+ FMath::Max(0.f, ExtraClearance);
	for (int32 RoadIndex = 0; RoadIndex < Grid::NumRoads; ++RoadIndex)
	{
		if (FMath::Abs(AlongCoordinate - Grid::RoadCenter(RoadIndex)) < Clearance)
		{
			return false;
		}
	}
	return true;
}
