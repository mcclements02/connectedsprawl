// The Connected Sprawl - Swappable athletic-shoe preset catalog.

#pragma once

#include "Characters/SprawlFootwearModule.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SprawlAthleticShoeModule.generated.h"

UENUM(BlueprintType)
enum class ESprawlAthleticShoePreset : uint8
{
	ZarriVelocity,
	MetroRunner,
	CourtHighTop,
	NightSprint
};

/** One complete, reusable athletic-shoe presentation. */
USTRUCT(BlueprintType)
struct CONNECTEDSPRAWL_API FSprawlAthleticShoeDefinition
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Athletic Shoes")
	FName PresetId = TEXT("ZarriVelocity");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Athletic Shoes")
	FText DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Athletic Shoes")
	ESprawlWardrobeFootwear Footwear =
		ESprawlWardrobeFootwear::AthleticTrainers;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Athletic Shoes")
	ESprawlWardrobeSocks Socks = ESprawlWardrobeSocks::Crew;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Athletic Shoes")
	FLinearColor UpperColor = FLinearColor(0.035f, 0.09f, 0.28f, 1.f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Athletic Shoes")
	FLinearColor SockColor = FLinearColor(0.025f, 0.045f, 0.10f, 1.f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Athletic Shoes")
	FLinearColor AccentColor = FLinearColor(0.10f, 0.56f, 0.48f, 1.f);
};

/**
 * Blueprint catalog and one-call swap mechanism shared by Zarri, named
 * characters, and pedestrians that own a USprawlFootwearModule.
 */
UCLASS()
class CONNECTEDSPRAWL_API USprawlAthleticShoeModule
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Athletic Shoes")
	static FSprawlAthleticShoeDefinition DevelopPreset(
		ESprawlAthleticShoePreset Preset);

	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Athletic Shoes")
	static bool ValidatePreset(
		const FSprawlAthleticShoeDefinition& Definition,
		FString& OutError);

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Athletic Shoes")
	static ESprawlAthleticShoePreset NextPreset(
		ESprawlAthleticShoePreset Current);

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Athletic Shoes")
	static int32 GetPresetCount() { return 4; }

	/** Rebuild a bound character's pair from one named preset. */
	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Athletic Shoes")
	static bool SwapToPreset(
		USprawlFootwearModule* FootwearModule,
		ESprawlAthleticShoePreset Preset);
};
