// The Connected Sprawl - Reference-fitted shirt and pants assembly.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"
#include "SprawlReferenceClothingModule.generated.h"

class AActor;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ESprawlReferenceClothingLayer : uint8
{
	Shirt,
	Pants
};

UENUM(BlueprintType)
enum class ESprawlReferenceClothingAnchor : uint8
{
	Spine,
	Pelvis,
	UpperArmLeft,
	UpperArmRight,
	LowerArmLeft,
	LowerArmRight,
	HandLeft,
	HandRight,
	ThighLeft,
	ThighRight,
	CalfLeft,
	CalfRight
};

USTRUCT(BlueprintType)
struct CONNECTEDSPRAWL_API FSprawlReferenceClothingPiece
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reference Clothing")
	FName PieceId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reference Clothing")
	FSoftObjectPath MeshPath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reference Clothing")
	ESprawlReferenceClothingLayer Layer = ESprawlReferenceClothingLayer::Shirt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reference Clothing")
	ESprawlReferenceClothingAnchor Anchor = ESprawlReferenceClothingAnchor::Spine;

	/** Bounds center in the Blender-authored 1.84 m body space, in centimeters. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reference Clothing")
	FVector AuthoredCenterCm = FVector::ZeroVector;

	/** True for sections that stretch and rotate between two live body bones. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reference Clothing")
	bool bFollowBoneSegment = false;

	/** Direction of a follower in the Blender-authored coordinate system. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reference Clothing")
	FVector AuthoredDirection = FVector::UpVector;

	/** Centerline length of a follower before Unreal scales it to live bones. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reference Clothing")
	float AuthoredLengthCm = 0.f;

	/** Fractions of the selected bone segment occupied by this mesh. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reference Clothing")
	float SegmentStartFraction = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reference Clothing")
	float SegmentEndFraction = 1.f;

	/** Forward/right offset from the live bone center for asymmetric body volume. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reference Clothing")
	FVector2D FollowerOffsetCm = FVector2D::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reference Clothing")
	bool bRequired = true;
};

USTRUCT(BlueprintType)
struct CONNECTEDSPRAWL_API FSprawlReferenceClothingKit
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reference Clothing")
	FName KitId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reference Clothing")
	float AuthoredHeightCm = 184.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Reference Clothing")
	TArray<FSprawlReferenceClothingPiece> Pieces;
};

/**
 * Maps Blender-authored shirt and pants sections to Zarri's live MetaHuman.
 * Torso/waist details ride spine and pelvis anchors; smooth limb shells are
 * refit between upper/lower arm, hand, thigh, and calf joints after animation.
 */
UCLASS(ClassGroup=(Sprawl), meta=(BlueprintSpawnableComponent))
class CONNECTEDSPRAWL_API USprawlReferenceClothingModule : public UActorComponent
{
	GENERATED_BODY()

public:
	USprawlReferenceClothingModule();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Reference Clothing")
	static FSprawlReferenceClothingKit CreateZarriReferenceKit();

	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Reference Clothing")
	static bool ValidateKit(
		const FSprawlReferenceClothingKit& Kit, FString& OutError);

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Reference Clothing")
	static FString DescribeKit(const FSprawlReferenceClothingKit& Kit);

	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Reference Clothing")
	bool ApplyToMetaHuman(AActor* VisualActor, USkeletalMeshComponent* BodyMesh);

	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Reference Clothing")
	void ClearClothing();

	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Reference Clothing")
	void SetLayerVisibility(
		ESprawlReferenceClothingLayer Layer, bool bVisible);

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Reference Clothing")
	int32 GetPieceCount() const { return SpawnedPieces.Num(); }

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Reference Clothing")
	int32 GetMissingRequiredPieceCount() const
	{
		return MissingRequiredPieceCount;
	}

private:
	UStaticMeshComponent* SpawnPiece(
		AActor* VisualActor,
		USkeletalMeshComponent* BodyMesh,
		UStaticMesh* Mesh,
		const FSprawlReferenceClothingPiece& Piece,
		FName AnchorBone,
		const FVector& OutfitOrigin,
		const FQuat& OutfitRotation,
		float BodyScale);

	void RefreshLiveSegmentFits();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> SpawnedPieces;

	TArray<ESprawlReferenceClothingLayer> SpawnedLayers;
	TWeakObjectPtr<USkeletalMeshComponent> LiveBodyMesh;
	TArray<TWeakObjectPtr<UStaticMeshComponent>> LiveFitComponents;
	TArray<FSprawlReferenceClothingPiece> LiveFitDefinitions;
	float LiveBodyScale = 1.f;
	int32 MissingRequiredPieceCount = 0;
};
