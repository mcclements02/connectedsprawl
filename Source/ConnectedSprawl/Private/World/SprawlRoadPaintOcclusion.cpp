// The Connected Sprawl - Road paint must obey scene depth under vehicles.

#include "World/SprawlRoadPaintOcclusion.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"

bool FSprawlRoadPaintOcclusion::IsDepthWritingMaterial(
	const UMaterialInterface* Material)
{
	if (!Material)
	{
		return false;
	}
	const EBlendMode BlendMode = Material->GetBlendMode();
	return BlendMode == BLEND_Opaque || BlendMode == BLEND_Masked;
}

UMaterialInterface* FSprawlRoadPaintOcclusion::SelectMaterial(
	UMaterialInterface* Preferred, UMaterialInterface* OpaqueFallback)
{
	if (IsDepthWritingMaterial(Preferred))
	{
		return Preferred;
	}
	return IsDepthWritingMaterial(OpaqueFallback) ? OpaqueFallback : nullptr;
}

void FSprawlRoadPaintOcclusion::Configure(
	UHierarchicalInstancedStaticMeshComponent* PaintMesh)
{
	if (!PaintMesh)
	{
		return;
	}
	PaintMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PaintMesh->SetCastShadow(false);
	PaintMesh->SetRenderInMainPass(true);
	PaintMesh->SetRenderCustomDepth(false);
	PaintMesh->SetReceivesDecals(false);
}
