// The Connected Sprawl - Drives pending Strategic Decisions and applies their outcomes.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DecisionSubsystem.generated.h"

class UStrategicDecision;
struct FDecisionBranch;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDecisionOffered, UStrategicDecision*, Decision);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDecisionResolved, UStrategicDecision*, Decision, FName, ChosenBranch);

/**
 * UDecisionSubsystem
 * ------------------
 * Orchestrates Strategic Decisions:
 *   1. A trigger (zone entry, phone call, scripted event) offers a decision
 *   2. UI presents the branches to the player
 *   3. Player selects a branch
 *   4. This subsystem applies mechanical effects through FounderSubsystem +
 *      FactionSubsystem and broadcasts resolution
 */
UCLASS()
class CONNECTEDSPRAWL_API UDecisionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Decisions")
	void OfferDecision(UStrategicDecision* Decision);

	UFUNCTION(BlueprintCallable, Category="Decisions")
	void ResolveDecision(UStrategicDecision* Decision, FName ChosenBranchId);

	UFUNCTION(BlueprintPure, Category="Decisions")
	bool HasResolvedDecision(FName DecisionId) const;

	UFUNCTION(BlueprintPure, Category="Decisions")
	FName GetResolvedBranch(FName DecisionId) const;

	TMap<FName, FName> CaptureResolvedDecisions() const { return ResolvedBranches; }
	void RestoreResolvedDecisions(const TMap<FName, FName>& State);
	void ResetProgress();

	/** Persist a one-shot system flag through the existing resolved-decision map. */
	void MarkResolvedSentinel(FName SentinelId, FName SentinelValue);

	UPROPERTY(BlueprintAssignable) FOnDecisionOffered OnDecisionOffered;
	UPROPERTY(BlueprintAssignable) FOnDecisionResolved OnDecisionResolved;

private:
	void ApplyBranchEffects(const FDecisionBranch& Branch);

	/** Decision id -> chosen branch. Prevents repeat rewards after loading. */
	UPROPERTY() TMap<FName, FName> ResolvedBranches;
};
