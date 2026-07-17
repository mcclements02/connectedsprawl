// The Connected Sprawl - Native decision modal.

#include "UI/SprawlDecisionModal.h"
#include "Missions/StrategicDecision.h"
#include "Factions/FactionTypes.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void USprawlBranchClickProxy::HandleClicked()
{
	if (Owner)
	{
		Owner->ChooseBranch(BranchId);
	}
}

void USprawlDecisionModal::PresentDecision(UStrategicDecision* InDecision)
{
	Super::PresentDecision(InDecision);
	BuildUI();
}

namespace
{
	FSlateFontInfo SizedFont(const UTextBlock* Text, int32 Size)
	{
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		return Font;
	}

	UTextBlock* MakeText(UWidgetTree* Tree, const FText& Content, int32 Size,
	                     const FLinearColor& Color, bool bWrap = false)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(Content);
		Text->SetFont(SizedFont(Text, Size));
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetAutoWrapText(bWrap);
		return Text;
	}
}

void USprawlDecisionModal::BuildUI()
{
	if (!CurrentDecision || !WidgetTree) return;

	ClickProxies.Reset();

	// Scrim covering the whole screen.
	UBorder* Scrim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Scrim"));
	Scrim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.60f));
	Scrim->SetHorizontalAlignment(HAlign_Center);
	Scrim->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Scrim;

	USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	CardSize->SetWidthOverride(760.f);
	Scrim->SetContent(CardSize);

	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Card->SetBrushColor(FLinearColor(0.020f, 0.024f, 0.035f, 0.97f));
	Card->SetPadding(FMargin(28.f));
	CardSize->AddChild(Card);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Card->SetContent(Column);

	// Faction-tinted contact line, title, prompt.
	FLinearColor FactionTint(0.65f, 0.65f, 0.70f);
	if (CurrentDecision->ContextFaction == EFaction::Corporate) FactionTint = FLinearColor(0.36f, 0.62f, 1.0f);
	if (CurrentDecision->ContextFaction == EFaction::Street)    FactionTint = FLinearColor(1.0f, 0.66f, 0.18f);

	const FText ContactLine = FText::FromString(
		TEXT("☎  ") + FName::NameToDisplayString(CurrentDecision->ProposerContactId.ToString(), false));

	auto AddRow = [&](UWidget* Widget, float TopPad)
	{
		UVerticalBoxSlot* RowSlot = Column->AddChildToVerticalBox(Widget);
		RowSlot->SetPadding(FMargin(0.f, TopPad, 0.f, 0.f));
		RowSlot->SetHorizontalAlignment(HAlign_Fill);
	};

	AddRow(MakeText(WidgetTree, ContactLine, 13, FactionTint), 0.f);
	AddRow(MakeText(WidgetTree, CurrentDecision->Title, 26, FLinearColor::White), 6.f);
	AddRow(MakeText(WidgetTree, CurrentDecision->Prompt, 15,
		FLinearColor(0.85f, 0.86f, 0.88f), /*bWrap*/ true), 12.f);

	// One button per branch: headline plus an effects summary line.
	for (const FDecisionBranch& Branch : CurrentDecision->Branches)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		Button->SetBackgroundColor(FLinearColor(0.10f, 0.13f, 0.18f));

		USprawlBranchClickProxy* Proxy = NewObject<USprawlBranchClickProxy>(this);
		Proxy->BranchId = Branch.BranchId;
		Proxy->Owner    = this;
		Button->OnClicked.AddDynamic(Proxy, &USprawlBranchClickProxy::HandleClicked);
		ClickProxies.Add(Proxy);

		const FString Effects = SummarizeBranchEffects(Branch);
		const FString Label   = Effects.IsEmpty()
			? Branch.Headline.ToString()
			: Branch.Headline.ToString() + TEXT("\n") + Effects;

		UTextBlock* ButtonText = MakeText(WidgetTree,
			FText::FromString(Label), 15, FLinearColor(0.92f, 0.93f, 0.95f), /*bWrap*/ true);
		ButtonText->SetJustification(ETextJustify::Center);
		Button->AddChild(ButtonText);
		if (UButtonSlot* TextSlot = Cast<UButtonSlot>(ButtonText->Slot))
		{
			TextSlot->SetPadding(FMargin(14.f, 10.f));
		}

		AddRow(Button, 14.f);
	}

	AddRow(MakeText(WidgetTree, NSLOCTEXT("Sprawl", "DecisionHint",
		"Every fork costs you something - money, trust, or soul."),
		11, FLinearColor(0.45f, 0.46f, 0.50f)), 16.f);
}

FString USprawlDecisionModal::SummarizeBranchEffects(const FDecisionBranch& Branch)
{
	TArray<FString> Parts;

	if (Branch.CashDelta != 0.f)
	{
		Parts.Add(FString::Printf(TEXT("%s$%.0f%s"),
			Branch.CashDelta > 0.f ? TEXT("+") : TEXT("−"),
			FMath::Abs(Branch.CashDelta),
			Branch.bCashIsDirty ? TEXT(" (dirty)") : TEXT("")));
	}
	if (Branch.DailyBurnDelta != 0.f)
	{
		Parts.Add(FString::Printf(TEXT("burn %+.0f/day"), Branch.DailyBurnDelta));
	}
	if (Branch.CorporateRepDelta) Parts.Add(FString::Printf(TEXT("corp %+d"), Branch.CorporateRepDelta));
	if (Branch.StreetRepDelta)    Parts.Add(FString::Printf(TEXT("street %+d"), Branch.StreetRepDelta));
	if (Branch.HeatDelta)         Parts.Add(FString::Printf(TEXT("heat %+d"), Branch.HeatDelta));
	if (Branch.MoralDebtDelta)    Parts.Add(FString::Printf(TEXT("debt %+d"), Branch.MoralDebtDelta));

	return FString::Join(Parts, TEXT("  ·  "));
}
