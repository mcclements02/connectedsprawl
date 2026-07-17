// The Connected Sprawl - Mission director.

#include "Missions/SprawlMissionDirector.h"
#include "Missions/StrategicDecision.h"
#include "Missions/DecisionSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/World.h"
#include "TimerManager.h"

bool USprawlMissionDirector::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void USprawlMissionDirector::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	IndexDecisions();

	UGameInstance* GI = InWorld.GetGameInstance();
	if (!GI) return;

	if (UPhoneSubsystem* Phone = GI->GetSubsystem<UPhoneSubsystem>())
	{
		Phone->OnRinging.AddDynamic(this, &USprawlMissionDirector::HandleRinging);
	}
	if (UDecisionSubsystem* Decisions = GI->GetSubsystem<UDecisionSubsystem>())
	{
		Decisions->OnDecisionResolved.AddDynamic(this, &USprawlMissionDirector::HandleResolved);
	}

	if (DecisionsById.Contains(OpeningDecisionId))
	{
		ScheduleDecisionCall(OpeningDecisionId, OpeningCallDelaySeconds);
	}
	else if (DecisionsById.Num() > 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[MissionDirector] Opening decision '%s' not found among %d indexed decisions"),
			*OpeningDecisionId.ToString(), DecisionsById.Num());
	}
}

void USprawlMissionDirector::IndexDecisions()
{
	DecisionsById.Reset();

	IAssetRegistry& Registry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	// Uncooked -game runs scan the registry asynchronously; without this the
	// BeginPlay-time scan can return zero decisions.
	Registry.WaitForCompletion();

	TArray<FAssetData> Assets;
	Registry.GetAssetsByClass(UStrategicDecision::StaticClass()->GetClassPathName(), Assets, true);

	for (const FAssetData& Data : Assets)
	{
		if (UStrategicDecision* Decision = Cast<UStrategicDecision>(Data.GetAsset()))
		{
			if (!Decision->DecisionId.IsNone())
			{
				DecisionsById.Add(Decision->DecisionId, Decision);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[MissionDirector] Indexed %d strategic decisions"), DecisionsById.Num());
}

void USprawlMissionDirector::ScheduleDecisionCall(FName DecisionId, float Delay)
{
	const TObjectPtr<UStrategicDecision>* Found = DecisionsById.Find(DecisionId);
	if (!Found || ScheduledIds.Contains(DecisionId)) return;

	UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UPhoneSubsystem* Phone = GI ? GI->GetSubsystem<UPhoneSubsystem>() : nullptr;
	if (!Phone) return;

	UStrategicDecision* Decision = *Found;

	FPhoneCall Call;
	Call.CallId          = DecisionId;
	Call.ContactId       = Decision->ProposerContactId;
	Call.DisplayName     = ContactDisplayName(Decision->ProposerContactId);
	Call.TriggerDecision = Decision;
	Call.DelaySeconds    = Delay;
	Call.Faction         = Decision->ContextFaction;

	Phone->SchedulePhoneCall(Call);
	ScheduledIds.Add(DecisionId);

	UE_LOG(LogTemp, Log, TEXT("[MissionDirector] Call '%s' from %s in %.0fs"),
		*DecisionId.ToString(), *Call.DisplayName.ToString(), Delay);
}

void USprawlMissionDirector::HandleRinging(const FPhoneCall& Call)
{
	if (!bAutoAnswer || !GetWorld()) return;

	// Give a phone UI the ring window; if nothing answered by then, Zarri
	// picks up himself. AnswerCall no-ops when the call was already handled.
	GetWorld()->GetTimerManager().SetTimer(
		AnswerTimer, this, &USprawlMissionDirector::AutoAnswer, RingAnswerSeconds, false);
}

void USprawlMissionDirector::AutoAnswer()
{
	UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	if (UPhoneSubsystem* Phone = GI ? GI->GetSubsystem<UPhoneSubsystem>() : nullptr)
	{
		if (Phone->IsRinging())
		{
			UE_LOG(LogTemp, Log, TEXT("[MissionDirector] Auto-answering"));
			Phone->AnswerCall();
		}
	}
}

void USprawlMissionDirector::HandleResolved(UStrategicDecision* Decision, FName ChosenBranch)
{
	if (!Decision) return;

	for (const FDecisionBranch& Branch : Decision->Branches)
	{
		if (Branch.BranchId != ChosenBranch) continue;

		if (!Branch.UnlocksDecisionId.IsNone())
		{
			ScheduleDecisionCall(Branch.UnlocksDecisionId, FollowUpDelaySeconds);
		}
		break;
	}
}

FText USprawlMissionDirector::ContactDisplayName(FName ContactId)
{
	static const TMap<FName, FText> KnownContacts = {
		{ TEXT("Rohan_IronForest"), NSLOCTEXT("Sprawl", "Rohan",   "Rohan · Iron Forest Ventures") },
		{ TEXT("Ty_Lows"),          NSLOCTEXT("Sprawl", "Ty",      "Ty") },
		{ TEXT("Maya_AGate"),       NSLOCTEXT("Sprawl", "Maya",    "Maya · A-Gate Tech") },
		{ TEXT("Marcus_Lows"),      NSLOCTEXT("Sprawl", "Marcus",  "Marcus · The Lows") },
		{ TEXT("DeShawn_RailYards"),NSLOCTEXT("Sprawl", "DeShawn", "DeShawn · Rail Yards") },
	};

	if (const FText* Known = KnownContacts.Find(ContactId))
	{
		return *Known;
	}
	return FText::FromString(FName::NameToDisplayString(ContactId.ToString(), false));
}
