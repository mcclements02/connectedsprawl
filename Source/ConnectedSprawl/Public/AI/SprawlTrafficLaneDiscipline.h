// The Connected Sprawl - Lane-following guidance and diagnostics.

#pragma once

#include "CoreMinimal.h"
#include "World/SprawlCityGridSubsystem.h"

struct CONNECTEDSPRAWL_API FSprawlTrafficGuidance
{
	FVector LookAheadTarget = FVector::ZeroVector;
	float LaneCenter = 0.f;
	float SignedCrossTrackError = 0.f;
	float LaneError = 0.f;
	float DesiredYaw = 0.f;
	float HeadingErrorDegrees = 0.f;
	float SteeringInput = 0.f;
	float SpeedScale = 1.f;
};

struct CONNECTEDSPRAWL_API FSprawlTrafficLaneSample
{
	float LaneError = 0.f;
	float ForwardAlignment = 1.f;
	float VelocityAlignment = 1.f;
	bool bWrongWay = false;
};

/** Pure, allocation-free lane controller shared by driving and its audit. */
class CONNECTEDSPRAWL_API FSprawlTrafficLaneDiscipline
{
public:
	static FSprawlTrafficGuidance ComputeGuidance(const FVector& Location,
		float ActorYawDegrees, float Speed, ESprawlHeading Heading,
		int32 RoadIndex);

	/**
	 * Return the exact lane-center pose for a small, stationary authored offset.
	 * Large errors are deliberately rejected so this never crosses a centreline.
	 */
	static bool BuildCanonicalLocation(const FVector& Location,
		ESprawlHeading Heading, int32 RoadIndex, FVector& OutLocation);

	static FSprawlTrafficLaneSample Measure(const FVector& Location,
		const FVector& ActorForward, const FVector& Velocity,
		ESprawlHeading PlannedHeading, int32 PlannedRoadIndex,
		float MinimumWrongWaySpeed = 100.f);

	static float DistanceToNearestIntersection(const FVector& Location,
		ESprawlHeading Heading);
};
