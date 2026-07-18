// The Connected Sprawl - progression persistence implementation.

#include "Save/SprawlSaveSubsystem.h"

#include "Save/SprawlSaveGame.h"
#include "Characters/ZarriCharacter.h"
#include "Factions/FactionSubsystem.h"
#include "Founder/FounderSubsystem.h"
#include "Missions/DecisionSubsystem.h"
#include "Missions/StrategicDecision.h"
#include "Vehicles/SprawlCar.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Parse.h"
#include "Subsystems/SubsystemCollection.h"

void USprawlSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UFounderSubsystem* Founder = Collection.InitializeDependency<UFounderSubsystem>();
	Collection.InitializeDependency<UFactionSubsystem>();
	UDecisionSubsystem* Decisions = Collection.InitializeDependency<UDecisionSubsystem>();

	if (Founder)
	{
		Founder->OnDayAdvanced.AddDynamic(this, &USprawlSaveSubsystem::HandleDayAdvanced);
	}
	if (Decisions)
	{
		Decisions->OnDecisionResolved.AddDynamic(
			this, &USprawlSaveSubsystem::HandleDecisionResolved);
	}

	BackgroundDelegateHandle =
		FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(
			this, &USprawlSaveSubsystem::HandleApplicationWillEnterBackground);

	if (bAutoLoad && !ShouldSuppressAutomaticPersistence())
	{
		LoadProgress();
	}
}

void USprawlSaveSubsystem::Deinitialize()
{
	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(BackgroundDelegateHandle);

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UFounderSubsystem* Founder = GI->GetSubsystem<UFounderSubsystem>())
		{
			Founder->OnDayAdvanced.RemoveDynamic(this, &USprawlSaveSubsystem::HandleDayAdvanced);
		}
		if (UDecisionSubsystem* Decisions = GI->GetSubsystem<UDecisionSubsystem>())
		{
			Decisions->OnDecisionResolved.RemoveDynamic(
				this, &USprawlSaveSubsystem::HandleDecisionResolved);
		}
	}

	Super::Deinitialize();
}

bool USprawlSaveSubsystem::SaveProgress()
{
	return SaveProgressToSlot(DefaultSlotName);
}

bool USprawlSaveSubsystem::LoadProgress()
{
	return LoadProgressFromSlot(DefaultSlotName, true);
}

bool USprawlSaveSubsystem::LoadProgressAndRestart()
{
	if (!LoadProgress())
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FName LevelName = PendingMapName;
	if (LevelName.IsNone())
	{
		LevelName = FName(*UGameplayStatics::GetCurrentLevelName(World, true));
	}
	UGameplayStatics::OpenLevel(World, LevelName);
	return true;
}

bool USprawlSaveSubsystem::HasSave() const
{
	return UGameplayStatics::DoesSaveGameExist(DefaultSlotName, 0);
}

bool USprawlSaveSubsystem::StartNewGame()
{
	const bool bDeleted = !HasSave() || DeleteProgressInSlot(DefaultSlotName);
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return false;
	}

	if (UFounderSubsystem* Founder = GI->GetSubsystem<UFounderSubsystem>())
	{
		Founder->ResetProgress();
	}
	if (UFactionSubsystem* Factions = GI->GetSubsystem<UFactionSubsystem>())
	{
		Factions->ResetProgress();
	}
	if (UDecisionSubsystem* Decisions = GI->GetSubsystem<UDecisionSubsystem>())
	{
		Decisions->ResetProgress();
	}

	bHasPendingPlayerTransform = false;
	bPendingWasDriving = false;
	PendingMapName = NAME_None;
	OnProgressLoaded.Broadcast(bDeleted);
	return bDeleted;
}

bool USprawlSaveSubsystem::SaveProgressToSlot(const FString& SlotName)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return false;
	}

	USprawlSaveGame* Save = Cast<USprawlSaveGame>(
		UGameplayStatics::CreateSaveGameObject(USprawlSaveGame::StaticClass()));
	if (!Save)
	{
		return false;
	}

	if (UFounderSubsystem* Founder = GI->GetSubsystem<UFounderSubsystem>())
	{
		Save->Founder = Founder->CaptureState();
	}
	if (UFactionSubsystem* Factions = GI->GetSubsystem<UFactionSubsystem>())
	{
		Save->Factions = Factions->CaptureState();
	}
	if (UDecisionSubsystem* Decisions = GI->GetSubsystem<UDecisionSubsystem>())
	{
		Save->ResolvedDecisionBranches = Decisions->CaptureResolvedDecisions();
	}

	Save->SaveVersion = USprawlSaveGame::LatestSaveVersion;
	Save->SavedAtUtc = FDateTime::UtcNow();

	if (UWorld* World = GetWorld())
	{
		Save->MapName = FName(*UGameplayStatics::GetCurrentLevelName(World, true));
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				Save->PlayerTransform = Pawn->GetActorTransform();
				Save->bHasPlayerTransform = true;
				Save->bWasDriving = Pawn->IsA<ASprawlCar>();
			}
		}
	}

	const FString ResolvedSlot = ResolveSlotName(SlotName);
	const bool bSaved = UGameplayStatics::SaveGameToSlot(Save, ResolvedSlot, 0);
	if (bSaved)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[Progression] Save complete: slot=%s day=%d cash=%.0f decisions=%d"),
			*ResolvedSlot, Save->Founder.CurrentDay, Save->Founder.Cash,
			Save->ResolvedDecisionBranches.Num());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Progression] Save failed: slot=%s"), *ResolvedSlot);
	}
	OnProgressSaved.Broadcast(bSaved);
	return bSaved;
}

bool USprawlSaveSubsystem::LoadProgressFromSlot(
	const FString& SlotName, bool bRestorePlayerState)
{
	const FString ResolvedSlot = ResolveSlotName(SlotName);
	if (!UGameplayStatics::DoesSaveGameExist(ResolvedSlot, 0))
	{
		return false;
	}

	USprawlSaveGame* Save = Cast<USprawlSaveGame>(
		UGameplayStatics::LoadGameFromSlot(ResolvedSlot, 0));
	if (!Save || Save->SaveVersion < 1 ||
		Save->SaveVersion > USprawlSaveGame::LatestSaveVersion)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[Progression] Refusing unsupported save in slot %s (version=%d latest=%d)"),
			*ResolvedSlot, Save ? Save->SaveVersion : -1,
			USprawlSaveGame::LatestSaveVersion);
		OnProgressLoaded.Broadcast(false);
		return false;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return false;
	}

	if (UFounderSubsystem* Founder = GI->GetSubsystem<UFounderSubsystem>())
	{
		Founder->RestoreState(Save->Founder);
	}
	if (UFactionSubsystem* Factions = GI->GetSubsystem<UFactionSubsystem>())
	{
		Factions->RestoreState(Save->Factions);
	}
	if (UDecisionSubsystem* Decisions = GI->GetSubsystem<UDecisionSubsystem>())
	{
		Decisions->RestoreResolvedDecisions(Save->ResolvedDecisionBranches);
	}

	if (bRestorePlayerState)
	{
		PendingPlayerTransform = Save->PlayerTransform;
		bHasPendingPlayerTransform = Save->bHasPlayerTransform;
		bPendingWasDriving = Save->bWasDriving;
		PendingMapName = Save->MapName;
	}

	UE_LOG(LogTemp, Log,
		TEXT("[Progression] Load complete: slot=%s version=%d day=%d cash=%.0f decisions=%d"),
		*ResolvedSlot, Save->SaveVersion, Save->Founder.CurrentDay,
		Save->Founder.Cash, Save->ResolvedDecisionBranches.Num());
	OnProgressLoaded.Broadcast(true);
	return true;
}

bool USprawlSaveSubsystem::DeleteProgressInSlot(const FString& SlotName) const
{
	const FString ResolvedSlot = ResolveSlotName(SlotName);
	return !UGameplayStatics::DoesSaveGameExist(ResolvedSlot, 0) ||
		UGameplayStatics::DeleteGameInSlot(ResolvedSlot, 0);
}

bool USprawlSaveSubsystem::ApplyPendingPlayerState(APlayerController* PlayerController)
{
	if (!bHasPendingPlayerTransform || !PlayerController)
	{
		return false;
	}

	APawn* Pawn = PlayerController->GetPawn();
	if (!Pawn)
	{
		return false;
	}

	bool bRestoredDriving = false;
	if (bPendingWasDriving)
	{
		AZarriCharacter* Zarri = Cast<AZarriCharacter>(Pawn);
		ASprawlCar* PlayerVehicle = nullptr;
		for (TActorIterator<ASprawlCar> It(PlayerController->GetWorld()); It; ++It)
		{
			if (!It->bAutoDrive)
			{
				PlayerVehicle = *It;
				break;
			}
		}

		if (Zarri && PlayerVehicle)
		{
			PlayerVehicle->SetActorTransform(
				PendingPlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
			Zarri->SetActorTransform(
				PendingPlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
			bRestoredDriving = Zarri->EnterVehicle(PlayerVehicle);
		}
	}

	if (!bRestoredDriving)
	{
		Pawn->SetActorTransform(
			PendingPlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}

	bHasPendingPlayerTransform = false;
	UE_LOG(LogTemp, Log, TEXT("[Progression] Restored player state (%s)"),
		bRestoredDriving ? TEXT("driving") : TEXT("on foot"));
	return true;
}

bool USprawlSaveSubsystem::RunRoundTripAudit(FString& OutSummary)
{
	UGameInstance* GI = GetGameInstance();
	UFounderSubsystem* Founder = GI ? GI->GetSubsystem<UFounderSubsystem>() : nullptr;
	UFactionSubsystem* Factions = GI ? GI->GetSubsystem<UFactionSubsystem>() : nullptr;
	UDecisionSubsystem* Decisions = GI ? GI->GetSubsystem<UDecisionSubsystem>() : nullptr;
	if (!Founder || !Factions || !Decisions)
	{
		OutSummary = TEXT("required progression subsystem missing");
		return false;
	}

	const FFounderPersistentState OriginalFounder = Founder->CaptureState();
	const FFactionPersistentState OriginalFactions = Factions->CaptureState();
	const TMap<FName, FName> OriginalDecisions = Decisions->CaptureResolvedDecisions();

	FFounderPersistentState ExpectedFounder = OriginalFounder;
	ExpectedFounder.Cash += 4321.f;
	ExpectedFounder.CurrentDay += 3;
	ExpectedFounder.RecurringDailyExpenses.Add(ECashFlowSource::LegalFees, 77.f);
	FCashFlowEntry AuditEntry;
	AuditEntry.Timestamp = FDateTime(2026, 7, 18, 12, 0, 0);
	AuditEntry.Source = ECashFlowSource::CleanContract;
	AuditEntry.Amount = 4321.f;
	AuditEntry.Note = FText::FromString(TEXT("Progression round-trip audit"));
	ExpectedFounder.Ledger.Add(AuditEntry);

	FFactionPersistentState ExpectedFactions = OriginalFactions;
	ExpectedFactions.CorporateRep = 13;
	ExpectedFactions.StreetRep = -7;
	ExpectedFactions.PoliceHeat = 19;
	ExpectedFactions.MoralDebt = 23;

	TMap<FName, FName> ExpectedDecisions = OriginalDecisions;
	ExpectedDecisions.Add(TEXT("ProgressionAuditDecision"), TEXT("AuditBranch"));

	Founder->RestoreState(ExpectedFounder);
	Factions->RestoreState(ExpectedFactions);
	Decisions->RestoreResolvedDecisions(ExpectedDecisions);

	const FString AuditSlot = DefaultSlotName + TEXT("_RoundTripAudit");
	const FString FutureSlot = DefaultSlotName + TEXT("_FutureVersionAudit");
	DeleteProgressInSlot(AuditSlot);
	DeleteProgressInSlot(FutureSlot);

	bool bRoundTrip = SaveProgressToSlot(AuditSlot);
	Founder->ResetProgress();
	Factions->ResetProgress();
	Decisions->ResetProgress();
	bRoundTrip = LoadProgressFromSlot(AuditSlot, false) && bRoundTrip;

	const FFounderPersistentState ActualFounder = Founder->CaptureState();
	const FFactionPersistentState ActualFactions = Factions->CaptureState();
	const float* AuditLegalFees =
		ActualFounder.RecurringDailyExpenses.Find(ECashFlowSource::LegalFees);
	const FCashFlowEntry* LoadedAuditEntry = ActualFounder.Ledger.Num() > 0
		? &ActualFounder.Ledger.Last() : nullptr;

	bRoundTrip = bRoundTrip &&
		FMath::IsNearlyEqual(ActualFounder.Cash, ExpectedFounder.Cash) &&
		ActualFounder.CurrentDay == ExpectedFounder.CurrentDay &&
		AuditLegalFees && FMath::IsNearlyEqual(*AuditLegalFees, 77.f) &&
		LoadedAuditEntry && FMath::IsNearlyEqual(LoadedAuditEntry->Amount, 4321.f) &&
		ActualFactions.CorporateRep == ExpectedFactions.CorporateRep &&
		ActualFactions.StreetRep == ExpectedFactions.StreetRep &&
		ActualFactions.PoliceHeat == ExpectedFactions.PoliceHeat &&
		ActualFactions.MoralDebt == ExpectedFactions.MoralDebt &&
		Decisions->GetResolvedBranch(TEXT("ProgressionAuditDecision")) == TEXT("AuditBranch");

	USprawlSaveGame* FutureSave = Cast<USprawlSaveGame>(
		UGameplayStatics::CreateSaveGameObject(USprawlSaveGame::StaticClass()));
	bool bFutureRejected = false;
	if (FutureSave)
	{
		FutureSave->SaveVersion = USprawlSaveGame::LatestSaveVersion + 1;
		bFutureRejected = UGameplayStatics::SaveGameToSlot(FutureSave, FutureSlot, 0) &&
			!LoadProgressFromSlot(FutureSlot, false);
	}

	DeleteProgressInSlot(AuditSlot);
	DeleteProgressInSlot(FutureSlot);
	Founder->RestoreState(OriginalFounder);
	Factions->RestoreState(OriginalFactions);
	Decisions->RestoreResolvedDecisions(OriginalDecisions);

	OutSummary = FString::Printf(
		TEXT("round_trip=%s future_version_rejected=%s day=%d cash=%.0f ledger=%d decisions=%d"),
		bRoundTrip ? TEXT("true") : TEXT("false"),
		bFutureRejected ? TEXT("true") : TEXT("false"),
		ActualFounder.CurrentDay, ActualFounder.Cash,
		ActualFounder.Ledger.Num(), ExpectedDecisions.Num());
	return bRoundTrip && bFutureRejected;
}

void USprawlSaveSubsystem::HandleDecisionResolved(
	UStrategicDecision* Decision, FName ChosenBranch)
{
	(void)Decision;
	(void)ChosenBranch;
	if (bAutosaveAfterDecision && !ShouldSuppressAutomaticPersistence())
	{
		SaveProgress();
	}
}

void USprawlSaveSubsystem::HandleDayAdvanced(int32 NewDay)
{
	(void)NewDay;
	if (bAutosaveAfterDay && !ShouldSuppressAutomaticPersistence())
	{
		SaveProgress();
	}
}

void USprawlSaveSubsystem::HandleApplicationWillEnterBackground()
{
	if (bAutosaveOnBackground && !ShouldSuppressAutomaticPersistence())
	{
		SaveProgress();
	}
}

bool USprawlSaveSubsystem::ShouldSuppressAutomaticPersistence() const
{
	return FParse::Param(FCommandLine::Get(), TEXT("NoSprawlAutoLoad")) ||
		FParse::Param(FCommandLine::Get(), TEXT("SprawlTrafficAudit")) ||
		FParse::Param(FCommandLine::Get(), TEXT("SprawlProgressionAudit"));
}

FString USprawlSaveSubsystem::ResolveSlotName(const FString& SlotName) const
{
	return SlotName.IsEmpty() ? DefaultSlotName : SlotName;
}
