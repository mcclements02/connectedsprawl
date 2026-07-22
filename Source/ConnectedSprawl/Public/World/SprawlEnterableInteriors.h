// The Connected Sprawl - lightweight enterable city interiors and portals.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SprawlEnterableInteriors.generated.h"

class ACharacter;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UTextRenderComponent;
class UWorld;

UENUM(BlueprintType)
enum class ESprawlBuildingKind : uint8
{
	Store,
	Office,
	Condo
};

/** One exterior doorway paired with one furnished, enclosed interior room. */
USTRUCT(BlueprintType)
struct CONNECTEDSPRAWL_API FSprawlEnterableBuildingSpec
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Building") FName Id = NAME_None;
	UPROPERTY(BlueprintReadOnly, Category="Building") FText DisplayName;
	UPROPERTY(BlueprintReadOnly, Category="Building") ESprawlBuildingKind Kind = ESprawlBuildingKind::Store;
	UPROPERTY(BlueprintReadOnly, Category="Building") FVector ExteriorDoorLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category="Building") FVector ExteriorReturnLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category="Building") FRotator ExteriorFacing = FRotator::ZeroRotator;
	UPROPERTY(BlueprintReadOnly, Category="Building") FVector InteriorCenter = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category="Building") FVector InteriorEntryLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category="Building") FVector InteriorExitLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category="Building") FVector2D InteriorHalfExtent = FVector2D(1100.f, 800.f);
	UPROPERTY(BlueprintReadOnly, Category="Building") FLinearColor AccentColor = FLinearColor::White;

	TArray<FTransform> Structure;
	TArray<FTransform> Furniture;
	TArray<FTransform> TextilePieces;
	TArray<FTransform> DarkFixtures;
	TArray<FTransform> AccentPieces;
	TArray<FTransform> ExteriorPieces;

	FBox GetInteriorBounds() const;
	bool IsValid() const;
};

/** Pure interaction result shared by runtime input and deterministic tests. */
struct CONNECTEDSPRAWL_API FSprawlInteriorTransition
{
	FName BuildingId = NAME_None;
	FText BuildingName;
	FTransform Target = FTransform::Identity;
	bool bEntering = false;

	bool IsValid() const { return !BuildingId.IsNone() && !BuildingName.IsEmpty(); }
};

/** Asset-independent layout for all first-wave enterable destinations. */
struct CONNECTEDSPRAWL_API FSprawlEnterableInteriorsLayout
{
	static constexpr int32 ExpectedBuildingCount = 3;
	static constexpr float InteractionRadius = 430.f;

	TArray<FSprawlEnterableBuildingSpec> Buildings;

	static FSprawlEnterableInteriorsLayout Build();
	bool IsValid() const;
	const FSprawlEnterableBuildingSpec* FindInterior(const FVector& Location) const;
	bool FindTransition(const FVector& Location,
		FSprawlInteriorTransition& OutTransition) const;
	FVector ResolveMapLocation(const FVector& Location) const;
};

/**
 * Builds three mobile-budget interiors above the streamed city and adds a
 * non-colliding entrance treatment to existing facades. E/F interaction uses
 * paired portals, so authored buildings and sidewalks do not need destructive
 * edits and every interior remains save/load compatible in the persistent map.
 */
UCLASS(NotPlaceable, Transient)
class CONNECTEDSPRAWL_API ASprawlEnterableInteriors : public AActor
{
	GENERATED_BODY()

public:
	ASprawlEnterableInteriors();

	virtual void BeginPlay() override;

	static ASprawlEnterableInteriors* EnsureForWorld(UWorld* World);
	static ASprawlEnterableInteriors* FindForWorld(UWorld* World);

	/** Enter or leave the nearest portal. Returns false away from a doorway. */
	bool TryInteract(ACharacter* Character);

	/** Localized context text such as "Enter Junction Market". */
	bool GetInteractionPrompt(const AActor* Actor, FText& OutPrompt) const;

	const TArray<FSprawlEnterableBuildingSpec>& GetBuildings() const
	{
		return RuntimeLayout.Buildings;
	}

	FVector ResolveMapLocation(const FVector& Location) const
	{
		return RuntimeLayout.ResolveMapLocation(Location);
	}

	int32 GetBuiltInstanceCount() const;
	int32 GetBuiltRealPropCount() const { return BuiltRealPropCount; }
	int32 GetResolvedPropTypeCount() const
	{
		return RuntimePropComponents.Num();
	}

private:
	void BuildInteriors();
	void BuildRealProps();
	void BuildLabels();

	UPROPERTY(VisibleAnywhere, Category="Interiors")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Structure;
	UPROPERTY(VisibleAnywhere, Category="Interiors")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Furniture;
	UPROPERTY(VisibleAnywhere, Category="Interiors")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Textiles;
	UPROPERTY(VisibleAnywhere, Category="Interiors")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> DarkFixtures;
	UPROPERTY(VisibleAnywhere, Category="Interiors")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> StoreAccents;
	UPROPERTY(VisibleAnywhere, Category="Interiors")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> OfficeAccents;
	UPROPERTY(VisibleAnywhere, Category="Interiors")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> CondoAccents;

	UPROPERTY(Transient) TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> ConcreteMaterial;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> OpaqueBase;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> RuntimeMaterials;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextRenderComponent>> RuntimeLabels;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> RuntimePropComponents;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMesh>> RuntimePropMeshes;

	FSprawlEnterableInteriorsLayout RuntimeLayout;
	TMap<TWeakObjectPtr<AActor>, double> LastTransitionTimes;
	int32 BuiltRealPropCount = 0;
};
