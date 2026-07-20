// The Connected Sprawl - Shared arcade vehicle drive-state policy.

#pragma once

#include "CoreMinimal.h"

struct CONNECTEDSPRAWL_API FSprawlVehicleDriveCommand
{
	float LongitudinalForce = 0.f;
	float YawRateDegrees = 0.f;
	bool bBraking = false;
	bool bReversing = false;
};

/** Pure force, brake/reverse, and steering-direction logic. */
class CONNECTEDSPRAWL_API FSprawlVehicleDriveLogic
{
public:
	static FSprawlVehicleDriveCommand ResolvePlayerCommand(
		float Throttle, float Steering, float ForwardSpeed,
		float EngineForce, float BrakeMultiplier, float TurnRate);

	/** AI owns target speed; this preserves gravity while applying planar drive. */
	static FVector BuildKinematicVelocity(
		const FVector& Forward, float TargetSpeed, const FVector& CurrentVelocity);

	/** AI cars may steer only while actually moving through their route. */
	static float ResolveKinematicYawRate(
		float Steering, float TargetSpeed, float TurnRate);
};
