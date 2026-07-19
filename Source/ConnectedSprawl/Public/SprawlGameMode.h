// The Connected Sprawl - Game mode.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SprawlGameMode.generated.h"

/**
 * ASprawlGameMode
 * ---------------
 * Default pawn is Zarri on foot. Vehicle is switched to when he enters the Mobile Office.
 * Spawns the ZonalStreamingManager in the persistent level if none exists.
 */
UCLASS()
class CONNECTEDSPRAWL_API ASprawlGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASprawlGameMode();

	virtual void StartPlay() override;

protected:
	/** Day tick — fires every DayLengthSeconds real-time seconds in the world. */
	UPROPERTY(EditAnywhere, Category="Sprawl") float DayLengthSeconds = 1200.f; // 20 real minutes = 1 in-game day

private:
	FTimerHandle DayTimerHandle;
	void OnDayTick();
	void ApplyLoadedPlayerState();
	void RunProgressionAudit();
	void BeginVisualAudit();
	void CaptureVisualAudit();
	void HandleVisualScreenshot(
		int32 Width, int32 Height, const TArray<FColor>& Colors);
	void HandleVisualAuditTimeout();
	FDelegateHandle VisualAuditScreenshotHandle;
	FTimerHandle VisualAuditTimeoutHandle;
};
