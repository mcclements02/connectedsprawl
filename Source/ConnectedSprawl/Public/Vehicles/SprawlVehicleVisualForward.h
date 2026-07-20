// The Connected Sprawl - Imported vehicle visual-forward resolution.

#pragma once

#include "CoreMinimal.h"

struct CONNECTEDSPRAWL_API FSprawlVehicleVisualForwardResult
{
	float RelativeYawDegrees = 0.f;
	bool bValid = false;
};

/** Pure named-axle orientation policy for imported split vehicle kits. */
class CONNECTEDSPRAWL_API FSprawlVehicleVisualForward
{
public:
	/** Return the yaw that maps the named front axle onto the pawn's +X forward. */
	static FSprawlVehicleVisualForwardResult ResolveRelativeYaw(
		const FVector& FrontLeft, const FVector& FrontRight,
		const FVector& RearLeft, const FVector& RearRight);
};
