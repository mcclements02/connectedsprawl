// The Connected Sprawl - durable founder, faction, mission, and location state.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SprawlSaveSubsystem.generated.h"

class APlayerController;
class UStrategicDecision;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProgressSaved, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProgressLoaded, bool, bSuccess);

/**
 * Small synchronous saves are deliberate here: iOS can suspend the process
 * immediately after the background delegate, so progression must be durable
 * before control returns to the platform.
 */
UCLASS(Config=Game)
class CONNECTEDSPRAWL_API USprawlSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="Progression")
	bool SaveProgress();

	UFUNCTION(BlueprintCallable, Category="Progression")
	bool LoadProgress();

	/** Load persistent state and restart the current/saved map to rebuild mission flow. */
	UFUNCTION(BlueprintCallable, Category="Progression")
	bool LoadProgressAndRestart();

	UFUNCTION(BlueprintPure, Category="Progression")
	bool HasSave() const;

	/** Delete the autosave and return every progression subsystem to baseline. */
	UFUNCTION(BlueprintCallable, Category="Progression")
	bool StartNewGame();

	/** Called after the default pawn and authored player car exist. */
	bool ApplyPendingPlayerState(APlayerController* PlayerController);

	/** Custom-slot operations used by the isolated runtime round-trip audit. */
	bool SaveProgressToSlot(const FString& SlotName);
	bool LoadProgressFromSlot(const FString& SlotName, bool bRestorePlayerState);
	bool DeleteProgressInSlot(const FString& SlotName) const;
	bool RunRoundTripAudit(FString& OutSummary);

	UPROPERTY(BlueprintAssignable, Category="Progression") FOnProgressSaved OnProgressSaved;
	UPROPERTY(BlueprintAssignable, Category="Progression") FOnProgressLoaded OnProgressLoaded;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Progression")
	FString DefaultSlotName = TEXT("ConnectedSprawl_Autosave");

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Progression") bool bAutoLoad = true;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Progression") bool bAutosaveAfterDecision = true;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Progression") bool bAutosaveAfterDay = true;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Progression") bool bAutosaveOnBackground = true;

private:
	UFUNCTION() void HandleDecisionResolved(UStrategicDecision* Decision, FName ChosenBranch);
	UFUNCTION() void HandleDayAdvanced(int32 NewDay);
	UFUNCTION() void HandleGameFlowCheckpoint();
	void HandleApplicationWillEnterBackground();
	bool ShouldSuppressAutomaticPersistence() const;
	FString ResolveSlotName(const FString& SlotName) const;

	FDelegateHandle BackgroundDelegateHandle;
	FTransform PendingPlayerTransform = FTransform::Identity;
	FName PendingMapName = NAME_None;
	bool bHasPendingPlayerTransform = false;
	bool bPendingWasDriving = false;
	bool bIsRestoringProgress = false;
};
