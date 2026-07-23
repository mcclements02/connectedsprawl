// The Connected Sprawl - always-on corner minimap.

#include "UI/SprawlMiniMapWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "World/SprawlCityGridSubsystem.h"
#include "World/SprawlCityMapSubsystem.h"

using Grid = USprawlCityGridSubsystem;

namespace
{
constexpr float MiniMapFrameThickness = 3.f;
constexpr float MiniMapLabelHeight = 20.f;
constexpr float MiniMapRefreshInterval = 1.f / 15.f;
constexpr float MiniMapLabelRefreshInterval = 0.5f;
constexpr float MiniMapLayoutTolerancePx = 0.25f;

FSlateFontInfo MiniMapFont(const UTextBlock* Text, int32 Size)
{
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = Size;
	return Font;
}

UTextBlock* MakeMiniMapText(UWidgetTree* Tree, const TCHAR* Name, int32 Size,
	const FLinearColor& Color)
{
	UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), Name);
	Text->SetFont(MiniMapFont(Text, Size));
	Text->SetColorAndOpacity(FSlateColor(Color));
	Text->SetJustification(ETextJustify::Center);
	Text->SetAutoWrapText(false);
	return Text;
}

UBorder* MakeMiniMapRect(UWidgetTree* Tree, const TCHAR* Name,
	const FLinearColor& Color)
{
	UBorder* Rect = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
	Rect->SetBrushColor(Color);
	Rect->SetVisibility(ESlateVisibility::HitTestInvisible);
	return Rect;
}
}

FVector2D USprawlMiniMapWidget::ProjectToMiniMap(
	const FVector& WorldLocation,
	const FVector& ViewerLocation,
	float ViewSpanCm,
	float MapSizePx)
{
	const float SafeSpan = FMath::IsFinite(ViewSpanCm)
		? FMath::Max(ViewSpanCm, 1.f) : 1.f;
	const float SafeSize = FMath::IsFinite(MapSizePx)
		? FMath::Max(MapSizePx, 0.f) : 0.f;
	const float Scale = SafeSize / SafeSpan;
	const float DeltaX = FMath::IsFinite(WorldLocation.X - ViewerLocation.X)
		? WorldLocation.X - ViewerLocation.X : 0.f;
	const float DeltaY = FMath::IsFinite(WorldLocation.Y - ViewerLocation.Y)
		? WorldLocation.Y - ViewerLocation.Y : 0.f;
	// North-up: world +Y is north, which rises toward the top of the face.
	return FVector2D(
		SafeSize * 0.5f + DeltaX * Scale,
		SafeSize * 0.5f - DeltaY * Scale);
}

float USprawlMiniMapWidget::ResolveMarkerAngle(float ViewerYawDegrees)
{
	const float SafeYaw = FMath::IsFinite(ViewerYawDegrees) ? ViewerYawDegrees : 0.f;
	// An up-pointing glyph reads as world +Y, so a yaw of 90 needs no rotation.
	return FRotator::ClampAxis(90.f - SafeYaw);
}

bool USprawlMiniMapWidget::IsPixelOnFace(
	const FVector2D& Pixel, float MapSizePx, float MarginPx)
{
	if (!FMath::IsFinite(Pixel.X) || !FMath::IsFinite(Pixel.Y))
	{
		return false;
	}
	const float Limit = FMath::Max(MapSizePx, 0.f) + FMath::Max(MarginPx, 0.f);
	const float Lower = -FMath::Max(MarginPx, 0.f);
	return Pixel.X >= Lower && Pixel.X <= Limit
		&& Pixel.Y >= Lower && Pixel.Y <= Limit;
}

void USprawlMiniMapWidget::SetViewSpanCm(float NewViewSpanCm)
{
	if (!FMath::IsFinite(NewViewSpanCm))
	{
		return;
	}
	ViewSpanCm = FMath::Clamp(NewViewSpanCm, 1500.f, Grid::Span);
	RefreshMiniMap(true);
}

void USprawlMiniMapWidget::BuildUI()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("MiniMapRoot"));
	WidgetTree->RootWidget = Root;

	// The frame sits a few units proud of the face so the map reads as an inset
	// instrument rather than as loose rectangles painted over the world.
	UBorder* Frame = MakeMiniMapRect(WidgetTree, TEXT("MiniMapFrame"),
		FLinearColor(0.55f, 0.62f, 0.74f, 0.85f));
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Frame))
	{
		Slot->SetAnchors(FAnchors(0.f, 0.f));
		Slot->SetPosition(FVector2D(-MiniMapFrameThickness, -MiniMapFrameThickness));
		Slot->SetSize(FVector2D(
			MiniMapSizePx + MiniMapFrameThickness * 2.f,
			MiniMapSizePx + MiniMapFrameThickness * 2.f));
		Slot->SetZOrder(0);
	}

	UBorder* Backdrop = MakeMiniMapRect(WidgetTree, TEXT("MiniMapBackdrop"),
		FLinearColor(0.012f, 0.018f, 0.028f, 0.94f));
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Backdrop))
	{
		Slot->SetAnchors(FAnchors(0.f, 0.f));
		Slot->SetPosition(FVector2D::ZeroVector);
		Slot->SetSize(FVector2D(MiniMapSizePx, MiniMapSizePx));
		Slot->SetZOrder(1);
	}

	MapFace = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("MiniMapFace"));
	MapFace->SetVisibility(ESlateVisibility::HitTestInvisible);
	// Scrolling geometry must stop at the frame instead of bleeding onto the HUD.
	MapFace->SetClipping(EWidgetClipping::ClipToBounds);
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(MapFace))
	{
		Slot->SetAnchors(FAnchors(0.f, 0.f));
		Slot->SetPosition(FVector2D::ZeroVector);
		Slot->SetSize(FVector2D(MiniMapSizePx, MiniMapSizePx));
		Slot->SetZOrder(2);
	}

	auto AddFaceChild = [&](UWidget* Child, int32 ZOrder) -> UCanvasPanelSlot*
	{
		UCanvasPanelSlot* Slot = MapFace->AddChildToCanvas(Child);
		if (Slot)
		{
			Slot->SetAlignment(FVector2D(0.5f, 0.5f));
			Slot->SetZOrder(ZOrder);
		}
		return Slot;
	};

	Waterfront = MakeMiniMapRect(WidgetTree, TEXT("MiniMapWaterfront"),
		FLinearColor(0.025f, 0.22f, 0.34f, 0.95f));
	AddFaceChild(Waterfront, 0);

	RoadCentres.Reset();
	RoadIsVertical.Reset();
	RoadRects.Reset();
	for (int32 Road = 0; Road < Grid::NumRoads; ++Road)
	{
		const float Centre = Grid::RoadCenter(Road);
		RoadCentres.Add(FVector2D(Centre, 0.f));
		RoadIsVertical.Add(true);
		RoadRects.Add(MakeMiniMapRect(WidgetTree,
			*FString::Printf(TEXT("MiniRoadV%d"), Road),
			FLinearColor(0.24f, 0.27f, 0.32f, 1.f)));

		RoadCentres.Add(FVector2D(0.f, Centre));
		RoadIsVertical.Add(false);
		RoadRects.Add(MakeMiniMapRect(WidgetTree,
			*FString::Printf(TEXT("MiniRoadH%d"), Road),
			FLinearColor(0.24f, 0.27f, 0.32f, 1.f)));
	}
	for (UBorder* Rect : RoadRects)
	{
		AddFaceChild(Rect, 1);
	}

	const USprawlCityMapSubsystem* MapSubsystem = GetWorld()
		? GetWorld()->GetSubsystem<USprawlCityMapSubsystem>() : nullptr;
	const TArray<FSprawlMapLandmark> Landmarks = MapSubsystem
		? MapSubsystem->GetMapLandmarks()
		: USprawlCityMapSubsystem::BuildDefaultLandmarks();
	LandmarkLocations.Reset();
	LandmarkDots.Reset();
	int32 DotIndex = 0;
	for (const FSprawlMapLandmark& Landmark : Landmarks)
	{
		UTextBlock* Dot = MakeMiniMapText(WidgetTree,
			*FString::Printf(TEXT("MiniLandmark%d"), DotIndex++), 11,
			Landmark.Color);
		Dot->SetText(FText::FromString(TEXT("●")));
		AddFaceChild(Dot, 3);
		LandmarkDots.Add(Dot);
		LandmarkLocations.Add(Landmark.WorldLocation);
	}

	PlayerMarker = MakeMiniMapText(WidgetTree, TEXT("MiniPlayerMarker"), 17,
		FLinearColor(1.f, 0.88f, 0.20f));
	PlayerMarker->SetText(FText::FromString(TEXT("▲")));
	if (UCanvasPanelSlot* Slot = AddFaceChild(PlayerMarker, 5))
	{
		Slot->SetPosition(FVector2D(MiniMapSizePx * 0.5f, MiniMapSizePx * 0.5f));
		Slot->SetSize(FVector2D(26.f, 26.f));
	}
	PlayerMarker->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

	UTextBlock* NorthPip = MakeMiniMapText(WidgetTree, TEXT("MiniMapNorth"), 11,
		FLinearColor(0.78f, 0.84f, 0.94f, 0.9f));
	NorthPip->SetText(NSLOCTEXT("Sprawl", "MiniMapNorth", "N"));
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(NorthPip))
	{
		Slot->SetAnchors(FAnchors(1.f, 0.f));
		Slot->SetAlignment(FVector2D(1.f, 0.f));
		Slot->SetPosition(FVector2D(-5.f, 3.f));
		Slot->SetSize(FVector2D(16.f, 16.f));
		Slot->SetZOrder(4);
	}

	LocationLabel = MakeMiniMapText(WidgetTree, TEXT("MiniMapLocation"), 12,
		FLinearColor(0.74f, 0.80f, 0.90f));
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(LocationLabel))
	{
		Slot->SetAnchors(FAnchors(0.f, 0.f));
		Slot->SetPosition(FVector2D(0.f, MiniMapSizePx + 5.f));
		Slot->SetSize(FVector2D(MiniMapSizePx, MiniMapLabelHeight));
		Slot->SetZOrder(4);
	}

	RefreshMiniMap(true);
}

void USprawlMiniMapWidget::NativeConstruct()
{
	BuildUI();
	Super::NativeConstruct();
	RefreshMiniMap(true);
}

void USprawlMiniMapWidget::NativeTick(
	const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	MiniMapRefreshAccumulator += InDeltaTime;
	LocationLabelRefreshAccumulator += InDeltaTime;
	if (MiniMapRefreshAccumulator < MiniMapRefreshInterval)
	{
		return;
	}
	const bool bUpdateLocationLabel =
		LocationLabelRefreshAccumulator >= MiniMapLabelRefreshInterval;
	MiniMapRefreshAccumulator = 0.f;
	if (bUpdateLocationLabel)
	{
		LocationLabelRefreshAccumulator = 0.f;
	}
	RefreshMiniMap(bUpdateLocationLabel);
}

void USprawlMiniMapWidget::PlaceRect(
	UBorder* Rect, const FVector2D& Centre, const FVector2D& Size) const
{
	if (!Rect)
	{
		return;
	}
	if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Rect->Slot))
	{
		if (!Slot->GetPosition().Equals(Centre, MiniMapLayoutTolerancePx))
		{
			Slot->SetPosition(Centre);
		}
		if (!Slot->GetSize().Equals(Size, MiniMapLayoutTolerancePx))
		{
			Slot->SetSize(Size);
		}
	}
}

void USprawlMiniMapWidget::RefreshMiniMap(bool bUpdateLocationLabel)
{
	if (!MapFace)
	{
		return;
	}
	const APlayerController* PC = GetOwningPlayer();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}
	const USprawlCityMapSubsystem* MapSubsystem = GetWorld()
		? GetWorld()->GetSubsystem<USprawlCityMapSubsystem>() : nullptr;
	const FVector Viewer = MapSubsystem
		? MapSubsystem->GetDisplayLocationForActor(Pawn)
		: Pawn->GetActorLocation();
	const float Scale = MiniMapSizePx / FMath::Max(ViewSpanCm, 1.f);

	if (PlayerMarker)
	{
		PlayerMarker->SetRenderTransformAngle(
			ResolveMarkerAngle(Pawn->GetActorRotation().Yaw));
	}

	if (Waterfront)
	{
		const FVector LakeMin(
			Grid::BlockCenter(4) - Grid::BlockSize * 0.5f,
			Grid::BlockCenter(0) - Grid::BlockSize * 0.5f, 0.f);
		const FVector LakeMax(
			Grid::BlockCenter(6) + Grid::BlockSize * 0.5f,
			Grid::BlockCenter(1) + Grid::BlockSize * 0.5f, 0.f);
		const FVector LakeCentre = (LakeMin + LakeMax) * 0.5f;
		PlaceRect(Waterfront,
			ProjectToMiniMap(LakeCentre, Viewer, ViewSpanCm, MiniMapSizePx),
			FVector2D(
				(LakeMax.X - LakeMin.X) * Scale,
				(LakeMax.Y - LakeMin.Y) * Scale));
	}

	const float RoadThickness = Grid::RoadWidth * Scale;
	const float FaceDiagonal = MiniMapSizePx * 2.f;
	for (int32 Index = 0; Index < RoadRects.Num(); ++Index)
	{
		if (!RoadCentres.IsValidIndex(Index) || !RoadIsVertical.IsValidIndex(Index))
		{
			continue;
		}
		const bool bVertical = RoadIsVertical[Index];
		const FVector RoadWorld(
			bVertical ? RoadCentres[Index].X : Viewer.X,
			bVertical ? Viewer.Y : RoadCentres[Index].Y,
			0.f);
		const FVector2D Pixel =
			ProjectToMiniMap(RoadWorld, Viewer, ViewSpanCm, MiniMapSizePx);
		PlaceRect(RoadRects[Index], Pixel, bVertical
			? FVector2D(RoadThickness, FaceDiagonal)
			: FVector2D(FaceDiagonal, RoadThickness));
	}

	for (int32 Index = 0; Index < LandmarkDots.Num(); ++Index)
	{
		UTextBlock* Dot = LandmarkDots[Index];
		if (!Dot || !LandmarkLocations.IsValidIndex(Index))
		{
			continue;
		}
		const FVector2D Pixel = ProjectToMiniMap(
			LandmarkLocations[Index], Viewer, ViewSpanCm, MiniMapSizePx);
		if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Dot->Slot))
		{
			const FVector2D DotSize(18.f, 18.f);
			if (!Slot->GetPosition().Equals(Pixel, MiniMapLayoutTolerancePx))
			{
				Slot->SetPosition(Pixel);
			}
			if (!Slot->GetSize().Equals(DotSize, MiniMapLayoutTolerancePx))
			{
				Slot->SetSize(DotSize);
			}
		}
		// Off-face dots are collapsed rather than clipped so they cost nothing.
		const ESlateVisibility DesiredVisibility = IsPixelOnFace(Pixel, MiniMapSizePx, 10.f)
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		if (Dot->GetVisibility() != DesiredVisibility)
		{
			Dot->SetVisibility(DesiredVisibility);
		}
	}

	if (!bUpdateLocationLabel || !LocationLabel || !MapSubsystem)
	{
		return;
	}
	const FSprawlMapLandmark* Nearest = nullptr;
	float NearestDistanceSq = TNumericLimits<float>::Max();
	for (const FSprawlMapLandmark& Landmark : MapSubsystem->GetLandmarksView())
	{
		const float DistanceSq =
			FVector::DistSquared2D(Viewer, Landmark.WorldLocation);
		if (DistanceSq < NearestDistanceSq)
		{
			NearestDistanceSq = DistanceSq;
			Nearest = &Landmark;
		}
	}
	if (Nearest)
	{
		const int32 NearestMeters = FMath::RoundToInt(
			FMath::Sqrt(NearestDistanceSq) / 100.f);
		if (!Nearest->Label.EqualTo(LastNearestLabel)
			|| NearestMeters != LastNearestMeters)
		{
			LocationLabel->SetText(FText::Format(
				NSLOCTEXT("Sprawl", "MiniMapNearest", "{0}  ·  {1} m"),
				Nearest->Label,
				FText::AsNumber(NearestMeters)));
			LastNearestLabel = Nearest->Label;
			LastNearestMeters = NearestMeters;
		}
	}
}
