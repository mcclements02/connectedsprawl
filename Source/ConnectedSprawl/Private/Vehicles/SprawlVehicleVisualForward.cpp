// The Connected Sprawl - Imported vehicle visual-forward resolution.

#include "Vehicles/SprawlVehicleVisualForward.h"

FSprawlVehicleVisualForwardResult FSprawlVehicleVisualForward::ResolveRelativeYaw(
	const FVector& FrontLeft, const FVector& FrontRight,
	const FVector& RearLeft, const FVector& RearRight)
{
	FSprawlVehicleVisualForwardResult Result;
	const FVector FrontAxle = (FrontLeft + FrontRight) * 0.5f;
	const FVector RearAxle = (RearLeft + RearRight) * 0.5f;
	const FVector AxleDirection = FrontAxle - RearAxle;
	if (AxleDirection.SizeSquared2D() < 1.f)
	{
		return Result;
	}

	const float NaturalYaw = FMath::RadiansToDegrees(
		FMath::Atan2(AxleDirection.Y, AxleDirection.X));
	Result.RelativeYawDegrees = FMath::UnwindDegrees(-NaturalYaw);
	Result.bValid = true;
	return Result;
}
