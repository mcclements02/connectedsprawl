// The Connected Sprawl - Decision modal.
// The UI that presents "Sign with the VC" vs "Take the street loan."

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DecisionModalWidget.generated.h"

class UStrategicDecision;

/**
 * UDecisionModalWidget
 * --------------------
 * C++ base for the Strategic Decision UI. Designers subclass in UMG
 * (WBP_DecisionModal) to visualize branches as buttons. The BP implements
 * OnPresent to lay out N buttons, each calling `ChooseBranch` on click.
 */
UCLASS(Abstract)
class CONNECTEDSPRAWL_API UDecisionModalWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Called by the PlayerController when a decision is offered. */
	UFUNCTION(BlueprintCallable, Category="Decision")
	void PresentDecision(UStrategicDecision* InDecision);

	/** UMG calls this from a branch button's OnClicked handler. */
	UFUNCTION(BlueprintCallable, Category="Decision")
	void ChooseBranch(FName BranchId);

	/** BP implements the visual presentation (spawning buttons, etc). */
	UFUNCTION(BlueprintImplementableEvent, Category="Decision")
	void OnPresent(UStrategicDecision* Decision);

	UPROPERTY(BlueprintReadOnly, Category="Decision")
	TObjectPtr<UStrategicDecision> CurrentDecision;
};
