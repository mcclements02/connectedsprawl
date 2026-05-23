// The Connected Sprawl - Faction model
// Two axes of progression: Corporate (Safe) vs Street (Fast).
// Reputation on one side typically subtracts from the other — but not always.
// "Moral Debt" is separate from reputation; even winning the street side costs you something.

#pragma once

#include "CoreMinimal.h"
#include "FactionTypes.generated.h"

UENUM(BlueprintType)
enum class EFaction : uint8
{
	Corporate UMETA(DisplayName = "Corporate (A-Gate / Iron Forest)"),
	Street    UMETA(DisplayName = "Street (The Lows)"),
	Neutral   UMETA(DisplayName = "Neutral / Civilian")
};

UENUM(BlueprintType)
enum class ECorporateTier : uint8
{
	Outsider     UMETA(DisplayName = "Outsider"),
	Freelancer   UMETA(DisplayName = "Freelancer"),
	ContractPartner UMETA(DisplayName = "Contract Partner"),
	VCBacked     UMETA(DisplayName = "VC-Backed Founder"),
	BoardMember  UMETA(DisplayName = "Board Member")
};

UENUM(BlueprintType)
enum class EStreetTier : uint8
{
	Civilian     UMETA(DisplayName = "Civilian"),
	Runner       UMETA(DisplayName = "Runner"),
	Lieutenant   UMETA(DisplayName = "Lieutenant"),
	Kingpin      UMETA(DisplayName = "Kingpin"),
	Untouchable  UMETA(DisplayName = "Untouchable")
};

/** A single named friend/NPC in either faction. */
USTRUCT(BlueprintType)
struct FFactionContact
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ContactId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EFaction Faction = EFaction::Neutral;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Bio;

	/** 0..100 relationship strength with Zarri. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Trust = 25;
};
