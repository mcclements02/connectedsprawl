// The Connected Sprawl - Procedural parking deck and traffic egress source.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/SprawlCityGridSubsystem.h"
#include "SprawlParkingGarage.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UWorld;

/** One covered driveway from the deck interior to a legal directed lane. */
struct CONNECTEDSPRAWL_API FSprawlParkingGarageExit
{
	FName Id = NAME_None;
	FTransform SpawnTransform = FTransform::Identity;
	TArray<FVector> DeparturePath;
	FTransform MergeTransform = FTransform::Identity;
	ESprawlHeading MergeHeading = ESprawlHeading::East;
	int32 RoadIndex = INDEX_NONE;

	/** Geometry and directed-lane contract used by runtime spawning and tests. */
	bool IsValid() const;
};

/** Deterministic low-poly deck layout, independent of the active world. */
struct CONNECTEDSPRAWL_API FSprawlParkingGarageLayout
{
	static constexpr int32 GarageBlockX = 3;
	static constexpr int32 GarageBlockY = 3;
	static constexpr int32 ExpectedDeckCount = 4;
	static constexpr float SiteHalfExtent = 900.f;

	FBox2D SiteBounds = FBox2D(ForceInit);
	TArray<FTransform> Structure;
	TArray<FTransform> Decks;
	TArray<FTransform> Ramps;
	TArray<FTransform> Rails;
	TArray<FTransform> Markings;
	TArray<FTransform> ParkedVehicles;
	TArray<FTransform> Signs;
	TArray<FTransform> Lights;
	TArray<FSprawlParkingGarageExit> Exits;
	int32 DeckCount = 0;

	static FVector2D GarageCenter();
	static FSprawlParkingGarageLayout Build();
	bool IsValid() const;
};

/**
 * Four-level central parking deck that is also the only runtime traffic source.
 * Elevated authored lot geometry is cleared from its footprint and narrow exit
 * corridors, while ground-level city surfaces remain intact. Cars begin behind
 * the facade and follow one of two covered driveways before normal lane AI takes
 * over.
 */
UCLASS(NotPlaceable, Transient)
class CONNECTEDSPRAWL_API ASprawlParkingGarage : public AActor
{
	GENERATED_BODY()

public:
	ASprawlParkingGarage();

	virtual void BeginPlay() override;

	/** Spawn exactly one parking deck for the active world. */
	static ASprawlParkingGarage* EnsureForWorld(UWorld* World);

	const TArray<FSprawlParkingGarageExit>& GetVehicleExits() const
	{
		return VehicleExits;
	}

	bool IsReadyForTraffic() const { return VehicleExits.Num() > 0; }

private:
	int32 ClearAuthoredSite(const FSprawlParkingGarageLayout& Layout);
	int32 RelocateDrivewayCars(const FSprawlParkingGarageLayout& Layout);
	void BuildGarage(const FSprawlParkingGarageLayout& Layout);

	UPROPERTY(VisibleAnywhere, Category="Parking Garage")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Structure;
	UPROPERTY(VisibleAnywhere, Category="Parking Garage")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Decks;
	UPROPERTY(VisibleAnywhere, Category="Parking Garage")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Ramps;
	UPROPERTY(VisibleAnywhere, Category="Parking Garage")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Rails;
	UPROPERTY(VisibleAnywhere, Category="Parking Garage")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Markings;
	UPROPERTY(VisibleAnywhere, Category="Parking Garage")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> ParkedVehicles;
	UPROPERTY(VisibleAnywhere, Category="Parking Garage")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Signs;
	UPROPERTY(VisibleAnywhere, Category="Parking Garage")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Lights;

	UPROPERTY(Transient) TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> ConcreteMaterial;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> AsphaltMaterial;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> OpaqueBase;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> RuntimeMaterials;
	TArray<FSprawlParkingGarageExit> VehicleExits;
};
