// The Connected Sprawl - Zonal Density streaming implementation.

#include "Streaming/ZonalStreamingManager.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

AZonalStreamingManager::AZonalStreamingManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f; // don't need every frame
}

void AZonalStreamingManager::BeginPlay()
{
	Super::BeginPlay();

	// Kick a first evaluation.
	EvaluateStreaming();
}

void AZonalStreamingManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TimeSinceEval += DeltaSeconds;
	if (TimeSinceEval < EvaluateInterval) return;
	TimeSinceEval = 0.f;

	EvaluateStreaming();
}

void AZonalStreamingManager::EvaluateStreaming()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !PC->GetPawn()) return;

	const APawn* Pawn = PC->GetPawn();
	const FVector PlayerLoc = Pawn->GetActorLocation();
	const float SpeedMph = Pawn->GetVelocity().Size() * 0.0223694f;
	const bool bArteryMode = SpeedMph >= ArterySpeedMph;

	// If flying down the highway, predict 3 seconds ahead.
	const FVector EvalLocation = bArteryMode
		? PlayerLoc + Pawn->GetVelocity() * 3.f
		: PlayerLoc;

	for (const FZoneDefinition& Zone : Zones)
	{
		const bool bIsLoaded   = LoadedLevels.Contains(Zone.ZoneId);
		const bool bInside     = IsWithinZone(EvalLocation, Zone, /*expand*/ false);
		const bool bStillNear  = IsWithinZone(EvalLocation, Zone, /*expand*/ true);

		if (!bIsLoaded && bInside)
		{
			LoadZone(Zone);
		}
		else if (bIsLoaded && !bStillNear)
		{
			UnloadZone(Zone.ZoneId);
		}
	}

	// Calculate player's current active zone
	EZoneId NewZone = EZoneId::Junction; // default fallback

	if (bArteryMode)
	{
		NewZone = EZoneId::Arteries;
	}
	else
	{
		float MinDistanceSq = FLT_MAX;
		bool bFoundInside = false;

		for (const FZoneDefinition& Zone : Zones)
		{
			const float DistSq = FVector::DistSquared(PlayerLoc, Zone.Center);
			const float RadiusSq = Zone.StreamRadius * Zone.StreamRadius;

			if (DistSq <= RadiusSq)
			{
				if (!bFoundInside || DistSq < MinDistanceSq)
				{
					bFoundInside = true;
					NewZone = Zone.ZoneId;
					MinDistanceSq = DistSq;
				}
			}
			else if (!bFoundInside && DistSq < MinDistanceSq)
			{
				NewZone = Zone.ZoneId;
				MinDistanceSq = DistSq;
			}
		}
	}

	if (NewZone != CurrentActiveZone)
	{
		EZoneId OldZone = CurrentActiveZone;
		CurrentActiveZone = NewZone;
		OnZoneChanged.Broadcast(CurrentActiveZone, OldZone);
		UE_LOG(LogTemp, Log, TEXT("[ZonalStreaming] Active Zone changed: %d -> %d"), (int32)OldZone, (int32)CurrentActiveZone);
	}
}

bool AZonalStreamingManager::IsWithinZone(const FVector& PlayerLocation, const FZoneDefinition& Zone, bool bUseHysteresisExpanded) const
{
	const float R = bUseHysteresisExpanded ? (Zone.StreamRadius + HysteresisPadding) : Zone.StreamRadius;
	return FVector::DistSquared(PlayerLocation, Zone.Center) <= (R * R);
}

void AZonalStreamingManager::LoadZone(const FZoneDefinition& Zone)
{
	if (Zone.HighDensityLevel.IsNull()) return;

	bool bSuccess = false;
	FLatentActionInfo LatentInfo; // default — fire-and-forget

	ULevelStreamingDynamic* Streaming = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(
		this,
		TSoftObjectPtr<UWorld>(Zone.HighDensityLevel),
		Zone.Center,
		FRotator::ZeroRotator,
		bSuccess);

	if (bSuccess && Streaming)
	{
		LoadedLevels.Add(Zone.ZoneId, Streaming);
		UE_LOG(LogTemp, Log, TEXT("[ZonalStreaming] LOAD zone=%d level=%s"),
			(int32)Zone.ZoneId, *Zone.HighDensityLevel.ToString());
	}
}

void AZonalStreamingManager::UnloadZone(EZoneId ZoneId)
{
	if (ULevelStreamingDynamic** Found = LoadedLevels.Find(ZoneId))
	{
		if (*Found)
		{
			(*Found)->SetShouldBeLoaded(false);
			(*Found)->SetShouldBeVisible(false);
		}
		LoadedLevels.Remove(ZoneId);
		UE_LOG(LogTemp, Log, TEXT("[ZonalStreaming] UNLOAD zone=%d"), (int32)ZoneId);
	}
}
