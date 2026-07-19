// The Connected Sprawl - run-level win, bailout, and bankruptcy flow.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SprawlGameFlowSubsystem.generated.h"

class UDecisionSubsystem;
class UFounderSubsystem;
class UStrategicDecision;

UENUM(BlueprintType)
enum class ESprawlRunOutcome : uint8
{
	Active,
	Victory,
	Bankruptcy
};

USTRUCT(BlueprintType)
struct FSprawlEndGameInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) ESprawlRunOutcome Outcome = ESprawlRunOutcome::Active;
	UPROPERTY(BlueprintReadOnly) FText Title;
	UPROPERTY(BlueprintReadOnly) FText Summary;
	UPROPERTY(BlueprintReadOnly) int32 DaysSurvived = 0;
	UPROPERTY(BlueprintReadOnly) int32 CompletedGigs = 0;
	UPROPERTY(BlueprintReadOnly) float Cash = 0.f;
	UPROPERTY(BlueprintReadOnly) int32 Heat = 0;
	UPROPERTY(BlueprintReadOnly) int32 MoralDebt = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnSprawlRunEnded, const FSprawlEndGameInfo&, Info);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSprawlFlowCheckpoint);

/**
 * Coordinates the closed earn -> burn -> win/lose loop. Persistent one-shot
 * flags live in UDecisionSubsystem's existing resolved map, so this adds no
 * save-schema burden.
 */
UCLASS(Config=Game)
class CONNECTEDSPRAWL_API USprawlGameFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category="Game Flow")
	bool IsGameOver() const { return bGameOver; }

	UFUNCTION(BlueprintPure, Category="Game Flow")
	bool HasWon() const { return bVictoryRecorded; }

	UFUNCTION(BlueprintPure, Category="Game Flow")
	bool IsBailoutPending() const { return bBailoutPending; }

	UFUNCTION(BlueprintPure, Category="Game Flow")
	bool CanAdvanceDay() const { return !bGameOver && !bBailoutPending; }

	FSprawlEndGameInfo GetCurrentEndGameInfo() const;
	void NotifyGigCompleted();
	void ResetProgress();
	void BeginProgressRestore();
	void EndProgressRestore(bool bOfferPendingBailout = false);

	/** Re-derive one-shot state after a load; optionally re-offer an owed bailout. */
	void RefreshAfterLoad(bool bOfferPendingBailout = false);

	/** Deterministic decision hook used only by -SprawlProgressionAudit. */
	bool ResolveBailoutForAudit(bool bTakeDirtyJob);

	UPROPERTY(BlueprintAssignable, Category="Game Flow")
	FOnSprawlRunEnded OnRunEnded;

	/** Persistence listens to terminal checkpoints and saves immediately. */
	UPROPERTY(BlueprintAssignable, Category="Game Flow")
	FOnSprawlFlowCheckpoint OnFlowCheckpoint;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Game Flow")
	float WinCashTarget = 25000.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Game Flow")
	FSoftObjectPath BailoutDecisionAsset =
		FSoftObjectPath(TEXT("/Game/Missions/Decisions/DA_DirtyBailout.DA_DirtyBailout"));

private:
	UFUNCTION() void HandleStartupBankrupt();
	UFUNCTION() void HandleCashChanged(float NewCash);
	UFUNCTION() void HandleDecisionResolved(
		UStrategicDecision* Decision, FName ChosenBranch);

	UStrategicDecision* GetOrCreateBailoutDecision();
	void OfferBailout();
	void TriggerVictory();
	void TriggerGameOver(const FText& Reason);
	void RebuildFlagsFromSentinels();

	UPROPERTY(Transient) TObjectPtr<UFounderSubsystem> Founder;
	UPROPERTY(Transient) TObjectPtr<UDecisionSubsystem> Decisions;
	UPROPERTY(Transient) TObjectPtr<UStrategicDecision> BailoutDecision;

	bool bBailoutPending = false;
	bool bVictoryRecorded = false;
	bool bGameOver = false;
	bool bRestoringProgress = false;
	int32 CompletedGigCount = 0;
	FText GameOverReason;
};
