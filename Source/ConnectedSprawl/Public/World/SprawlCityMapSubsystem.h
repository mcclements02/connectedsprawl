// The Connected Sprawl - shared city-map data and coordinate projection.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SprawlCityMapSubsystem.generated.h"

UENUM(BlueprintType)
enum class ESprawlMapLandmarkType : uint8
{
	Store,
	Office,
	Condo,
	Parking,
	Waterfront
};

USTRUCT(BlueprintType)
struct CONNECTEDSPRAWL_API FSprawlMapLandmark
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Map") FName Id = NAME_None;
	UPROPERTY(BlueprintReadOnly, Category="Map") FText Label;
	UPROPERTY(BlueprintReadOnly, Category="Map") ESprawlMapLandmarkType Type = ESprawlMapLandmarkType::Store;
	UPROPERTY(BlueprintReadOnly, Category="Map") FVector WorldLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category="Map") FLinearColor Color = FLinearColor::White;
	UPROPERTY(BlueprintReadOnly, Category="Map") bool bEnterable = false;

	bool IsValid() const { return !Id.IsNone() && !Label.IsEmpty(); }
};

/**
 * One source of map markers for native UI, Blueprint, and future mission GPS.
 * Interior player positions are projected back onto their exterior doorway so
 * the map never shows Zarri floating above the city in an interior pocket.
 */
UCLASS()
class CONNECTEDSPRAWL_API USprawlCityMapSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintPure, Category="Sprawl|Map")
	TArray<FSprawlMapLandmark> GetMapLandmarks() const { return Landmarks; }

	const TArray<FSprawlMapLandmark>& GetLandmarksView() const { return Landmarks; }

	UFUNCTION(BlueprintPure, Category="Sprawl|Map")
	FVector GetDisplayLocationForActor(const AActor* Actor) const;

	/** World XY projected into a bottom-left-origin [0,1] city map. */
	UFUNCTION(BlueprintPure, Category="Sprawl|Map")
	static FVector2D WorldToMapNormalized(const FVector& WorldLocation);

	static TArray<FSprawlMapLandmark> BuildDefaultLandmarks();

private:
	UPROPERTY(Transient)
	TArray<FSprawlMapLandmark> Landmarks;
};
