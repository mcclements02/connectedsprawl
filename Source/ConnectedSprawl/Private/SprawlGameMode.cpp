// The Connected Sprawl - Game mode.

#include "SprawlGameMode.h"
#include "SprawlPlayerController.h"
#include "Characters/ZarriCharacter.h"
#include "Factions/FactionSubsystem.h"
#include "Founder/FounderSubsystem.h"
#include "Founder/SprawlGameFlowSubsystem.h"
#include "Missions/DecisionSubsystem.h"
#include "Missions/SprawlGigSubsystem.h"
#include "Save/SprawlSaveSubsystem.h"
#include "World/SprawlCityGridSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "ImageUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"

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
		// Let the pawn, streamed collision, and world subsystems finish their
		// first frame before the audit asks the gig system for a ground target.
		FTimerHandle ProgressionAuditStartHandle;
		World->GetTimerManager().SetTimer(
			ProgressionAuditStartHandle, this,
			&ASprawlGameMode::RunProgressionAudit, 1.f, false);
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("SprawlVisualAudit")))
	{
		FTimerHandle VisualStartHandle;
		World->GetTimerManager().SetTimer(
			VisualStartHandle, this, &ASprawlGameMode::BeginVisualAudit,
			3.f, false);
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
		USprawlGameFlowSubsystem* Flow =
			GI->GetSubsystem<USprawlGameFlowSubsystem>();
		if ((!Flow || Flow->CanAdvanceDay()))
		{
			if (UFounderSubsystem* Founder = GI->GetSubsystem<UFounderSubsystem>())
			{
				Founder->AdvanceDay();
			}
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
	UFounderSubsystem* Founder = GI ? GI->GetSubsystem<UFounderSubsystem>() : nullptr;
	UFactionSubsystem* Factions = GI ? GI->GetSubsystem<UFactionSubsystem>() : nullptr;
	UDecisionSubsystem* Decisions = GI ? GI->GetSubsystem<UDecisionSubsystem>() : nullptr;
	USprawlGameFlowSubsystem* Flow =
		GI ? GI->GetSubsystem<USprawlGameFlowSubsystem>() : nullptr;
	USprawlGigSubsystem* Gigs = GetWorld()
		? GetWorld()->GetSubsystem<USprawlGigSubsystem>() : nullptr;

	FString SaveSummary;
	const bool bSaveRoundTrip = Saves && Saves->RunRoundTripAudit(SaveSummary);
	bool bGigStarted = false;
	bool bGigPaid = false;
	bool bBailoutOffered = false;
	bool bBailoutResolved = false;
	bool bVictoryRecorded = false;
	float GigPayout = 0.f;

	if (Founder && Factions && Decisions && Flow && Gigs)
	{
		const float CashBeforeGig = Founder->GetCash();
		bGigStarted = Gigs->ForceStartGigForAudit();
		GigPayout = Gigs->GetActiveGig().Payout;
		bGigPaid = bGigStarted && Gigs->CompleteActiveGigForAudit() &&
			FMath::IsNearlyEqual(
				Founder->GetCash() - CashBeforeGig, GigPayout, 0.5f);

		const int32 HeatBefore = Factions->GetHeat();
		const int32 DebtBefore = Factions->GetMoralDebt();
		Founder->PayExpense(Founder->GetCash() + 1.f,
			ECashFlowSource::LegalFees,
			FText::FromString(TEXT("Progression audit bankruptcy")));
		Founder->AdvanceDay();
		bBailoutOffered = Flow->IsBailoutPending();
		bBailoutResolved = bBailoutOffered && Flow->ResolveBailoutForAudit(true) &&
			Founder->GetCash() > 0.f &&
			Factions->GetHeat() > HeatBefore &&
			Factions->GetMoralDebt() > DebtBefore &&
			Decisions->GetResolvedBranch(TEXT("DirtyBailout")) == TEXT("TakeDirtyJob");

		const float VictoryDelta = Flow->WinCashTarget - Founder->GetCash();
		if (VictoryDelta > 0.f)
		{
			Founder->AddIncome(VictoryDelta, ECashFlowSource::CleanContract,
				FText::FromString(TEXT("Progression audit victory")), false);
		}
		bVictoryRecorded = Flow->HasWon() &&
			Decisions->HasResolvedDecision(TEXT("_SprawlVictory"));
	}

	const bool bPassed = bSaveRoundTrip && bGigStarted && bGigPaid &&
		bBailoutOffered && bBailoutResolved && bVictoryRecorded;
	const FString Summary = FString::Printf(
		TEXT("%s gig_started=%s gig_paid=%s payout=%.0f bailout_offered=%s bailout_resolved=%s victory=%s"),
		*SaveSummary,
		bGigStarted ? TEXT("true") : TEXT("false"),
		bGigPaid ? TEXT("true") : TEXT("false"), GigPayout,
		bBailoutOffered ? TEXT("true") : TEXT("false"),
		bBailoutResolved ? TEXT("true") : TEXT("false"),
		bVictoryRecorded ? TEXT("true") : TEXT("false"));
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

void ASprawlGameMode::BeginVisualAudit()
{
	if (!GEngine || !GEngine->GameViewport)
	{
		UE_LOG(LogTemp, Error, TEXT("[VisualAudit] FAIL: game viewport unavailable"));
		FPlatformMisc::RequestExitWithStatus(true, 1, TEXT("SprawlVisualAudit"));
		return;
	}
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			// The authored start is intentionally tucked beside the player car.
			// Move only the audit pawn to an open boulevard so the capture measures
			// the city grade rather than one nearby facade.
			const FVector AuditLocation(
				USprawlCityGridSubsystem::LaneCenter(
					2, ESprawlHeading::North),
				USprawlCityGridSubsystem::BlockCenter(2), 130.f);
			const FRotator AuditRotation(0.f, 90.f, 0.f);
			Pawn->SetActorLocationAndRotation(
				AuditLocation, AuditRotation, false, nullptr,
				ETeleportType::TeleportPhysics);
			PC->SetControlRotation(FRotator(-8.f, 90.f, 0.f));
		}
	}
	FTimerHandle CameraSettleHandle;
	GetWorld()->GetTimerManager().SetTimer(
		CameraSettleHandle, this, &ASprawlGameMode::CaptureVisualAudit,
		0.75f, false);
}

void ASprawlGameMode::CaptureVisualAudit()
{
	if (!GEngine || !GEngine->GameViewport)
	{
		UE_LOG(LogTemp, Error, TEXT("[VisualAudit] FAIL: game viewport unavailable at capture"));
		FPlatformMisc::RequestExitWithStatus(true, 1, TEXT("SprawlVisualAudit"));
		return;
	}

	VisualAuditScreenshotHandle = UGameViewportClient::OnScreenshotCaptured().AddUObject(
		this, &ASprawlGameMode::HandleVisualScreenshot);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			VisualAuditTimeoutHandle, this,
			&ASprawlGameMode::HandleVisualAuditTimeout, 20.f, false);
	}
	// -SprawlAuditShowUI composites the HUD (touch buttons, panels) into the
	// captured frame so control layout can be verified headlessly.
	const bool bShowUI = FParse::Param(
		FCommandLine::Get(), TEXT("SprawlAuditShowUI"));
	FScreenshotRequest::RequestScreenshot(
		bShowUI, /*bInRestrictToGameViewport*/ true);
	UE_LOG(LogTemp, Display, TEXT("[VisualAudit] screenshot requested"));
}

void ASprawlGameMode::HandleVisualScreenshot(
	int32 Width, int32 Height, const TArray<FColor>& Colors)
{
	UGameViewportClient::OnScreenshotCaptured().Remove(VisualAuditScreenshotHandle);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(VisualAuditTimeoutHandle);
	}

	const int64 ExpectedPixels = static_cast<int64>(Width) * Height;
	if (Width <= 0 || Height <= 0 || Colors.Num() != ExpectedPixels)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[VisualAudit] FAIL: invalid capture %dx%d pixels=%d"),
			Width, Height, Colors.Num());
		FPlatformMisc::RequestExitWithStatus(true, 1, TEXT("SprawlVisualAudit"));
		return;
	}

	const FString ScreenshotDirectory =
		FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"));
	IFileManager::Get().MakeDirectory(*ScreenshotDirectory, true);
	const FString ScreenshotPath = FPaths::Combine(
		ScreenshotDirectory, TEXT("SprawlVisualAudit.png"));
	TArray64<uint8> CompressedScreenshot;
	FImageUtils::PNGCompressImageArray(
		Width, Height,
		TArrayView64<const FColor>(Colors.GetData(), Colors.Num()),
		CompressedScreenshot);
	if (FFileHelper::SaveArrayToFile(CompressedScreenshot, *ScreenshotPath))
	{
		UE_LOG(LogTemp, Display, TEXT("[VisualAudit] saved %s"), *ScreenshotPath);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[VisualAudit] could not save measured frame to %s"),
			*ScreenshotPath);
	}

	double LumaSum = 0.0;
	double RedSum = 0.0;
	double BlueSum = 0.0;
	int64 CrushedPixels = 0;
	int64 ClippedPixels = 0;
	for (const FColor& Color : Colors)
	{
		const double Luma = 0.2126 * Color.R + 0.7152 * Color.G + 0.0722 * Color.B;
		LumaSum += Luma;
		RedSum += Color.R;
		BlueSum += Color.B;
		CrushedPixels += Luma < 12.0 ? 1 : 0;
		ClippedPixels += Luma > 245.0 ? 1 : 0;
	}

	const double PixelCount = static_cast<double>(Colors.Num());
	const double MeanLuma = LumaSum / PixelCount;
	const double CrushedPercent = 100.0 * CrushedPixels / PixelCount;
	const double ClippedPercent = 100.0 * ClippedPixels / PixelCount;
	const double WarmRatio = RedSum / FMath::Max(1.0, BlueSum);
	const bool bPassed = MeanLuma >= 95.0 && MeanLuma <= 145.0 &&
		CrushedPercent < 12.0 && ClippedPercent < 3.0 &&
		WarmRatio >= 1.00 && WarmRatio <= 1.25;

	if (bPassed)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[VisualAudit] PASS: size=%dx%d mean_luma=%.2f crushed=%.2f%% clipped=%.2f%% red_blue=%.3f"),
			Width, Height, MeanLuma, CrushedPercent, ClippedPercent, WarmRatio);
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("[VisualAudit] FAIL: size=%dx%d mean_luma=%.2f crushed=%.2f%% clipped=%.2f%% red_blue=%.3f gates=luma[95,145],crushed<12,clipped<3,red_blue[1.00,1.25]"),
			Width, Height, MeanLuma, CrushedPercent, ClippedPercent, WarmRatio);
	}
	FPlatformMisc::RequestExitWithStatus(
		true, bPassed ? 0 : 1, TEXT("SprawlVisualAudit"));
}

void ASprawlGameMode::HandleVisualAuditTimeout()
{
	UGameViewportClient::OnScreenshotCaptured().Remove(VisualAuditScreenshotHandle);
	UE_LOG(LogTemp, Error, TEXT("[VisualAudit] FAIL: screenshot timed out"));
	FPlatformMisc::RequestExitWithStatus(true, 1, TEXT("SprawlVisualAudit"));
}
