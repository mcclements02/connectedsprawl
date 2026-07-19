// The Connected Sprawl - run-level win, bailout, and bankruptcy flow.

#include "Founder/SprawlGameFlowSubsystem.h"

#include "Factions/FactionSubsystem.h"
#include "Founder/FounderSubsystem.h"
#include "Missions/DecisionSubsystem.h"
#include "Missions/StrategicDecision.h"
#include "Subsystems/SubsystemCollection.h"

namespace
{
const FName DirtyBailoutId(TEXT("DirtyBailout"));
const FName TakeDirtyJobBranch(TEXT("TakeDirtyJob"));
const FName ShutDownBranch(TEXT("ShutDown"));
const FName VictorySentinel(TEXT("_SprawlVictory"));
const FName VictoryValue(TEXT("CashTargetReached"));
const FName GameOverSentinel(TEXT("_SprawlGameOver"));
const FName BankruptcyValue(TEXT("Bankruptcy"));
}

void USprawlGameFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Founder = Collection.InitializeDependency<UFounderSubsystem>();
	Decisions = Collection.InitializeDependency<UDecisionSubsystem>();

	if (Founder)
	{
		Founder->OnStartupBankrupt.AddDynamic(
			this, &USprawlGameFlowSubsystem::HandleStartupBankrupt);
		Founder->OnCashChanged.AddDynamic(
			this, &USprawlGameFlowSubsystem::HandleCashChanged);
	}
	if (Decisions)
	{
		Decisions->OnDecisionResolved.AddDynamic(
			this, &USprawlGameFlowSubsystem::HandleDecisionResolved);
	}
	RebuildFlagsFromSentinels();
}

void USprawlGameFlowSubsystem::Deinitialize()
{
	if (Founder)
	{
		Founder->OnStartupBankrupt.RemoveDynamic(
			this, &USprawlGameFlowSubsystem::HandleStartupBankrupt);
		Founder->OnCashChanged.RemoveDynamic(
			this, &USprawlGameFlowSubsystem::HandleCashChanged);
	}
	if (Decisions)
	{
		Decisions->OnDecisionResolved.RemoveDynamic(
			this, &USprawlGameFlowSubsystem::HandleDecisionResolved);
	}
	Founder = nullptr;
	Decisions = nullptr;
	BailoutDecision = nullptr;
	Super::Deinitialize();
}

void USprawlGameFlowSubsystem::NotifyGigCompleted()
{
	++CompletedGigCount;
}

void USprawlGameFlowSubsystem::ResetProgress()
{
	bRestoringProgress = false;
	bBailoutPending = false;
	bVictoryRecorded = false;
	bGameOver = false;
	CompletedGigCount = 0;
	GameOverReason = FText::GetEmpty();
}

void USprawlGameFlowSubsystem::BeginProgressRestore()
{
	bRestoringProgress = true;
}

void USprawlGameFlowSubsystem::EndProgressRestore(bool bOfferPendingBailout)
{
	bRestoringProgress = false;
	RefreshAfterLoad(bOfferPendingBailout);
	if (!bVictoryRecorded && !bGameOver && Founder
		&& Founder->GetCash() >= WinCashTarget)
	{
		TriggerVictory();
	}
}

void USprawlGameFlowSubsystem::RefreshAfterLoad(bool bOfferPendingBailout)
{
	RebuildFlagsFromSentinels();
	if (!bGameOver && Founder && Founder->GetCash() <= 0.f && bOfferPendingBailout)
	{
		HandleStartupBankrupt();
	}
}

bool USprawlGameFlowSubsystem::ResolveBailoutForAudit(bool bTakeDirtyJob)
{
	if (!Decisions || !bBailoutPending)
	{
		return false;
	}
	UStrategicDecision* Decision = GetOrCreateBailoutDecision();
	if (!Decision)
	{
		return false;
	}
	Decisions->ResolveDecision(
		Decision, bTakeDirtyJob ? TakeDirtyJobBranch : ShutDownBranch);
	return Decisions->HasResolvedDecision(DirtyBailoutId);
}

void USprawlGameFlowSubsystem::RebuildFlagsFromSentinels()
{
	bVictoryRecorded = Decisions && Decisions->HasResolvedDecision(VictorySentinel);
	bGameOver = Decisions &&
		(Decisions->HasResolvedDecision(GameOverSentinel) ||
		 Decisions->GetResolvedBranch(DirtyBailoutId) == ShutDownBranch);
	bBailoutPending = false;
	if (bGameOver && GameOverReason.IsEmpty())
	{
		GameOverReason = NSLOCTEXT("Sprawl", "PersistedBankruptcy",
			"The company ran out of cash and the bailout path is closed.");
	}
}

void USprawlGameFlowSubsystem::HandleCashChanged(float NewCash)
{
	if (!bRestoringProgress && !bVictoryRecorded && NewCash >= WinCashTarget)
	{
		TriggerVictory();
	}
}

void USprawlGameFlowSubsystem::HandleStartupBankrupt()
{
	if (bRestoringProgress || bGameOver || !Decisions)
	{
		return;
	}

	if (Decisions->HasResolvedDecision(DirtyBailoutId))
	{
		TriggerGameOver(NSLOCTEXT("Sprawl", "SecondBankruptcy",
			"The bailout is spent. With no cash left, the startup shuts down."));
		return;
	}

	if (!bBailoutPending)
	{
		OfferBailout();
	}
}

void USprawlGameFlowSubsystem::HandleDecisionResolved(
	UStrategicDecision* Decision, FName ChosenBranch)
{
	if (!Decision || Decision->DecisionId != DirtyBailoutId)
	{
		return;
	}

	bBailoutPending = false;
	if (ChosenBranch == ShutDownBranch)
	{
		TriggerGameOver(NSLOCTEXT("Sprawl", "ChoseShutdown",
			"Zarri chose to close the company instead of taking dirty money."));
	}
	else if (ChosenBranch == TakeDirtyJobBranch && Founder && Founder->GetCash() <= 0.f)
	{
		TriggerGameOver(NSLOCTEXT("Sprawl", "BailoutInsufficient",
			"The emergency job could not cover the company's debt."));
	}
}

UStrategicDecision* USprawlGameFlowSubsystem::GetOrCreateBailoutDecision()
{
	if (BailoutDecision)
	{
		return BailoutDecision;
	}

	BailoutDecision = Cast<UStrategicDecision>(BailoutDecisionAsset.TryLoad());
	if (BailoutDecision)
	{
		return BailoutDecision;
	}

	// Runtime fallback keeps bankruptcy playable even before the authoring pass
	// has produced the data asset in a fresh checkout.
	BailoutDecision = NewObject<UStrategicDecision>(this, TEXT("RuntimeDirtyBailout"));
	BailoutDecision->DecisionId = DirtyBailoutId;
	BailoutDecision->Title = NSLOCTEXT("Sprawl", "BailoutTitle", "Last Money on the Table");
	BailoutDecision->Prompt = NSLOCTEXT("Sprawl", "BailoutPrompt",
		"The account is empty. Marcus can float one dirty job, or Zarri can shut the company down cleanly.");
	BailoutDecision->ProposerContactId = TEXT("Marcus_Lows");
	BailoutDecision->ContextFaction = EFaction::Street;

	FDecisionBranch DirtyJob;
	DirtyJob.BranchId = TakeDirtyJobBranch;
	DirtyJob.Headline = NSLOCTEXT("Sprawl", "TakeDirtyJob", "Take the emergency street job.");
	DirtyJob.Pitch = NSLOCTEXT("Sprawl", "TakeDirtyJobPitch",
		"Two thousand now. Heat, obligation, and no second rescue.");
	DirtyJob.CashDelta = 2000.f;
	DirtyJob.StreetRepDelta = 5;
	DirtyJob.HeatDelta = 12;
	DirtyJob.MoralDebtDelta = 15;
	DirtyJob.bCashIsDirty = true;

	FDecisionBranch ShutDown;
	ShutDown.BranchId = ShutDownBranch;
	ShutDown.Headline = NSLOCTEXT("Sprawl", "ShutDown", "Shut the company down.");
	ShutDown.Pitch = NSLOCTEXT("Sprawl", "ShutDownPitch",
		"No dirty debt. No startup. This run ends here.");
	BailoutDecision->Branches = { DirtyJob, ShutDown };
	return BailoutDecision;
}

void USprawlGameFlowSubsystem::OfferBailout()
{
	UStrategicDecision* Decision = GetOrCreateBailoutDecision();
	if (!Decision || !Decisions)
	{
		TriggerGameOver(NSLOCTEXT("Sprawl", "BailoutUnavailable",
			"No bailout decision is available, so the company shuts down."));
		return;
	}

	bBailoutPending = true;
	Decisions->OfferDecision(Decision);
}

void USprawlGameFlowSubsystem::TriggerVictory()
{
	if (bVictoryRecorded || !Decisions)
	{
		return;
	}
	bVictoryRecorded = true;
	Decisions->MarkResolvedSentinel(VictorySentinel, VictoryValue);
	const FSprawlEndGameInfo Info = GetCurrentEndGameInfo();
	OnRunEnded.Broadcast(Info);
	OnFlowCheckpoint.Broadcast();
}

void USprawlGameFlowSubsystem::TriggerGameOver(const FText& Reason)
{
	if (bGameOver)
	{
		return;
	}
	bGameOver = true;
	bBailoutPending = false;
	GameOverReason = Reason;
	if (Decisions)
	{
		Decisions->MarkResolvedSentinel(GameOverSentinel, BankruptcyValue);
	}
	const FSprawlEndGameInfo Info = GetCurrentEndGameInfo();
	OnRunEnded.Broadcast(Info);
	OnFlowCheckpoint.Broadcast();
}

FSprawlEndGameInfo USprawlGameFlowSubsystem::GetCurrentEndGameInfo() const
{
	FSprawlEndGameInfo Info;
	Info.Outcome = bGameOver
		? ESprawlRunOutcome::Bankruptcy
		: (bVictoryRecorded ? ESprawlRunOutcome::Victory : ESprawlRunOutcome::Active);
	Info.Title = bGameOver
		? NSLOCTEXT("Sprawl", "BankruptcyTitle", "Startup Over")
		: NSLOCTEXT("Sprawl", "VictoryTitle", "Runway Secured");
	Info.Summary = bGameOver
		? GameOverReason
		: NSLOCTEXT("Sprawl", "VictorySummary",
			"You built $25,000 in cash. The city stays open if you want to keep playing.");
	Info.CompletedGigs = CompletedGigCount;
	if (Founder)
	{
		Info.DaysSurvived = FMath::Max(0, Founder->GetCurrentDay() - 1);
		Info.Cash = Founder->GetCash();
	}
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const UFactionSubsystem* Factions = GI->GetSubsystem<UFactionSubsystem>())
		{
			Info.Heat = Factions->GetHeat();
			Info.MoralDebt = Factions->GetMoralDebt();
		}
	}
	return Info;
}
