// The Connected Sprawl - City grid road network.

#include "World/SprawlCityGridSubsystem.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

namespace
{
constexpr float IntersectionLeaseSeconds = 6.f;
static_assert(USprawlCityGridSubsystem::SignalCycle * 0.5f
	== USprawlCityGridSubsystem::SignalGreenTime
		+ USprawlCityGridSubsystem::SignalAmberTime
		+ USprawlCityGridSubsystem::SignalClearanceTime,
	"Each signal half-cycle must include green, amber, and all-red clearance.");
}

float USprawlCityGridSubsystem::GetCycleTime(int32 Ix, int32 Iy) const
{
	const UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.f;
	// Checkerboard stagger: adjacent intersections run a half-cycle apart.
	const float Offset = ((Ix + Iy) & 1) ? SignalCycle * 0.5f : 0.f;
	return FMath::Fmod(Now + Offset, SignalCycle);
}

ESprawlSignal USprawlCityGridSubsystem::GetSignal(int32 Ix, int32 Iy, bool bForNorthSouth) const
{
	const float T = GetCycleTime(Ix, Iy);
	const float Half = SignalCycle * 0.5f;

	// First half-cycle serves north/south, second serves east/west.
	const bool bInFirstHalf = T < Half;
	const float HalfT = bInFirstHalf ? T : T - Half;
	const bool bServed = (bForNorthSouth == bInFirstHalf);

	if (!bServed)
	{
		return ESprawlSignal::Red;
	}
	if (HalfT < SignalGreenTime)
	{
		return ESprawlSignal::Green;
	}
	if (HalfT < SignalGreenTime + SignalAmberTime)
	{
		return ESprawlSignal::Amber;
	}
	return ESprawlSignal::Red;
}

bool USprawlCityGridSubsystem::TryReserveIntersection(int32 Ix, int32 Iy, AActor* Vehicle)
{
	if (!Vehicle || Ix < 0 || Ix >= NumRoads || Iy < 0 || Iy >= NumRoads)
	{
		return false;
	}

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	FIntersectionReservation& Reservation = IntersectionReservations.FindOrAdd(
		ReservationKey(Ix, Iy));
	AActor* CurrentVehicle = Reservation.Vehicle.Get();
	if (CurrentVehicle && CurrentVehicle != Vehicle && Reservation.ExpiresAt > Now)
	{
		return false;
	}

	Reservation.Vehicle = Vehicle;
	Reservation.ExpiresAt = Now + IntersectionLeaseSeconds;
	return true;
}

void USprawlCityGridSubsystem::ReleaseIntersection(
	int32 Ix, int32 Iy, const AActor* Vehicle)
{
	if (!Vehicle)
	{
		return;
	}

	const int32 Key = ReservationKey(Ix, Iy);
	if (const FIntersectionReservation* Reservation = IntersectionReservations.Find(Key))
	{
		if (Reservation->Vehicle.Get() == Vehicle)
		{
			IntersectionReservations.Remove(Key);
		}
	}
}

bool USprawlCityGridSubsystem::HasIntersectionReservation(
	int32 Ix, int32 Iy, const AActor* Vehicle) const
{
	if (!Vehicle)
	{
		return false;
	}

	const FIntersectionReservation* Reservation = IntersectionReservations.Find(
		ReservationKey(Ix, Iy));
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	return Reservation && Reservation->Vehicle.Get() == Vehicle
		&& Reservation->ExpiresAt > Now;
}

void USprawlCityGridSubsystem::Deinitialize()
{
	IntersectionReservations.Reset();
	Super::Deinitialize();
}
