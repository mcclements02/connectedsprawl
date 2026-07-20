// The Connected Sprawl - Shared arcade vehicle drive-state policy.

#include "Vehicles/SprawlVehicleDriveLogic.h"

namespace
{
constexpr float InputDeadZone = 0.05f;
constexpr float SteeringSpeedFloor = 25.f;
constexpr float ReverseEngagementSpeed = 60.f;
}

FSprawlVehicleDriveCommand FSprawlVehicleDriveLogic::ResolvePlayerCommand(
	float Throttle, float Steering, float ForwardSpeed, float EngineForce,
	float BrakeMultiplier, float TurnRate)
{
	FSprawlVehicleDriveCommand Command;
	const float ClampedThrottle = FMath::Clamp(Throttle, -1.f, 1.f);
	const float ClampedSteering = FMath::Clamp(Steering, -1.f, 1.f);
	if (FMath::Abs(ClampedThrottle) > InputDeadZone)
	{
		Command.bBraking = ClampedThrottle * ForwardSpeed < 0.f
			&& FMath::Abs(ForwardSpeed) > ReverseEngagementSpeed;
		Command.bReversing = ClampedThrottle < -InputDeadZone
			&& ForwardSpeed <= ReverseEngagementSpeed;
		Command.LongitudinalForce = ClampedThrottle * FMath::Max(0.f, EngineForce)
			* (Command.bBraking ? FMath::Max(1.f, BrakeMultiplier) : 1.f);
	}

	if (FMath::Abs(ClampedSteering) > InputDeadZone
		&& FMath::Abs(ForwardSpeed) > SteeringSpeedFloor)
	{
		const float SpeedFactor = FMath::Clamp(
			FMath::Abs(ForwardSpeed) / 520.f, 0.35f, 1.f);
		Command.YawRateDegrees = ClampedSteering * FMath::Sign(ForwardSpeed)
			* FMath::Max(0.f, TurnRate) * SpeedFactor;
	}
	return Command;
}

FVector FSprawlVehicleDriveLogic::BuildKinematicVelocity(
	const FVector& Forward, float TargetSpeed, const FVector& CurrentVelocity)
{
	FVector PlanarForward(Forward.X, Forward.Y, 0.f);
	PlanarForward.Normalize();
	const FVector Drive = PlanarForward * FMath::Max(0.f, TargetSpeed);
	return FVector(Drive.X, Drive.Y, CurrentVelocity.Z);
}

float FSprawlVehicleDriveLogic::ResolveKinematicYawRate(
	float Steering, float TargetSpeed, float TurnRate)
{
	if (FMath::Abs(Steering) <= InputDeadZone
		|| TargetSpeed <= SteeringSpeedFloor)
	{
		return 0.f;
	}
	return FMath::Clamp(Steering, -1.f, 1.f) * FMath::Max(0.f, TurnRate);
}
