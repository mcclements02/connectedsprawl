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

	/**
	 * Replace only the visual yaw with the named-axle result, retaining any
	 * deliberate pitch or roll supplied by the asset author. Returns the
	 * authored rotation unchanged when the axle data is not usable.
	 */
	static FRotator ResolveAlignedRotation(
		const FRotator& AuthoredRotation,
		const FVector& FrontLeft, const FVector& FrontRight,
		const FVector& RearLeft, const FVector& RearRight);

	/** Blender Y-forward imports need -90 degrees to point their nose at UE +X. */
	static FRotator ResolveBlenderYForwardRotation(const FRotator& AuthoredRotation);
};
