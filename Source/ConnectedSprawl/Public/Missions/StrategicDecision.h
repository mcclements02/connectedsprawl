// The Connected Sprawl - Missions aren't tasks, they are Strategic Decisions.
// Every job Zarri accepts shifts his company, his reputation, and his soul.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Factions/FactionTypes.h"
#include "Founder/FounderSubsystem.h"
#include "StrategicDecision.generated.h"

/** One branch of a decision — e.g. "Take the VC check" vs "Take the street gig". */
USTRUCT(BlueprintType)
struct FDecisionBranch
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName BranchId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Headline;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(MultiLine=true)) FText Pitch;

	// --- Immediate mechanical effects if the player chooses this branch ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CashDelta = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float DailyBurnDelta = 0.f; // e.g. new rent, new hire
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 CorporateRepDelta = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 StreetRepDelta = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 HeatDelta = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MoralDebtDelta = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool  bCashIsDirty = false;

	/** Optional follow-up mission seeded by this branch. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName UnlocksDecisionId;
};

/**
 * UStrategicDecision
 * ------------------
 * A data-driven mission. Defined as a Primary Data Asset so designers can stamp
 * new decisions from the editor without touching code.
 *
 * Example:
 *   - Id: "FirstVC"
 *   - Prompt: "Iron Forest Ventures wants a 15% stake for $50k."
 *   - Branches:
 *       * AcceptVC   -> +50000 Cash, +25 CorpRep, -10 StreetRep, +80 DailyBurn
 *       * Counter    -> negotiation mini-game
 *       * StreetLoan -> +25000 Cash (dirty), +20 StreetRep, +15 MoralDebt
 */
UCLASS(BlueprintType)
class CONNECTEDSPRAWL_API UStrategicDecision : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName DecisionId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine=true)) FText Prompt;

	/** The NPC proposing this decision. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ProposerContactId;

	/** Zone where this decision is triggered geographically. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EFaction ContextFaction = EFaction::Neutral;

	/** Two to four branches per decision (usually two — the fork is the point). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FDecisionBranch> Branches;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("StrategicDecision"), DecisionId);
	}
};
