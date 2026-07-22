// The Connected Sprawl - Keep in-car drivers logical, hidden, and mobile-cheap.

#pragma once

#include "CoreMinimal.h"

class USkeletalMeshComponent;

/** Immutable rendering decision for the car-mounted driver representation. */
struct CONNECTEDSPRAWL_API FSprawlDriverVisibilityDecision
{
	bool bKeepLogicalOccupancy = false;
	bool bRenderMountedMesh = false;
	bool bLoadAvatarAssets = false;
	bool bTickPose = false;
};

/**
 * Separates vehicle occupancy from rendering.
 *
 * Cars use dark/opaque cabin glass and never need an attached full-body driver
 * mesh. Keeping that mesh hidden prevents heads from clipping through roofs and
 * avoids loading or ticking one skeletal avatar per traffic car. The logical
 * occupant remains available to entry, carjacking, possession, and save code;
 * an ejected driver is rendered by its ordinary exterior pedestrian actor.
 */
class CONNECTEDSPRAWL_API FSprawlDriverVisibility
{
public:
	static FSprawlDriverVisibilityDecision Resolve(bool bHasSeatedDriver);

	/** Enforce the decision on the legacy car-mounted skeletal component. */
	static void ApplyToMountedMesh(
		USkeletalMeshComponent* DriverMesh, bool bHasSeatedDriver);
};
