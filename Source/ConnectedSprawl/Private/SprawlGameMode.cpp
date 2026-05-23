// The Connected Sprawl - Game mode.

#include "SprawlGameMode.h"
#include "SprawlPlayerController.h"
#include "Characters/ZarriCharacter.h"
#include "Founder/FounderSubsystem.h"
#include "Engine/World.h"
#include "TimerManager.h"

ASprawlGameMode::ASprawlGameMode()
{
	PrimaryActorTick.bCanEverTick = false;

	// Sensible C++ defaults so pressing Play spawns the right classes.
	// BP subclasses override these with asset-bound versions (skeletal mesh, input assets).
	DefaultPawnClass      = AZarriCharacter::StaticClass();
	PlayerControllerClass = ASprawlPlayerController::StaticClass();
}

void ASprawlGameMode::StartPlay()
{
	Super::StartPlay();

	UWorld* World = GetWorld();
	if (!World) return;

	World->GetTimerManager().SetTimer(
		DayTimerHandle,
		this, &ASprawlGameMode::OnDayTick,
		DayLengthSeconds,
		/*bLoop*/ true);

	UE_LOG(LogTemp, Log, TEXT("[Sprawl] GameMode started. DayLength=%.0fs"), DayLengthSeconds);
}

void ASprawlGameMode::OnDayTick()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFounderSubsystem* Founder = GI->GetSubsystem<UFounderSubsystem>())
		{
			Founder->AdvanceDay();
		}
	}
}
