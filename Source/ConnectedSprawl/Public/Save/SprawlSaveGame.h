// The Connected Sprawl - versioned progression save payload.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Factions/FactionSubsystem.h"
#include "Founder/FounderSubsystem.h"
#include "SprawlSaveGame.generated.h"

/** Serializable state only; runtime orchestration lives in USprawlSaveSubsystem. */
UCLASS()
class CONNECTEDSPRAWL_API USprawlSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 LatestSaveVersion = 1;

	UPROPERTY(SaveGame) int32 SaveVersion = LatestSaveVersion;
	UPROPERTY(SaveGame) FDateTime SavedAtUtc;
	UPROPERTY(SaveGame) FFounderPersistentState Founder;
	UPROPERTY(SaveGame) FFactionPersistentState Factions;
	UPROPERTY(SaveGame) TMap<FName, FName> ResolvedDecisionBranches;
	UPROPERTY(SaveGame) FName MapName = NAME_None;
	UPROPERTY(SaveGame) FTransform PlayerTransform = FTransform::Identity;
	UPROPERTY(SaveGame) bool bHasPlayerTransform = false;
	UPROPERTY(SaveGame) bool bWasDriving = false;
};
