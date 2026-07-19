// The Connected Sprawl - native fallback HUD with no Widget Blueprint dependency.

#pragma once

#include "CoreMinimal.h"
#include "UI/SprawlHUDWidget.h"
#include "Missions/SprawlGigSubsystem.h"
#include "Phone/PhoneSubsystem.h"
#include "SprawlNativeHUD.generated.h"

class UBorder;
class UTextBlock;

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

private:
	UFUNCTION() void HandleGigStarted(const FSprawlGigStatus& Gig);
	UFUNCTION() void HandleGigCompleted(const FSprawlGigStatus& Gig);
	UFUNCTION() void HandlePhoneRinging(const FPhoneCall& Call);
	UFUNCTION() void HandlePhoneAnswered(const FPhoneCall& Call);

	void RefreshObjective();

	UPROPERTY() TObjectPtr<UTextBlock> StatsText;
	UPROPERTY() TObjectPtr<UTextBlock> ObjectiveText;
	UPROPERTY() TObjectPtr<UTextBlock> PhoneText;
	UPROPERTY() TObjectPtr<UBorder> PhoneBorder;
	FSprawlGigStatus CurrentGig;
	float DistanceRefreshAccumulator = 0.f;
	float CompletedMessageRemaining = 0.f;
};
