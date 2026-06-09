// The Connected Sprawl - City grid road network.

#include "World/SprawlCityGridSubsystem.h"
#include "Engine/World.h"

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
