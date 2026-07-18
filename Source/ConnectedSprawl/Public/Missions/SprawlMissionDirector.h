// The Connected Sprawl - Mission director.
// Seeds the DRIVE -> DECIDE -> EARN -> BURN loop: finds authored Strategic
// Decisions, rings Zarri's phone with the opener, and chains follow-ups from
// whichever branch the player picks.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Phone/PhoneSubsystem.h"
#include "SprawlMissionDirector.generated.h"

class UStrategicDecision;
class UDecisionSubsystem;

/**
 * USprawlMissionDirector
 * ----------------------
 * World subsystem that turns authored UStrategicDecision assets into a living
 * mission flow:
 *   1. On BeginPlay, indexes every StrategicDecision asset under /Game.
 *   2. Schedules the opening phone call (OpeningDecisionId, default "FirstVC").
 *   3. Auto-answers after RingAnswerSeconds so the loop works before any
 *      phone UI exists (a UI that answers first simply wins the race).
 *   4. On decision resolution, follows the chosen branch's UnlocksDecisionId
 *      and schedules the next call.
 *
 * All pacing knobs are Config so DefaultGame.ini can retune without code.
 */
UCLASS(Config=Game)
class CONNECTEDSPRAWL_API USprawlMissionDirector : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	/** First decision offered after level start. */
	UPROPERTY(Config) FName OpeningDecisionId = TEXT("FirstVC");

	/** Seconds of free driving before the opening call rings. */
	UPROPERTY(Config) float OpeningCallDelaySeconds = 40.f;

	/** Seconds between resolving a decision and the follow-up call. */
	UPROPERTY(Config) float FollowUpDelaySeconds = 75.f;

	/** How long the phone rings before Zarri picks up on his AirPods. */
	UPROPERTY(Config) float RingAnswerSeconds = 4.f;

	/** Disable to require an explicit AnswerCall from UI/input. */
	UPROPERTY(Config) bool bAutoAnswer = true;

private:
	UFUNCTION() void HandleRinging(const FPhoneCall& Call);
	UFUNCTION() void HandleResolved(UStrategicDecision* Decision, FName ChosenBranch);
	UFUNCTION() void AutoAnswer();

	void IndexDecisions();
	FName FindResumeDecisionId(const UDecisionSubsystem* Decisions) const;
	void ScheduleDecisionCall(FName DecisionId, float Delay);
	static FText ContactDisplayName(FName ContactId);

	UPROPERTY() TMap<FName, TObjectPtr<UStrategicDecision>> DecisionsById;
	TSet<FName> ScheduledIds;
	FTimerHandle AnswerTimer;
};
