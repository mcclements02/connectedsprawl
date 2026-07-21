// The Connected Sprawl - Keep seated occupants' heads under the roofline.

#pragma once

#include "CoreMinimal.h"

struct FSprawlVehicleOccupantSeat;

/** Result of fitting a seated occupant under a cabin ceiling. */
struct CONNECTEDSPRAWL_API FSprawlOccupantHeadroomFit
{
	/** Adjusted pelvis height (actor space). */
	float SeatZ = 0.f;
	/** Uniform visual scale multiplier (1 when no shrink was needed). */
	float Scale = 1.f;
	bool bValid = false;
};

/**
 * The pelvis clamp keeps a seated occupant inside the cabin volume, but it has
 * no head term: on a low body the hip point can be legal while the head still
 * pokes through the roof. This fits the head too — first by sinking the seat
 * within the containment volume, then by a bounded uniform shrink; a body too
 * low for both suppresses the visual instead of clipping it.
 */
struct CONNECTEDSPRAWL_API FSprawlOccupantHeadroom
{
	/** Seated head height above the pelvis, as a fraction of standing height. */
	static constexpr float SeatedHeadFraction = 0.40f;
	/** Smallest acceptable visual shrink before the occupant is suppressed. */
	static constexpr float MinScale = 0.80f;

	static FSprawlOccupantHeadroomFit Fit(
		const FSprawlVehicleOccupantSeat& Seat, float StandingHeightCm);
};
