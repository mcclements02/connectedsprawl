// The Connected Sprawl - Phone call scheduler.

#include "Phone/PhoneSubsystem.h"
#include "Missions/DecisionSubsystem.h"
#include "Missions/StrategicDecision.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UPhoneSubsystem::SchedulePhoneCall(const FPhoneCall& Call)
{
	PendingCall   = Call;
	bHasPending   = true;

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
	UE_LOG(LogTemp, Log, TEXT("[Phone] Ringing: %s"), *PendingCall.CallId.ToString());
	OnRinging.Broadcast(PendingCall);
}

void UPhoneSubsystem::AnswerCall()
{
	if (!bHasPending) return;

	bIsOnCall = true;
	OnAnswered.Broadcast(PendingCall);

	// If this call carries a Strategic Decision payload, offer it.
	if (PendingCall.TriggerDecision)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UDecisionSubsystem* Decisions = GI->GetSubsystem<UDecisionSubsystem>())
			{
				Decisions->OfferDecision(PendingCall.TriggerDecision);
			}
		}
	}
	bHasPending = false;
}

void UPhoneSubsystem::DeclineCall()
{
	bHasPending = false;
	bIsOnCall   = false;
	UE_LOG(LogTemp, Log, TEXT("[Phone] Declined %s"), *PendingCall.CallId.ToString());
}

void UPhoneSubsystem::EndCall()
{
	bIsOnCall = false;
	UE_LOG(LogTemp, Log, TEXT("[Phone] Call ended"));
}
