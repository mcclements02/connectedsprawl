// The Connected Sprawl - City grid road network.
// Single source of truth for the procedural city's street layout: lane
// centerlines, intersections, the lake footprint, and traffic-signal phases.
// Mirrors the layout constants in Content/Python/build_city.py.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SprawlCityGridSubsystem.generated.h"

class AActor;

/** Cardinal travel direction on the road grid. */
UENUM(BlueprintType)
enum class ESprawlHeading : uint8
{
	East,   // +X
	North,  // +Y
	West,   // -X
	South   // -Y
};

/** Traffic-signal state for one approach axis of an intersection. */
UENUM(BlueprintType)
enum class ESprawlSignal : uint8
{
	Green,
	Amber,
	Red
};

/**
 * USprawlCityGridSubsystem
 * ------------------------
 * The city is an N x N grid of square blocks separated by roads. Roads run
 * the full span of the map on both axes; their crossings form intersections.
 * Cars and pedestrians query this subsystem instead of hard-coding layout
 * numbers, so a single edit here (or a bigger build script) rescales the
 * whole simulation.
 *
 * Traffic signals are purely functional: phase is derived from world time
 * and the intersection's grid coordinates, so every car and every signal
 * head computes the identical answer with no registration or replication.
 */
UCLASS()
class CONNECTEDSPRAWL_API USprawlCityGridSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// --- Layout constants (must match Content/Python/build_city.py) ---
	static constexpr int32 NumBlocks   = 7;       // blocks per axis
	static constexpr float BlockSize   = 2000.f;  // cm
	static constexpr float RoadWidth   = 600.f;   // cm
	static constexpr float Step        = BlockSize + RoadWidth;       // 2600
	static constexpr float Span        = NumBlocks * Step;            // 18200
	/** Inner face of the invisible perimeter walls around the authored city. */
	static constexpr float CityBoundaryHalfExtent = Span * 0.5f;      // 9100
	static constexpr float LaneOffset  = 150.f;   // right-hand lane center from road centerline
	/** Curbside parking center, recessed enough that a 210cm-wide car clears the live lane. */
	static constexpr float ParkingOffset = 410.f;
	/** Painted stop-line center and the matching safe vehicle-center hold distance. */
	static constexpr float StopLineDistance = 540.f;
	static constexpr float VehicleStopDistance = 790.f;
	static constexpr int32 NumRoads    = NumBlocks - 1;               // roads per axis

	/** Center of block index i (0..NumBlocks-1) on either axis. */
	static float BlockCenter(int32 Index)
	{
		return -Span * 0.5f + Step * 0.5f + Index * Step;
	}

	/** Centerline of road index r (0..NumRoads-1) on either axis. */
	static float RoadCenter(int32 Index)
	{
		return (BlockCenter(Index) + BlockCenter(Index + 1)) * 0.5f;
	}

	/** Road index whose centerline is nearest to Coord; clamped to valid range. */
	static int32 NearestRoadIndex(float Coord)
	{
		const float First = RoadCenter(0);
		const int32 Index = FMath::RoundToInt((Coord - First) / Step);
		return FMath::Clamp(Index, 0, NumRoads - 1);
	}

	/** Block index containing Coord; clamped to valid range. */
	static int32 NearestBlockIndex(float Coord)
	{
		const float First = BlockCenter(0);
		const int32 Index = FMath::RoundToInt((Coord - First) / Step);
		return FMath::Clamp(Index, 0, NumBlocks - 1);
	}

	/** World XY of intersection (Ix, Iy): vertical road Ix crossing horizontal road Iy. */
	static FVector2D IntersectionCenter(int32 Ix, int32 Iy)
	{
		return FVector2D(RoadCenter(Ix), RoadCenter(Iy));
	}

	/** True if the point sits over the lake (blocks gx 4..6, gy 0..1), with margin. */
	static bool IsOverLake(float X, float Y, float Margin = 200.f)
	{
		const float LX0 = BlockCenter(4) - Step * 0.5f;
		const float LX1 = BlockCenter(6) + Step * 0.5f;
		const float LY0 = BlockCenter(0) - Step * 0.5f;
		const float LY1 = BlockCenter(1) + Step * 0.5f;
		return X > LX0 - Margin && X < LX1 + Margin &&
		       Y > LY0 - Margin && Y < LY1 + Margin;
	}

	/** True if the point lies within the road grid's drivable extent. */
	static bool IsInsideRoadGrid(float X, float Y, float Margin = 300.f)
	{
		const float Lo = RoadCenter(0) - Margin;
		const float Hi = RoadCenter(NumRoads - 1) + Margin;
		return X > Lo && X < Hi && Y > Lo && Y < Hi;
	}

	/** True while a point remains inside the physical city perimeter. */
	static bool IsInsideCityBounds(float X, float Y, float ExtraMargin = 0.f)
	{
		const float Limit = CityBoundaryHalfExtent + FMath::Max(0.f, ExtraMargin);
		return FMath::Abs(X) <= Limit && FMath::Abs(Y) <= Limit;
	}

	/** Roads plus their curbside parking shoulder, used for safe recovery anchors. */
	static bool IsNearDrivableRoad(float X, float Y, float Shoulder = 180.f)
	{
		if (!IsInsideCityBounds(X, Y) || IsOverLake(X, Y, 50.f))
		{
			return false;
		}

		float NearestX = TNumericLimits<float>::Max();
		float NearestY = TNumericLimits<float>::Max();
		for (int32 RoadIndex = 0; RoadIndex < NumRoads; ++RoadIndex)
		{
			NearestX = FMath::Min(NearestX, FMath::Abs(X - RoadCenter(RoadIndex)));
			NearestY = FMath::Min(NearestY, FMath::Abs(Y - RoadCenter(RoadIndex)));
		}
		return FMath::Min(NearestX, NearestY) <= RoadWidth * 0.5f + Shoulder;
	}

	/** Unit forward vector for a cardinal heading. */
	static FVector HeadingVector(ESprawlHeading Heading)
	{
		switch (Heading)
		{
		case ESprawlHeading::East:  return FVector(1, 0, 0);
		case ESprawlHeading::North: return FVector(0, 1, 0);
		case ESprawlHeading::West:  return FVector(-1, 0, 0);
		default:                    return FVector(0, -1, 0);
		}
	}

	/** Actor yaw (degrees) for a cardinal heading. */
	static float HeadingYaw(ESprawlHeading Heading)
	{
		switch (Heading)
		{
		case ESprawlHeading::East:  return 0.f;
		case ESprawlHeading::North: return 90.f;
		case ESprawlHeading::West:  return 180.f;
		default:                    return 270.f;
		}
	}

	/** Cardinal heading nearest to a yaw in degrees. */
	static ESprawlHeading HeadingFromYaw(float Yaw)
	{
		const int32 Quadrant = FMath::RoundToInt(FMath::UnwindDegrees(Yaw) / 90.f);
		switch ((Quadrant % 4 + 4) % 4)
		{
		case 0:  return ESprawlHeading::East;
		case 1:  return ESprawlHeading::North;
		case 2:  return ESprawlHeading::West;
		default: return ESprawlHeading::South;
		}
	}

	/** True for headings that travel along the Y axis (on a vertical road). */
	static bool IsNorthSouth(ESprawlHeading Heading)
	{
		return Heading == ESprawlHeading::North || Heading == ESprawlHeading::South;
	}

	/**
	 * Right-hand-traffic lane center for travel on the given road.
	 * For a vertical road (north/south travel) this returns the lane X;
	 * for a horizontal road it returns the lane Y.
	 */
	static float LaneCenter(int32 RoadIndex, ESprawlHeading Heading)
	{
		// UE is left-handed (X fwd, Y right, Z up): the right-hand side of
		// travel is north -> -X, south -> +X, east -> +Y, west -> -Y.
		float Side = 0.f;
		switch (Heading)
		{
		case ESprawlHeading::North: Side = -1.f; break;
		case ESprawlHeading::South: Side = +1.f; break;
		case ESprawlHeading::East:  Side = +1.f; break;
		case ESprawlHeading::West:  Side = -1.f; break;
		}
		return RoadCenter(RoadIndex) + Side * LaneOffset;
	}

	// --- Traffic signals -------------------------------------------------

	/** Full signal cycle: 7s green, 1s amber, 1s all-red clearance per axis. */
	static constexpr float SignalCycle      = 18.f;
	static constexpr float SignalGreenTime  = 7.f;
	static constexpr float SignalAmberTime  = 1.f;
	static constexpr float SignalClearanceTime = 1.f;

	/**
	 * Signal state for traffic on the given axis at an intersection.
	 * bForNorthSouth = true asks for the lights governing N/S travel.
	 * Phases are staggered checkerboard-style so traffic pulses through
	 * the grid instead of the whole city stopping at once.
	 */
	ESprawlSignal GetSignal(int32 Ix, int32 Iy, bool bForNorthSouth) const;

	/** Seconds into this intersection's signal cycle. */
	float GetCycleTime(int32 Ix, int32 Iy) const;

	// --- Intersection right-of-way ---------------------------------------

	/**
	 * Conservatively lease an intersection to one vehicle at a time. Signals
	 * still decide which approach may request it; the lease prevents turning
	 * cars and a backed-up exit lane from blocking or colliding in the box.
	 */
	bool TryReserveIntersection(int32 Ix, int32 Iy, AActor* Vehicle);

	/** Release a lease only when it belongs to Vehicle. */
	void ReleaseIntersection(int32 Ix, int32 Iy, const AActor* Vehicle);

	/** True when Vehicle currently owns the live lease. */
	bool HasIntersectionReservation(int32 Ix, int32 Iy, const AActor* Vehicle) const;

	virtual void Deinitialize() override;

private:
	struct FIntersectionReservation
	{
		TWeakObjectPtr<AActor> Vehicle;
		float ExpiresAt = 0.f;
	};

	static int32 ReservationKey(int32 Ix, int32 Iy)
	{
		return Ix * NumRoads + Iy;
	}

	TMap<int32, FIntersectionReservation> IntersectionReservations;
};
