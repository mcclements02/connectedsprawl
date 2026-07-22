// The Connected Sprawl - native fallback HUD with no Widget Blueprint dependency.

#pragma once

#include "CoreMinimal.h"
#include "UI/SprawlHUDWidget.h"
#include "Missions/SprawlGigSubsystem.h"
#include "Phone/PhoneSubsystem.h"
#include "SprawlNativeHUD.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UTextBlock;
class ASprawlPlayerController;

UCLASS()
class CONNECTEDSPRAWL_API USprawlNativeHUD : public USprawlHUDWidget
{
	GENERATED_BODY()

public:
	void BuildUI();
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	virtual void NativeRefresh() override;

	// Touch look: drags on the right-side pane bubble up to the widget root.
	virtual FReply NativeOnTouchStarted(
		const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchMoved(
		const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchEnded(
		const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

private:
	UFUNCTION() void HandleGigStarted(const FSprawlGigStatus& Gig);
	UFUNCTION() void HandleGigCompleted(const FSprawlGigStatus& Gig);
	UFUNCTION() void HandlePhoneRinging(const FPhoneCall& Call);
	UFUNCTION() void HandlePhoneAnswered(const FPhoneCall& Call);

	// Contextual touch buttons: JUMP/SPRINT/MELEE/ENTER on foot, GAS/BRAKE/EXIT in car.
	UFUNCTION() void HandlePrimaryPressed();
	UFUNCTION() void HandlePrimaryReleased();
	UFUNCTION() void HandleSecondaryPressed();
	UFUNCTION() void HandleSecondaryReleased();
	UFUNCTION() void HandleInteractButtonPressed();
	UFUNCTION() void HandleMeleeButtonPressed();
	UFUNCTION() void HandleMapButtonPressed();

	void RefreshObjective();
	void BuildTouchControls(UCanvasPanel* Root);
	void RefreshTouchButtonLabels(bool bForce = false);
	ASprawlPlayerController* GetSprawlPC() const;

	UPROPERTY() TObjectPtr<UTextBlock> StatsText;
	UPROPERTY() TObjectPtr<UTextBlock> ObjectiveText;
	UPROPERTY() TObjectPtr<UTextBlock> PhoneText;
	UPROPERTY() TObjectPtr<UBorder> PhoneBorder;

	UPROPERTY() TObjectPtr<UBorder> LookPane;
	UPROPERTY() TObjectPtr<UButton> PrimaryButton;
	UPROPERTY() TObjectPtr<UButton> SecondaryButton;
	UPROPERTY() TObjectPtr<UButton> InteractButton;
	UPROPERTY() TObjectPtr<UButton> MeleeButton;
	UPROPERTY() TObjectPtr<UButton> MapButton;
	UPROPERTY() TObjectPtr<UTextBlock> PrimaryLabel;
	UPROPERTY() TObjectPtr<UTextBlock> SecondaryLabel;
	UPROPERTY() TObjectPtr<UTextBlock> InteractLabel;
	UPROPERTY() TObjectPtr<UTextBlock> MeleeLabel;
	UPROPERTY() TObjectPtr<UTextBlock> MapLabel;

	void RefreshInteractHint();

	/** Desktop key hint ("E — drive"); touch builds real buttons instead. */
	UPROPERTY() TObjectPtr<UBorder> InteractHintBorder;
	UPROPERTY() TObjectPtr<UTextBlock> InteractHintText;

	FSprawlGigStatus CurrentGig;
	float DistanceRefreshAccumulator = 0.f;
	float CompletedMessageRemaining = 0.f;
	float HintRefreshAccumulator = 0.f;

	int32 LookFingerIndex = INDEX_NONE;
	FVector2D LastLookPosition = FVector2D::ZeroVector;
	bool bTouchButtonsInCarMode = false;
	bool bTouchControlsBuilt = false;
	bool bTouchGasHeld = false;
	bool bTouchBrakeHeld = false;
	float TouchModeRefreshAccumulator = 0.f;
};
