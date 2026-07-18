// The Connected Sprawl - Faction reputation & heat management.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Factions/FactionTypes.h"
#include "FactionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRepChanged, EFaction, Faction, int32, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHeatChanged, int32, NewHeat);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMoralDebtChanged, int32, NewDebt);

/** Faction values persisted independently of the subsystem's runtime object. */
USTRUCT(BlueprintType)
struct FFactionPersistentState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, SaveGame) int32 CorporateRep = 0;
	UPROPERTY(BlueprintReadOnly, SaveGame) int32 StreetRep = 0;
	UPROPERTY(BlueprintReadOnly, SaveGame) int32 PoliceHeat = 0;
	UPROPERTY(BlueprintReadOnly, SaveGame) int32 MoralDebt = 0;
};

/**
 * FactionSubsystem
 * ----------------
 * Tracks Zarri's standing with the Corporate and Street worlds.
 *
 * Design rules (from the GDD):
 *   - Accepting a Corporate mission typically gains CorporateRep but loses StreetRep.
 *   - Accepting a Street mission gains StreetRep and Cash quickly, but:
 *       * Police Heat goes UP (timer until cops scramble on sight)
 *       * Moral Debt goes UP (you owe someone a favor — can be called in mid-mission)
 *   - Some missions are "bridge" missions and can raise both; rare, valuable.
 */
UCLASS()
class CONNECTEDSPRAWL_API UFactionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// --- Reputation ---
	UFUNCTION(BlueprintCallable, Category="Faction")
	void ModifyReputation(EFaction Faction, int32 Delta);

	UFUNCTION(BlueprintPure, Category="Faction")
	int32 GetReputation(EFaction Faction) const;

	UFUNCTION(BlueprintPure, Category="Faction")
	ECorporateTier GetCorporateTier() const;

	UFUNCTION(BlueprintPure, Category="Faction")
	EStreetTier GetStreetTier() const;

	// --- Police Heat (0..100). Cools over time unless crimes are committed. ---
	UFUNCTION(BlueprintCallable, Category="Faction|Heat")
	void AddHeat(int32 Delta);

	UFUNCTION(BlueprintCallable, Category="Faction|Heat")
	void CoolHeat(int32 Delta);

	UFUNCTION(BlueprintPure, Category="Faction|Heat")
	int32 GetHeat() const { return PoliceHeat; }

	// --- Moral Debt (favors owed to the street). ---
	UFUNCTION(BlueprintCallable, Category="Faction|Debt")
	void IncurMoralDebt(int32 Amount);

	UFUNCTION(BlueprintCallable, Category="Faction|Debt")
	void RepayMoralDebt(int32 Amount);

	UFUNCTION(BlueprintPure, Category="Faction|Debt")
	int32 GetMoralDebt() const { return MoralDebt; }

	FFactionPersistentState CaptureState() const;
	void RestoreState(const FFactionPersistentState& State);
	void ResetProgress();

	// --- Events ---
	UPROPERTY(BlueprintAssignable) FOnRepChanged       OnReputationChanged;
	UPROPERTY(BlueprintAssignable) FOnHeatChanged      OnHeatChanged;
	UPROPERTY(BlueprintAssignable) FOnMoralDebtChanged OnMoralDebtChanged;

protected:
	/** -100..+100 for each faction. */
	UPROPERTY() int32 CorporateRep = 0;
	UPROPERTY() int32 StreetRep    = 0;

	/** 0..100. Above 80, cops actively hunt Zarri. */
	UPROPERTY() int32 PoliceHeat = 0;

	/** 0..100. Tracks owed favors; triggers "we need you NOW" interruptions. */
	UPROPERTY() int32 MoralDebt = 0;
};
