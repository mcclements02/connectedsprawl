// The Connected Sprawl - Living environment: day/night cycle, sun and moon,
// fog moods, and streetlights that come on at dusk around the player.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SprawlEnvironmentController.generated.h"

class ADirectionalLight;
class ASkyLight;
class AExponentialHeightFog;
class UPointLightComponent;

/**
 * ASprawlEnvironmentController
 * ----------------------------
 * Drop one into the level. It finds the existing sun, sky light, and height
 * fog, then runs a continuous day/night cycle:
 *
 *   - The sun arcs across the sky over DayLengthMinutes of real time, going
 *     warm and low at dawn/dusk, and is swapped for a dim blue moon at night.
 *   - Fog density and color shift with the hour: clear afternoons, hazy
 *     golden evenings, dark blue nights.
 *   - Actors tagged "Streetlight" get a pooled point light that switches on
 *     at dusk. Only the lights nearest the player are active at once, so a
 *     whole city of lamps stays cheap.
 */
UCLASS()
class CONNECTEDSPRAWL_API ASprawlEnvironmentController : public AActor
{
	GENERATED_BODY()

public:
	ASprawlEnvironmentController();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Real minutes for a full 24h in-game day. */
	UPROPERTY(EditAnywhere, Category="Environment") float DayLengthMinutes = 18.f;

	/** Clock time at startup, in hours (0..24). */
	UPROPERTY(EditAnywhere, Category="Environment") float StartHour = 8.071530f;

	/** Freeze the clock (for screenshots / missions). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Environment") bool bPaused = false;

	/** Max sun elevation at noon, degrees. */
	UPROPERTY(EditAnywhere, Category="Environment") float MaxSunElevation = 62.f;

	/** Compass yaw the sun travels along. */
	UPROPERTY(EditAnywhere, Category="Environment") float SunYaw = -38.f;

	/** Authentic balanced daytime targets shared with city_color_pass.py. */
	UPROPERTY(EditAnywhere, Category="Environment|Daylight") float DaySunIntensity = 5.2f;
	UPROPERTY(EditAnywhere, Category="Environment|Daylight")
	FLinearColor DaySunColor = FLinearColor(1.f, 0.86f, 0.70f, 1.f);
	UPROPERTY(EditAnywhere, Category="Environment|Daylight") float DaySkyIntensity = 1.85f;
	UPROPERTY(EditAnywhere, Category="Environment|Daylight") float DayFogDensity = 0.006f;

	/**
	 * Playable-night floors. Auto-exposure is disabled project-wide, so night
	 * readability must come from real light: a stronger blue moon, a higher
	 * ambient sky floor, and thinner fog than the old pitch-black mix.
	 */
	UPROPERTY(EditAnywhere, Category="Environment|Night") float NightMoonIntensity = 1.35f;
	UPROPERTY(EditAnywhere, Category="Environment|Night")
	FLinearColor NightMoonColor = FLinearColor(0.62f, 0.72f, 1.f, 1.f);
	UPROPERTY(EditAnywhere, Category="Environment|Night") float NightSkyIntensity = 0.60f;
	UPROPERTY(EditAnywhere, Category="Environment|Night") float NightFogDensity = 0.0075f;

	/** Streetlights switched on near the player at night (point-light budget). */
	UPROPERTY(EditAnywhere, Category="Environment") int32 MaxActiveStreetlights = 26;
	UPROPERTY(EditAnywhere, Category="Environment") float StreetlightIntensity = 13500.f;
	UPROPERTY(EditAnywhere, Category="Environment") float StreetlightRadius = 3000.f;

	/** Current clock time in hours (0..24). */
	UFUNCTION(BlueprintCallable, Category="Environment")
	float GetHour() const { return Hour; }

	/** True between dusk and dawn. */
	UFUNCTION(BlueprintCallable, Category="Environment")
	bool IsNight() const { return bNight; }

protected:
	UPROPERTY() TObjectPtr<ADirectionalLight> Sun;
	UPROPERTY() TObjectPtr<ASkyLight> Sky;
	UPROPERTY() TObjectPtr<AExponentialHeightFog> Fog;

	/** World positions of every "Streetlight"-tagged lamp in the level. */
	TArray<FVector> LampPositions;

	/** Pooled point lights, repositioned onto the nearest lamps each update. */
	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> LampLights;

	float Hour = 8.071530f;
	bool bNight = false;
	float LampUpdateTimer = 0.f;

	void FindSceneActors();
	void BuildLampPool();
	void UpdateSunAndSky();
	void UpdateStreetlights(const FVector& PlayerLoc);
};
