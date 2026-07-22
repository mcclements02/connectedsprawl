// The Connected Sprawl - native fallback HUD with no Widget Blueprint dependency.

#include "UI/SprawlNativeHUD.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/ZarriCharacter.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "SprawlPlayerController.h"
#include "Vehicles/SprawlCar.h"
#include "Widgets/Input/SVirtualJoystick.h"

namespace
{
FSlateFontInfo NativeHUDSizedFont(const UTextBlock* Text, int32 Size)
{
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = Size;
	return Font;
}

UTextBlock* MakeNativeHUDText(UWidgetTree* Tree, const TCHAR* Name, int32 Size,
	const FLinearColor& Color, ETextJustify::Type Justification)
{
	UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	Text->SetFont(NativeHUDSizedFont(Text, Size));
	Text->SetColorAndOpacity(FSlateColor(Color));
	Text->SetJustification(Justification);
	Text->SetAutoWrapText(true);
	return Text;
}

UBorder* MakeNativeHUDPanel(UWidgetTree* Tree, const TCHAR* Name)
{
	UBorder* Panel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
	Panel->SetBrushColor(FLinearColor(0.015f, 0.020f, 0.030f, 0.82f));
	Panel->SetPadding(FMargin(14.f, 10.f));
	return Panel;
}
}

void USprawlNativeHUD::BuildUI()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("HUDRoot"));
	WidgetTree->RootWidget = Root;

	UBorder* StatsPanel = MakeNativeHUDPanel(WidgetTree, TEXT("StatsPanel"));
	// Read-only panels must not swallow touches meant for movement or look.
	StatsPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	StatsText = MakeNativeHUDText(WidgetTree, TEXT("StatsText"), 18,
		FLinearColor(0.94f, 0.96f, 1.f), ETextJustify::Left);
	StatsPanel->SetContent(StatsText);
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(StatsPanel))
	{
		Slot->SetAnchors(FAnchors(0.f, 0.f));
		Slot->SetPosition(FVector2D(24.f, 24.f));
		Slot->SetSize(FVector2D(350.f, 70.f));
	}

	UBorder* ObjectivePanel = MakeNativeHUDPanel(WidgetTree, TEXT("ObjectivePanel"));
	ObjectivePanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	ObjectiveText = MakeNativeHUDText(WidgetTree, TEXT("ObjectiveText"), 17,
		FLinearColor(0.45f, 0.86f, 1.f), ETextJustify::Center);
	ObjectivePanel->SetContent(ObjectiveText);
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(ObjectivePanel))
	{
		Slot->SetAnchors(FAnchors(0.5f, 0.f));
		Slot->SetAlignment(FVector2D(0.5f, 0.f));
		Slot->SetPosition(FVector2D(0.f, 24.f));
		Slot->SetSize(FVector2D(720.f, 64.f));
	}

	PhoneBorder = MakeNativeHUDPanel(WidgetTree, TEXT("PhonePanel"));
	PhoneBorder->SetBrushColor(FLinearColor(0.035f, 0.18f, 0.27f, 0.94f));
	PhoneText = MakeNativeHUDText(WidgetTree, TEXT("PhoneText"), 19,
		FLinearColor::White, ETextJustify::Center);
	PhoneBorder->SetContent(PhoneText);
	PhoneBorder->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(PhoneBorder))
	{
		Slot->SetAnchors(FAnchors(0.5f, 1.f));
		Slot->SetAlignment(FVector2D(0.5f, 1.f));
		Slot->SetPosition(FVector2D(0.f, -34.f));
		Slot->SetSize(FVector2D(660.f, 68.f));
	}

	InteractHintBorder = MakeNativeHUDPanel(WidgetTree, TEXT("InteractHintPanel"));
	InteractHintBorder->SetVisibility(ESlateVisibility::Collapsed);
	InteractHintText = MakeNativeHUDText(WidgetTree, TEXT("InteractHintText"), 17,
		FLinearColor(0.95f, 0.92f, 0.55f), ETextJustify::Center);
	InteractHintBorder->SetContent(InteractHintText);
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(InteractHintBorder))
	{
		Slot->SetAnchors(FAnchors(0.5f, 1.f));
		Slot->SetAlignment(FVector2D(0.5f, 1.f));
		Slot->SetPosition(FVector2D(0.f, -116.f));
		Slot->SetSize(FVector2D(520.f, 52.f));
	}

	MapButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(), TEXT("MapButton"));
	MapButton->SetBackgroundColor(FLinearColor(0.04f, 0.10f, 0.18f, 0.82f));
	MapLabel = MakeNativeHUDText(WidgetTree, TEXT("MapButtonLabel"), 18,
		FLinearColor(0.82f, 0.91f, 1.f), ETextJustify::Center);
	MapLabel->SetText(NSLOCTEXT("Sprawl", "MapButton", "MAP  [M]"));
	MapButton->AddChild(MapLabel);
	MapButton->OnPressed.AddDynamic(this, &USprawlNativeHUD::HandleMapButtonPressed);
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(MapButton))
	{
		Slot->SetAnchors(FAnchors(1.f, 0.f));
		Slot->SetAlignment(FVector2D(1.f, 0.f));
		Slot->SetPosition(FVector2D(-24.f, 24.f));
		Slot->SetSize(FVector2D(152.f, 54.f));
		Slot->SetZOrder(20);
	}

	BuildTouchControls(Root);
}

void USprawlNativeHUD::BuildTouchControls(UCanvasPanel* Root)
{
	if (bTouchControlsBuilt || !Root || !WidgetTree
		|| !SVirtualJoystick::ShouldDisplayTouchInterface())
	{
		return;
	}
	bTouchControlsBuilt = true;

	// Right-side look pane. The engine's single left virtual joystick owns
	// movement; drags here bubble up to the NativeOnTouch* overrides and turn
	// the camera, so there is no second joystick on screen.
	LookPane = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("LookPane"));
	LookPane->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.f));
	LookPane->SetVisibility(ESlateVisibility::Visible);
	if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(LookPane))
	{
		Slot->SetAnchors(FAnchors(0.42f, 0.05f, 1.f, 0.64f));
		Slot->SetOffsets(FMargin(0.f));
		Slot->SetZOrder(5);
	}

	auto MakeTouchButton = [&](const TCHAR* Name, const FText& Label,
		TObjectPtr<UTextBlock>& OutLabel, FVector2D Position, FVector2D Size)
		-> UButton*
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(), Name);
		Button->SetBackgroundColor(FLinearColor(0.05f, 0.07f, 0.11f, 0.62f));
		UTextBlock* LabelText = MakeNativeHUDText(WidgetTree,
			*(FString(Name) + TEXT("Label")), 22,
			FLinearColor(0.92f, 0.95f, 1.f), ETextJustify::Center);
		LabelText->SetText(Label);
		Button->AddChild(LabelText);
		OutLabel = LabelText;
		if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Button))
		{
			Slot->SetAnchors(FAnchors(1.f, 1.f));
			Slot->SetAlignment(FVector2D(1.f, 1.f));
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
			Slot->SetZOrder(10);
		}
		return Button;
	};

	PrimaryButton = MakeTouchButton(TEXT("TouchPrimary"),
		NSLOCTEXT("Sprawl", "TouchJump", "JUMP"), PrimaryLabel,
		FVector2D(-34.f, -34.f), FVector2D(168.f, 150.f));
	PrimaryButton->OnPressed.AddDynamic(
		this, &USprawlNativeHUD::HandlePrimaryPressed);
	PrimaryButton->OnReleased.AddDynamic(
		this, &USprawlNativeHUD::HandlePrimaryReleased);

	SecondaryButton = MakeTouchButton(TEXT("TouchSecondary"),
		NSLOCTEXT("Sprawl", "TouchSprint", "SPRINT"), SecondaryLabel,
		FVector2D(-222.f, -34.f), FVector2D(150.f, 116.f));
	SecondaryButton->OnPressed.AddDynamic(
		this, &USprawlNativeHUD::HandleSecondaryPressed);
	SecondaryButton->OnReleased.AddDynamic(
		this, &USprawlNativeHUD::HandleSecondaryReleased);

	InteractButton = MakeTouchButton(TEXT("TouchInteract"),
		NSLOCTEXT("Sprawl", "TouchEnter", "ENTER"), InteractLabel,
		FVector2D(-34.f, -204.f), FVector2D(168.f, 88.f));
	InteractButton->OnPressed.AddDynamic(
		this, &USprawlNativeHUD::HandleInteractButtonPressed);

	MeleeButton = MakeTouchButton(TEXT("TouchMelee"),
		NSLOCTEXT("Sprawl", "TouchMelee", "PUNCH\nKICK"), MeleeLabel,
		FVector2D(-222.f, -170.f), FVector2D(150.f, 116.f));
	MeleeButton->OnPressed.AddDynamic(
		this, &USprawlNativeHUD::HandleMeleeButtonPressed);

	RefreshTouchButtonLabels(true);
}

ASprawlPlayerController* USprawlNativeHUD::GetSprawlPC() const
{
	return Cast<ASprawlPlayerController>(GetOwningPlayer());
}

void USprawlNativeHUD::RefreshTouchButtonLabels(bool bForce)
{
	if (!bTouchControlsBuilt || !PrimaryLabel || !SecondaryLabel || !InteractLabel
		|| !MeleeButton)
	{
		return;
	}
	const ASprawlPlayerController* PC = GetSprawlPC();
	const bool bInCar = PC && PC->IsDrivingVehicle();
	if (!bForce && bInCar == bTouchButtonsInCarMode)
	{
		return;
	}
	bTouchButtonsInCarMode = bInCar;
	bTouchGasHeld = false;
	bTouchBrakeHeld = false;
	PrimaryLabel->SetText(bInCar
		? NSLOCTEXT("Sprawl", "TouchGas", "GAS")
		: NSLOCTEXT("Sprawl", "TouchJump", "JUMP"));
	SecondaryLabel->SetText(bInCar
		? NSLOCTEXT("Sprawl", "TouchBrake", "BRAKE")
		: NSLOCTEXT("Sprawl", "TouchSprint", "SPRINT"));
	InteractLabel->SetText(bInCar
		? NSLOCTEXT("Sprawl", "TouchExit", "EXIT")
		: NSLOCTEXT("Sprawl", "TouchEnter", "ENTER"));
	MeleeButton->SetVisibility(
		bInCar ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

void USprawlNativeHUD::HandlePrimaryPressed()
{
	ASprawlPlayerController* PC = GetSprawlPC();
	if (!PC)
	{
		return;
	}
	if (PC->IsDrivingVehicle())
	{
		bTouchGasHeld = true;
		PC->TouchThrottlePressed(1.f);
	}
	else
	{
		PC->TouchJumpPressed();
	}
}

void USprawlNativeHUD::HandlePrimaryReleased()
{
	ASprawlPlayerController* PC = GetSprawlPC();
	if (!PC)
	{
		return;
	}
	bTouchGasHeld = false;
	if (PC->IsDrivingVehicle() && bTouchBrakeHeld)
	{
		PC->TouchThrottlePressed(-1.f); // brake pedal is still down
	}
	else
	{
		PC->TouchThrottleReleased();
	}
	PC->TouchJumpReleased();
}

void USprawlNativeHUD::HandleSecondaryPressed()
{
	ASprawlPlayerController* PC = GetSprawlPC();
	if (!PC)
	{
		return;
	}
	if (PC->IsDrivingVehicle())
	{
		bTouchBrakeHeld = true;
		PC->TouchThrottlePressed(-1.f);
	}
	else
	{
		PC->TouchSprintPressed();
	}
}

void USprawlNativeHUD::HandleSecondaryReleased()
{
	ASprawlPlayerController* PC = GetSprawlPC();
	if (!PC)
	{
		return;
	}
	bTouchBrakeHeld = false;
	if (PC->IsDrivingVehicle() && bTouchGasHeld)
	{
		PC->TouchThrottlePressed(1.f); // gas pedal is still down
	}
	else
	{
		PC->TouchThrottleReleased();
	}
	PC->TouchSprintReleased();
}

void USprawlNativeHUD::HandleInteractButtonPressed()
{
	if (ASprawlPlayerController* PC = GetSprawlPC())
	{
		PC->TouchInteractPressed();
		RefreshTouchButtonLabels(true);
	}
}

void USprawlNativeHUD::HandleMeleeButtonPressed()
{
	if (ASprawlPlayerController* PC = GetSprawlPC();
		PC && !PC->IsDrivingVehicle())
	{
		PC->TouchMeleePressed();
	}
}

void USprawlNativeHUD::HandleMapButtonPressed()
{
	if (ASprawlPlayerController* PC = GetSprawlPC())
	{
		PC->TouchMapPressed();
	}
}

FReply USprawlNativeHUD::NativeOnTouchStarted(
	const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	// Only drags that begin over the look pane reach here: every other HUD
	// element is either hit-test-invisible or a button that handles itself.
	if (!bTouchControlsBuilt || LookFingerIndex != INDEX_NONE)
	{
		return Super::NativeOnTouchStarted(InGeometry, InGestureEvent);
	}
	LookFingerIndex = static_cast<int32>(InGestureEvent.GetPointerIndex());
	LastLookPosition = InGestureEvent.GetScreenSpacePosition();
	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply USprawlNativeHUD::NativeOnTouchMoved(
	const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (LookFingerIndex == INDEX_NONE
		|| static_cast<int32>(InGestureEvent.GetPointerIndex()) != LookFingerIndex)
	{
		return Super::NativeOnTouchMoved(InGeometry, InGestureEvent);
	}
	const FVector2D Position = InGestureEvent.GetScreenSpacePosition();
	const FVector2D PixelDelta = Position - LastLookPosition;
	LastLookPosition = Position;

	const float ViewportScale = FMath::Max(
		0.5f, UWidgetLayoutLibrary::GetViewportScale(GetWorld()));
	if (ASprawlPlayerController* PC = GetSprawlPC())
	{
		PC->ApplyTouchLook(PixelDelta / ViewportScale);
	}
	return FReply::Handled();
}

FReply USprawlNativeHUD::NativeOnTouchEnded(
	const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	if (LookFingerIndex != INDEX_NONE
		&& static_cast<int32>(InGestureEvent.GetPointerIndex()) == LookFingerIndex)
	{
		LookFingerIndex = INDEX_NONE;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnTouchEnded(InGeometry, InGestureEvent);
}

void USprawlNativeHUD::NativeConstruct()
{
	BuildUI();
	Super::NativeConstruct();

	if (UWorld* World = GetWorld())
	{
		if (USprawlGigSubsystem* Gigs = World->GetSubsystem<USprawlGigSubsystem>())
		{
			Gigs->OnGigStarted.AddDynamic(this, &USprawlNativeHUD::HandleGigStarted);
			Gigs->OnGigCompleted.AddDynamic(this, &USprawlNativeHUD::HandleGigCompleted);
			CurrentGig = Gigs->GetActiveGig();
		}
	}
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPhoneSubsystem* Phone = GI->GetSubsystem<UPhoneSubsystem>())
		{
			Phone->OnRinging.AddDynamic(this, &USprawlNativeHUD::HandlePhoneRinging);
			Phone->OnAnswered.AddDynamic(this, &USprawlNativeHUD::HandlePhoneAnswered);
			if (Phone->IsRinging() && PhoneBorder && PhoneText)
			{
				PhoneText->SetText(NSLOCTEXT("Sprawl", "IncomingCallGeneric",
					"Incoming call — [E] Answer"));
				PhoneBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		}
	}
	RefreshObjective();
}

void USprawlNativeHUD::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		if (USprawlGigSubsystem* Gigs = World->GetSubsystem<USprawlGigSubsystem>())
		{
			Gigs->OnGigStarted.RemoveDynamic(this, &USprawlNativeHUD::HandleGigStarted);
			Gigs->OnGigCompleted.RemoveDynamic(this, &USprawlNativeHUD::HandleGigCompleted);
		}
	}
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPhoneSubsystem* Phone = GI->GetSubsystem<UPhoneSubsystem>())
		{
			Phone->OnRinging.RemoveDynamic(this, &USprawlNativeHUD::HandlePhoneRinging);
			Phone->OnAnswered.RemoveDynamic(this, &USprawlNativeHUD::HandlePhoneAnswered);
		}
	}
	Super::NativeDestruct();
}

void USprawlNativeHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	DistanceRefreshAccumulator += InDeltaTime;
	CompletedMessageRemaining = FMath::Max(0.f, CompletedMessageRemaining - InDeltaTime);
	if (DistanceRefreshAccumulator >= 0.2f)
	{
		DistanceRefreshAccumulator = 0.f;
		RefreshObjective();
	}
	TouchModeRefreshAccumulator += InDeltaTime;
	if (TouchModeRefreshAccumulator >= 0.25f)
	{
		TouchModeRefreshAccumulator = 0.f;
		RefreshTouchButtonLabels();
	}
	HintRefreshAccumulator += InDeltaTime;
	if (HintRefreshAccumulator >= 0.25f)
	{
		HintRefreshAccumulator = 0.f;
		RefreshInteractHint();
	}
}

void USprawlNativeHUD::RefreshInteractHint()
{
	if (!InteractHintBorder || !InteractHintText)
	{
		return;
	}
	// Touch platforms surface real ENTER/GAS/BRAKE buttons instead of a hint.
	if (bTouchControlsBuilt)
	{
		InteractHintBorder->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	APlayerController* PC = GetOwningPlayer();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	FText Hint;
	if (Cast<ASprawlCar>(Pawn))
	{
		Hint = FText::FromString(
			TEXT("W accelerate  ·  S brake / reverse  ·  E exit  ·  M map"));
	}
	else if (const AZarriCharacter* Zarri = Cast<AZarriCharacter>(Pawn))
	{
		FText BuildingPrompt;
		if (Zarri->GetBuildingInteractionPrompt(BuildingPrompt))
		{
			Hint = FText::Format(
				NSLOCTEXT("Sprawl", "BuildingHUDHint",
					"E — {0}  ·  LMB / X — punch / kick  ·  M map"),
				BuildingPrompt);
		}
		else if (Zarri->HasNearbyEnterableVehicle())
		{
			Hint = FText::FromString(
				TEXT("E — enter car  ·  LMB / X — punch / kick  ·  M map"));
		}
		else
		{
			Hint = FText::FromString(
				TEXT("LMB / X — punch / kick  ·  M — city map"));
		}
	}
	if (Hint.IsEmpty())
	{
		InteractHintBorder->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	InteractHintText->SetText(Hint);
	InteractHintBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void USprawlNativeHUD::NativeRefresh()
{
	if (!StatsText)
	{
		return;
	}
	const FString Runway = DisplayRunwayDays >= 99999.f
		? TEXT("∞")
		: FString::Printf(TEXT("%.1f days"), DisplayRunwayDays);
	StatsText->SetText(FText::FromString(FString::Printf(
		TEXT("CASH  $%.0f\nRUNWAY  %s   ·   DAY %d"),
		DisplayCash, *Runway, DisplayDay)));
}

void USprawlNativeHUD::HandleGigStarted(const FSprawlGigStatus& Gig)
{
	CurrentGig = Gig;
	CompletedMessageRemaining = 0.f;
	RefreshObjective();
}

void USprawlNativeHUD::HandleGigCompleted(const FSprawlGigStatus& Gig)
{
	CurrentGig = FSprawlGigStatus();
	CompletedMessageRemaining = 4.f;
	if (ObjectiveText)
	{
		ObjectiveText->SetText(FText::Format(
			NSLOCTEXT("Sprawl", "GigCompleteHUD", "Gig complete  +${0}"),
			FText::AsNumber(FMath::RoundToInt(Gig.Payout))));
	}
}

void USprawlNativeHUD::HandlePhoneRinging(const FPhoneCall& Call)
{
	if (PhoneBorder && PhoneText)
	{
		PhoneText->SetText(FText::Format(
			NSLOCTEXT("Sprawl", "IncomingCallHUD", "Incoming call — {0} — [E] Answer"),
			Call.DisplayName));
		PhoneBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void USprawlNativeHUD::HandlePhoneAnswered(const FPhoneCall& Call)
{
	(void)Call;
	if (PhoneBorder)
	{
		PhoneBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USprawlNativeHUD::RefreshObjective()
{
	if (!ObjectiveText || CompletedMessageRemaining > 0.f)
	{
		return;
	}
	if (!CurrentGig.bActive)
	{
		ObjectiveText->SetText(NSLOCTEXT("Sprawl", "NoActiveGig",
			"Explore the city · another clean gig is coming"));
		return;
	}

	float DistanceMeters = 0.f;
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (const APawn* Pawn = PC->GetPawn())
		{
			DistanceMeters = FVector::Dist2D(
				Pawn->GetActorLocation(), CurrentGig.TargetLocation) / 100.f;
		}
	}
	ObjectiveText->SetText(FText::Format(
		NSLOCTEXT("Sprawl", "ActiveGigDistance", "{0}  ·  {1} m"),
		CurrentGig.Objective, FText::AsNumber(FMath::RoundToInt(DistanceMeters))));
}
