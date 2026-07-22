// The Connected Sprawl - Street dressing repair: signs, junction props, and
// signal poles standing correctly on the sidewalks.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/SprawlCityGridSubsystem.h"
#include "SprawlStreetDressing.generated.h"

class UStaticMeshComponent;
class UWorld;

/** Pure kerb/corner placement math shared by the repair actor and tests. */
struct CONNECTEDSPRAWL_API FSprawlKerbPlacement
{
	/** Sidewalk band offsets from a road centreline (kerb at RoadWidth/2). */
	static constexpr float SidewalkInnerOffset =
		USprawlCityGridSubsystem::RoadWidth * 0.5f + 70.f;
	static constexpr float SidewalkOuterOffset =
		USprawlCityGridSubsystem::BlockSize * 0.5f - 80.f;
	static constexpr float SignKerbOffset = 850.f;    // A-boards mid-band
	static constexpr float PropCornerOffset = 880.f;  // junction dressing
	static constexpr float SignalCornerOffset = 760.f; // poles hug the kerb
	static constexpr float SidewalkTopZ = 14.f;

	/** Nearest kerb-band point beside the nearest road, keeping the side. */
	static FVector2D KerbPointFor(const FVector2D& Position, float Offset);
	/** Nearest junction-corner point, keeping the actor's quadrant. */
	static FVector2D JunctionCornerFor(const FVector2D& Position, float Offset);
	/** True when the point stands inside a carriageway corridor. */
	static bool IsInCarriageway(const FVector2D& Position);
	/** True only on the paved pedestrian band, never a lane, bay, or intersection. */
	static bool IsOnSidewalk(const FVector2D& Position);
	/** True when the point sits close to a non-park block centre. */
	static bool IsAtStrandedBlockCentre(const FVector2D& Position);
};

/**
 * Authoring passes left street furniture in impossible states the ledger has
 * tracked for a while: junction dressing recentred onto block centres,
 * A-board signs drifting onto asphalt after the street widening, and missing
 * / old single-pole traffic signals. This actor keeps every freestanding sign
 * inside the explicit sidewalk band and ensures one complete signal model at
 * every dry intersection. Matching is by static-mesh name, so sign repair
 * works in cooked builds with no editor labels.
 */
UCLASS(NotPlaceable, Transient)
class CONNECTEDSPRAWL_API ASprawlStreetDressing : public AActor
{
	GENERATED_BODY()

public:
	ASprawlStreetDressing();

	virtual void BeginPlay() override;

	/** Spawn exactly one dressing-repair actor for the active world. */
	static ASprawlStreetDressing* EnsureForWorld(UWorld* World);

private:
	int32 RepairFoldoutSigns();
	int32 RepairStrandedJunctionProps();
	int32 RepairWallSignsInCarriageway();
	int32 EnsureIntersectionSignals(int32& OutSpawned);
};
