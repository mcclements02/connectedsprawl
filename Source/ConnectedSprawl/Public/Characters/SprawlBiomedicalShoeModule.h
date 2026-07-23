// The Connected Sprawl - Cordero Biomedical high-top development profile.

#pragma once

#include "Characters/SprawlFootwearModule.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SprawlBiomedicalShoeModule.generated.h"

/** Project-owned performance footwear developed for Zarri and crowd variants. */
USTRUCT(BlueprintType)
struct CONNECTEDSPRAWL_API FSprawlBiomedicalShoeDefinition
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Biomedical Shoe")
	FName PresetId = TEXT("CorderoBioCircuitHighTop");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Biomedical Shoe")
	FText DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Biomedical Shoe")
	ESprawlWardrobeFootwear Footwear =
		ESprawlWardrobeFootwear::HighTopSneakers;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Biomedical Shoe")
	ESprawlWardrobeSocks Socks = ESprawlWardrobeSocks::Crew;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Biomedical Shoe")
	FLinearColor NanofiberUpperColor = FLinearColor(0.80f, 0.82f, 0.82f, 1.f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Biomedical Shoe")
	FLinearColor SupportColor = FLinearColor(0.42f, 0.025f, 0.045f, 1.f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Biomedical Shoe")
	FLinearColor CircuitColor = FLinearColor(0.82f, 0.53f, 0.12f, 1.f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Biomedical Shoe")
	FLinearColor SockColor = FLinearColor(0.70f, 0.72f, 0.73f, 1.f);

	/** Intended imported asset path; runtime falls back to fitted procedural high-tops. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Biomedical Shoe")
	FSoftObjectPath PresentationMesh =
		FSoftObjectPath(TEXT("/Game/Characters/Footwear/SM_CorderoBiomedical_HighTopPair.SM_CorderoBiomedical_HighTopPair"));

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Biomedical Shoe")
	FString BlenderSource =
		TEXT("Content/MetaHumans/Source/Footwear/CorderoBiomedical/CorderoBiomedical_HighTop.blend");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Biomedical Shoe")
	int32 TractionPodCountPerShoe = 10;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Biomedical Shoe")
	float ReferenceLengthCm = 28.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Biomedical Shoe")
	bool bHasCircuitWindow = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Biomedical Shoe")
	bool bHasHeelAirUnit = true;
};

/**
 * Blueprint-facing development and swap contract for the original Cordero
 * Biomedical Bio-Circuit high-top. The asset profile is reusable by Zarri,
 * named characters, and pedestrian wardrobe variants.
 */
UCLASS()
class CONNECTEDSPRAWL_API USprawlBiomedicalShoeModule
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Biomedical Shoe")
	static FSprawlBiomedicalShoeDefinition DevelopBioCircuitHighTop();

	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Biomedical Shoe")
	static bool ValidateDefinition(
		const FSprawlBiomedicalShoeDefinition& Definition,
		FString& OutError);

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Biomedical Shoe")
	static FString DescribeDefinition(
		const FSprawlBiomedicalShoeDefinition& Definition);

	/** Apply the fitted high-top fallback immediately through the live foot follower. */
	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Biomedical Shoe")
	static bool SwapToBioCircuitHighTop(USprawlFootwearModule* FootwearModule);
};
