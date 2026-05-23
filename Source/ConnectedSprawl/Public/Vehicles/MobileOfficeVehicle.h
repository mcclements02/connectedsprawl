// The Connected Sprawl - Zarri's car. Not just a car — a mobile office.
// Drives. Takes calls. Plays music. Holds your last deck pitch.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "MobileOfficeVehicle.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UAudioComponent;

/**
 * AMobileOfficeVehicle
 * --------------------
 * Zarri's daily driver. Functions as:
 *   - A standard ChaosVehicle pawn (drive it)
 *   - An ambient office (NPC calls, pitch prep, cash pickup trunk)
 *   - A mission staging point (some missions require entering the car first)
 */
UCLASS()
class CONNECTEDSPRAWL_API AMobileOfficeVehicle : public AWheeledVehiclePawn
{
	GENERATED_BODY()

public:
	AMobileOfficeVehicle();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// --- Office features ---

	/** Returns true if a dialogue call can start now (not mid-combat, not wanted). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Office")
	bool CanTakeCall() const;

	/** Called by the PhoneSubsystem when a scripted call fires while driving. */
	UFUNCTION(BlueprintCallable, Category="Office")
	void TakeCall(FName ContactId);

	/** Cash stashed in the trunk (dirty income must be laundered before it's useable as burn-rate cash). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Office") float TrunkCash = 0.f;

	/** Current speed in mph (for highway music/camera ramp). */
	UFUNCTION(BlueprintPure, Category="Office")
	float GetSpeedMph() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UAudioComponent> RadioAudio;

	/** Tuned in `Tick` — at 90mph+ the camera pulls out & FoV widens ("Artery Mode"). */
	void UpdateHighwayFeel(float DeltaSeconds);
};
