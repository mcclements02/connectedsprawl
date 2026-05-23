// The Connected Sprawl - Faction reputation & heat management.

#include "Factions/FactionSubsystem.h"

void UFactionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
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
