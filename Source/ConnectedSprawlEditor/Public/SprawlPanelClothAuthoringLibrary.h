// The Connected Sprawl - Reproducible Panel Cloth authoring commands.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SprawlPanelClothAuthoringLibrary.generated.h"

/** Editor-only entry point used by Python, automation, and the Panel Cloth UI. */
UCLASS()
class CONNECTEDSPRAWLEDITOR_API USprawlPanelClothAuthoringLibrary
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Create or refresh Zarri's hoodie from Unreal's Static Mesh Cloth template.
	 * The graph remains editable in the source asset; a graph-free runtime asset
	 * is baked alongside it for standalone and packaged builds.
	 */
	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Panel Cloth")
	static bool CreateOrUpdateZarriHoodieAsset(
		UPARAM(DisplayName="Status") FString& OutMessage);
};
