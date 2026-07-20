// The Connected Sprawl - Keep visible occupants inside vehicle bodywork.

#pragma once

#include "CoreMinimal.h"

class AActor;
class USceneComponent;

struct CONNECTEDSPRAWL_API FSprawlVehicleOccupantSeat
{
	FVector Location = FVector::ZeroVector;
	FBox ContainmentBounds = FBox(ForceInit);
	bool bValid = false;
};

/** Pure/actor-space placement policy for seated vehicle occupants. */
class CONNECTEDSPRAWL_API FSprawlVehicleOccupantPlacement
{
public:
	/** Compute a conservative driver pelvis location from bodywork only. */
	static FSprawlVehicleOccupantSeat ComputeFromBodyBounds(const FBox& BodyBounds);

	/** Measure visible bodywork, excluding wheels and the occupant itself. */
	static FSprawlVehicleOccupantSeat ComputeForVehicle(
		const AActor* Vehicle, const USceneComponent* OccupantComponent);

	static bool IsContained(const FSprawlVehicleOccupantSeat& Seat);
	static bool IsLocationContained(
		const FSprawlVehicleOccupantSeat& Seat, const FVector& Location);
};
