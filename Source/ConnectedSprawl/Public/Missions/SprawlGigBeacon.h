// The Connected Sprawl - lightweight world marker for repeatable city gigs.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SprawlGigBeacon.generated.h"

class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class USprawlGigSubsystem;

UCLASS()
class CONNECTEDSPRAWL_API ASprawlGigBeacon : public AActor
{
	GENERATED_BODY()

public:
	ASprawlGigBeacon();
	void Configure(USprawlGigSubsystem* InOwner);

private:
	UFUNCTION()
	void HandleTriggerOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UPROPERTY() TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> PillarMesh;
	UPROPERTY() TObjectPtr<USphereComponent> Trigger;
	UPROPERTY() TObjectPtr<USprawlGigSubsystem> OwnerSubsystem;
};
