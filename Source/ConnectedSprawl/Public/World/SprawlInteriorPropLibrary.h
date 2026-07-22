// The Connected Sprawl - reusable real-mesh furniture and interior-item catalog.

#pragma once

#include "CoreMinimal.h"

class UStaticMesh;
enum class ESprawlBuildingKind : uint8;

/** Real prop silhouettes shared by the store, office, and condo. */
enum class ESprawlInteriorPropType : uint8
{
	DisplayTable,
	ChairA,
	ChairB,
	LoungeBench,
	ProductCan,
	ProductBottle,
	Planter,
	Foliage,
	RecycleBin,
	PosterStand
};

/** One loadable mesh definition. The first available path wins. */
struct CONNECTEDSPRAWL_API FSprawlInteriorPropDefinition
{
	ESprawlInteriorPropType Type = ESprawlInteriorPropType::DisplayTable;
	FName Id = NAME_None;
	TArray<FString> AssetPaths;
	bool bCollision = false;
	bool bCastShadow = false;

	bool IsValid() const;
};

/** A desired world-space bounding box whose base rests on Target.Location.Z. */
struct CONNECTEDSPRAWL_API FSprawlInteriorPropPlacement
{
	ESprawlInteriorPropType Type = ESprawlInteriorPropType::DisplayTable;
	FTransform Target = FTransform::Identity;

	FVector GetDesiredSize() const
	{
		return Target.GetScale3D() * 100.f;
	}
};

/**
 * Pure layout and asset-resolution module for realistic interior props.
 * Marketplace meshes remain optional: deterministic modular fixtures still
 * furnish each room if no candidate mesh can be loaded.
 */
class CONNECTEDSPRAWL_API FSprawlInteriorPropLibrary
{
public:
	static const TArray<FSprawlInteriorPropDefinition>& GetDefinitions();
	static const FSprawlInteriorPropDefinition* FindDefinition(
		ESprawlInteriorPropType Type);
	static TArray<FSprawlInteriorPropPlacement> BuildForBuilding(
		ESprawlBuildingKind Kind, const FVector& InteriorCenter);
	static bool IsValidForRoom(ESprawlBuildingKind Kind,
		const FVector& InteriorCenter, const FVector2D& InteriorHalfExtent,
		const TArray<FSprawlInteriorPropPlacement>& Placements);
	static int32 MinimumPropCount(ESprawlBuildingKind Kind);

	/** Loads the first installed candidate without changing the catalog. */
	static UStaticMesh* ResolveMesh(ESprawlInteriorPropType Type,
		FString* OutResolvedPath = nullptr);

	/** Fits an authored mesh to a target size and seats its lowest point. */
	static FTransform FitMeshToTarget(const FTransform& Target,
		const UStaticMesh* Mesh);
};
