// The Connected Sprawl - Authored modular streetwear assembly for Zarri.

#pragma once

#include "Characters/SprawlWardrobeModule.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"
#include "SprawlStreetwearModule.generated.h"

class AActor;
class UMeshComponent;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** Visual layer used to choose palette and mobile presentation policy. */
UENUM(BlueprintType)
enum class ESprawlStreetwearLayer : uint8
{
	Hoodie,
	Bomber,
	Cargo,
	Beanie,
	Footwear
};

/** Live MetaHuman bone that drives one rigid modular garment piece. */
UENUM(BlueprintType)
enum class ESprawlStreetwearAnchor : uint8
{
	Spine,
	Pelvis,
	Head,
	UpperArmLeft,
	UpperArmRight,
	LowerArmLeft,
	LowerArmRight,
	ThighLeft,
	ThighRight,
	CalfLeft,
	CalfRight,
	FootLeft,
	FootRight
};

/** One imported mesh and its position in the authored 1.84 m body space. */
USTRUCT(BlueprintType)
struct CONNECTEDSPRAWL_API FSprawlStreetwearPieceDefinition
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Streetwear")
	FName PieceId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Streetwear")
	FSoftObjectPath MeshPath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Streetwear")
	ESprawlStreetwearLayer Layer = ESprawlStreetwearLayer::Hoodie;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Streetwear")
	ESprawlStreetwearAnchor Anchor = ESprawlStreetwearAnchor::Spine;

	/** Bounds centre in the original Blender body space, expressed in cm. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Streetwear")
	FVector AuthoredCenterCm = FVector::ZeroVector;

	/** Small layer separation; the bomber is slightly roomier than the hoodie. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Streetwear")
	float UniformScaleMultiplier = 1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Streetwear")
	bool bRequired = true;
};

/** Complete authored outfit catalog. */
USTRUCT(BlueprintType)
struct CONNECTEDSPRAWL_API FSprawlStreetwearKit
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Streetwear")
	FName KitId = NAME_None;

	/** Standing-height reference used to fit the source meshes to a live body. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Streetwear")
	float AuthoredHeightCm = 184.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Streetwear")
	TArray<FSprawlStreetwearPieceDefinition> Pieces;
};

/**
 * Loads project-owned streetwear meshes and assembles them on the live Zarri
 * MetaHuman. Each rigid piece follows the nearest useful bone, which keeps the
 * imported art attached during locomotion without adding per-frame component
 * ticks. The original default garments are hidden only after the complete kit
 * has loaded successfully.
 */
UCLASS(ClassGroup=(Sprawl), meta=(BlueprintSpawnableComponent))
class CONNECTEDSPRAWL_API USprawlStreetwearModule : public UActorComponent
{
	GENERATED_BODY()

public:
	USprawlStreetwearModule();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Streetwear")
	static FSprawlStreetwearKit CreateZarriNanobananaKit();

	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Streetwear")
	static bool ValidateKit(const FSprawlStreetwearKit& Kit, FString& OutError);

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Streetwear")
	static FString DescribeKit(const FSprawlStreetwearKit& Kit);

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Streetwear")
	static bool SupportsOutfit(const FSprawlWardrobeOutfit& Outfit);

	/** Equip the authored kit and hide the assembled MetaHuman defaults. */
	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Streetwear")
	bool ApplyToMetaHuman(
		AActor* VisualActor,
		USkeletalMeshComponent* BodyMesh,
		const FSprawlWardrobeOutfit& Outfit);

	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Streetwear")
	void ClearStreetwear();

	/** Show or hide one authored rigid layer without disturbing the others. */
	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Streetwear")
	void SetLayerVisibility(ESprawlStreetwearLayer Layer, bool bVisible);

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Streetwear")
	int32 GetPieceCount() const { return SpawnedPieces.Num(); }

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Streetwear")
	int32 GetMissingRequiredPieceCount() const { return MissingRequiredPieceCount; }

private:
	UStaticMeshComponent* SpawnPiece(
		AActor* VisualActor,
		USkeletalMeshComponent* BodyMesh,
		UStaticMesh* Mesh,
		const FSprawlStreetwearPieceDefinition& Piece,
		FName AnchorBone,
		const FVector& OutfitOrigin,
		const FQuat& OutfitRotation,
		float BodyScale,
		const FSprawlWardrobeOutfit& Outfit);

	void HideBaseGarments(AActor* VisualActor);
	void HideBareFeet(USkeletalMeshComponent* BodyMesh);
	void RefreshLiveSegmentFits();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> SpawnedPieces;

	TArray<ESprawlStreetwearLayer> SpawnedPieceLayers;

	TArray<TWeakObjectPtr<UMeshComponent>> HiddenBaseGarments;
	TWeakObjectPtr<USkeletalMeshComponent> HiddenFootBodyMesh;
	TArray<FName> HiddenFootBones;
	TWeakObjectPtr<USkeletalMeshComponent> LiveFitBodyMesh;
	TArray<TWeakObjectPtr<UStaticMeshComponent>> LiveFitComponents;
	TArray<FSprawlStreetwearPieceDefinition> LiveFitDefinitions;
	float LiveFitBodyScale = 1.f;
	int32 MissingRequiredPieceCount = 0;
};
