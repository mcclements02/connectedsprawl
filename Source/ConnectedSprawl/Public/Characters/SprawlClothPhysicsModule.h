// The Connected Sprawl - Real clothing physics and fabric simulation module.

#pragma once

#include "Characters/SprawlWardrobeModule.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "SprawlClothPhysicsModule.generated.h"

class AActor;
class USkeletalMeshComponent;

UENUM(BlueprintType)
enum class ESprawlClothFabricType : uint8
{
	CottonFleece,
	TechNylon,
	HeavyCanvas,
	WoolKnit,
	Denim,
	Leather,
	AthleticMesh,
	SilkBlend
};

/** Physical simulation profile for realistic fabric behavior in Unreal and Blender. */
USTRUCT(BlueprintType)
struct CONNECTEDSPRAWL_API FSprawlClothPhysicsProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloth Physics")
	FName PresetId = TEXT("CottonFleece_Default");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloth Physics")
	ESprawlClothFabricType FabricType = ESprawlClothFabricType::CottonFleece;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloth Physics")
	FText DisplayName;

	/** Mass density in grams per square meter (g/m²). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloth Physics|Mass", meta=(ClampMin="20.0", ClampMax="1200.0"))
	float MassDensityGsm = 420.f;

	/** Resistance to stretch along warp/weft axes (0.0 = elastic, 1.0 = rigid). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloth Physics|Stiffness", meta=(ClampMin="0.0", ClampMax="1.0"))
	float StretchingStiffness = 0.85f;

	/** Resistance to bending across folds (0.0 = ultra-drapey, 1.0 = cardboard stiff). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloth Physics|Stiffness", meta=(ClampMin="0.0", ClampMax="1.0"))
	float BendingStiffness = 0.45f;

	/** Resistance to diagonal shearing (0.0 = soft knit, 1.0 = heavy weave). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloth Physics|Stiffness", meta=(ClampMin="0.0", ClampMax="1.0"))
	float ShearStiffness = 0.70f;

	/** Aerodynamic drag and kinetic motion damping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloth Physics|Dynamics", meta=(ClampMin="0.0", ClampMax="5.0"))
	float AirDamping = 0.35f;

	/** Friction against body skin, undergarments, and collateral surfaces. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloth Physics|Dynamics", meta=(ClampMin="0.0", ClampMax="2.0"))
	float FrictionCoefficient = 0.40f;

	/** Multiplier for gravity force applied to fabric folds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloth Physics|Dynamics", meta=(ClampMin="0.0", ClampMax="3.0"))
	float GravityScale = 1.0f;

	/** Multiplier for ambient and locomotion-induced wind response. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloth Physics|Dynamics", meta=(ClampMin="0.0", ClampMax="5.0"))
	float WindResponseScale = 1.2f;

	/** Minimum safety distance in cm to prevent mesh penetration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloth Physics|Collision", meta=(ClampMin="0.1", ClampMax="10.0"))
	float CollisionMarginCm = 0.8f;

	/** Maximum allowed vertex stretch displacement from rest position in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloth Physics|Limits", meta=(ClampMin="0.5", ClampMax="50.0"))
	float MaxDisplacementCm = 12.f;

	/** Minimum vertex weight for waist/shoulder/collar attachment anchors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloth Physics|Anchors", meta=(ClampMin="0.0", ClampMax="1.0"))
	float PinWeightThreshold = 0.8f;

	/** Matching Blender setup key used in authoring tools. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cloth Physics|Blender")
	FString BlenderPresetName = TEXT("COTTON_FLEECE");

	bool operator==(const FSprawlClothPhysicsProfile& Other) const;
	bool operator!=(const FSprawlClothPhysicsProfile& Other) const
	{
		return !(*this == Other);
	}
};

/**
 * Component providing realistic cloth physics setup, fabric solver profiles,
 * and locomotion wind response for MetaHumans and Zarri character garments.
 */
UCLASS(ClassGroup=(Sprawl), meta=(BlueprintSpawnableComponent))
class CONNECTEDSPRAWL_API USprawlClothPhysicsModule : public UActorComponent
{
	GENERATED_BODY()

public:
	USprawlClothPhysicsModule();

	/** Construct a deterministic physical profile for a fabric type. */
	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Cloth Physics")
	static FSprawlClothPhysicsProfile DevelopPhysicsProfile(ESprawlClothFabricType FabricType);

	/** Validate that all physical parameters fall within physical limits. */
	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Cloth Physics")
	static bool ValidatePhysicsProfile(
		const FSprawlClothPhysicsProfile& Profile, FString& OutError);

	/** Generate a human-readable summary of fabric physics settings. */
	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Cloth Physics")
	static FString DescribePhysicsProfile(const FSprawlClothPhysicsProfile& Profile);

	/** Map top/bottom/outerwear layer to appropriate fabric physics type. */
	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Cloth Physics")
	static ESprawlClothFabricType ResolveTopFabric(ESprawlWardrobeTop Top);

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Cloth Physics")
	static ESprawlClothFabricType ResolveBottomFabric(ESprawlWardrobeBottom Bottom);

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Cloth Physics")
	static ESprawlClothFabricType ResolveOuterwearFabric(ESprawlWardrobeOuterwear Outerwear);

	/** Apply physical solver parameters to a targeted clothing skeletal mesh. */
	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Cloth Physics")
	bool ApplyFabricPhysics(
		USkeletalMeshComponent* MeshComponent, ESprawlClothFabricType FabricType);

	/** Configure clothing physics across all active outfit layers on an actor. */
	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Cloth Physics")
	bool ConfigureOutfitPhysics(
		AActor* VisualActor, const FSprawlWardrobeOutfit& Outfit);

	/** Update aerodynamic wind forces based on locomotion speed and wind vector. */
	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Cloth Physics")
	void SimulateWindAndLocomotion(
		USkeletalMeshComponent* MeshComponent,
		float SpeedKph,
		FVector WindDirection);

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Cloth Physics")
	const FSprawlClothPhysicsProfile& GetActiveProfile() const { return ActiveProfile; }

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Cloth Physics")
	bool IsPhysicsSimulationEnabled() const { return bSimulationEnabled; }

	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Cloth Physics")
	void SetSimulationEnabled(bool bEnabled) { bSimulationEnabled = bEnabled; }

private:
	UPROPERTY(EditAnywhere, Category="Cloth Physics")
	FSprawlClothPhysicsProfile ActiveProfile;

	UPROPERTY(EditAnywhere, Category="Cloth Physics")
	bool bSimulationEnabled = true;

	UPROPERTY(Transient)
	float CurrentWindIntensity = 0.f;
};
