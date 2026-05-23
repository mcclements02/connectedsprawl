// The Connected Sprawl - Zonal visuals manager
// Manages post-process weight blending, fog interpolation, and dynamic cVars per zone.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Streaming/ZonalStreamingManager.h"
#include "ZonalVisualsManager.generated.h"

class APostProcessVolume;
class AExponentialHeightFog;

USTRUCT(BlueprintType)
struct FZoneVisualSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visuals")
	EZoneId ZoneId = EZoneId::Junction;

	/** The post-process volume for this zone. It should be unbound (globally active). 
	    We will dynamically interpolate its BlendWeight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visuals")
	TObjectPtr<APostProcessVolume> PostProcessVolume = nullptr;

	/** Target fog density for this zone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visuals")
	float FogDensity = 0.02f;

	/** Target fog inscattering color for this zone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visuals")
	FLinearColor FogInscatteringColor = FLinearColor(0.45f, 0.55f, 0.65f, 1.f);

	/** Target volumetric fog extinction scale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visuals")
	float VolumetricFogExtinctionScale = 1.0f;

	/** Target volumetric fog albedo/scattering color. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visuals")
	FColor VolumetricFogAlbedo = FColor(128, 128, 128, 255);

	/** Whether volumetric fog should be enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visuals")
	bool bEnableVolumetricFog = false;

	/** Console variables to apply on entering this zone. E.g. r.ReflectionMethod = 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visuals")
	TMap<FString, FString> ConsoleVariables;
};

/**
 * AZonalVisualsManager
 * -------------------
 * Manages post-processing profiles, height fog parameters, and rendering path 
 * console variables dynamically depending on the player's active zone.
 */
UCLASS()
class CONNECTEDSPRAWL_API AZonalVisualsManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AZonalVisualsManager();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	/** Configured visuals per zone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zonal Visuals")
	TArray<FZoneVisualSettings> ZoneSettings;

	/** Speed of post-process weight and fog parameter interpolation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Zonal Visuals")
	float InterpSpeed = 2.0f;

protected:
	/** Found height fog actor in the level. */
	UPROPERTY(Transient)
	TObjectPtr<AExponentialHeightFog> HeightFogActor = nullptr;

	/** Found streaming manager actor in the level. */
	UPROPERTY(Transient)
	TObjectPtr<AZonalStreamingManager> StreamingManager = nullptr;

	/** Active target zone. Updated via streaming manager event. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Zonal Visuals")
	EZoneId TargetZone = EZoneId::Junction;

	/** Finds the Exponential Height Fog actor in the level. */
	void FindFogActor();

	/** Finds the Zonal Streaming Manager in the level. */
	void FindStreamingManager();

	/** Delegate handler for when the active zone changes. */
	UFUNCTION()
	void OnActiveZoneChanged(EZoneId NewZoneId, EZoneId OldZoneId);

	/** Applies console variables for the specified zone. */
	void ApplyConsoleVariables(EZoneId ZoneId);
};
