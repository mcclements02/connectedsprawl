// The Connected Sprawl - Deterministic, reusable MetaHuman wardrobe layers.

#pragma once

#include "Characters/SprawlAthleticShoeModule.h"
#include "Characters/SprawlCharacterDeveloper.h"
#include "Characters/SprawlFootwearModule.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "SprawlWardrobeModule.generated.h"

class AActor;
class UMeshComponent;
class UProceduralMeshComponent;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;
class USprawlFootwearModule;

UENUM(BlueprintType)
enum class ESprawlWardrobeTop : uint8
{
	Tee,
	Polo,
	Knit,
	ButtonDown,
	Hoodie
};

UENUM(BlueprintType)
enum class ESprawlWardrobeBottom : uint8
{
	TailoredShorts,
	Jeans,
	Trousers,
	Cargo,
	Joggers
};

UENUM(BlueprintType)
enum class ESprawlWardrobeOuterwear : uint8
{
	None,
	UtilityJacket,
	BomberJacket,
	Blazer,
	TrackJacket
};

UENUM(BlueprintType)
enum class ESprawlWardrobeHeadwear : uint8
{
	None,
	BaseballCap,
	Beanie
};

/** Complete, replication-safe outfit direction resolved from a broad style. */
USTRUCT(BlueprintType)
struct CONNECTEDSPRAWL_API FSprawlWardrobeOutfit
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wardrobe")
	FName OutfitId = TEXT("Street_00000000");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wardrobe|Layers")
	ESprawlWardrobeTop Top = ESprawlWardrobeTop::Tee;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wardrobe|Layers")
	ESprawlWardrobeBottom Bottom = ESprawlWardrobeBottom::Jeans;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wardrobe|Layers")
	ESprawlWardrobeOuterwear Outerwear = ESprawlWardrobeOuterwear::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wardrobe|Layers")
	ESprawlWardrobeHeadwear Headwear = ESprawlWardrobeHeadwear::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wardrobe|Layers")
	ESprawlWardrobeFootwear Footwear = ESprawlWardrobeFootwear::LowTopSneakers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wardrobe|Layers")
	ESprawlWardrobeSocks Socks = ESprawlWardrobeSocks::Crew;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wardrobe|Palette")
	FLinearColor TopColor = FLinearColor(0.16f, 0.18f, 0.22f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wardrobe|Palette")
	FLinearColor BottomColor = FLinearColor(0.025f, 0.03f, 0.045f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wardrobe|Palette")
	FLinearColor OuterwearColor = FLinearColor(0.04f, 0.055f, 0.09f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wardrobe|Palette")
	FLinearColor AccentColor = FLinearColor(0.12f, 0.55f, 0.48f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wardrobe|Palette")
	FLinearColor ShoeColor = FLinearColor(0.035f, 0.04f, 0.05f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wardrobe|Palette")
	FLinearColor SockColor = FLinearColor(0.72f, 0.74f, 0.76f, 1.f);

	bool operator==(const FSprawlWardrobeOutfit& Other) const;
	bool operator!=(const FSprawlWardrobeOutfit& Other) const
	{
		return !(*this == Other);
	}
};

/**
 * Applies a complete outfit consistently to an assembled MetaHuman.
 *
 * The authored MetaHuman shirt/short garment remains the fitted base layer.
 * This component recolors that base and adds low-cost bone-bound footwear,
 * socks, optional headwear, and optional jacket details. Accessories have no
 * collision, navigation, tick, decals, or shadows, preserving the mobile LOD
 * and crowd budgets while authored wardrobe meshes are expanded in future.
 */
UCLASS(ClassGroup=(Sprawl), meta=(BlueprintSpawnableComponent))
class CONNECTEDSPRAWL_API USprawlWardrobeModule : public UActorComponent
{
	GENERATED_BODY()

public:
	USprawlWardrobeModule();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Stable outfit with mandatory top, bottom, socks, and shoes. */
	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Wardrobe")
	static FSprawlWardrobeOutfit DevelopOutfit(
		ESprawlWardrobeStyle Style, int32 Seed);

	/** Zarri's approved founder streetwear; keeps his hair visible. */
	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Wardrobe")
	static FSprawlWardrobeOutfit CreateZarriOutfit(int32 Seed = 20050101);

	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Wardrobe")
	static bool ValidateOutfit(
		const FSprawlWardrobeOutfit& Outfit, FString& OutError);

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Wardrobe")
	static FString DescribeOutfit(const FSprawlWardrobeOutfit& Outfit);

	/** Tint assembled garments without creating accessory geometry. */
	static void ApplyGarmentPalette(
		AActor* VisualActor, const FSprawlWardrobeOutfit& Outfit);

	/** Replace any previous accessories and bind this outfit to Body bones. */
	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Wardrobe")
	bool ApplyToMetaHuman(
		AActor* VisualActor,
		USkeletalMeshComponent* BodyMesh,
		const FSprawlWardrobeOutfit& Outfit);

	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Wardrobe")
	void ClearWardrobe();

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Wardrobe")
	int32 GetAccessoryCount() const;

	/** Swap the current character to a named athletic pair in one call. */
	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Wardrobe")
	bool SwapAthleticShoes(ESprawlAthleticShoePreset Preset);

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Wardrobe")
	USprawlFootwearModule* GetFootwearModule() const { return FootwearModule; }

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMeshComponent>> SpawnedAccessories;

	UPROPERTY(Transient)
	TObjectPtr<USprawlFootwearModule> FootwearModule;

	UStaticMeshComponent* SpawnAccessory(
		AActor* VisualActor,
		USkeletalMeshComponent* BodyMesh,
		UStaticMesh* Mesh,
		FName BoneName,
		const FTransform& WorldTransform,
		const FLinearColor& Color,
		FName ComponentName);

	UProceduralMeshComponent* SpawnProceduralAccessory(
		AActor* VisualActor,
		USkeletalMeshComponent* BodyMesh,
		FName BoneName,
		const FTransform& WorldTransform,
		const TArray<FVector>& Vertices,
		const TArray<int32>& Triangles,
		const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0,
		const FLinearColor& Color,
		FName ComponentName);
};
