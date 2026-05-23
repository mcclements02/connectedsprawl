// The Connected Sprawl - Decision modal.

#include "UI/DecisionModalWidget.h"
#include "Missions/StrategicDecision.h"
#include "Missions/DecisionSubsystem.h"
#include "GameFramework/PlayerController.h"

void UDecisionModalWidget::PresentDecision(UStrategicDecision* InDecision)
{
	CurrentDecision = InDecision;
	OnPresent(InDecision);
}

void UDecisionModalWidget::ChooseBranch(FName BranchId)
{
	if (!CurrentDecision) return;

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UDecisionSubsystem* Decisions = GI->GetSubsystem<UDecisionSubsystem>())
		{
			Decisions->ResolveDecision(CurrentDecision, BranchId);
		}
	}

	// Close the modal, resume gameplay.
	RemoveFromParent();
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetPause(false);
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
}
