// The Connected Sprawl - HUD.
// Displays the three numbers that matter: Cash, Runway (days), Police Heat.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Factions/FactionTypes.h"
#include "SprawlHUDWidget.generated.h"

/**
 * USprawlHUDWidget
 * ----------------
 * C++ base class for the HUD. Designers inherit in UMG (WBP_SprawlHUD) and
 * bind these three exposed text/property values to on-screen elements.
 *
 * Internally subscribes to FounderSubsystem and FactionSubsystem events so
 * updates are push-based (no tick polling).
 */
UCLASS(Abstract)
class CONNECTEDSPRAWL_API USprawlHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// BindWidget text blocks can be hooked in BP; these are the properties BP can consume.
	UPROPERTY(BlueprintReadOnly, Category="HUD") float DisplayCash = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="HUD") float DisplayRunwayDays = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="HUD") int32 DisplayHeat = 0;
	UPROPERTY(BlueprintReadOnly, Category="HUD") int32 DisplayMoralDebt = 0;
	UPROPERTY(BlueprintReadOnly, Category="HUD") int32 DisplayCorporateRep = 0;
	UPROPERTY(BlueprintReadOnly, Category="HUD") int32 DisplayStreetRep = 0;
	UPROPERTY(BlueprintReadOnly, Category="HUD") int32 DisplayDay = 1;

	/** Fired when any value changes so UMG can refresh any bindings. */
	UFUNCTION(BlueprintImplementableEvent, Category="HUD")
	void OnHUDRefreshed();

protected:
	/** C++ widgets update here while the existing BP event remains compatible. */
	virtual void NativeRefresh();
	void RefreshHUD();

	UFUNCTION() void OnCashChanged(float NewCash);
	UFUNCTION() void OnRunwayChanged(float NewDays);
	UFUNCTION() void OnDayAdvanced(int32 NewDay);
	UFUNCTION() void OnHeatChanged(int32 NewHeat);
	UFUNCTION() void OnMoralDebtChanged(int32 NewDebt);
	UFUNCTION() void OnReputationChanged(EFaction Faction, int32 NewValue);
};
