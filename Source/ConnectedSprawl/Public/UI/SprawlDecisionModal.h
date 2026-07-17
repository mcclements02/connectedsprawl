// The Connected Sprawl - Native decision modal.
// A concrete, WidgetTree-built implementation of the decision UI so the core
// loop plays without any UMG asset. A WBP subclass of UDecisionModalWidget can
// replace it at any time via BP_SprawlPlayerController.DecisionModalClass.

#pragma once

#include "CoreMinimal.h"
#include "UI/DecisionModalWidget.h"
#include "SprawlDecisionModal.generated.h"

class UButton;
class USprawlDecisionModal;

/** GC-rooted click relay: UButton::OnClicked carries no payload, so each
 *  branch button gets a proxy that remembers which branch it resolves. */
UCLASS()
class CONNECTEDSPRAWL_API USprawlBranchClickProxy : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY() FName BranchId;
	UPROPERTY() TObjectPtr<USprawlDecisionModal> Owner;

	UFUNCTION() void HandleClicked();
};

/**
 * USprawlDecisionModal
 * --------------------
 * Builds the full modal (dark scrim, card, title, prompt, one button per
 * branch with an effects summary) in C++ when PresentDecision is called.
 */
UCLASS()
class CONNECTEDSPRAWL_API USprawlDecisionModal : public UDecisionModalWidget
{
	GENERATED_BODY()

public:
	virtual void PresentDecision(UStrategicDecision* InDecision) override;

private:
	void BuildUI();
	static FString SummarizeBranchEffects(const struct FDecisionBranch& Branch);

	UPROPERTY() TArray<TObjectPtr<USprawlBranchClickProxy>> ClickProxies;
};
