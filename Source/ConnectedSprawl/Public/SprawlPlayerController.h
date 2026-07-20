// The Connected Sprawl - Player controller.

#pragma once

#include "CoreMinimal.h"
#include "Founder/SprawlGameFlowSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "SprawlPlayerController.generated.h"

class UUserWidget;

/**
 * ASprawlPlayerController
 * -----------------------
 * Creates the HUD and owns the reference to the Decision modal root widget.
 * Persistent across foot <-> vehicle possession swaps.
 */
UCLASS()
class CONNECTEDSPRAWL_API ASprawlPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// --- Touch HUD routing (single joystick + buttons; no second stick) ---
	/** Drag-look delta from the HUD's look pane, in DPI-normalized units. */
	void ApplyTouchLook(const FVector2D& Delta);
	void TouchJumpPressed();
	void TouchJumpReleased();
	void TouchSprintPressed();
	void TouchSprintReleased();
	/** +1 gas, -1 brake/reverse; released clears it. */
	void TouchThrottlePressed(float Direction);
	void TouchThrottleReleased();
	/** Enter a nearby car on foot, or step out when driving. */
	void TouchInteractPressed() { OnInteractPressed(); }
	/** True while the possessed pawn is a car (drives HUD button labels). */
	bool IsDrivingVehicle() const;

	/** Degrees per DPI-normalized drag unit. Legacy input scales apply on top. */
	UPROPERTY(EditAnywhere, Category="Input|Touch") float TouchLookYawScale = 0.14f;
	UPROPERTY(EditAnywhere, Category="Input|Touch") float TouchLookPitchScale = 0.10f;

	/** Root HUD widget (cash/runway/heat). Assigned in BP subclass. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	/** Decision modal widget. Assigned in BP subclass. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<UUserWidget> DecisionModalClass;

	/** Optional override; native end-game UI is the default. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<UUserWidget> EndGameModalClass;

	UPROPERTY(BlueprintReadOnly, Category="UI")
	TObjectPtr<UUserWidget> HUDWidget;

	UPROPERTY(BlueprintReadOnly, Category="UI")
	TObjectPtr<UUserWidget> DecisionWidget;

	UPROPERTY(BlueprintReadOnly, Category="UI")
	TObjectPtr<UUserWidget> EndGameWidget;

protected:
	virtual void SetupInputComponent() override;
	void EnsureUIInitialized();
	void OnEscapePressed();
	void OnOnePressed();
	void OnSavePressed();
	void OnLoadPressed();

	/** Esc frees the OS cursor from the window; clicking the game recaptures. */
	void SetMouseCaptured(bool bCaptured);
	void OnRecaptureClick();

	bool bUIInitialized = false;

	/** E key — enter a nearby car on foot, or step out when driving. */
	void OnInteractPressed();

	UFUNCTION()
	void HandleDecisionOffered(class UStrategicDecision* Decision);

	UFUNCTION()
	void HandleRunEnded(const FSprawlEndGameInfo& Info);

	void PresentPendingEndGame();
	FSprawlEndGameInfo PendingEndGameInfo;
	bool bHasPendingEndGame = false;
};
