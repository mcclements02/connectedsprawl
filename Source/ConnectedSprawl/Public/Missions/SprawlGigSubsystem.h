// The Connected Sprawl - repeatable clean-income city gigs.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SprawlGigSubsystem.generated.h"

class ASprawlGigBeacon;

USTRUCT(BlueprintType)
struct FSprawlGigStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) bool bActive = false;
	UPROPERTY(BlueprintReadOnly) FName GigId = NAME_None;
	UPROPERTY(BlueprintReadOnly) FVector TargetLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly) float Payout = 0.f;
	UPROPERTY(BlueprintReadOnly) FText Objective;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnSprawlGigStarted, const FSprawlGigStatus&, Gig);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnSprawlGigCompleted, const FSprawlGigStatus&, Gig);

/** Ephemeral, low-cost repeatable delivery/errand loop. */
UCLASS(Config=Game)
class CONNECTEDSPRAWL_API USprawlGigSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category="Gigs")
	FSprawlGigStatus GetActiveGig() const { return ActiveGig; }

	UFUNCTION(BlueprintCallable, Category="Gigs")
	bool ForceStartGigForAudit();

	UFUNCTION(BlueprintCallable, Category="Gigs")
	bool CompleteActiveGigForAudit();

	/** Called only by the currently owned overlap beacon. */
	bool CompleteActiveGig(ASprawlGigBeacon* SourceBeacon);

	UPROPERTY(BlueprintAssignable, Category="Gigs") FOnSprawlGigStarted OnGigStarted;
	UPROPERTY(BlueprintAssignable, Category="Gigs") FOnSprawlGigCompleted OnGigCompleted;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Gigs")
	float FirstGigDelaySeconds = 90.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Gigs")
	float GigCooldownSeconds = 45.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Gigs")
	float MinimumPayout = 250.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Gigs")
	float MaximumPayout = 650.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Gigs")
	float MinimumTargetDistance = 4000.f;

private:
	void ScheduleNextGig(float DelaySeconds);
	void StartNextGig();
	bool ChooseTarget(FVector& OutTarget) const;
	void DestroyActiveBeacon();
	bool IsRuntimeAudit() const;

	UPROPERTY() TObjectPtr<ASprawlGigBeacon> ActiveBeacon;
	FSprawlGigStatus ActiveGig;
	FTimerHandle GigTimer;
	int32 CompletedGigCount = 0;
};
