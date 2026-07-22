// The Connected Sprawl - native full-city map overlay.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SprawlCityMapWidget.generated.h"

class UCanvasPanel;
class UTextBlock;

/** Lazily-created native map; no texture or Widget Blueprint dependency. */
UCLASS()
class CONNECTEDSPRAWL_API USprawlCityMapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void BuildUI();
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void RefreshPlayerMarker();

	UPROPERTY() TObjectPtr<UCanvasPanel> MapCanvas;
	UPROPERTY() TObjectPtr<UTextBlock> PlayerMarker;
	UPROPERTY() TObjectPtr<UTextBlock> LocationText;

	float RefreshAccumulator = 0.f;
};
