// The Connected Sprawl - Pedestrian AI controller.

#include "AI/SprawlPedestrianAI.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "GameFramework/Pawn.h"

ASprawlPedestrianAI::ASprawlPedestrianAI()
{
	bAttachToPawn = false;
}

void ASprawlPedestrianAI::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Stagger the first move so the crowd doesn't step off in lockstep.
	GetWorldTimerManager().SetTimer(
		MoveTimer, this, &ASprawlPedestrianAI::PickNewDestination,
		FMath::FRandRange(0.4f, 4.0f), false);
}

void ASprawlPedestrianAI::PickNewDestination()
{
	APawn* P = GetPawn();
	if (!P)
	{
		return;
	}

	UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(GetWorld());
	FNavLocation Dest;
	if (Nav && Nav->GetRandomReachablePointInRadius(
			P->GetActorLocation(), WanderRadius, Dest))
	{
		MoveToLocation(Dest.Location, 60.f);
	}
	else
	{
		// Nav mesh not ready yet — retry shortly.
		GetWorldTimerManager().SetTimer(
			MoveTimer, this, &ASprawlPedestrianAI::PickNewDestination, 2.0f, false);
	}
}

void ASprawlPedestrianAI::OnMoveCompleted(FAIRequestID RequestID,
                                          const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	// Pause a beat, then head somewhere new.
	GetWorldTimerManager().SetTimer(
		MoveTimer, this, &ASprawlPedestrianAI::PickNewDestination,
		FMath::FRandRange(0.8f, 4.5f), false);
}
