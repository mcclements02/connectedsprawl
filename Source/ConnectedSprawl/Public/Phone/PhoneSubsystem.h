// The Connected Sprawl - Phone call scheduler.
// Scripted calls drive the narrative & offer decisions while driving.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Factions/FactionTypes.h"
#include "PhoneSubsystem.generated.h"

class UStrategicDecision;

USTRUCT(BlueprintType)
struct FPhoneCall
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName CallId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ContactId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName;

	/** Optional — if set, this call offers a Strategic Decision when answered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UStrategicDecision> TriggerDecision;

	/** In-game seconds until the call fires. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float DelaySeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) EFaction Faction = EFaction::Neutral;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhoneRinging, const FPhoneCall&, Call);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhoneAnswered, const FPhoneCall&, Call);

/**
 * UPhoneSubsystem
 * ---------------
 * Queues and fires scripted phone calls. The car extends the ring time so
 * highway sequences get a chance to play out; on-foot calls fire instantly.
 */
UCLASS()
class CONNECTEDSPRAWL_API UPhoneSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Phone")
	void SchedulePhoneCall(const FPhoneCall& Call);

	UFUNCTION(BlueprintCallable, Category="Phone")
	void AnswerCall();

	UFUNCTION(BlueprintCallable, Category="Phone")
	void DeclineCall();

	UFUNCTION(BlueprintCallable, Category="Phone")
	void EndCall();

	UFUNCTION(BlueprintPure, Category="Phone")
	bool IsRinging() const { return bHasPending; }

	UFUNCTION(BlueprintPure, Category="Phone")
	bool IsOnCall() const { return bIsOnCall; }

	UPROPERTY(BlueprintAssignable) FOnPhoneRinging  OnRinging;
	UPROPERTY(BlueprintAssignable) FOnPhoneAnswered OnAnswered;

private:
	FPhoneCall PendingCall;
	bool bHasPending = false;
	bool bIsOnCall = false;
	FTimerHandle RingTimer;

	void FireRing();
};
