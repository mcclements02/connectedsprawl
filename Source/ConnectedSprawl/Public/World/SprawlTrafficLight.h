// The Connected Sprawl - Intersection traffic signal.
// Visual-only: the authoritative phase lives in USprawlCityGridSubsystem,
// so cars and signal heads can never disagree.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/SprawlCityGridSubsystem.h"
#include "SprawlTrafficLight.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * ASprawlTrafficLight
 * -------------------
 * A signal pole for one intersection: a vertical pole on the corner with two
 * lamp heads, one facing north/south traffic and one facing east/west. Each
 * head is a single lamp whose color tracks that axis' phase (green / amber /
 * red). Drop it near IntersectionCenter(Ix, Iy); it figures the rest out.
 */
UCLASS()
class CONNECTEDSPRAWL_API ASprawlTrafficLight : public AActor
{
	GENERATED_BODY()

public:
	ASprawlTrafficLight();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Intersection grid coordinates. Auto-detected from location if < 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Signal") int32 IntersectionX = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Signal") int32 IntersectionY = -1;

protected:
	UPROPERTY(VisibleAnywhere, Category="Signal") TObjectPtr<UStaticMeshComponent> Pole;
	UPROPERTY(VisibleAnywhere, Category="Signal") TObjectPtr<UStaticMeshComponent> HeadNS;
	UPROPERTY(VisibleAnywhere, Category="Signal") TObjectPtr<UStaticMeshComponent> HeadEW;
	UPROPERTY(VisibleAnywhere, Category="Signal") TObjectPtr<UStaticMeshComponent> LampNS;
	UPROPERTY(VisibleAnywhere, Category="Signal") TObjectPtr<UStaticMeshComponent> LampEW;

	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> LampMatNS;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> LampMatEW;

	ESprawlSignal LastNS = ESprawlSignal::Red;
	ESprawlSignal LastEW = ESprawlSignal::Red;
	bool bFirstUpdate = true;

	void ApplySignalColor(UMaterialInstanceDynamic* Mat, ESprawlSignal Signal) const;
};
