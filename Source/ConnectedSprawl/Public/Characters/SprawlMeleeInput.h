// The Connected Sprawl - Reliable on-foot melee input routing.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

class AZarriCharacter;
class UInputComponent;

/**
 * Keeps keyboard and gamepad melee bindings on the possessed character's
 * input component. Pawn input sits above the player controller in Unreal's
 * input stack, so these bindings cannot be shadowed by Enhanced Input.
 *
 * Mouse capture and touch stay controller-owned because those paths also
 * manage cursor and HUD state.
 */
struct CONNECTEDSPRAWL_API FSprawlMeleeInput
{
	/** Direct on-foot keys: keyboard X and the gamepad's left face button. */
	static const TArray<FKey>& GetOnFootKeys();

	/** Bind all direct keys once to the supplied possessed Zarri pawn. */
	static void BindOnFoot(UInputComponent* InputComponent,
		AZarriCharacter* Character);

	/** Pure contract helper used by automation and input diagnostics. */
	static bool IsOnFootKey(const FKey& Key);
};
