// The Connected Sprawl - Seat occupants inside the car, not on it.

#pragma once

#include "CoreMinimal.h"

class AActor;
class USceneComponent;

/**
 * FSprawlDriverSeat
 * -----------------
 * The seated occupant used to be pinned at a fixed offset tuned for the
 * prototype kitbash. Every imported vehicle body is a different size, so that
 * one offset left drivers half outside the shell — sitting on the boot of a
 * low car, sunk into the floor of a tall one.
 *
 * The seat is derived from the body the car is actually wearing instead:
 * measured from the visible bodywork, a driver sits ahead of centre, offset to
 * the driver's side, at the height a seated occupant's hips would be inside
 * the cabin. Any body, any scale, always inside.
 */
struct CONNECTEDSPRAWL_API FSprawlDriverSeat
{
	/** Fractions of the measured body used to place an occupant. */
	static constexpr float ForwardFraction = 0.08f;   // ahead of body centre
	static constexpr float SideFraction    = 0.26f;   // toward the driver's side
	static constexpr float HipHeightFraction = 0.44f; // up from the body floor

	/**
	 * Seat location in the vehicle's actor space.
	 * @param Vehicle       Car to measure.
	 * @param IgnoredSeat   The occupant mesh itself, excluded from the measure.
	 * @param OutLocation   Receives the seat position.
	 * @return false when the vehicle has no visible bodywork to measure.
	 */
	static bool ComputeSeatLocation(const AActor* Vehicle,
		const USceneComponent* IgnoredSeat, FVector& OutLocation);
};
