// The Connected Sprawl - Day/night contrast targets.

#include "World/SprawlLightingContrast.h"

FSprawlLightingContrastProfile FSprawlLightingContrast::BuildProfile(
	float SunElevationDegrees)
{
	FSprawlLightingContrastProfile Profile;
	const float DayAmount = FMath::Clamp(SunElevationDegrees / 12.f, 0.f, 1.f);
	const float Twilight = FMath::Clamp(
		1.f - FMath::Abs(SunElevationDegrees) / 10.f, 0.f, 1.f);

	// Direct light remains stable so the brightness budget does not jump. The
	// reduced sky and haze fill restore readable building and vehicle form.
	Profile.DirectLightMultiplier = FMath::Lerp(0.96f, 1.f, DayAmount);
	Profile.SkyLightMultiplier = FMath::Lerp(0.92f, 0.82f, DayAmount);
	Profile.FogDensityMultiplier = FMath::Lerp(0.88f, 0.74f, DayAmount);
	Profile.FogDensityMultiplier = FMath::Lerp(
		Profile.FogDensityMultiplier, 0.90f, Twilight);
	return Profile;
}
