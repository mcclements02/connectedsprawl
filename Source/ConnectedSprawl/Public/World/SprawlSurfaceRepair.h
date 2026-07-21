// The Connected Sprawl - Runtime repair of surfaces that fell back to the
// engine checker (WorldGridMaterial) or the default material.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SprawlSurfaceRepair.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UWorld;

/**
 * Some authored ground materials chain up to a parent that is not shipped with
 * this project (RealKit / DatasmithContent / marketplace masters). When that
 * parent fails to resolve at runtime the whole slot collapses to the engine
 * checker (`WorldGridMaterial`) or the default material — the tan diamond grid
 * seen on the bare ground plane where `city_surfaces.py` leaves the lake-edge
 * blocks unslabbed.
 *
 * This actor scans the live world once at BeginPlay and rebinds every such
 * broken slot to a project-owned master (`M_Simple_Opaque`), tinted by role:
 * lake-adjacent surfaces read as a waterfront promenade, everything else as
 * street asphalt. It owns no collision, tick, or map data, so no cook or
 * missing dependency can ever bring the checker back.
 */
UCLASS(NotPlaceable, Transient)
class CONNECTEDSPRAWL_API ASprawlSurfaceRepair : public AActor
{
	GENERATED_BODY()

public:
	ASprawlSurfaceRepair();

	virtual void BeginPlay() override;

	/** Spawn exactly one runtime repair actor for the active world. */
	static ASprawlSurfaceRepair* EnsureForWorld(UWorld* World);

	/** True when the material resolves to the engine checker (WorldGridMaterial). */
	static bool IsCheckerMaterial(const UMaterialInterface* Material);

	/** True when the material is missing or resolves to the surface default. */
	static bool IsMissingMaterial(const UMaterialInterface* Material);

private:
	int32 RepairWorldSurfaces();
	UMaterialInstanceDynamic* MakeRepairMaterial(const FLinearColor& Color,
		float Roughness);

	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> OpaqueBase;
	/** Blender-baked worn street asphalt (world-projected); preferred repair. */
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> AsphaltMaterial;
	/** Blender-baked jointed concrete for the waterfront promenade band. */
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> ConcreteMaterial;
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> RepairMaterials;
};
