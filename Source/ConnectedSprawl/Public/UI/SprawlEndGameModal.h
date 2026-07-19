// The Connected Sprawl - native victory/bankruptcy modal.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Founder/SprawlGameFlowSubsystem.h"
#include "SprawlEndGameModal.generated.h"

UCLASS()
class CONNECTEDSPRAWL_API USprawlEndGameModal : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Game Flow")
	void PresentEndGame(const FSprawlEndGameInfo& Info);

private:
	void BuildUI();

	UFUNCTION() void HandleContinueClicked();
	UFUNCTION() void HandleStartOverClicked();
	UFUNCTION() void HandleQuitClicked();

	FSprawlEndGameInfo CurrentInfo;
};
