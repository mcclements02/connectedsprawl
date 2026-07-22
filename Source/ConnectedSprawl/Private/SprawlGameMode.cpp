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
#include "World/SprawlCityGates.h"
#include "World/SprawlCityGridSubsystem.h"
#include "World/SprawlCityMapSubsystem.h"
#include "World/SprawlCitySkyline.h"
#include "World/SprawlEnterableInteriors.h"
#include "World/SprawlLakeBasin.h"
#include "World/SprawlParkingGarage.h"
#include "World/SprawlStreetDressing.h"
#include "World/SprawlSurfaceRepair.h"
#include "World/SprawlWaterfrontScenery.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "ImageUtils.h"
#include "Components/InputComponent.h"
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
	ASprawlWaterfrontScenery::EnsureForWorld(World);
	// Rebind any ground surface that fell back to the engine checker before the
	// player ever sees it. Runs after the waterfront actor so its translucent
	// water surface already exists and is skipped by the repair scan.
	ASprawlSurfaceRepair::EnsureForWorld(World);
	// Sink the lake into a basin below the ground (hides the flat city floor
	// over the lake, rebuilds a ground ring, and walls the water in a dip).
	ASprawlLakeBasin::EnsureForWorld(World);
	// Ring the horizon with low-poly proxy buildings so the perimeter reads
	// as more city instead of the edge of the world.
	ASprawlCitySkyline::EnsureForWorld(World);
	// Replace one central authored lot with a four-level parking deck. Runtime
	// traffic is born behind its facade and drives out through a legal merge.
	ASprawlParkingGarage::EnsureForWorld(World);
	// Add three portal-based, furnished destinations without rewriting the
	// marketplace facades or loading separate interior levels on iPhone.
	ASprawlEnterableInteriors::EnsureForWorld(World);
	// Re-seat mis-placed street dressing: A-board signs onto the kerb band,
	// stranded junction props onto junction corners, signal poles onto their
	// sidewalk corners at the live road width.
	ASprawlStreetDressing::EnsureForWorld(World);
	// Close each street mouth with a city-limit gate so the perimeter reads
	// as a boundary between the edge buildings, not the edge of the world.
	ASprawlCityGates::EnsureForWorld(World);

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
	else if (FParse::Param(FCommandLine::Get(), TEXT("SprawlInteriorsAudit")))
	{
		FTimerHandle InteriorAuditStartHandle;
		World->GetTimerManager().SetTimer(
			InteriorAuditStartHandle, this,
			&ASprawlGameMode::RunInteriorMapAudit, 1.f, false);
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

void ASprawlGameMode::RunInteriorMapAudit()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	AZarriCharacter* Zarri = PC ? Cast<AZarriCharacter>(PC->GetPawn()) : nullptr;
	ASprawlEnterableInteriors* Interiors =
		ASprawlEnterableInteriors::FindForWorld(World);
	USprawlCityMapSubsystem* CityMap = World
		? World->GetSubsystem<USprawlCityMapSubsystem>() : nullptr;
	const FSprawlEnterableInteriorsLayout Layout =
		FSprawlEnterableInteriorsLayout::Build();

	bool bEntered = false;
	bool bInside = false;
	bool bMapProjectsToDoor = false;
	bool bExitRoute = false;
	bool bMapOpened = false;
	int32 PausedSafeMapBindings = 0;
	if (Zarri && Interiors && Layout.Buildings.Num() > 0)
	{
		const FSprawlEnterableBuildingSpec& Store = Layout.Buildings[0];
		Zarri->SetActorLocationAndRotation(
			Store.ExteriorReturnLocation, Store.ExteriorFacing,
			false, nullptr, ETeleportType::TeleportPhysics);
		bEntered = Interiors->TryInteract(Zarri);
		bInside = Store.GetInteriorBounds().IsInsideOrOn(
			Zarri->GetActorLocation());
		bMapProjectsToDoor = CityMap
			&& CityMap->GetDisplayLocationForActor(Zarri).Equals(
				Store.ExteriorReturnLocation, 1.f);
		FSprawlInteriorTransition ExitTransition;
		bExitRoute = Layout.FindTransition(
			Store.InteriorExitLocation, ExitTransition)
			&& !ExitTransition.bEntering
			&& ExitTransition.Target.GetLocation().Equals(
				Store.ExteriorReturnLocation, 1.f);
	}
	if (ASprawlPlayerController* SprawlPC = Cast<ASprawlPlayerController>(PC))
	{
		if (SprawlPC->InputComponent)
		{
			for (const FInputKeyBinding& Binding :
				SprawlPC->InputComponent->KeyBindings)
			{
				PausedSafeMapBindings += Binding.Chord.Key == EKeys::M
					&& Binding.KeyEvent == IE_Pressed
					&& Binding.bExecuteWhenPaused ? 1 : 0;
			}
		}
		SprawlPC->SetCityMapOpen(true);
		bMapOpened = SprawlPC->IsCityMapOpen();
		SprawlPC->SetCityMapOpen(false);
	}
	const int32 LandmarkCount = CityMap
		? CityMap->GetLandmarksView().Num() : 0;
	const int32 InstanceCount = Interiors
		? Interiors->GetBuiltInstanceCount() : 0;
	const int32 RealPropCount = Interiors
		? Interiors->GetBuiltRealPropCount() : 0;
	const int32 RealPropTypes = Interiors
		? Interiors->GetResolvedPropTypeCount() : 0;
	const bool bPassed = Layout.IsValid() && Interiors && Zarri && CityMap
		&& Layout.Buildings.Num()
			== FSprawlEnterableInteriorsLayout::ExpectedBuildingCount
		&& InstanceCount >= 150 && RealPropCount >= 70 && RealPropTypes >= 10
		&& LandmarkCount >= 5
		&& bEntered && bInside && bMapProjectsToDoor && bExitRoute
		&& bMapOpened && PausedSafeMapBindings == 1;
	if (bPassed)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[InteriorsAudit] PASS buildings=%d instances=%d real_props=%d "
				"real_types=%d landmarks=%d "
				"entered=%d inside=%d map_door=%d exit_route=%d "
				"map_open=%d map_bindings=%d"),
			Layout.Buildings.Num(), InstanceCount, RealPropCount, RealPropTypes,
			LandmarkCount,
			bEntered, bInside, bMapProjectsToDoor, bExitRoute,
			bMapOpened, PausedSafeMapBindings);
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("[InteriorsAudit] FAIL layout=%d buildings=%d instances=%d "
				"real_props=%d real_types=%d landmarks=%d entered=%d inside=%d "
				"map_door=%d exit_route=%d "
				"map_open=%d map_bindings=%d"),
			Layout.IsValid(), Layout.Buildings.Num(), InstanceCount,
			RealPropCount, RealPropTypes, LandmarkCount, bEntered, bInside,
			bMapProjectsToDoor, bExitRoute,
			bMapOpened, PausedSafeMapBindings);
	}
	FPlatformMisc::RequestExitWithStatus(
		true, bPassed ? 0 : 1, TEXT("SprawlInteriorsAudit"));
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
			FVector AuditLocation(
				USprawlCityGridSubsystem::LaneCenter(
					2, ESprawlHeading::North),
				USprawlCityGridSubsystem::BlockCenter(2), 130.f);
			FRotator AuditRotation(0.f, 90.f, 0.f);
			float ViewPitch = -8.f;
			// Tight full-body framing lets the wardrobe pass certify fitted base
			// colors, shoes, socks, and jacket detail at gameplay resolution.
			if (FParse::Param(FCommandLine::Get(), TEXT("SprawlAuditWardrobe")))
			{
				AuditLocation = FVector(
					USprawlCityGridSubsystem::BlockCenter(2) - 700.f,
					USprawlCityGridSubsystem::BlockCenter(2) - 700.f, 130.f);
				AuditRotation = FRotator(0.f, 45.f, 0.f);
				ViewPitch = -4.f;
				if (AZarriCharacter* Zarri = Cast<AZarriCharacter>(Pawn))
				{
					Zarri->PrepareWardrobeVisualAudit();
				}
			}
			// -SprawlAuditPark frames a park lawn so the Blender grass clumps
			// and grass-field ground are verifiable headlessly.
			else if (FParse::Param(FCommandLine::Get(), TEXT("SprawlAuditPark")))
			{
				// Offset from the block centre so the mature park trees frame
				// the lawn instead of swallowing the camera.
				AuditLocation = FVector(
					USprawlCityGridSubsystem::BlockCenter(2) - 620.f,
					USprawlCityGridSubsystem::BlockCenter(2) - 760.f, 300.f);
				AuditRotation = FRotator(0.f, 62.f, 0.f);
				ViewPitch = -26.f;
			}
			// -SprawlAuditGarage faces the signed north facade and its covered
			// westbound exit, so the deck silhouette and street merge are visible.
			else if (FParse::Param(FCommandLine::Get(), TEXT("SprawlAuditGarage")))
			{
				const FVector2D GarageCenter =
					FSprawlParkingGarageLayout::GarageCenter();
				AuditLocation = FVector(
					GarageCenter.X, GarageCenter.Y + 2600.f, 180.f);
				AuditRotation = FRotator(0.f, -90.f, 0.f);
				ViewPitch = 8.f;
			}
			// Interior audit flags frame each furnished room from its entry so the
			// authored props, modular fixtures, circulation, and portal target are
			// independently inspectable.
			else if (FParse::Param(FCommandLine::Get(),
					TEXT("SprawlAuditStoreInterior"))
				|| FParse::Param(FCommandLine::Get(),
					TEXT("SprawlAuditOfficeInterior"))
				|| FParse::Param(FCommandLine::Get(),
					TEXT("SprawlAuditCondoInterior")))
			{
				const FSprawlEnterableInteriorsLayout Interiors =
					FSprawlEnterableInteriorsLayout::Build();
				const int32 InteriorIndex = FParse::Param(FCommandLine::Get(),
					TEXT("SprawlAuditCondoInterior")) ? 2
					: FParse::Param(FCommandLine::Get(),
						TEXT("SprawlAuditOfficeInterior")) ? 1 : 0;
				if (Interiors.Buildings.IsValidIndex(InteriorIndex))
				{
					AuditLocation = Interiors.Buildings[InteriorIndex].InteriorEntryLocation;
					AuditRotation = FRotator(0.f, 90.f, 0.f);
					ViewPitch = -8.f;
				}
			}
			// -SprawlAuditJunction frames an intersection corner, so signal
			// poles, crosswalks, and re-seated kerb dressing are verifiable.
			else if (FParse::Param(FCommandLine::Get(), TEXT("SprawlAuditJunction")))
			{
				const float RoadX = USprawlCityGridSubsystem::RoadCenter(2);
				const float RoadY = USprawlCityGridSubsystem::RoadCenter(2);
				AuditLocation = FVector(RoadX - 300.f, RoadY - 1500.f, 320.f);
				AuditRotation = FRotator(0.f, 90.f, 0.f);
				ViewPitch = -16.f;
			}
			// -SprawlAuditEdge stands at the west perimeter looking outward,
			// so the skyline ring and horizon treatment are verifiable.
			else if (FParse::Param(FCommandLine::Get(), TEXT("SprawlAuditEdge")))
			{
				// Down the street toward the west gate at RoadCenter(2), with
				// the skyline and mountain ring behind it.
				AuditLocation = FVector(
					-USprawlCityGridSubsystem::CityBoundaryHalfExtent + 1900.f,
					USprawlCityGridSubsystem::RoadCenter(2), 420.f);
				AuditRotation = FRotator(0.f, 180.f, 0.f);
				ViewPitch = -8.f;
			}
			// -SprawlAuditWaterfront frames the lake shore instead, so the
			// capture measures the ground-surface and water repair (the reported
			// checker + wrong-colour water) rather than an inland boulevard.
			else if (FParse::Param(FCommandLine::Get(), TEXT("SprawlAuditWaterfront")))
			{
				const FSprawlWaterfrontSceneryLayout Lake =
					FSprawlWaterfrontSceneryLayout::Build();
				const FVector2D Half = Lake.WaterSize * 0.5f;
				AuditLocation = FVector(
					Lake.WaterCenter.X - Half.X * 0.30f,
					Lake.WaterCenter.Y + Half.Y + 1500.f, 360.f);
				const FVector ToLake = Lake.WaterCenter - AuditLocation;
				AuditRotation = FRotator(0.f,
					FMath::RadiansToDegrees(FMath::Atan2(ToLake.Y, ToLake.X)), 0.f);
				ViewPitch = -22.f;
			}
			Pawn->SetActorLocationAndRotation(
				AuditLocation, AuditRotation, false, nullptr,
				ETeleportType::TeleportPhysics);
			PC->SetControlRotation(
				FRotator(ViewPitch, AuditRotation.Yaw, 0.f));
		}
		if (FParse::Param(FCommandLine::Get(), TEXT("SprawlAuditMap")))
		{
			if (ASprawlPlayerController* SprawlPC =
				Cast<ASprawlPlayerController>(PC))
			{
				SprawlPC->SetCityMapOpen(true);
				// The ordinary map pauses play. The visual-audit timer must keep
				// advancing long enough to capture its completed native widget tree.
				SprawlPC->SetPause(false);
			}
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
		FCommandLine::Get(), TEXT("SprawlAuditShowUI"))
		|| FParse::Param(FCommandLine::Get(), TEXT("SprawlAuditMap"));
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
	const bool bMapAudit = FParse::Param(
		FCommandLine::Get(), TEXT("SprawlAuditMap"));
	const bool bInteriorAudit = FParse::Param(FCommandLine::Get(),
			TEXT("SprawlAuditStoreInterior"))
		|| FParse::Param(FCommandLine::Get(),
			TEXT("SprawlAuditOfficeInterior"))
		|| FParse::Param(FCommandLine::Get(),
			TEXT("SprawlAuditCondoInterior"));
	const bool bWardrobeAudit = FParse::Param(
		FCommandLine::Get(), TEXT("SprawlAuditWardrobe"));
	const bool bPassed = bMapAudit
		? MeanLuma >= 12.0 && MeanLuma <= 110.0
			&& CrushedPercent < 90.0 && ClippedPercent < 3.0
		: bInteriorAudit
			? MeanLuma >= 25.0 && MeanLuma <= 160.0
				&& CrushedPercent < 50.0 && ClippedPercent < 5.0
			: bWardrobeAudit
				? MeanLuma >= 45.0 && MeanLuma <= 170.0
					&& CrushedPercent < 35.0 && ClippedPercent < 5.0
			: MeanLuma >= 95.0 && MeanLuma <= 145.0
				&& CrushedPercent < 12.0 && ClippedPercent < 3.0
				&& WarmRatio >= 1.00 && WarmRatio <= 1.25;
	const TCHAR* AuditMode = bMapAudit
		? TEXT("map") : bInteriorAudit ? TEXT("interior")
			: bWardrobeAudit ? TEXT("wardrobe") : TEXT("city");

	if (bPassed)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[VisualAudit] PASS: mode=%s size=%dx%d mean_luma=%.2f crushed=%.2f%% clipped=%.2f%% red_blue=%.3f"),
			AuditMode, Width, Height, MeanLuma, CrushedPercent,
			ClippedPercent, WarmRatio);
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("[VisualAudit] FAIL: mode=%s size=%dx%d mean_luma=%.2f crushed=%.2f%% clipped=%.2f%% red_blue=%.3f"),
			AuditMode, Width, Height, MeanLuma, CrushedPercent,
			ClippedPercent, WarmRatio);
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
