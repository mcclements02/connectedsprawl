// The Connected Sprawl - Procedural road markings: dashed centerlines,
// crosswalks, and stop lines, generated from the city grid as instanced
// meshes so the whole street network costs a few draw calls.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SprawlRoadMarkings.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UBoxComponent;

/**
 * ASprawlRoadMarkings
 * -------------------
 * Drop one into the level; on BeginPlay it paints the entire grid:
 *   - dashed lane-divider lines down every road (broken at intersections),
 *   - zebra crosswalks across all four approaches of every intersection,
 *   - stop lines for the approach lane at each signal.
 * Everything reads from USprawlCityGridSubsystem, so markings always agree
 * with where the AI actually drives.
 */
UCLASS()
class CONNECTEDSPRAWL_API ASprawlRoadMarkings : public AActor
{
	GENERATED_BODY()

public:
	ASprawlRoadMarkings();

	virtual void BeginPlay() override;

	/** Dash length / gap spacing along the centerline (cm). */
	UPROPERTY(EditAnywhere, Category="Markings") float DashLength = 190.f;
	UPROPERTY(EditAnywhere, Category="Markings") float DashSpacing = 520.f;

protected:
	UPROPERTY(VisibleAnywhere, Category="Markings")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> PaintMesh;

	/** Invisible collision walls keep vehicles inside the authored city. */
	UPROPERTY(VisibleAnywhere, Category="Boundaries") TObjectPtr<UBoxComponent> NorthBoundary;
	UPROPERTY(VisibleAnywhere, Category="Boundaries") TObjectPtr<UBoxComponent> SouthBoundary;
	UPROPERTY(VisibleAnywhere, Category="Boundaries") TObjectPtr<UBoxComponent> EastBoundary;
	UPROPERTY(VisibleAnywhere, Category="Boundaries") TObjectPtr<UBoxComponent> WestBoundary;

	void BuildMarkings();
	void AddStripe(const FVector& Center, float LengthX, float LengthY);
	UBoxComponent* CreateBoundary(const TCHAR* Name, const FVector& Location,
		const FVector& HalfExtent);
};
