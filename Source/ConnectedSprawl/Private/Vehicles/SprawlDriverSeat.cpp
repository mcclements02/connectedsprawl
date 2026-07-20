// The Connected Sprawl - Seat occupants inside the car, not on it.

#include "Vehicles/SprawlDriverSeat.h"

#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"

bool FSprawlDriverSeat::ComputeSeatLocation(const AActor* Vehicle,
	const USceneComponent* IgnoredSeat, FVector& OutLocation)
{
	if (!Vehicle)
	{
		return false;
	}

	const FTransform ActorTransform = Vehicle->GetActorTransform();
	FBox Body(ForceInit);

	TInlineComponentArray<UMeshComponent*> MeshComponents(Vehicle);
	for (const UMeshComponent* MeshComponent : MeshComponents)
	{
		if (!MeshComponent || MeshComponent == IgnoredSeat
			|| !MeshComponent->IsVisible() || MeshComponent->bHiddenInGame)
		{
			continue;
		}
		// Measured in actor space so nested parts land in the same frame.
		const FTransform Relative =
			MeshComponent->GetComponentTransform().GetRelativeTransform(ActorTransform);
		const FBoxSphereBounds Bounds = MeshComponent->CalcBounds(Relative);
		if (Bounds.BoxExtent.IsNearlyZero())
		{
			continue;
		}
		Body += FBox::BuildAABB(Bounds.Origin, Bounds.BoxExtent);
	}

	if (!Body.IsValid)
	{
		return false;
	}

	const FVector Center = Body.GetCenter();
	const FVector Size = Body.GetSize();
	OutLocation = FVector(
		Center.X + Size.X * ForwardFraction,
		Center.Y - Size.Y * SideFraction,
		Body.Min.Z + Size.Z * HipHeightFraction);
	return true;
}
