// The Connected Sprawl - Road-marking fit and approach-clearance layout.

#include "World/SprawlRoadMarkingLayout.h"
#include "World/SprawlCityGridSubsystem.h"

using Grid = USprawlCityGridSubsystem;

FSprawlRoadMarkingFit FSprawlRoadMarkingLayout::GetFit()
{
	return FSprawlRoadMarkingFit();
}

void FSprawlRoadMarkingLayout::BuildRoadIntervals(
	TArray<FSprawlRoadMarkingSegment>& OutIntervals)
{
	OutIntervals.Reset();
	const float GridLo = Grid::RoadCenter(0) - Grid::Step * 0.5f;
	const float GridHi = Grid::RoadCenter(Grid::NumRoads - 1) + Grid::Step * 0.5f;
	// The stop line is outside the intersection box. Leave one short visual
	// buffer beyond it, so moving traffic never appears to drive through paint.
	const float ApproachClearance = Grid::StopLineDistance + 80.f;
	float SegmentStart = GridLo;
	for (int32 RoadIndex = 0; RoadIndex < Grid::NumRoads; ++RoadIndex)
	{
		const float Intersection = Grid::RoadCenter(RoadIndex);
		const float SegmentEnd = Intersection - ApproachClearance;
		if (SegmentEnd > SegmentStart)
		{
			OutIntervals.Add({ SegmentStart, SegmentEnd });
		}
		SegmentStart = Intersection + ApproachClearance;
	}
	if (GridHi > SegmentStart)
	{
		OutIntervals.Add({ SegmentStart, GridHi });
	}
}

void FSprawlRoadMarkingLayout::AppendCenteredDashCenters(
	const TArray<FSprawlRoadMarkingSegment>& Intervals, float DashLength,
	float DashSpacing, TArray<float>& OutCenters)
{
	OutCenters.Reset();
	const float SafeLength = FMath::Max(1.f, DashLength);
	const float SafeSpacing = FMath::Max(SafeLength, DashSpacing);
	for (const FSprawlRoadMarkingSegment& Interval : Intervals)
	{
		const float Available = Interval.EndAlong - Interval.StartAlong;
		if (Available < SafeLength)
		{
			continue;
		}
		const int32 Count = FMath::FloorToInt((Available - SafeLength) / SafeSpacing) + 1;
		const float Coverage = SafeLength + (Count - 1) * SafeSpacing;
		float Center = (Interval.StartAlong + Interval.EndAlong - Coverage) * 0.5f
			+ SafeLength * 0.5f;
		for (int32 DashIndex = 0; DashIndex < Count; ++DashIndex)
		{
			OutCenters.Add(Center);
			Center += SafeSpacing;
		}
	}
}
