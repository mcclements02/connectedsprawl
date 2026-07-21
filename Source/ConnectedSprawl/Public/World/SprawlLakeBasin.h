// The Connected Sprawl - Sinks the lake into a basin below the ground.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SprawlLakeBasin.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMesh;
class UStaticMeshComponent;
class UWorld;

/**
 * The city floor `City_Ground` is one opaque plane at Z0 that also spans the
 * lake, so any lowered water would be occluded by it and the lake looks flush
 * with the street. This actor makes the lake a real dip:
 *   - hides `City_Ground`'s visual (its collision stays as an invisible floor),
 *   - rebuilds the visible city ground as a flat ring around the lake, and
 *   - drops a walled basin (quay sides + bed) beneath the lowered water surface.
 * The water level is read from the waterfront layout so the two stay in sync.
 * Spawned once at runtime; no tick.
 */
UCLASS(NotPlaceable, Transient)
class CONNECTEDSPRAWL_API ASprawlLakeBasin : public AActor
{
	GENERATED_BODY()

public:
	ASprawlLakeBasin();

	virtual void BeginPlay() override;

	/** Spawn exactly one basin actor for the active world. */
	static ASprawlLakeBasin* EnsureForWorld(UWorld* World);

private:
	/** Hide the wide flat city-ground plane's visual (collision is kept). */
	int32 HideCityGroundPlane();
	/** Hide small props left floating over the lake once the water is lowered. */
	int32 HideFloatingLakeProps();
	/** Build the ground ring, basin walls, and bed around/under the lake. */
	void BuildBasin();
	/** Spawn one box (size in cm) attached to this actor. A textured material
	 *  wins when supplied; otherwise a tinted MID of the opaque master. */
	UStaticMeshComponent* AddBox(const FString& Name, const FVector& Center,
		const FVector& SizeCm, const FLinearColor& Color, float Roughness,
		bool bCollide, UMaterialInterface* Textured = nullptr);

	UPROPERTY(Transient) TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> OpaqueBase;
	/** Blender-baked worn asphalt for the ground ring (world-projected). */
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> AsphaltMaterial;
	/** Blender-baked jointed concrete for the quay walls. */
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> ConcreteMaterial;
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;
};
