// The Connected Sprawl - Sit vehicles on their tyres, not their floorpan.

#include "Vehicles/SprawlVehicleStance.h"

#include "Components/MeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"

double FSprawlVehicleStance::ComputeVisualBottomZ(
	const UStaticMesh* Body, const TArray<UStaticMesh*>& Wheels)
{
	double BottomZ = TNumericLimits<double>::Max();
	if (Body)
	{
		BottomZ = Body->GetBoundingBox().Min.Z;
	}
	// The split vehicle kits keep each part in the source car's own space, so
	// wheel bounds are directly comparable with the body's.
	for (const UStaticMesh* Wheel : Wheels)
	{
		if (Wheel)
		{
			BottomZ = FMath::Min(BottomZ, Wheel->GetBoundingBox().Min.Z);
		}
	}
	return BottomZ == TNumericLimits<double>::Max() ? 0.0 : BottomZ;
}

float FSprawlVehicleStance::ComputeGroundedOffsetZ(const UStaticMesh* Body,
	const TArray<UStaticMesh*>& Wheels, float Scale, float HullHalfHeight)
{
	const double VisualBottomZ = ComputeVisualBottomZ(Body, Wheels);
	return static_cast<float>(-HullHalfHeight - VisualBottomZ * Scale);
}

bool FSprawlVehicleStance::ComputeVisibleBottomZ(
	const AActor* Vehicle, float& OutBottomZ)
{
	if (!Vehicle)
	{
		return false;
	}

	const FTransform ActorTransform = Vehicle->GetActorTransform();
	double Lowest = TNumericLimits<double>::Max();

	TInlineComponentArray<UMeshComponent*> MeshComponents(Vehicle);
	for (const UMeshComponent* MeshComponent : MeshComponents)
	{
		if (!MeshComponent || !MeshComponent->IsVisible()
			|| MeshComponent->bHiddenInGame)
		{
			continue;
		}
		// Re-derive each component's bounds in actor space so nested parts
		// (wheels hanging off their steering pivots) are measured correctly.
		const FTransform RelativeTransform =
			MeshComponent->GetComponentTransform().GetRelativeTransform(ActorTransform);
		const FBoxSphereBounds Bounds = MeshComponent->CalcBounds(RelativeTransform);
		if (Bounds.BoxExtent.IsNearlyZero())
		{
			continue;
		}
		Lowest = FMath::Min(Lowest, Bounds.Origin.Z - Bounds.BoxExtent.Z);
	}

	if (Lowest == TNumericLimits<double>::Max())
	{
		return false;
	}
	OutBottomZ = static_cast<float>(Lowest);
	return true;
}
