// The Connected Sprawl - Zonal Density streaming
// Implements the "Lite Tech Strategy" from the design doc:
//   - Load only the 1-mile radius around Zarri at full density
//   - Keep everything outside as low-poly silhouettes
//   - Mask transitions using highway speed / motion blur

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/StreamableManager.h"
#include "ZonalStreamingManager.generated.h"

class ULevelStreamingDynamic;

UENUM(BlueprintType)
enum class EZoneId : uint8
{
	Junction     UMETA(DisplayName = "The Junction (Core)"),
	Arteries     UMETA(DisplayName = "The 85-Express (Highways)"),
	IronForest   UMETA(DisplayName = "Iron Forest (North)"),
	RailYards    UMETA(DisplayName = "Rail Yards (South)")
};

USTRUCT(BlueprintType)
struct FZoneDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) EZoneId ZoneId = EZoneId::Junction;

	/** Sub-level to stream at full density when the player is within radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowedClasses="/Script/Engine.World"))
	FSoftObjectPath HighDensityLevel;

	/** Low-poly silhouette level used as the distant proxy. Always loaded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowedClasses="/Script/Engine.World"))
	FSoftObjectPath ProxyLevel;

	/** World-space center of the zone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector Center = FVector::ZeroVector;

	/** Streaming radius in centimeters. 1 mile ≈ 160934 cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float StreamRadius = 160934.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnZoneChanged, EZoneId, NewZoneId, EZoneId, OldZoneId);

/**
 * AZonalStreamingManager
 * ----------------------
 * A single actor placed in the persistent level. It tracks the player's Mobile
 * Office, chooses which zones to stream in at full density, and swaps out proxy
 * silhouettes. Tuned for iPhone: targets 60fps with a ~1-mile high-detail ring.
 *
 * Transition rules:
 *   - While below 85mph: hysteresis load/unload at radius edge.
 *   - While above 85mph ("Artery Mode"): aggressively pre-stream the next zone
 *     along the player's velocity vector, and rely on motion blur to hide pops.
 */
UCLASS()
class CONNECTEDSPRAWL_API AZonalStreamingManager : public AActor
{
	GENERATED_BODY()

public:
	AZonalStreamingManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Broadcasts when the player's active zone changes. */
	UPROPERTY(BlueprintAssignable, Category="Streaming")
	FOnZoneChanged OnZoneChanged;

	/** Gets the player's current active zone. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Streaming")
	EZoneId GetCurrentZone() const { return CurrentActiveZone; }

	/** Authored in the editor / data. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Streaming")
	TArray<FZoneDefinition> Zones;

	/** How often to re-evaluate streaming decisions (seconds). */
	UPROPERTY(EditAnywhere, Category="Streaming") float EvaluateInterval = 0.5f;

	/** Above this speed in mph, use predictive pre-streaming. */
	UPROPERTY(EditAnywhere, Category="Streaming") float ArterySpeedMph = 85.f;

	/** Hysteresis padding (cm). Prevents thrash at zone boundaries. */
	UPROPERTY(EditAnywhere, Category="Streaming") float HysteresisPadding = 20000.f;

protected:
	float TimeSinceEval = 0.f;

	/** Currently-loaded high-density sublevels, keyed by ZoneId. */
	UPROPERTY() TMap<EZoneId, ULevelStreamingDynamic*> LoadedLevels;

	/** The current active zone of the player. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Streaming")
	EZoneId CurrentActiveZone = EZoneId::Junction;

	void EvaluateStreaming();
	bool IsWithinZone(const FVector& PlayerLocation, const FZoneDefinition& Zone, bool bUseHysteresisExpanded) const;

	void LoadZone(const FZoneDefinition& Zone);
	void UnloadZone(EZoneId ZoneId);
};
