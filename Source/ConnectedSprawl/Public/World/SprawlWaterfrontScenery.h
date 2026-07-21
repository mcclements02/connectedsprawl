// The Connected Sprawl - Stable, mobile-lightweight lake and horizon scenery.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SprawlWaterfrontScenery.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UStaticMesh;
class UStaticMeshComponent;
class UWorld;

/** Deterministic world-space layout shared by runtime construction and tests. */
struct CONNECTEDSPRAWL_API FSprawlWaterfrontSceneryLayout
{
	FVector WaterCenter = FVector::ZeroVector;
	FVector2D WaterSize = FVector2D::ZeroVector;
	TArray<FTransform> ShoreTransforms;
	TArray<FTransform> MountainTransforms;
	TArray<FTransform> FoothillTransforms;

	static FSprawlWaterfrontSceneryLayout Build();
	bool IsValid() const;
};

/**
 * Runtime-only waterfront repair. It overlays the lake with GPU-animated,
 * translucent ocean water and replaces the broken marketplace vista with a
 * few instanced silhouette meshes. It owns no collision, tick, or map data.
 */
UCLASS(NotPlaceable, Transient)
class CONNECTEDSPRAWL_API ASprawlWaterfrontScenery : public AActor
{
	GENERATED_BODY()

public:
	ASprawlWaterfrontScenery();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** Spawn exactly one runtime repair actor for the active world. */
	static ASprawlWaterfrontScenery* EnsureForWorld(UWorld* World);

protected:
	UPROPERTY(VisibleAnywhere, Category="Waterfront")
	TObjectPtr<UStaticMeshComponent> WaterSurface;
	UPROPERTY(VisibleAnywhere, Category="Waterfront")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Shoreline;
	UPROPERTY(VisibleAnywhere, Category="Waterfront")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Mountains;
	UPROPERTY(VisibleAnywhere, Category="Waterfront")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Foothills;

	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> WaterMaterial;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> ShoreMaterial;
	UPROPERTY() TObjectPtr<UStaticMesh> LegacyMountainMesh;
	/** Project-owned translucent master used when MI_WaterPond's DatasmithContent parent is absent. */
	UPROPERTY() TObjectPtr<UMaterialInterface> SafeWaterFallback;

private:
	void RebuildGeometry();
	int32 HideLegacyMountainVistas();
	void BuildRuntimeMaterials();
};
