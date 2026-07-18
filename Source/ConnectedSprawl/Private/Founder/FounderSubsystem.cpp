// The Connected Sprawl - Zarri's 20-Year-Old Hustle

#include "Founder/FounderSubsystem.h"

void UFounderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ResetProgress();

	UE_LOG(LogTemp, Log, TEXT("[Founder] Initialized. Cash=$%.0f DailyBurn=$%.0f Runway=%.1f days"),
		Cash, GetDailyBurn(), GetRunwayDays());
}

void UFounderSubsystem::Deinitialize()
{
	Ledger.Empty();
	RecurringDailyExpenses.Empty();
	Super::Deinitialize();
}

void UFounderSubsystem::AdvanceDay()
{
	const float DailyBurn = GetDailyBurn();
	if (DailyBurn > 0.f)
	{
		for (const TPair<ECashFlowSource, float>& Pair : RecurringDailyExpenses)
		{
			PayExpense(Pair.Value, Pair.Key, FText::FromString(TEXT("Daily recurring")));
		}
	}

	++CurrentDay;
	BroadcastRunway();

	if (Cash <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Founder] BANKRUPT! Cash=$%.0f — forced dirty job incoming."), Cash);
		OnStartupBankrupt.Broadcast();
	}

	OnDayAdvanced.Broadcast(CurrentDay);
}

void UFounderSubsystem::AddIncome(float Amount, ECashFlowSource Source, const FText& Note, bool bIsDirty)
{
	if (Amount <= 0.f) return;

	Cash += Amount;

	FCashFlowEntry Entry;
	Entry.Timestamp = FDateTime::UtcNow();
	Entry.Source    = Source;
	Entry.Amount    = Amount;
	Entry.Note      = Note;
	Entry.bIsDirty  = bIsDirty;
	Ledger.Add(Entry);

	OnCashChanged.Broadcast(Cash);
	BroadcastRunway();
}

void UFounderSubsystem::PayExpense(float Amount, ECashFlowSource Source, const FText& Note)
{
	if (Amount <= 0.f) return;

	Cash -= Amount;

	FCashFlowEntry Entry;
	Entry.Timestamp = FDateTime::UtcNow();
	Entry.Source    = Source;
	Entry.Amount    = -Amount;
	Entry.Note      = Note;
	Entry.bIsDirty  = false;
	Ledger.Add(Entry);

	OnCashChanged.Broadcast(Cash);
	BroadcastRunway();
}

void UFounderSubsystem::AddRecurringExpense(ECashFlowSource Source, float DailyAmount)
{
	RecurringDailyExpenses.FindOrAdd(Source) = DailyAmount;
	BroadcastRunway();
}

void UFounderSubsystem::AdjustRecurringExpense(ECashFlowSource Source, float DailyDelta)
{
	float& Amount = RecurringDailyExpenses.FindOrAdd(Source);
	Amount = FMath::Max(0.f, Amount + DailyDelta);
	BroadcastRunway();
}

void UFounderSubsystem::RemoveRecurringExpense(ECashFlowSource Source)
{
	RecurringDailyExpenses.Remove(Source);
	BroadcastRunway();
}

float UFounderSubsystem::GetDailyBurn() const
{
	float Total = 0.f;
	for (const TPair<ECashFlowSource, float>& Pair : RecurringDailyExpenses)
	{
		Total += Pair.Value;
	}
	return Total;
}

float UFounderSubsystem::GetRunwayDays() const
{
	const float Burn = GetDailyBurn();
	if (Burn <= KINDA_SMALL_NUMBER) return TNumericLimits<float>::Max();
	return FMath::Max(Cash, 0.f) / Burn;
}

FFounderPersistentState UFounderSubsystem::CaptureState() const
{
	FFounderPersistentState State;
	State.Cash = Cash;
	State.CurrentDay = CurrentDay;
	State.RecurringDailyExpenses = RecurringDailyExpenses;
	State.Ledger = Ledger;
	return State;
}

void UFounderSubsystem::RestoreState(const FFounderPersistentState& State)
{
	Cash = State.Cash;
	CurrentDay = FMath::Max(1, State.CurrentDay);
	RecurringDailyExpenses = State.RecurringDailyExpenses;
	Ledger = State.Ledger;

	OnCashChanged.Broadcast(Cash);
	BroadcastRunway();
}

void UFounderSubsystem::ResetProgress()
{
	Cash = 2500.f;
	CurrentDay = 1;
	Ledger.Reset();
	RecurringDailyExpenses.Reset();

	// Baseline startup: modest rent + a part-time engineer.
	RecurringDailyExpenses.Add(ECashFlowSource::OfficeRent, 40.f);
	RecurringDailyExpenses.Add(ECashFlowSource::Payroll, 120.f);
	RecurringDailyExpenses.Add(ECashFlowSource::VehicleUpkeep, 15.f);

	OnCashChanged.Broadcast(Cash);
	BroadcastRunway();
}

void UFounderSubsystem::BroadcastRunway()
{
	OnRunwayChanged.Broadcast(GetRunwayDays());
}
