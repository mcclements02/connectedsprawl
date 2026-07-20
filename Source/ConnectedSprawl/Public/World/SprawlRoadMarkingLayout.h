// The Connected Sprawl - Road-marking fit and approach-clearance layout.

#pragma once

#include "CoreMinimal.h"

struct CONNECTEDSPRAWL_API FSprawlRoadMarkingFit
{
	/** Centre height and cube scale that read as paint, not raised blocks. */
	float SurfaceZ = 0.8f;
	float MeshThickness = 0.012f;
};

struct CONNECTEDSPRAWL_API FSprawlRoadMarkingSegment
{
	float StartAlong = 0.f;
	float EndAlong = 0.f;
};

/** Pure grid-aware geometry used by the runtime road-paint actor. */
class CONNECTEDSPRAWL_API FSprawlRoadMarkingLayout
{
public:
	static FSprawlRoadMarkingFit GetFit();

	/** Build dry-road intervals whose ends sit behind the stop-line approaches. */
	static void BuildRoadIntervals(TArray<FSprawlRoadMarkingSegment>& OutIntervals);

	/** Centre dashes in each interval; no dash can overlap an intersection. */
	static void AppendCenteredDashCenters(
		const TArray<FSprawlRoadMarkingSegment>& Intervals,
		float DashLength, float DashSpacing, TArray<float>& OutCenters);
};
