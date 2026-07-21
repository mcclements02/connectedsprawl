// The Connected Sprawl - Keep seated occupants' heads under the roofline.

#include "Vehicles/SprawlOccupantHeadroom.h"

#include "Vehicles/SprawlVehicleOccupantPlacement.h"

FSprawlOccupantHeadroomFit FSprawlOccupantHeadroom::Fit(
	const FSprawlVehicleOccupantSeat& Seat, float StandingHeightCm)
{
	FSprawlOccupantHeadroomFit Result;
	if (!Seat.bValid || StandingHeightCm <= 0.f)
	{
		return Result;
	}
	// The containment ceiling already sits ~12% of the body height below the
	// actual roof, so fitting the head under it leaves a real visual margin.
	const float Ceiling = Seat.ContainmentBounds.Max.Z;
	const float Floor = Seat.ContainmentBounds.Min.Z;
	const float HeadAboveHips = StandingHeightCm * SeatedHeadFraction;

	Result.SeatZ = Seat.Location.Z;
	Result.Scale = 1.f;
	const float HeadTop = Seat.Location.Z + HeadAboveHips;
	if (HeadTop <= Ceiling)
	{
		Result.bValid = true;
		return Result;
	}

	// First sink the seat as far as the containment floor allows...
	const float Deficit = HeadTop - Ceiling;
	const float Sink = FMath::Min(Deficit, Seat.Location.Z - Floor);
	Result.SeatZ = Seat.Location.Z - Sink;

	// ...then absorb any remainder with a bounded uniform shrink.
	const float Remaining = Deficit - Sink;
	if (Remaining > 0.f)
	{
		Result.Scale = (HeadAboveHips - Remaining) / HeadAboveHips;
	}
	Result.bValid = Result.Scale >= MinScale;
	Result.Scale = FMath::Max(Result.Scale, MinScale);
	return Result;
}
