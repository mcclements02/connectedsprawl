// The Connected Sprawl - Day/night contrast targets.

#pragma once

#include "CoreMinimal.h"

/**
 * Lighting values that preserve readable shadows without relying on automatic
 * exposure. The environment controller applies this profile to scene actors.
 */
struct CONNECTEDSPRAWL_API FSprawlLightingContrastProfile
{
	float DirectLightMultiplier = 1.f;
	float SkyLightMultiplier = 1.f;
	float FogDensityMultiplier = 1.f;
	FLinearColor DayFogColor = FLinearColor(0.53f, 0.58f, 0.68f, 1.f);
};

/** Pure lighting policy shared by runtime code and visual diagnostics. */
class CONNECTEDSPRAWL_API FSprawlLightingContrast
{
public:
	/** Build a safe profile for the current solar elevation in degrees. */
	static FSprawlLightingContrastProfile BuildProfile(float SunElevationDegrees);
};
