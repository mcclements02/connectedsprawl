// The Connected Sprawl - Low-poly skyline ring beyond the playable city.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SprawlCitySkyline.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
class UWorld;

/** Deterministic skyline layout shared by runtime construction and tests. */
struct CONNECTEDSPRAWL_API FSprawlSkylineLayout
{
	static constexpr int32 NumFacadeStyles = 4;

	TArray<FTransform> BuildingTransforms;
	/** Parallel to BuildingTransforms; selects one of the facade styles. */
	TArray<int32> FacadeIndices;
	TArray<FTransform> RoofTransforms;
	/** Dark rooftop-mechanical blocks on tall towers (renders with roofs). */
	TArray<FTransform> CrownTransforms;

	static FSprawlSkylineLayout Build();
	bool IsValid() const;
};

/**
 * Standing at the city perimeter previously showed a bare plane meeting the
 * sky — the literal edge of the world. This actor rings the city with two
 * rows of low-poly proxy buildings (per the GDD: silhouette proxies beyond
 * the streaming radius): instanced cubes wearing the same world-projected
 * facade materials as the real city, with flat dark roof caps, taller in the
 * back row, and a gap along the lake's south face so the bay still opens to
 * the mountain horizon. No tick, no collision, no navigation, no shadows.
 */
UCLASS(NotPlaceable, Transient)
class CONNECTEDSPRAWL_API ASprawlCitySkyline : public AActor
{
	GENERATED_BODY()

public:
	ASprawlCitySkyline();

	virtual void BeginPlay() override;

	/** Spawn exactly one skyline actor for the active world. */
	static ASprawlCitySkyline* EnsureForWorld(UWorld* World);

private:
	void BuildRing();
	/** Ground band from the city floor's edge out past both proxy rows, so
	 *  the towers stand on land instead of floating over sky-fog void. */
	void BuildApron();

	UPROPERTY(VisibleAnywhere, Category="Skyline")
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> FacadeMeshes;
	UPROPERTY(VisibleAnywhere, Category="Skyline")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Roofs;
	UPROPERTY(VisibleAnywhere, Category="Skyline")
	TArray<TObjectPtr<UStaticMeshComponent>> ApronSlabs;

	UPROPERTY(Transient) TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> OpaqueBase;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> ApronAsphalt;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> RoofMaterial;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> ApronMaterial;
};
