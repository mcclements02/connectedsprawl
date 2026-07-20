// The Connected Sprawl - Sit vehicles on their tyres, not their floorpan.

#pragma once

#include "CoreMinimal.h"

class AActor;
class UStaticMesh;

/**
 * FSprawlVehicleStance
 * --------------------
 * A SprawlCar rests on its collision hull, so the hull's underside is the
 * contact plane: anything modelled below it ends up under the tarmac.
 *
 * The original assembly aligned the *body* mesh's underside to that plane.
 * On a real car the tyres hang below the floorpan, so every wheel sank into
 * the road — the car appeared to skate on its chassis with its tyres buried.
 *
 * These helpers measure the lowest point of the assembled visual (body **and**
 * wheels) and seat that on the contact plane instead, which is what gives a
 * car its ride height: tyres on the tarmac, floorpan clear above it.
 */
struct CONNECTEDSPRAWL_API FSprawlVehicleStance
{
	/** Lowest point of body + wheels, in the source asset's own space. */
	static double ComputeVisualBottomZ(const UStaticMesh* Body,
		const TArray<UStaticMesh*>& Wheels);

	/**
	 * Relative Z for the assembled visual so its lowest point lands on the
	 * hull's contact plane (-HullHalfHeight).
	 */
	static float ComputeGroundedOffsetZ(const UStaticMesh* Body,
		const TArray<UStaticMesh*>& Wheels, float Scale, float HullHalfHeight);

	/** Centre height for a prototype wheel so its tread meets the contact plane. */
	static float ComputePrototypeWheelCenterZ(float WheelRadius, float HullHalfHeight)
	{
		return -HullHalfHeight + WheelRadius;
	}

	/**
	 * Lowest point of everything currently visible on a vehicle, in actor
	 * space. Measured from the live components rather than the source assets,
	 * so it also covers bodies assigned after construction and level instances
	 * whose transforms were serialised by an authoring pass.
	 */
	static bool ComputeVisibleBottomZ(const AActor* Vehicle, float& OutBottomZ);
};
