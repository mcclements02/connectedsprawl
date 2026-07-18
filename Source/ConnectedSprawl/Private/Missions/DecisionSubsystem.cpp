// The Connected Sprawl - Strategic Decisions subsystem.

#include "Missions/DecisionSubsystem.h"
#include "Missions/StrategicDecision.h"
#include "Founder/FounderSubsystem.h"
#include "Factions/FactionSubsystem.h"

void UDecisionSubsystem::OfferDecision(UStrategicDecision* Decision)
{
	if (!Decision) return;
	if (HasResolvedDecision(Decision->DecisionId))
	{
		UE_LOG(LogTemp, Log, TEXT("[Decision] Ignoring already-resolved offer: %s"),
			*Decision->DecisionId.ToString());
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("[Decision] Offered: %s"), *Decision->DecisionId.ToString());
	OnDecisionOffered.Broadcast(Decision);
}

void UDecisionSubsystem::ResolveDecision(UStrategicDecision* Decision, FName ChosenBranchId)
{
	if (!Decision) return;
	if (HasResolvedDecision(Decision->DecisionId))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Decision] Refusing duplicate resolution: %s"),
			*Decision->DecisionId.ToString());
		return;
	}

	const FDecisionBranch* Branch = Decision->Branches.FindByPredicate(
		[&](const FDecisionBranch& B) { return B.BranchId == ChosenBranchId; });
	if (!Branch)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Decision] Invalid branch %s for decision %s"),
			*ChosenBranchId.ToString(), *Decision->DecisionId.ToString());
		return;
	}

	ApplyBranchEffects(*Branch);
	ResolvedBranches.Add(Decision->DecisionId, ChosenBranchId);
	UE_LOG(LogTemp, Log, TEXT("[Decision] Resolved %s -> %s"),
		*Decision->DecisionId.ToString(), *ChosenBranchId.ToString());

	OnDecisionResolved.Broadcast(Decision, ChosenBranchId);
}

bool UDecisionSubsystem::HasResolvedDecision(FName DecisionId) const
{
	return ResolvedBranches.Contains(DecisionId);
}

FName UDecisionSubsystem::GetResolvedBranch(FName DecisionId) const
{
	if (const FName* Branch = ResolvedBranches.Find(DecisionId))
	{
		return *Branch;
	}
	return NAME_None;
}

void UDecisionSubsystem::RestoreResolvedDecisions(const TMap<FName, FName>& State)
{
	ResolvedBranches = State;
}

void UDecisionSubsystem::ResetProgress()
{
	ResolvedBranches.Reset();
}

void UDecisionSubsystem::ApplyBranchEffects(const FDecisionBranch& Branch)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	if (UFounderSubsystem* Founder = GI->GetSubsystem<UFounderSubsystem>())
	{
		if (Branch.CashDelta > 0.f)
		{
			const ECashFlowSource Src = Branch.bCashIsDirty
				? ECashFlowSource::StreetJob
				: ECashFlowSource::CleanContract;
			Founder->AddIncome(Branch.CashDelta, Src,
				FText::FromName(Branch.BranchId), Branch.bCashIsDirty);
		}
		else if (Branch.CashDelta < 0.f)
		{
			Founder->PayExpense(-Branch.CashDelta, ECashFlowSource::LegalFees,
				FText::FromName(Branch.BranchId));
		}

		if (!FMath::IsNearlyZero(Branch.DailyBurnDelta))
		{
			// Track as payroll adjustment (designer-facing sources can be
			// split later). Delta-based: other burn sources stay untouched.
			Founder->AdjustRecurringExpense(ECashFlowSource::Payroll, Branch.DailyBurnDelta);
		}
	}

	if (UFactionSubsystem* Factions = GI->GetSubsystem<UFactionSubsystem>())
	{
		if (Branch.CorporateRepDelta) Factions->ModifyReputation(EFaction::Corporate, Branch.CorporateRepDelta);
		if (Branch.StreetRepDelta)    Factions->ModifyReputation(EFaction::Street,    Branch.StreetRepDelta);
		if (Branch.HeatDelta > 0)     Factions->AddHeat(Branch.HeatDelta);
		if (Branch.HeatDelta < 0)     Factions->CoolHeat(-Branch.HeatDelta);
		if (Branch.MoralDebtDelta > 0) Factions->IncurMoralDebt(Branch.MoralDebtDelta);
		if (Branch.MoralDebtDelta < 0) Factions->RepayMoralDebt(-Branch.MoralDebtDelta);
	}
}
