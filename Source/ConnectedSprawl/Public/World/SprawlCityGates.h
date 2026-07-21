// The Connected Sprawl - City-limit gates where streets meet the boundary.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SprawlCityGates.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UWorld;

/** Deterministic gate layout shared by runtime construction and tests. */
struct CONNECTEDSPRAWL_API FSprawlCityGateLayout
{
	/** One transform per concrete piece (pillars, beams, flanking stubs). */
	TArray<FTransform> Pieces;
	int32 GateCount = 0;

	static FSprawlCityGateLayout Build();
	bool IsValid() const;
};

/**
 * Perimeter streets used to bleed straight onto the horizon apron, reading as
 * the literal edge of the world. Every crossing road now ends at a city-limit
 * gate — two concrete pillars carrying a beam over the carriageway, with low
 * flanking wall stubs tying into the skyline ring behind. Roads meeting the
 * lake's bay stay open water. Instanced cubes wearing the kit concrete;
 * no tick, no navigation, pillars block like street furniture.
 */
UCLASS(NotPlaceable, Transient)
class CONNECTEDSPRAWL_API ASprawlCityGates : public AActor
{
	GENERATED_BODY()

public:
	ASprawlCityGates();

	virtual void BeginPlay() override;

	/** Spawn exactly one gates actor for the active world. */
	static ASprawlCityGates* EnsureForWorld(UWorld* World);

private:
	UPROPERTY(VisibleAnywhere, Category="Gates")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Pieces;

	UPROPERTY(Transient) TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> ConcreteMaterial;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> OpaqueBase;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> FallbackMaterial;
};
