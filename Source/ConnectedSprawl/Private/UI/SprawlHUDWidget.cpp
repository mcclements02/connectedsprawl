// The Connected Sprawl - HUD.

#include "UI/SprawlHUDWidget.h"
#include "Founder/FounderSubsystem.h"
#include "Factions/FactionSubsystem.h"

void USprawlHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	if (UFounderSubsystem* Founder = GI->GetSubsystem<UFounderSubsystem>())
	{
		Founder->OnCashChanged.AddDynamic(this, &USprawlHUDWidget::OnCashChanged);
		Founder->OnRunwayChanged.AddDynamic(this, &USprawlHUDWidget::OnRunwayChanged);
		Founder->OnDayAdvanced.AddDynamic(this, &USprawlHUDWidget::OnDayAdvanced);

		// Seed initial values.
		DisplayCash = Founder->GetCash();
		DisplayRunwayDays = Founder->GetRunwayDays();
		DisplayDay = Founder->GetCurrentDay();
	}

	if (UFactionSubsystem* Factions = GI->GetSubsystem<UFactionSubsystem>())
	{
		Factions->OnHeatChanged.AddDynamic(this, &USprawlHUDWidget::OnHeatChanged);
		Factions->OnMoralDebtChanged.AddDynamic(this, &USprawlHUDWidget::OnMoralDebtChanged);
		Factions->OnReputationChanged.AddDynamic(this, &USprawlHUDWidget::OnReputationChanged);

		DisplayHeat         = Factions->GetHeat();
		DisplayMoralDebt    = Factions->GetMoralDebt();
		DisplayCorporateRep = Factions->GetReputation(EFaction::Corporate);
		DisplayStreetRep    = Factions->GetReputation(EFaction::Street);
	}

	RefreshHUD();
}

void USprawlHUDWidget::NativeDestruct()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFounderSubsystem* Founder = GI->GetSubsystem<UFounderSubsystem>())
		{
			Founder->OnCashChanged.RemoveDynamic(this, &USprawlHUDWidget::OnCashChanged);
			Founder->OnRunwayChanged.RemoveDynamic(this, &USprawlHUDWidget::OnRunwayChanged);
			Founder->OnDayAdvanced.RemoveDynamic(this, &USprawlHUDWidget::OnDayAdvanced);
		}
		if (UFactionSubsystem* Factions = GI->GetSubsystem<UFactionSubsystem>())
		{
			Factions->OnHeatChanged.RemoveDynamic(this, &USprawlHUDWidget::OnHeatChanged);
			Factions->OnMoralDebtChanged.RemoveDynamic(this, &USprawlHUDWidget::OnMoralDebtChanged);
			Factions->OnReputationChanged.RemoveDynamic(this, &USprawlHUDWidget::OnReputationChanged);
		}
	}
	Super::NativeDestruct();
}

void USprawlHUDWidget::NativeRefresh()
{
}

void USprawlHUDWidget::RefreshHUD()
{
	NativeRefresh();
	OnHUDRefreshed();
}

void USprawlHUDWidget::OnCashChanged(float NewCash)     { DisplayCash = NewCash;         RefreshHUD(); }
void USprawlHUDWidget::OnRunwayChanged(float NewDays)   { DisplayRunwayDays = NewDays;   RefreshHUD(); }
void USprawlHUDWidget::OnDayAdvanced(int32 NewDay)      { DisplayDay = NewDay;           RefreshHUD(); }
void USprawlHUDWidget::OnHeatChanged(int32 NewHeat)     { DisplayHeat = NewHeat;         RefreshHUD(); }
void USprawlHUDWidget::OnMoralDebtChanged(int32 NewDbt) { DisplayMoralDebt = NewDbt;     RefreshHUD(); }

void USprawlHUDWidget::OnReputationChanged(EFaction Faction, int32 NewValue)
{
	if (Faction == EFaction::Corporate) DisplayCorporateRep = NewValue;
	else if (Faction == EFaction::Street) DisplayStreetRep = NewValue;
	RefreshHUD();
}
