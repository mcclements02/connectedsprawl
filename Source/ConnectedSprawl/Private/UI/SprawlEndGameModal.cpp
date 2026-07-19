// The Connected Sprawl - native victory/bankruptcy modal.

#include "UI/SprawlEndGameModal.h"

#include "Save/SprawlSaveSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
FSlateFontInfo EndGameSizedFont(const UTextBlock* Text, int32 Size)
{
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = Size;
	return Font;
}

UTextBlock* MakeEndGameText(UWidgetTree* Tree, const FText& Content, int32 Size,
	const FLinearColor& Color, ETextJustify::Type Justification = ETextJustify::Center)
{
	UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Text->SetText(Content);
	Text->SetFont(EndGameSizedFont(Text, Size));
	Text->SetColorAndOpacity(FSlateColor(Color));
	Text->SetJustification(Justification);
	Text->SetAutoWrapText(true);
	return Text;
}
}

void USprawlEndGameModal::PresentEndGame(const FSprawlEndGameInfo& Info)
{
	CurrentInfo = Info;
	BuildUI();
}

void USprawlEndGameModal::BuildUI()
{
	if (!WidgetTree)
	{
		return;
	}
	WidgetTree->RootWidget = nullptr;

	UBorder* Scrim = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("EndGameScrim"));
	Scrim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.72f));
	Scrim->SetHorizontalAlignment(HAlign_Center);
	Scrim->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Scrim;

	USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	CardSize->SetWidthOverride(680.f);
	Scrim->SetContent(CardSize);

	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	const bool bVictory = CurrentInfo.Outcome == ESprawlRunOutcome::Victory;
	Card->SetBrushColor(bVictory
		? FLinearColor(0.025f, 0.09f, 0.11f, 0.98f)
		: FLinearColor(0.12f, 0.025f, 0.025f, 0.98f));
	Card->SetPadding(FMargin(32.f));
	CardSize->AddChild(Card);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Card->SetContent(Column);
	auto AddRow = [&](UWidget* Widget, float TopPadding)
	{
		if (UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(Widget))
		{
			Slot->SetPadding(FMargin(0.f, TopPadding, 0.f, 0.f));
			Slot->SetHorizontalAlignment(HAlign_Fill);
		}
	};

	AddRow(MakeEndGameText(WidgetTree, CurrentInfo.Title, 32,
		bVictory ? FLinearColor(0.35f, 0.92f, 0.96f) : FLinearColor(1.f, 0.48f, 0.38f)), 0.f);
	AddRow(MakeEndGameText(WidgetTree, CurrentInfo.Summary, 17,
		FLinearColor(0.9f, 0.91f, 0.93f)), 14.f);
	const FText Stats = FText::FromString(FString::Printf(
		TEXT("Days survived: %d   ·   Gigs: %d\nCash: $%.0f   ·   Heat: %d   ·   Moral debt: %d"),
		CurrentInfo.DaysSurvived, CurrentInfo.CompletedGigs, CurrentInfo.Cash,
		CurrentInfo.Heat, CurrentInfo.MoralDebt));
	AddRow(MakeEndGameText(WidgetTree, Stats, 15,
		FLinearColor(0.68f, 0.72f, 0.78f)), 18.f);

	auto AddButton = [&](const FText& Label) -> UButton*
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		Button->SetBackgroundColor(FLinearColor(0.10f, 0.21f, 0.27f));
		UTextBlock* LabelText = MakeEndGameText(
			WidgetTree, Label, 17, FLinearColor::White);
		Button->AddChild(LabelText);
		if (UButtonSlot* Slot = Cast<UButtonSlot>(LabelText->Slot))
		{
			Slot->SetPadding(FMargin(14.f, 11.f));
		}
		AddRow(Button, 14.f);
		return Button;
	};

	if (bVictory)
	{
		UButton* Continue = AddButton(
			NSLOCTEXT("Sprawl", "KeepPlaying", "Keep Playing"));
		Continue->OnClicked.AddDynamic(
			this, &USprawlEndGameModal::HandleContinueClicked);
	}
	else
	{
		UButton* StartOver = AddButton(
			NSLOCTEXT("Sprawl", "StartOver", "Start Over"));
		StartOver->OnClicked.AddDynamic(
			this, &USprawlEndGameModal::HandleStartOverClicked);
	}
	UButton* Quit = AddButton(NSLOCTEXT("Sprawl", "QuitGame", "Quit"));
	Quit->OnClicked.AddDynamic(this, &USprawlEndGameModal::HandleQuitClicked);
}

void USprawlEndGameModal::HandleContinueClicked()
{
	RemoveFromParent();
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetPause(false);
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void USprawlEndGameModal::HandleStartOverClicked()
{
	bool bReset = false;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (USprawlSaveSubsystem* Saves = GI->GetSubsystem<USprawlSaveSubsystem>())
		{
			bReset = Saves->StartNewGame();
		}
	}
	if (!bReset)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameFlow] Start Over aborted: autosave reset failed"));
		return;
	}
	const FString CurrentMap = UGameplayStatics::GetCurrentLevelName(this, true);
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetPause(false);
	}
	UGameplayStatics::OpenLevel(this,
		CurrentMap.IsEmpty() ? FName(TEXT("TestMap")) : FName(*CurrentMap));
}

void USprawlEndGameModal::HandleQuitClicked()
{
	UKismetSystemLibrary::QuitGame(
		GetWorld(), GetOwningPlayer(), EQuitPreference::Quit, false);
}
