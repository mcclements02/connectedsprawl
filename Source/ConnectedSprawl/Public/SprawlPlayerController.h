// The Connected Sprawl - Player controller.

#pragma once

#include "CoreMinimal.h"
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

	/** Root HUD widget (cash/runway/heat). Assigned in BP subclass. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	/** Decision modal widget. Assigned in BP subclass. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<UUserWidget> DecisionModalClass;

	UPROPERTY(BlueprintReadOnly, Category="UI")
	TObjectPtr<UUserWidget> HUDWidget;

	UPROPERTY(BlueprintReadOnly, Category="UI")
	TObjectPtr<UUserWidget> DecisionWidget;

protected:
	virtual void SetupInputComponent() override;
	void OnEscapePressed();
	void OnOnePressed();

	/** E key — enter a nearby car on foot, or step out when driving. */
	void OnInteractPressed();

	UFUNCTION()
	void HandleDecisionOffered(class UStrategicDecision* Decision);
};
