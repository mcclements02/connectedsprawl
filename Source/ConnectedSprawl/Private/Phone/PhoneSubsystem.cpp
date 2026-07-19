// The Connected Sprawl - Phone call scheduler.

#include "Phone/PhoneSubsystem.h"
#include "Missions/DecisionSubsystem.h"
#include "Missions/StrategicDecision.h"
#include "Engine/World.h"
#include "CoreGlobals.h"
#include "TimerManager.h"

void UPhoneSubsystem::SchedulePhoneCall(const FPhoneCall& Call)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RingTimer);
	}
	PendingCall   = Call;
	bHasPending   = true;
	bIsRinging    = false;

	UWorld* World = GetWorld();
	if (!World) return;

	if (Call.DelaySeconds <= 0.f)
	{
		FireRing();
	}
	else
	{
		World->GetTimerManager().SetTimer(
			RingTimer,
			FTimerDelegate::CreateUObject(this, &UPhoneSubsystem::FireRing),
			Call.DelaySeconds, false);
	}
}

void UPhoneSubsystem::FireRing()
{
	if (!bHasPending) return;
	bIsRinging = true;
	UE_LOG(LogTemp, Log, TEXT("[Phone] Ringing: %s"), *PendingCall.CallId.ToString());
	OnRinging.Broadcast(PendingCall);
}

bool UPhoneSubsystem::TryAnswerRingingCall()
{
	if (LastAnsweredInputFrame == GFrameCounter)
	{
		return true;
	}
	if (!bIsRinging)
	{
		return false;
	}

	LastAnsweredInputFrame = GFrameCounter;
	AnswerCall();
	return true;
}

void UPhoneSubsystem::AnswerCall()
{
	if (!bHasPending || !bIsRinging) return;

	const FPhoneCall AnsweredCall = PendingCall;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RingTimer);
	}
	bHasPending = false;
	bIsRinging = false;
	PendingCall = FPhoneCall();

	bIsOnCall = true;
	OnAnswered.Broadcast(AnsweredCall);

	// If this call carries a Strategic Decision payload, offer it.
	if (AnsweredCall.TriggerDecision)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UDecisionSubsystem* Decisions = GI->GetSubsystem<UDecisionSubsystem>())
			{
				Decisions->OfferDecision(AnsweredCall.TriggerDecision);
			}
		}
	}
}

void UPhoneSubsystem::DeclineCall()
{
	const FName DeclinedId = PendingCall.CallId;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RingTimer);
	}
	bHasPending = false;
	bIsRinging = false;
	bIsOnCall   = false;
	PendingCall = FPhoneCall();
	UE_LOG(LogTemp, Log, TEXT("[Phone] Declined %s"), *DeclinedId.ToString());
}

void UPhoneSubsystem::EndCall()
{
	bIsOnCall = false;
	UE_LOG(LogTemp, Log, TEXT("[Phone] Call ended"));
}

void UPhoneSubsystem::ResetRuntimeState()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RingTimer);
	}
	PendingCall = FPhoneCall();
	bHasPending = false;
	bIsRinging = false;
	bIsOnCall = false;
	LastAnsweredInputFrame = MAX_uint64;
}
