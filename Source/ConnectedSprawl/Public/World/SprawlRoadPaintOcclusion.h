// The Connected Sprawl - Road paint must obey scene depth under vehicles.

#pragma once

#include "CoreMinimal.h"

class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;

/** Runtime guard against translucent/custom-depth road paint rendering. */
class CONNECTEDSPRAWL_API FSprawlRoadPaintOcclusion
{
public:
	static bool IsDepthWritingMaterial(const UMaterialInterface* Material);
	static UMaterialInterface* SelectMaterial(
		UMaterialInterface* Preferred, UMaterialInterface* OpaqueFallback);
	static void Configure(UHierarchicalInstancedStaticMeshComponent* PaintMesh);
};
