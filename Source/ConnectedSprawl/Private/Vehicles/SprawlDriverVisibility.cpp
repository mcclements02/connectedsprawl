// The Connected Sprawl - Keep in-car drivers logical, hidden, and mobile-cheap.

#include "Vehicles/SprawlDriverVisibility.h"

#include "Components/SkeletalMeshComponent.h"

FSprawlDriverVisibilityDecision FSprawlDriverVisibility::Resolve(
	bool bHasSeatedDriver)
{
	FSprawlDriverVisibilityDecision Decision;
	Decision.bKeepLogicalOccupancy = bHasSeatedDriver;
	// Mounted occupants are intentionally never rendered. Exterior characters
	// use their own actor and are outside this policy.
	Decision.bRenderMountedMesh = false;
	Decision.bLoadAvatarAssets = false;
	Decision.bTickPose = false;
	return Decision;
}

void FSprawlDriverVisibility::ApplyToMountedMesh(
	USkeletalMeshComponent* DriverMesh, bool bHasSeatedDriver)
{
	if (!DriverMesh)
	{
		return;
	}

	const FSprawlDriverVisibilityDecision Decision = Resolve(bHasSeatedDriver);
	DriverMesh->Stop();
	DriverMesh->SetVisibility(Decision.bRenderMountedMesh, true);
	DriverMesh->SetHiddenInGame(!Decision.bRenderMountedMesh);
	DriverMesh->SetComponentTickEnabled(Decision.bTickPose);
	if (!Decision.bLoadAvatarAssets)
	{
		// Clear stale serialized/hot-reload state as well as preventing new loads.
		DriverMesh->SetSkeletalMesh(nullptr);
	}
}
