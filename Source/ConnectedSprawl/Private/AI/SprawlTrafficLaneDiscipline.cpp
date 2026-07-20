// The Connected Sprawl - Lane-following guidance and diagnostics.

#include "AI/SprawlTrafficLaneDiscipline.h"

using Grid = USprawlCityGridSubsystem;

FSprawlTrafficGuidance FSprawlTrafficLaneDiscipline::ComputeGuidance(
	const FVector& Location, float ActorYawDegrees, float Speed,
	ESprawlHeading Heading, int32 RoadIndex)
{
	FSprawlTrafficGuidance Guidance;
	Guidance.LaneCenter = Grid::LaneCenter(RoadIndex, Heading);
	const bool bNorthSouth = Grid::IsNorthSouth(Heading);
	const float LaneCoordinate = bNorthSouth ? Location.X : Location.Y;
	Guidance.SignedCrossTrackError = Guidance.LaneCenter - LaneCoordinate;
	Guidance.LaneError = FMath::Abs(Guidance.SignedCrossTrackError);

	// Scale pursuit distances with the lane rather than a historical road size.
	const float LookAheadDistance = FMath::Clamp(FMath::Abs(Speed) * 0.36f,
		Grid::LaneWidth * 0.69f, Grid::LaneWidth * 1.32f);
	Guidance.LookAheadTarget =
		Location + Grid::HeadingVector(Heading) * LookAheadDistance;
	if (bNorthSouth)
	{
		Guidance.LookAheadTarget.X = Guidance.LaneCenter;
	}
	else
	{
		Guidance.LookAheadTarget.Y = Guidance.LaneCenter;
	}

	Guidance.DesiredYaw = FMath::RadiansToDegrees(FMath::Atan2(
		Guidance.LookAheadTarget.Y - Location.Y,
		Guidance.LookAheadTarget.X - Location.X));
	Guidance.HeadingErrorDegrees = FMath::FindDeltaAngleDegrees(
		ActorYawDegrees, Guidance.DesiredYaw);
	const float FullSteerError = FMath::Max(15.f, Grid::LaneWidth * (24.f / 350.f));
	Guidance.SteeringInput = FMath::Clamp(
		Guidance.HeadingErrorDegrees / FullSteerError, -1.f, 1.f);

	// A migrated/off-line car first converges gently instead of crossing a
	// narrow lane at full cruise speed. Exact-lane cars remain at 1.0.
	const float ErrorAlpha = FMath::Clamp(
		(Guidance.LaneError - Grid::LaneWidth * 0.10f)
		/ (Grid::LaneWidth * 0.35f), 0.f, 1.f);
	Guidance.SpeedScale = FMath::Lerp(1.f, 0.55f, ErrorAlpha);
	return Guidance;
}

bool FSprawlTrafficLaneDiscipline::BuildCanonicalLocation(
	const FVector& Location, ESprawlHeading Heading, int32 RoadIndex,
	FVector& OutLocation)
{
	OutLocation = Location;
	const float LaneCenter = Grid::LaneCenter(RoadIndex, Heading);
	const float LaneCoordinate = Grid::IsNorthSouth(Heading)
		? Location.X : Location.Y;
	const float Error = FMath::Abs(LaneCoordinate - LaneCenter);
	if (Error < 1.f || Error > Grid::LaneWidth * 0.49f
		|| DistanceToNearestIntersection(Location, Heading)
			<= Grid::RoadWidth * 0.5f + 450.f)
	{
		return false;
	}

	if (Grid::IsNorthSouth(Heading))
	{
		OutLocation.X = LaneCenter;
	}
	else
	{
		OutLocation.Y = LaneCenter;
	}
	return true;
}

FSprawlTrafficLaneSample FSprawlTrafficLaneDiscipline::Measure(
	const FVector& Location, const FVector& ActorForward,
	const FVector& Velocity, ESprawlHeading PlannedHeading,
	int32 PlannedRoadIndex, float MinimumWrongWaySpeed)
{
	FSprawlTrafficLaneSample Sample;
	const bool bNorthSouth = Grid::IsNorthSouth(PlannedHeading);
	const float LaneCoordinate = bNorthSouth ? Location.X : Location.Y;
	Sample.LaneError = FMath::Abs(LaneCoordinate
		- Grid::LaneCenter(PlannedRoadIndex, PlannedHeading));

	const FVector PlannedDirection = Grid::HeadingVector(PlannedHeading);
	FVector FlatForward(ActorForward.X, ActorForward.Y, 0.f);
	if (FlatForward.Normalize())
	{
		Sample.ForwardAlignment = FVector::DotProduct(
			FlatForward, PlannedDirection);
	}

	FVector FlatVelocity(Velocity.X, Velocity.Y, 0.f);
	const float Speed = FlatVelocity.Size();
	if (FlatVelocity.Normalize())
	{
		Sample.VelocityAlignment = FVector::DotProduct(
			FlatVelocity, PlannedDirection);
	}
	Sample.bWrongWay = Speed >= FMath::Max(0.f, MinimumWrongWaySpeed)
		&& Sample.VelocityAlignment < -0.20f;
	return Sample;
}

float FSprawlTrafficLaneDiscipline::DistanceToNearestIntersection(
	const FVector& Location, ESprawlHeading Heading)
{
	const float AlongCoordinate = Grid::IsNorthSouth(Heading)
		? Location.Y : Location.X;
	float Distance = TNumericLimits<float>::Max();
	for (int32 RoadIndex = 0; RoadIndex < Grid::NumRoads; ++RoadIndex)
	{
		Distance = FMath::Min(Distance,
			FMath::Abs(AlongCoordinate - Grid::RoadCenter(RoadIndex)));
	}
	return Distance;
}
