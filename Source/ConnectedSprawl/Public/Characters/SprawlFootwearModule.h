// The Connected Sprawl - Anatomically fitted MetaHuman footwear presentation.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "SprawlFootwearModule.generated.h"

class AActor;
class UMeshComponent;
class UProceduralMeshComponent;
class USkeletalMeshComponent;

UENUM(BlueprintType)
enum class ESprawlWardrobeFootwear : uint8
{
	LowTopSneakers,
	HighTopSneakers,
	DressShoes,
	WorkBoots,
	RunningShoes,
	AthleticTrainers
};

UENUM(BlueprintType)
enum class ESprawlWardrobeSocks : uint8
{
	Ankle,
	Crew,
	Dress
};

/** Real-world dimensions developed from the live heel-to-ball bone distance. */
USTRUCT(BlueprintType)
struct CONNECTEDSPRAWL_API FSprawlFootwearDimensions
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Footwear")
	float ShoeLengthCm = 26.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Footwear")
	float ShoeWidthCm = 9.8f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Footwear")
	float UpperHeightCm = 8.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Footwear")
	float SoleThicknessCm = 2.2f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Footwear")
	float SockLengthCm = 14.f;
};

/**
 * Builds and attaches fitted footwear from the MetaHuman foot, ball, and calf
 * bones. Shoes use an independent animated IK-foot or calf anchor, so the Body
 * foot render bones can be hidden after a complete replacement pair exists
 * without freezing or collapsing the attached footwear.
 */
UCLASS(ClassGroup=(Sprawl), meta=(BlueprintSpawnableComponent))
class CONNECTEDSPRAWL_API USprawlFootwearModule : public UActorComponent
{
	GENERATED_BODY()

public:
	USprawlFootwearModule();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Scale a footwear style from a live character's heel-to-ball distance. */
	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Footwear")
	static FSprawlFootwearDimensions DevelopDimensions(
		ESprawlWardrobeFootwear Footwear,
		ESprawlWardrobeSocks Socks,
		float HeelToBallDistanceCm);

	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Footwear")
	static bool ValidateDimensions(
		const FSprawlFootwearDimensions& Dimensions,
		FString& OutError);

	/**
	 * Resolve a scale-safe presentation transform from an animated skeletal
	 * anchor. Useful for custom footwear that performs its own bone following.
	 */
	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Footwear")
	static FTransform ResolveBoneFollowTransform(
		const FTransform& BoneWorldTransform,
		const FTransform& PresentationRelativeTransform);

	/**
	 * Follow an animated lower-leg/IK anchor for position while retaining the
	 * shoe's body-relative facing, avoiding the vertical-shoe artifact that a
	 * rigid calf rotation creates during a run cycle.
	 */
	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Footwear")
	static FTransform ResolveAnimatedShoeTransform(
		const FTransform& AnchorWorldTransform,
		const FTransform& ShoeAnchorRelativeTransform,
		const FTransform& BodyWorldTransform,
		const FTransform& ShoeBodyRelativeFacing);

	/**
	 * Pick the orientation source for a fitted shoe. An IK-foot anchor already
	 * carries real ankle roll and toe-off, so the shoe inherits it and stays on
	 * the foot through a stride; a calf anchor points up the leg and would
	 * stand the shoe vertical, so that case keeps body-relative facing.
	 */
	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Footwear")
	static FTransform ResolveFittedShoeTransform(
		const FTransform& AnchorWorldTransform,
		const FTransform& ShoeAnchorRelativeTransform,
		const FTransform& BodyWorldTransform,
		const FTransform& ShoeBodyRelativeFacing,
		bool bAnchorDrivesRotation);

	/** Replace the current pair with fitted shoes and independently bound socks. */
	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Footwear")
	bool ApplyToMetaHuman(
		AActor* VisualActor,
		USkeletalMeshComponent* BodyMesh,
		ESprawlWardrobeFootwear Footwear,
		ESprawlWardrobeSocks Socks,
		const FLinearColor& ShoeColor,
		const FLinearColor& SockColor,
		const FLinearColor& AccentColor);

	/** Replace only the bound shoes/socks, retaining the current character. */
	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Footwear")
	bool SwapFootwear(
		ESprawlWardrobeFootwear Footwear,
		ESprawlWardrobeSocks Socks,
		const FLinearColor& ShoeColor,
		const FLinearColor& SockColor,
		const FLinearColor& AccentColor);

	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Footwear")
	void ClearFootwear();

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Footwear")
	int32 GetPresentationPieceCount() const { return SpawnedPieces.Num(); }

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Footwear")
	bool HasFittedPair() const { return FittedShoeCount == 2; }

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Footwear")
	bool IsFollowingAnimatedFeet() const;

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Footwear")
	ESprawlWardrobeFootwear GetCurrentFootwear() const
	{
		return CurrentFootwear;
	}

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> SpawnedPieces;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> FittedShoes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMeshComponent>> HiddenFeetMeshes;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> FootwearBodyMesh;

	UPROPERTY(Transient)
	TObjectPtr<AActor> FootwearVisualActor;

	UPROPERTY(Transient)
	TArray<FName> HiddenFootBones;

	TArray<FName> ShoeAnchorBones;
	TArray<FTransform> ShoeAnchorOffsets;
	TArray<FTransform> ShoeBodyFacingOffsets;
	TArray<bool> ShoeAnchorDrivesRotation;

	int32 FittedShoeCount = 0;
	ESprawlWardrobeFootwear CurrentFootwear =
		ESprawlWardrobeFootwear::LowTopSneakers;

	void UpdateShoeFollowers();

	UProceduralMeshComponent* CreatePresentationComponent(
		AActor* VisualActor,
		USkeletalMeshComponent* BodyMesh,
		FName BoneName,
		const FTransform& WorldTransform,
		FName ComponentName);
};
