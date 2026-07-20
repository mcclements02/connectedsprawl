// The Connected Sprawl - Directed traffic routing rules.

#pragma once

#include "CoreMinimal.h"
#include "World/SprawlCityGridSubsystem.h"

/** A legal move through one grid intersection. */
enum class ESprawlTrafficManeuver : uint8
{
	Straight,
	Right,
	Left,
	UTurn,
	None
};

/** The next intersection reached by a directed lane. */
struct CONNECTEDSPRAWL_API FSprawlTrafficApproach
{
	int32 CrossingRoadIndex = INDEX_NONE;
	int32 IntersectionX = INDEX_NONE;
	int32 IntersectionY = INDEX_NONE;
	float DistanceAhead = TNumericLimits<float>::Max();
	FVector2D Center = FVector2D::ZeroVector;
};

/**
 * Allocation-free topology for the actor traffic system.
 *
 * Geometry remains owned by USprawlCityGridSubsystem. This helper adds the
 * directed-lane contract: find the next approach, reject lake/map exits, and
 * keep a U-turn as a real reserved intersection move instead of reversing a
 * car in the middle of an approach lane.
 */
class CONNECTEDSPRAWL_API FSprawlTrafficRoute
{
public:
	static ESprawlHeading OppositeHeading(ESprawlHeading Heading);

	static bool TryGetNextApproach(const FVector& Location,
		ESprawlHeading Heading, int32 RoadIndex,
		FSprawlTrafficApproach& OutApproach, float MinimumDistance = -80.f);

	static void ResolveManeuver(ESprawlHeading InHeading, int32 InRoadIndex,
		int32 CrossingRoadIndex, ESprawlTrafficManeuver Maneuver,
		ESprawlHeading& OutHeading, int32& OutRoadIndex);

	static bool IsManeuverLegal(ESprawlHeading Heading, int32 RoadIndex,
		int32 CrossingRoadIndex, ESprawlTrafficManeuver Maneuver);

	/** Weighted straight/right/left choice, with a legal U-turn only as fallback. */
	static ESprawlTrafficManeuver ChooseManeuver(ESprawlHeading Heading,
		int32 RoadIndex, int32 CrossingRoadIndex, float RandomUnit);

	/** True when this approach has a legal exit that is not an immediate U-turn. */
	static bool HasForwardExit(ESprawlHeading Heading, int32 RoadIndex,
		int32 CrossingRoadIndex);

	/** Runtime spawn contract: outside a junction with a reachable forward exit. */
	static bool IsSpawnRouteViable(const FVector& Location,
		ESprawlHeading Heading, int32 RoadIndex);

	static bool IsClearOfIntersection(const FVector& Location,
		ESprawlHeading Heading, float ExtraClearance = 400.f);
};
