// The Connected Sprawl - Instanced ground cover: grass and wildflowers for
// the park blocks, so green space reads as vegetation instead of paint.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SprawlGroundCover.generated.h"

class UHierarchicalInstancedStaticMeshComponent;

/**
 * ASprawlGroundCover
 * ------------------
 * One actor covers the whole city. On BeginPlay it scatters thousands of
 * instanced grass blades (plus a sprinkle of flowers) across the park
 * blocks. Everything is HISM, so the GPU treats an entire meadow as a
 * handful of draw calls, and instances LOD/cull away with distance.
 */
UCLASS()
class CONNECTEDSPRAWL_API ASprawlGroundCover : public AActor
{
	GENERATED_BODY()

public:
	ASprawlGroundCover();

	virtual void BeginPlay() override;

	/** Park blocks (grid coords) to plant. Matches build_city.py's PARKS. */
	UPROPERTY(EditAnywhere, Category="GroundCover")
	TArray<FIntPoint> ParkBlocks = { FIntPoint(1, 5), FIntPoint(2, 2), FIntPoint(5, 5) };

	/** Grass blades per park block. */
	UPROPERTY(EditAnywhere, Category="GroundCover") int32 GrassPerPark = 1400;

	/** Flowers per park block. */
	UPROPERTY(EditAnywhere, Category="GroundCover") int32 FlowersPerPark = 26;

	/** Beyond this distance instances cull entirely (cm). */
	UPROPERTY(EditAnywhere, Category="GroundCover") float CullDistance = 14000.f;

protected:
	UPROPERTY(VisibleAnywhere, Category="GroundCover")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> GrassMesh;

	UPROPERTY(VisibleAnywhere, Category="GroundCover")
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> FlowerMeshes;

	void PlantParks();
};
