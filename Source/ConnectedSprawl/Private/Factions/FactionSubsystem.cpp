// The Connected Sprawl - Faction reputation & heat management.

#include "Factions/FactionSubsystem.h"

void UFactionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ResetProgress();
	UE_LOG(LogTemp, Log, TEXT("[Faction] Initialized. Corp=0 Street=0 Heat=0 Debt=0"));
}

void UFactionSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UFactionSubsystem::ModifyReputation(EFaction Faction, int32 Delta)
{
	switch (Faction)
	{
	case EFaction::Corporate:
		CorporateRep = FMath::Clamp(CorporateRep + Delta, -100, 100);
		OnReputationChanged.Broadcast(EFaction::Corporate, CorporateRep);
		break;
	case EFaction::Street:
		StreetRep = FMath::Clamp(StreetRep + Delta, -100, 100);
		OnReputationChanged.Broadcast(EFaction::Street, StreetRep);
		break;
	default: break;
	}
}

int32 UFactionSubsystem::GetReputation(EFaction Faction) const
{
	switch (Faction)
	{
	case EFaction::Corporate: return CorporateRep;
	case EFaction::Street:    return StreetRep;
	default:                  return 0;
	}
}

ECorporateTier UFactionSubsystem::GetCorporateTier() const
{
	if (CorporateRep >= 80) return ECorporateTier::BoardMember;
	if (CorporateRep >= 60) return ECorporateTier::VCBacked;
	if (CorporateRep >= 40) return ECorporateTier::ContractPartner;
	if (CorporateRep >= 20) return ECorporateTier::Freelancer;
	return ECorporateTier::Outsider;
}

EStreetTier UFactionSubsystem::GetStreetTier() const
{
	if (StreetRep >= 80) return EStreetTier::Untouchable;
	if (StreetRep >= 60) return EStreetTier::Kingpin;
	if (StreetRep >= 40) return EStreetTier::Lieutenant;
	if (StreetRep >= 20) return EStreetTier::Runner;
	return EStreetTier::Civilian;
}

void UFactionSubsystem::AddHeat(int32 Delta)
{
	PoliceHeat = FMath::Clamp(PoliceHeat + Delta, 0, 100);
	OnHeatChanged.Broadcast(PoliceHeat);
}

void UFactionSubsystem::CoolHeat(int32 Delta)
{
	PoliceHeat = FMath::Clamp(PoliceHeat - Delta, 0, 100);
	OnHeatChanged.Broadcast(PoliceHeat);
}

void UFactionSubsystem::IncurMoralDebt(int32 Amount)
{
	MoralDebt = FMath::Clamp(MoralDebt + Amount, 0, 100);
	OnMoralDebtChanged.Broadcast(MoralDebt);
}

void UFactionSubsystem::RepayMoralDebt(int32 Amount)
{
	MoralDebt = FMath::Clamp(MoralDebt - Amount, 0, 100);
	OnMoralDebtChanged.Broadcast(MoralDebt);
}

FFactionPersistentState UFactionSubsystem::CaptureState() const
{
	FFactionPersistentState State;
	State.CorporateRep = CorporateRep;
	State.StreetRep = StreetRep;
	State.PoliceHeat = PoliceHeat;
	State.MoralDebt = MoralDebt;
	return State;
}

void UFactionSubsystem::RestoreState(const FFactionPersistentState& State)
{
	CorporateRep = FMath::Clamp(State.CorporateRep, -100, 100);
	StreetRep = FMath::Clamp(State.StreetRep, -100, 100);
	PoliceHeat = FMath::Clamp(State.PoliceHeat, 0, 100);
	MoralDebt = FMath::Clamp(State.MoralDebt, 0, 100);

	OnReputationChanged.Broadcast(EFaction::Corporate, CorporateRep);
	OnReputationChanged.Broadcast(EFaction::Street, StreetRep);
	OnHeatChanged.Broadcast(PoliceHeat);
	OnMoralDebtChanged.Broadcast(MoralDebt);
}

void UFactionSubsystem::ResetProgress()
{
	RestoreState(FFactionPersistentState());
}
