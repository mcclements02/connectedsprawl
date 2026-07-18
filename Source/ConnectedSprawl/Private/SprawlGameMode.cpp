// The Connected Sprawl - Game mode.

#include "SprawlGameMode.h"
#include "SprawlPlayerController.h"
#include "Characters/ZarriCharacter.h"
#include "Founder/FounderSubsystem.h"
#include "Save/SprawlSaveSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
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

	if (FParse::Param(FCommandLine::Get(), TEXT("SprawlProgressionAudit")))
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &ASprawlGameMode::RunProgressionAudit));
	}
	else
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &ASprawlGameMode::ApplyLoadedPlayerState));
	}

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

void ASprawlGameMode::ApplyLoadedPlayerState()
{
	UGameInstance* GI = GetGameInstance();
	UWorld* World = GetWorld();
	if (!GI || !World)
	{
		return;
	}

	if (USprawlSaveSubsystem* Saves = GI->GetSubsystem<USprawlSaveSubsystem>())
	{
		Saves->ApplyPendingPlayerState(World->GetFirstPlayerController());
	}
}

void ASprawlGameMode::RunProgressionAudit()
{
	UGameInstance* GI = GetGameInstance();
	USprawlSaveSubsystem* Saves = GI ? GI->GetSubsystem<USprawlSaveSubsystem>() : nullptr;
	FString Summary;
	const bool bPassed = Saves && Saves->RunRoundTripAudit(Summary);
	if (bPassed)
	{
		UE_LOG(LogTemp, Display, TEXT("[ProgressionAudit] PASS: %s"), *Summary);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[ProgressionAudit] FAIL: %s"), *Summary);
	}
	FPlatformMisc::RequestExitWithStatus(true, bPassed ? 0 : 1, TEXT("SprawlProgressionAudit"));
}
