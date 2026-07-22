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
class USceneComponent;

/**
 * ASprawlTrafficLight
 * -------------------
 * A complete signal set for one intersection: four sidewalk-corner poles,
 * each with a north/south and east/west three-lens head. The actor owns only
 * visuals; its colors always query the authoritative city-grid signal phase.
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

	/** Move the actor to the logical intersection centre, retaining its phase IDs. */
	void SnapToIntersection(float GroundZ = 14.f);

protected:
	UPROPERTY(VisibleAnywhere, Category="Signal") TObjectPtr<USceneComponent> SignalRoot;
	UPROPERTY(VisibleAnywhere, Category="Signal") TArray<TObjectPtr<UStaticMeshComponent>> Poles;
	UPROPERTY(VisibleAnywhere, Category="Signal") TArray<TObjectPtr<UStaticMeshComponent>> HeadsNS;
	UPROPERTY(VisibleAnywhere, Category="Signal") TArray<TObjectPtr<UStaticMeshComponent>> HeadsEW;
	UPROPERTY(VisibleAnywhere, Category="Signal") TArray<TObjectPtr<UStaticMeshComponent>> RedLampsNS;
	UPROPERTY(VisibleAnywhere, Category="Signal") TArray<TObjectPtr<UStaticMeshComponent>> AmberLampsNS;
	UPROPERTY(VisibleAnywhere, Category="Signal") TArray<TObjectPtr<UStaticMeshComponent>> GreenLampsNS;
	UPROPERTY(VisibleAnywhere, Category="Signal") TArray<TObjectPtr<UStaticMeshComponent>> RedLampsEW;
	UPROPERTY(VisibleAnywhere, Category="Signal") TArray<TObjectPtr<UStaticMeshComponent>> AmberLampsEW;
	UPROPERTY(VisibleAnywhere, Category="Signal") TArray<TObjectPtr<UStaticMeshComponent>> GreenLampsEW;

	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> RedMatsNS;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> AmberMatsNS;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> GreenMatsNS;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> RedMatsEW;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> AmberMatsEW;
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> GreenMatsEW;

	ESprawlSignal LastNS = ESprawlSignal::Red;
	ESprawlSignal LastEW = ESprawlSignal::Red;
	bool bFirstUpdate = true;

	void ResolveIntersectionIndices();
	void ApplySignalState(const TArray<TObjectPtr<UMaterialInstanceDynamic>>& RedMats,
		const TArray<TObjectPtr<UMaterialInstanceDynamic>>& AmberMats,
		const TArray<TObjectPtr<UMaterialInstanceDynamic>>& GreenMats,
		ESprawlSignal Signal) const;
	void ApplyLensColor(UMaterialInstanceDynamic* Mat, const FLinearColor& Color,
		bool bActive) const;
};
