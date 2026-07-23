// The Connected Sprawl - reusable real-mesh furniture and interior-item catalog.

#include "World/SprawlInteriorPropLibrary.h"

#include "Engine/StaticMesh.h"
#include "UObject/UObjectGlobals.h"
#include "World/SprawlEnterableInteriors.h"

namespace
{
constexpr float InteriorPropShapeMeshSize = 100.f;

void AddProp(TArray<FSprawlInteriorPropPlacement>& Out,
	ESprawlInteriorPropType Type, const FVector& RoomCenter,
	const FVector& OffsetFromFloor, const FVector& DesiredSize,
	float Yaw = 0.f)
{
	FSprawlInteriorPropPlacement& Placement = Out.AddDefaulted_GetRef();
	Placement.Type = Type;
	Placement.Target = FTransform(
		FRotator(0.f, Yaw, 0.f), RoomCenter + OffsetFromFloor,
		DesiredSize / InteriorPropShapeMeshSize);
}

void AddStoreProps(TArray<FSprawlInteriorPropPlacement>& Out,
	const FVector& C)
{
	AddProp(Out, ESprawlInteriorPropType::DisplayTable, C,
		FVector(-360.f, 90.f, 0.f), FVector(270.f, 125.f, 88.f));
	AddProp(Out, ESprawlInteriorPropType::DisplayTable, C,
		FVector(360.f, 90.f, 0.f), FVector(270.f, 125.f, 88.f));
	AddProp(Out, ESprawlInteriorPropType::ChairA, C,
		FVector(0.f, 620.f, 0.f), FVector(58.f, 58.f, 98.f), 180.f);
	AddProp(Out, ESprawlInteriorPropType::PosterStand, C,
		FVector(920.f, -420.f, 0.f), FVector(72.f, 42.f, 150.f), -20.f);
	AddProp(Out, ESprawlInteriorPropType::RecycleBin, C,
		FVector(-920.f, -420.f, 0.f), FVector(55.f, 55.f, 90.f));

	for (const float X : { -930.f, 930.f })
	{
		AddProp(Out, ESprawlInteriorPropType::Planter, C,
			FVector(X, 610.f, 0.f), FVector(48.f, 48.f, 50.f));
		AddProp(Out, ESprawlInteriorPropType::Foliage, C,
			FVector(X, 610.f, 36.f), FVector(72.f, 72.f, 105.f));
	}

	for (const float TableX : { -360.f, 360.f })
	{
		for (const float XOffset : { -82.f, -28.f, 28.f, 82.f })
		{
			for (const float YOffset : { 58.f, 116.f })
			{
				AddProp(Out, ESprawlInteriorPropType::ProductCan, C,
					FVector(TableX + XOffset, YOffset, 89.f),
					FVector(9.f, 9.f, 17.f));
			}
		}
	}

	for (const float X : { -300.f, -180.f, -60.f, 60.f, 180.f, 300.f })
	{
		AddProp(Out, ESprawlInteriorPropType::ProductBottle, C,
			FVector(X, 476.f, 111.f), FVector(9.f, 9.f, 25.f));
	}
	for (const float X : { -840.f, 840.f })
	{
		for (const float Y : { -260.f, 0.f, 260.f })
		{
			AddProp(Out, ESprawlInteriorPropType::ProductCan, C,
				FVector(X, Y, 156.f), FVector(9.f, 9.f, 17.f));
		}
	}
}

void AddOfficeProps(TArray<FSprawlInteriorPropPlacement>& Out,
	const FVector& C)
{
	for (const float X : { -510.f, 510.f })
	{
		for (const float Y : { -30.f, 250.f })
		{
			AddProp(Out, ESprawlInteriorPropType::DisplayTable, C,
				FVector(X, Y, 0.f), FVector(300.f, 140.f, 76.f));
			AddProp(Out, ESprawlInteriorPropType::ChairA, C,
				FVector(X, Y - 125.f, 0.f), FVector(58.f, 58.f, 100.f));
			AddProp(Out, ESprawlInteriorPropType::ProductBottle, C,
				FVector(X + 95.f, Y, 77.f), FVector(8.f, 8.f, 23.f));
		}
	}

	AddProp(Out, ESprawlInteriorPropType::DisplayTable, C,
		FVector(0.f, 455.f, 0.f), FVector(390.f, 165.f, 76.f));
	for (const FVector& Seat : {
		FVector(-220.f, 455.f, 0.f), FVector(220.f, 455.f, 0.f),
		FVector(-90.f, 590.f, 0.f), FVector(90.f, 590.f, 0.f) })
	{
		AddProp(Out, ESprawlInteriorPropType::ChairB, C, Seat,
			FVector(58.f, 58.f, 100.f), Seat.Y > 500.f ? 180.f : 90.f);
	}
	AddProp(Out, ESprawlInteriorPropType::LoungeBench, C,
		FVector(-850.f, 560.f, 0.f), FVector(185.f, 70.f, 92.f), 90.f);
	AddProp(Out, ESprawlInteriorPropType::PosterStand, C,
		FVector(900.f, 570.f, 0.f), FVector(68.f, 38.f, 145.f));
	for (const float X : { -910.f, 910.f })
	{
		AddProp(Out, ESprawlInteriorPropType::Planter, C,
			FVector(X, -420.f, 0.f), FVector(48.f, 48.f, 50.f));
		AddProp(Out, ESprawlInteriorPropType::Foliage, C,
			FVector(X, -420.f, 36.f), FVector(72.f, 72.f, 105.f));
	}
}

void AddCondoProps(TArray<FSprawlInteriorPropPlacement>& Out,
	const FVector& C)
{
	AddProp(Out, ESprawlInteriorPropType::DisplayTable, C,
		FVector(260.f, -20.f, 0.f), FVector(210.f, 125.f, 76.f));
	for (const FVector& Seat : {
		FVector(100.f, -20.f, 0.f), FVector(420.f, -20.f, 0.f),
		FVector(260.f, -125.f, 0.f), FVector(260.f, 85.f, 0.f) })
	{
		AddProp(Out, ESprawlInteriorPropType::ChairA, C, Seat,
			FVector(56.f, 56.f, 98.f), FMath::Atan2(
				260.f - Seat.X, Seat.Y + 20.f) * 180.f / PI);
	}
	AddProp(Out, ESprawlInteriorPropType::PosterStand, C,
		FVector(900.f, 575.f, 0.f), FVector(68.f, 38.f, 145.f));
	AddProp(Out, ESprawlInteriorPropType::RecycleBin, C,
		FVector(-820.f, -520.f, 0.f), FVector(52.f, 52.f, 86.f));
	AddProp(Out, ESprawlInteriorPropType::Planter, C,
		FVector(-930.f, 600.f, 0.f), FVector(50.f, 50.f, 52.f));
	AddProp(Out, ESprawlInteriorPropType::Foliage, C,
		FVector(-930.f, 600.f, 38.f), FVector(78.f, 78.f, 115.f));

	for (const float Y : { -430.f, -300.f, -170.f, -40.f, 90.f })
	{
		AddProp(Out, ESprawlInteriorPropType::ProductBottle, C,
			FVector(-900.f, Y, 213.f), FVector(9.f, 9.f, 25.f));
	}
	for (const float Y : { 240.f, 300.f, 360.f })
	{
		AddProp(Out, ESprawlInteriorPropType::ProductCan, C,
			FVector(360.f, Y, 0.f), FVector(10.f, 10.f, 18.f));
	}
}
}

bool FSprawlInteriorPropDefinition::IsValid() const
{
	return !Id.IsNone() && !AssetPaths.IsEmpty()
		&& AssetPaths.ContainsByPredicate([](const FString& Path)
		{
			return Path.StartsWith(TEXT("/Game/")) && Path.Contains(TEXT("."));
		});
}

const TArray<FSprawlInteriorPropDefinition>&
	FSprawlInteriorPropLibrary::GetDefinitions()
{
	static const TArray<FSprawlInteriorPropDefinition> Definitions = {
		{ ESprawlInteriorPropType::DisplayTable, TEXT("DisplayTable"), {
			TEXT("/Game/Downtown_West/Assets/props/prop_table/SM_prop_dining_table_a.SM_prop_dining_table_a") }, true, true },
		{ ESprawlInteriorPropType::ChairA, TEXT("ChairA"), {
			TEXT("/Game/Downtown_West/Assets/props/prop_table/SM_prop_dining_chair_a.SM_prop_dining_chair_a"),
			TEXT("/Game/Downtown_West/Assets/props/prop_table/SM_prop_dining_chair_b.SM_prop_dining_chair_b") }, true, true },
		{ ESprawlInteriorPropType::ChairB, TEXT("ChairB"), {
			TEXT("/Game/Downtown_West/Assets/props/prop_table/SM_prop_dining_chair_b.SM_prop_dining_chair_b"),
			TEXT("/Game/Downtown_West/Assets/props/prop_table/SM_prop_dining_chair_a.SM_prop_dining_chair_a") }, true, true },
		{ ESprawlInteriorPropType::LoungeBench, TEXT("LoungeBench"), {
			TEXT("/Game/Downtown_West/Assets/props/prop_bench_wood/SM_bench_wood_a.SM_bench_wood_a"),
			TEXT("/Game/Downtown_West/Assets/props/prop_bench_wood/SM_bench_wood_b.SM_bench_wood_b") }, true, true },
		{ ESprawlInteriorPropType::ProductCan, TEXT("ProductCan"), {
			TEXT("/Game/Downtown_West/Assets/props/prop_food/SM_food_can_lemore.SM_food_can_lemore"),
			TEXT("/Game/Downtown_West/Assets/props/prop_food/SM_food_can_grape.SM_food_can_grape") }, false, false },
		{ ESprawlInteriorPropType::ProductBottle, TEXT("ProductBottle"), {
			TEXT("/Game/Downtown_West/Assets/props/prop_food/SM_bottle_seasaltsoda.SM_bottle_seasaltsoda"),
			TEXT("/Game/Downtown_West/Assets/props/prop_food/SM_food_bottle_sprout.SM_food_bottle_sprout") }, false, false },
		{ ESprawlInteriorPropType::Planter, TEXT("Planter"), {
			TEXT("/Game/Downtown_West/Assets/props/prop_pots/SM_pot_a.SM_pot_a"),
			TEXT("/Game/Downtown_West/Assets/props/prop_pots/SM_pot_b.SM_pot_b") }, false, true },
		{ ESprawlInteriorPropType::Foliage, TEXT("Foliage"), {
			TEXT("/Game/Downtown_West/Assets/foliage/shrub_evergreen/SM_evergreen_plant_shrub_evergreen_b.SM_evergreen_plant_shrub_evergreen_b"),
			TEXT("/Game/Downtown_West/Assets/foliage/shrub_evergreen/SM_evergreen_plant_shrub_evergreen_a.SM_evergreen_plant_shrub_evergreen_a") }, false, true },
		{ ESprawlInteriorPropType::RecycleBin, TEXT("RecycleBin"), {
			TEXT("/Game/Downtown_West/Assets/props/prop_recycle_bin/SM_prop_recycle_bin.SM_prop_recycle_bin") }, true, true },
		{ ESprawlInteriorPropType::PosterStand, TEXT("PosterStand"), {
			TEXT("/Game/Downtown_West/Assets/props/prop_poster_stands/SM_prop_poster_stand.SM_prop_poster_stand"),
			TEXT("/Game/Downtown_West/Assets/props/prop_poster_stands/SM_prop_poster_pillar_with_posters.SM_prop_poster_pillar_with_posters") }, true, true }
	};
	return Definitions;
}

const FSprawlInteriorPropDefinition* FSprawlInteriorPropLibrary::FindDefinition(
	ESprawlInteriorPropType Type)
{
	return GetDefinitions().FindByPredicate([Type](const auto& Definition)
	{
		return Definition.Type == Type;
	});
}

TArray<FSprawlInteriorPropPlacement>
	FSprawlInteriorPropLibrary::BuildForBuilding(
		ESprawlBuildingKind Kind, const FVector& InteriorCenter)
{
	TArray<FSprawlInteriorPropPlacement> Props;
	switch (Kind)
	{
	case ESprawlBuildingKind::Store: AddStoreProps(Props, InteriorCenter); break;
	case ESprawlBuildingKind::Office: AddOfficeProps(Props, InteriorCenter); break;
	case ESprawlBuildingKind::Condo: AddCondoProps(Props, InteriorCenter); break;
	}
	return Props;
}

int32 FSprawlInteriorPropLibrary::MinimumPropCount(ESprawlBuildingKind Kind)
{
	switch (Kind)
	{
	case ESprawlBuildingKind::Store: return 35;
	case ESprawlBuildingKind::Office: return 20;
	case ESprawlBuildingKind::Condo: return 15;
	}
	return TNumericLimits<int32>::Max();
}

bool FSprawlInteriorPropLibrary::IsValidForRoom(ESprawlBuildingKind Kind,
	const FVector& InteriorCenter, const FVector2D& InteriorHalfExtent,
	const TArray<FSprawlInteriorPropPlacement>& Placements)
{
	if (Placements.Num() < MinimumPropCount(Kind))
	{
		return false;
	}
	TSet<ESprawlInteriorPropType> Types;
	for (const FSprawlInteriorPropPlacement& Placement : Placements)
	{
		const FVector Location = Placement.Target.GetLocation();
		const FVector Size = Placement.GetDesiredSize();
		if (!FindDefinition(Placement.Type) || Size.GetMin() <= UE_SMALL_NUMBER
			|| FMath::Abs(Location.X - InteriorCenter.X)
				> InteriorHalfExtent.X - 40.f
			|| FMath::Abs(Location.Y - InteriorCenter.Y)
				> InteriorHalfExtent.Y - 40.f
			|| Location.Z < InteriorCenter.Z
			|| Location.Z > InteriorCenter.Z + 240.f)
		{
			return false;
		}
		// Preserve the central path between the entry and exit portals.
		if (FMath::Abs(Location.X - InteriorCenter.X) < 150.f
			&& Location.Y - InteriorCenter.Y < -100.f)
		{
			return false;
		}
		Types.Add(Placement.Type);
	}
	return Types.Num() >= 6;
}

UStaticMesh* FSprawlInteriorPropLibrary::ResolveMesh(
	ESprawlInteriorPropType Type, FString* OutResolvedPath)
{
	if (OutResolvedPath)
	{
		OutResolvedPath->Reset();
	}
	const FSprawlInteriorPropDefinition* Definition = FindDefinition(Type);
	if (!Definition)
	{
		return nullptr;
	}
	for (const FString& Path : Definition->AssetPaths)
	{
		if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path))
		{
			if (OutResolvedPath)
			{
				*OutResolvedPath = Path;
			}
			return Mesh;
		}
	}
	return nullptr;
}

FTransform FSprawlInteriorPropLibrary::FitMeshToTarget(
	const FTransform& Target, const UStaticMesh* Mesh)
{
	if (!Mesh)
	{
		return Target;
	}
	const FBoxSphereBounds Bounds = Mesh->GetBounds();
	const FVector MeshSize = Bounds.BoxExtent * 2.f;
	if (MeshSize.GetMin() <= UE_SMALL_NUMBER)
	{
		return Target;
	}
	const FVector Scale = Target.GetScale3D() * InteriorPropShapeMeshSize / MeshSize;
	FVector Location = Target.GetLocation();
	Location.Z -= (Bounds.Origin.Z - Bounds.BoxExtent.Z) * Scale.Z;
	return FTransform(Target.GetRotation(), Location, Scale);
}
