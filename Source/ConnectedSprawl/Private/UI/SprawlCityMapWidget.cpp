// The Connected Sprawl - native full-city map overlay.

#include "UI/SprawlCityMapWidget.h"

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
constexpr float MapSize = 720.f;

FSlateFontInfo MapFont(const UTextBlock* Text, int32 Size)
{
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = Size;
	return Font;
}

UTextBlock* MakeMapText(UWidgetTree* Tree, const TCHAR* Name, int32 Size,
	const FLinearColor& Color, ETextJustify::Type Justification)
{
	UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), Name);
	Text->SetFont(MapFont(Text, Size));
	Text->SetColorAndOpacity(FSlateColor(Color));
	Text->SetJustification(Justification);
	Text->SetAutoWrapText(false);
	return Text;
}

FVector2D ToMapPixel(const FVector& WorldLocation)
{
	const FVector2D Normalized =
		USprawlCityMapSubsystem::WorldToMapNormalized(WorldLocation);
	return FVector2D(Normalized.X * MapSize, (1.f - Normalized.Y) * MapSize);
}
}

void USprawlCityMapWidget::BuildUI()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("CityMapRoot"));
	WidgetTree->RootWidget = Root;

	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("MapBackdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.006f, 0.010f, 0.018f, 0.95f));
	Backdrop->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Backdrop))
	{
		Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		Slot->SetOffsets(FMargin(0.f));
		Slot->SetZOrder(0);
	}

	UTextBlock* Title = MakeMapText(WidgetTree, TEXT("MapTitle"), 32,
		FLinearColor(0.94f, 0.96f, 1.f), ETextJustify::Center);
	Title->SetText(NSLOCTEXT("Sprawl", "CityMapTitle",
		"THE CONNECTED SPRAWL  ·  CITY MAP"));
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Title))
	{
		Slot->SetAnchors(FAnchors(0.5f, 0.f));
		Slot->SetAlignment(FVector2D(0.5f, 0.f));
		Slot->SetPosition(FVector2D(0.f, 28.f));
		Slot->SetSize(FVector2D(900.f, 54.f));
		Slot->SetZOrder(3);
	}

	UBorder* MapFrame = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("MapFrame"));
	MapFrame->SetBrushColor(FLinearColor(0.07f, 0.085f, 0.11f, 1.f));
	MapFrame->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(MapFrame))
	{
		Slot->SetAnchors(FAnchors(0.5f, 0.5f));
		Slot->SetAlignment(FVector2D(0.5f, 0.5f));
		Slot->SetPosition(FVector2D::ZeroVector);
		Slot->SetSize(FVector2D(MapSize + 28.f, MapSize + 28.f));
		Slot->SetZOrder(1);
	}

	MapCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("MapCanvas"));
	MapCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(MapCanvas))
	{
		Slot->SetAnchors(FAnchors(0.5f, 0.5f));
		Slot->SetAlignment(FVector2D(0.5f, 0.5f));
		Slot->SetPosition(FVector2D::ZeroVector);
		Slot->SetSize(FVector2D(MapSize, MapSize));
		Slot->SetZOrder(2);
	}

	auto AddMapRect = [&](const TCHAR* Name, const FVector2D& Position,
		const FVector2D& Size, const FLinearColor& Color, int32 ZOrder)
	{
		UBorder* Rect = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), Name);
		Rect->SetBrushColor(Color);
		Rect->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UCanvasPanelSlot* Slot = MapCanvas->AddChildToCanvas(Rect))
		{
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
			Slot->SetZOrder(ZOrder);
		}
	};

	// The lake is one translucent geographic mass behind the road grid.
	const FVector LakeMin(
		Grid::BlockCenter(4) - Grid::BlockSize * 0.5f,
		Grid::BlockCenter(0) - Grid::BlockSize * 0.5f, 0.f);
	const FVector LakeMax(
		Grid::BlockCenter(6) + Grid::BlockSize * 0.5f,
		Grid::BlockCenter(1) + Grid::BlockSize * 0.5f, 0.f);
	const FVector2D LakeTopLeft(
		ToMapPixel(LakeMin).X, ToMapPixel(LakeMax).Y);
	const FVector2D LakeBottomRight(
		ToMapPixel(LakeMax).X, ToMapPixel(LakeMin).Y);
	AddMapRect(TEXT("Waterfront"), LakeTopLeft,
		LakeBottomRight - LakeTopLeft,
		FLinearColor(0.025f, 0.22f, 0.34f, 0.92f), 0);

	for (int32 Road = 0; Road < Grid::NumRoads; ++Road)
	{
		const float X = ToMapPixel(FVector(Grid::RoadCenter(Road), 0.f, 0.f)).X;
		const float Y = ToMapPixel(FVector(0.f, Grid::RoadCenter(Road), 0.f)).Y;
		AddMapRect(*FString::Printf(TEXT("RoadV%d"), Road),
			FVector2D(X - 7.f, 0.f), FVector2D(14.f, MapSize),
			FLinearColor(0.22f, 0.25f, 0.30f, 1.f), 1);
		AddMapRect(*FString::Printf(TEXT("RoadH%d"), Road),
			FVector2D(0.f, Y - 7.f), FVector2D(MapSize, 14.f),
			FLinearColor(0.22f, 0.25f, 0.30f, 1.f), 1);
	}

	const USprawlCityMapSubsystem* MapSubsystem = GetWorld()
		? GetWorld()->GetSubsystem<USprawlCityMapSubsystem>() : nullptr;
	const TArray<FSprawlMapLandmark> Landmarks = MapSubsystem
		? MapSubsystem->GetMapLandmarks()
		: USprawlCityMapSubsystem::BuildDefaultLandmarks();
	int32 MarkerIndex = 0;
	for (const FSprawlMapLandmark& Landmark : Landmarks)
	{
		UTextBlock* Marker = MakeMapText(WidgetTree,
			*FString::Printf(TEXT("Landmark%d"), MarkerIndex++), 16,
			Landmark.Color, ETextJustify::Center);
		Marker->SetText(FText::FromString(FString::Printf(TEXT("●  %s"),
			*Landmark.Label.ToString().ToUpper())));
		const FVector2D Pixel = ToMapPixel(Landmark.WorldLocation);
		if (UCanvasPanelSlot* Slot = MapCanvas->AddChildToCanvas(Marker))
		{
			Slot->SetPosition(Pixel);
			Slot->SetAlignment(FVector2D(0.5f, 0.5f));
			Slot->SetSize(FVector2D(250.f, 38.f));
			Slot->SetZOrder(4);
		}
	}

	PlayerMarker = MakeMapText(WidgetTree, TEXT("PlayerMarker"), 19,
		FLinearColor(1.f, 0.88f, 0.20f), ETextJustify::Center);
	PlayerMarker->SetText(NSLOCTEXT("Sprawl", "MapPlayer", "◆  ZARRI"));
	if (UCanvasPanelSlot* Slot = MapCanvas->AddChildToCanvas(PlayerMarker))
	{
		Slot->SetAlignment(FVector2D(0.5f, 0.5f));
		Slot->SetSize(FVector2D(160.f, 38.f));
		Slot->SetZOrder(8);
	}

	LocationText = MakeMapText(WidgetTree, TEXT("MapLocationText"), 18,
		FLinearColor(0.72f, 0.78f, 0.88f), ETextJustify::Center);
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(LocationText))
	{
		Slot->SetAnchors(FAnchors(0.5f, 1.f));
		Slot->SetAlignment(FVector2D(0.5f, 1.f));
		Slot->SetPosition(FVector2D(0.f, -54.f));
		Slot->SetSize(FVector2D(980.f, 56.f));
		Slot->SetZOrder(3);
	}

	UTextBlock* CloseHint = MakeMapText(WidgetTree, TEXT("MapCloseHint"), 17,
		FLinearColor(0.95f, 0.92f, 0.55f), ETextJustify::Right);
	CloseHint->SetText(NSLOCTEXT("Sprawl", "MapCloseHint", "M / MAP  —  close"));
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(CloseHint))
	{
		Slot->SetAnchors(FAnchors(1.f, 0.f));
		Slot->SetAlignment(FVector2D(1.f, 0.f));
		Slot->SetPosition(FVector2D(-28.f, 28.f));
		Slot->SetSize(FVector2D(260.f, 42.f));
		Slot->SetZOrder(3);
	}

	RefreshPlayerMarker();
}

void USprawlCityMapWidget::NativeConstruct()
{
	BuildUI();
	Super::NativeConstruct();
	RefreshPlayerMarker();
}

void USprawlCityMapWidget::NativeTick(
	const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshAccumulator += InDeltaTime;
	if (RefreshAccumulator >= 0.10f)
	{
		RefreshAccumulator = 0.f;
		RefreshPlayerMarker();
	}
}

void USprawlCityMapWidget::RefreshPlayerMarker()
{
	APlayerController* PC = GetOwningPlayer();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	const USprawlCityMapSubsystem* MapSubsystem = GetWorld()
		? GetWorld()->GetSubsystem<USprawlCityMapSubsystem>() : nullptr;
	if (!Pawn || !MapSubsystem || !PlayerMarker)
	{
		return;
	}
	const FVector DisplayLocation =
		MapSubsystem->GetDisplayLocationForActor(Pawn);
	if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(PlayerMarker->Slot))
	{
		Slot->SetPosition(ToMapPixel(DisplayLocation));
	}

	if (!LocationText)
	{
		return;
	}
	const FSprawlMapLandmark* Nearest = nullptr;
	float NearestDistanceSq = TNumericLimits<float>::Max();
	for (const FSprawlMapLandmark& Landmark : MapSubsystem->GetLandmarksView())
	{
		const float DistanceSq = FVector::DistSquared2D(
			DisplayLocation, Landmark.WorldLocation);
		if (DistanceSq < NearestDistanceSq)
		{
			NearestDistanceSq = DistanceSq;
			Nearest = &Landmark;
		}
	}
	if (Nearest)
	{
		LocationText->SetText(FText::Format(
			NSLOCTEXT("Sprawl", "MapNearest",
				"Nearest destination: {0}  ·  {1} m  ·  yellow = Zarri"),
			Nearest->Label,
			FText::AsNumber(FMath::RoundToInt(
				FMath::Sqrt(NearestDistanceSq) / 100.f))));
	}
}
