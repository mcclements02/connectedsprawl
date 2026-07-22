// The Connected Sprawl - Data-driven city character and MetaHuman briefs.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SprawlCharacterDeveloper.generated.h"

class AActor;
class UAnimSequence;

/** District identity used to cast pedestrians for the place they occupy. */
UENUM(BlueprintType)
enum class ESprawlCharacterDistrict : uint8
{
	Junction,
	IronForest,
	RailYards,
	Arteries
};

/** Lightweight civilian roles; these drive schedule, gait, and wardrobe. */
UENUM(BlueprintType)
enum class ESprawlCharacterRole : uint8
{
	Resident,
	Commuter,
	TechWorker,
	ServiceWorker,
	WarehouseWorker,
	Courier,
	Founder,
	Student,
	NightWorker
};

/** What the pedestrian is plausibly doing at the current city hour. */
UENUM(BlueprintType)
enum class ESprawlCharacterActivity : uint8
{
	Commuting,
	Working,
	RunningErrand,
	Socializing,
	Exercising,
	HeadingHome
};

/** A small wardrobe vocabulary that maps cleanly onto MetaHuman Creator. */
UENUM(BlueprintType)
enum class ESprawlWardrobeStyle : uint8
{
	Streetwear,
	SmartCasual,
	Corporate,
	Workwear,
	Athletic
};

/**
 * Runtime-safe output of the Character Developer.
 *
 * It contains only compact values and soft authoring text: no model weights,
 * network client, or hard MetaHuman asset dependency enters the mobile build.
 */
USTRUCT(BlueprintType)
struct CONNECTEDSPRAWL_API FSprawlCharacterProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity")
	FName CharacterId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity")
	int32 Seed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="City")
	ESprawlCharacterDistrict HomeDistrict = ESprawlCharacterDistrict::Junction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="City")
	ESprawlCharacterDistrict WorkDistrict = ESprawlCharacterDistrict::Junction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="City")
	ESprawlCharacterRole Role = ESprawlCharacterRole::Resident;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="City")
	ESprawlCharacterActivity Activity = ESprawlCharacterActivity::RunningErrand;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="City")
	FString Destination;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schedule", meta=(ClampMin="0.0", ClampMax="24.0"))
	float ActiveStartHour = 7.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schedule", meta=(ClampMin="0.0", ClampMax="24.0"))
	float ActiveEndHour = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance")
	ESprawlWardrobeStyle Wardrobe = ESprawlWardrobeStyle::Streetwear;

	/** Lightweight /Game/Pedestrians variant used when no named MetaHuman is assigned. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance")
	FString PreferredAvatarVariant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance", meta=(ClampMin="150.0", ClampMax="205.0"))
	float HeightCm = 170.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior", meta=(ClampMin="0.6", ClampMax="1.5"))
	float WalkSpeedScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior", meta=(ClampMin="0.0", ClampMax="1.0"))
	float CrossChance = 0.30f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Behavior", meta=(ClampMin="0.0", ClampMax="1.0"))
	float IdleTalkChance = 0.20f;

	/** Paste-ready description for a designer working in MetaHuman Creator. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Authoring", meta=(MultiLine="true"))
	FString MetaHumanCreatorBrief;

	/** License-neutral prompt for a full-body turnaround reference sheet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Authoring", meta=(MultiLine="true"))
	FString ReferenceImagePrompt;
};

/**
 * Optional named-character asset. Crowd extras use generated profiles; hero
 * and mission NPC assets can bind those same fields to an assembled MetaHuman.
 */
UCLASS(BlueprintType)
class CONNECTEDSPRAWL_API USprawlCharacterDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character")
	FSprawlCharacterProfile Profile;

	/** Assembled Optimized/Low MetaHuman actor; never synchronously loaded by crowds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|MetaHuman")
	TSoftClassPtr<AActor> MetaHumanVisualClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|MetaHuman|Animation")
	TSoftObjectPtr<UAnimSequence> IdleAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|MetaHuman|Animation")
	TSoftObjectPtr<UAnimSequence> WalkAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|MetaHuman|Animation")
	TSoftObjectPtr<UAnimSequence> RunAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|MetaHuman|Animation")
	TSoftObjectPtr<UAnimSequence> TalkAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|MetaHuman|Animation")
	TSoftObjectPtr<UAnimSequence> SitAnimation;

	/** Optional wheel-ready pose; falls back to SitAnimation when unset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|MetaHuman|Animation")
	TSoftObjectPtr<UAnimSequence> DriveAnimation;

	/** Hub model provenance from the offline authoring tool, not a runtime dependency. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Character|Provenance")
	FString HuggingFaceModelId;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};

/**
 * Deterministic character casting and authoring helpers.
 *
 * A seed/location/hour always produces the same valid resident. This makes
 * tests and captures repeatable while preserving variety between people.
 */
UCLASS()
class CONNECTEDSPRAWL_API USprawlCharacterDeveloper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Character Developer")
	static ESprawlCharacterDistrict DistrictForLocation(const FVector& WorldLocation);

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Character Developer")
	static FSprawlCharacterProfile DevelopCharacter(
		int32 Seed, const FVector& WorldLocation, float CityHour);

	/** Density relative to the iPhone-safe TargetCount cap (never exceeds 1). */
	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Character Developer")
	static float PopulationMultiplier(
		ESprawlCharacterDistrict District, float CityHour);

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Character Developer")
	static FString BuildMetaHumanCreatorBrief(const FSprawlCharacterProfile& Profile);

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Character Developer")
	static FString BuildReferenceImagePrompt(const FSprawlCharacterProfile& Profile);

	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Character Developer")
	static bool ValidateProfile(
		const FSprawlCharacterProfile& Profile, FString& OutError);

	/** Reads the live environment controller, falling back to the authored 08:04 start. */
	static float ResolveCityHour(const UWorld* World);
};
