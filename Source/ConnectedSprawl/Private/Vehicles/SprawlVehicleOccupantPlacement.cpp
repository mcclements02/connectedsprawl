// The Connected Sprawl - Keep visible occupants inside vehicle bodywork.

#include "Vehicles/SprawlVehicleOccupantPlacement.h"

#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"

namespace
{
bool IsBodywork(const UMeshComponent* Mesh, const USceneComponent* Occupant)
{
	if (!Mesh || Mesh == Occupant || !Mesh->IsVisible() || Mesh->bHiddenInGame)
	{
		return false;
	}
	const FString Name = Mesh->GetName();
	return !Name.StartsWith(TEXT("Wheel"))
		&& !Name.Contains(TEXT("Driver"))
		&& !Mesh->ComponentHasTag(TEXT("Sprawl.VehicleWheel"));
}
}

FSprawlVehicleOccupantSeat FSprawlVehicleOccupantPlacement::ComputeFromBodyBounds(
	const FBox& BodyBounds)
{
	FSprawlVehicleOccupantSeat Seat;
	if (!BodyBounds.IsValid)
	{
		return Seat;
	}
	const FVector Size = BodyBounds.GetSize();
	if (Size.X < 180.f || Size.Y < 90.f || Size.Z < 55.f)
	{
		return Seat;
	}

	// The pelvis stays in the inner cabin volume. Keeping it farther from the
	// door than the former quarter-width offset prevents seated poses from
	// protruding through narrow imported bodies.
	const FVector Inset(Size.X * 0.18f, Size.Y * 0.18f, Size.Z * 0.16f);
	Seat.ContainmentBounds = FBox(
		BodyBounds.Min + Inset,
		BodyBounds.Max - FVector(Inset.X, Inset.Y, Size.Z * 0.12f));
	const FVector Center = BodyBounds.GetCenter();
	Seat.Location = FVector(
		Center.X + Size.X * 0.05f,
		Center.Y - Size.Y * 0.18f,
		BodyBounds.Min.Z + Size.Z * 0.44f);
	Seat.Location.X = FMath::Clamp(Seat.Location.X,
		Seat.ContainmentBounds.Min.X, Seat.ContainmentBounds.Max.X);
	Seat.Location.Y = FMath::Clamp(Seat.Location.Y,
		Seat.ContainmentBounds.Min.Y, Seat.ContainmentBounds.Max.Y);
	Seat.Location.Z = FMath::Clamp(Seat.Location.Z,
		Seat.ContainmentBounds.Min.Z, Seat.ContainmentBounds.Max.Z);
	Seat.bValid = IsContained(Seat);
	return Seat;
}

FSprawlVehicleOccupantSeat FSprawlVehicleOccupantPlacement::ComputeForVehicle(
	const AActor* Vehicle, const USceneComponent* OccupantComponent)
{
	if (!Vehicle)
	{
		return FSprawlVehicleOccupantSeat();
	}
	const FTransform ActorTransform = Vehicle->GetActorTransform();
	FBox BodyBounds(ForceInit);
	TInlineComponentArray<UMeshComponent*> MeshComponents(Vehicle);
	for (const UMeshComponent* Mesh : MeshComponents)
	{
		if (!IsBodywork(Mesh, OccupantComponent))
		{
			continue;
		}
		const FTransform Relative =
			Mesh->GetComponentTransform().GetRelativeTransform(ActorTransform);
		const FBoxSphereBounds Bounds = Mesh->CalcBounds(Relative);
		if (!Bounds.BoxExtent.IsNearlyZero())
		{
			BodyBounds += FBox::BuildAABB(Bounds.Origin, Bounds.BoxExtent);
		}
	}
	return ComputeFromBodyBounds(BodyBounds);
}

bool FSprawlVehicleOccupantPlacement::IsContained(
	const FSprawlVehicleOccupantSeat& Seat)
{
	return IsLocationContained(Seat, Seat.Location);
}

bool FSprawlVehicleOccupantPlacement::IsLocationContained(
	const FSprawlVehicleOccupantSeat& Seat, const FVector& Location)
{
	return Seat.ContainmentBounds.IsValid
		&& Seat.ContainmentBounds.IsInsideOrOn(Location);
}
