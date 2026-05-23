// The Connected Sprawl - Zarri's 20-Year-Old Hustle
// FounderSubsystem: Manages Zarri's startup finances, burn rate, and runway.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FounderSubsystem.generated.h"

/** Broad classification of income/expense events. */
UENUM(BlueprintType)
enum class ECashFlowSource : uint8
{
	// Income sources
	CleanContract   UMETA(DisplayName = "Clean Contract"),
	VCInvestment    UMETA(DisplayName = "VC Investment"),
	StreetJob       UMETA(DisplayName = "Street Job (Dirty)"),
	StreetLiquidity UMETA(DisplayName = "Street Liquidity"),
	RaceWinning     UMETA(DisplayName = "Race Winnings"),

	// Expense sources
	Payroll         UMETA(DisplayName = "Payroll"),
	OfficeRent      UMETA(DisplayName = "Office Rent"),
	VehicleUpkeep   UMETA(DisplayName = "Vehicle Upkeep"),
	LegalFees       UMETA(DisplayName = "Legal Fees"),
	BribesAndDebt   UMETA(DisplayName = "Bribes & Moral Debt")
};

/** A single cash event journal entry. */
USTRUCT(BlueprintType)
struct FCashFlowEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FDateTime Timestamp;
	UPROPERTY(BlueprintReadOnly) ECashFlowSource Source = ECashFlowSource::CleanContract;
	UPROPERTY(BlueprintReadOnly) float Amount = 0.f;      // positive = income, negative = expense
	UPROPERTY(BlueprintReadOnly) FText Note;
	UPROPERTY(BlueprintReadOnly) bool bIsDirty = false;   // raised heat / moral debt
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCashChanged, float, NewCash);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRunwayChanged, float, DaysRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStartupBankrupt);

/**
 * FounderSubsystem
 * ----------------
 * Tracks Zarri's startup economy:
 *   - Cash on hand
 *   - Monthly Burn Rate (payroll + rent + upkeep + ...)
 *   - Runway (Cash / DailyBurn)
 *
 * Ticks once per in-game day. When cash hits zero, the player is forced into
 * "dirty job" territory or risk bankruptcy.
 */
UCLASS()
class CONNECTEDSPRAWL_API UFounderSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// --- Subsystem lifecycle ---
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Called by the GameState timer once per in-game day. */
	UFUNCTION(BlueprintCallable, Category="Founder")
	void AdvanceDay();

	// --- Cash operations ---
	UFUNCTION(BlueprintCallable, Category="Founder|Cash")
	void AddIncome(float Amount, ECashFlowSource Source, const FText& Note, bool bIsDirty = false);

	UFUNCTION(BlueprintCallable, Category="Founder|Cash")
	void PayExpense(float Amount, ECashFlowSource Source, const FText& Note);

	// --- Burn rate mutation ---
	UFUNCTION(BlueprintCallable, Category="Founder|Burn")
	void AddRecurringExpense(ECashFlowSource Source, float DailyAmount);

	UFUNCTION(BlueprintCallable, Category="Founder|Burn")
	void RemoveRecurringExpense(ECashFlowSource Source);

	// --- Queries ---
	UFUNCTION(BlueprintPure, Category="Founder") float GetCash() const { return Cash; }
	UFUNCTION(BlueprintPure, Category="Founder") float GetDailyBurn() const;
	UFUNCTION(BlueprintPure, Category="Founder") float GetRunwayDays() const;
	UFUNCTION(BlueprintPure, Category="Founder") bool IsInDanger() const { return GetRunwayDays() < 7.f; }

	// --- Events ---
	UPROPERTY(BlueprintAssignable) FOnCashChanged    OnCashChanged;
	UPROPERTY(BlueprintAssignable) FOnRunwayChanged  OnRunwayChanged;
	UPROPERTY(BlueprintAssignable) FOnStartupBankrupt OnStartupBankrupt;

protected:
	/** Current cash on hand (USD). */
	UPROPERTY(VisibleAnywhere, Category="Founder") float Cash = 2500.f;

	/** Recurring daily expenses keyed by source (e.g. rent, payroll). */
	UPROPERTY() TMap<ECashFlowSource, float> RecurringDailyExpenses;

	/** Full journal of cash events for the UI "ledger" screen. */
	UPROPERTY() TArray<FCashFlowEntry> Ledger;

	void BroadcastRunway();
};
