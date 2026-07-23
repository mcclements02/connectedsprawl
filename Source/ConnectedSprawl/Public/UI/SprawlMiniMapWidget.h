// The Connected Sprawl - always-on corner minimap.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SprawlMiniMapWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UTextBlock;

/**
 * Small player-centred minimap for the HUD corner, distinct from the
 * full-screen city map. Scrolls the road grid, waterfront, and landmark dots
 * underneath a fixed centre marker, so the player reads local streets at a
 * glance without opening the full map.
 */
UCLASS()
class CONNECTEDSPRAWL_API USprawlMiniMapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Edge length of the square map face, in slate units. */
	static constexpr float MiniMapSizePx = 216.f;

	/** World distance covered edge-to-edge by the map face, in centimetres. */
	static constexpr float DefaultViewSpanCm = 9000.f;

	void BuildUI();
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/**
	 * Project a world location into minimap pixel space around the viewer.
	 * Screen +X follows world +X and screen +Y falls as world +Y rises, matching
	 * the full-screen city map's north-up orientation.
	 */
	UFUNCTION(BlueprintPure, Category="Connected Sprawl|MiniMap")
	static FVector2D ProjectToMiniMap(
		const FVector& WorldLocation,
		const FVector& ViewerLocation,
		float ViewSpanCm,
		float MapSizePx);

	/** Convert a world yaw into the clockwise slate angle for an up-pointing glyph. */
	UFUNCTION(BlueprintPure, Category="Connected Sprawl|MiniMap")
	static float ResolveMarkerAngle(float ViewerYawDegrees);

	/** True when a projected pixel still lands on the visible map face. */
	UFUNCTION(BlueprintPure, Category="Connected Sprawl|MiniMap")
	static bool IsPixelOnFace(const FVector2D& Pixel, float MapSizePx, float MarginPx);

	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|MiniMap")
	void SetViewSpanCm(float NewViewSpanCm);

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|MiniMap")
	float GetViewSpanCm() const { return ViewSpanCm; }

private:
	void RefreshMiniMap(bool bUpdateLocationLabel);
	void PlaceRect(UBorder* Rect, const FVector2D& Centre, const FVector2D& Size) const;

	UPROPERTY() TObjectPtr<UCanvasPanel> MapFace;
	UPROPERTY() TObjectPtr<UBorder> Waterfront;
	UPROPERTY() TObjectPtr<UTextBlock> PlayerMarker;
	UPROPERTY() TObjectPtr<UTextBlock> LocationLabel;
	UPROPERTY() TArray<TObjectPtr<UBorder>> RoadRects;
	UPROPERTY() TArray<TObjectPtr<UTextBlock>> LandmarkDots;

	TArray<FVector2D> RoadCentres;
	TArray<bool> RoadIsVertical;
	TArray<FVector> LandmarkLocations;

	float ViewSpanCm = DefaultViewSpanCm;
	float MiniMapRefreshAccumulator = 0.f;
	float LocationLabelRefreshAccumulator = 0.f;
	int32 LastNearestMeters = INDEX_NONE;
	FText LastNearestLabel;
};
