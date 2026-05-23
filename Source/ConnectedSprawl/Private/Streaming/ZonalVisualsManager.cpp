// The Connected Sprawl - Zonal visuals manager implementation

#include "Streaming/ZonalVisualsManager.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

AZonalVisualsManager::AZonalVisualsManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0f; // Tick every frame for smooth visuals transition
}

void AZonalVisualsManager::BeginPlay()
{
	Super::BeginPlay();

	FindFogActor();
	FindStreamingManager();

	// Initialize PP volumes weights to 0.f
	for (const FZoneVisualSettings& Settings : ZoneSettings)
	{
		if (Settings.PostProcessVolume)
		{
			Settings.PostProcessVolume->BlendWeight = 0.0f;
			Settings.PostProcessVolume->bUnbound = true; // Ensure they are globally active
			Settings.PostProcessVolume->bEnabled = true;
		}
	}

	if (StreamingManager)
	{
		StreamingManager->OnZoneChanged.AddDynamic(this, &AZonalVisualsManager::OnActiveZoneChanged);
		TargetZone = StreamingManager->GetCurrentZone();
	}
	else
	{
		TargetZone = EZoneId::Junction;
	}

	// Apply initial console variables
	ApplyConsoleVariables(TargetZone);
}

void AZonalVisualsManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 1. Blend Post Process volumes smoothly
	for (const FZoneVisualSettings& Settings : ZoneSettings)
	{
		if (Settings.PostProcessVolume)
		{
			const float TargetWeight = (Settings.ZoneId == TargetZone) ? 1.0f : 0.0f;
			Settings.PostProcessVolume->BlendWeight = FMath::FInterpTo(
				Settings.PostProcessVolume->BlendWeight,
				TargetWeight,
				DeltaTime,
				InterpSpeed
			);
		}
	}

	// 2. Interpolate Exponential Height Fog parameters
	if (HeightFogActor && HeightFogActor->GetComponent())
	{
		UExponentialHeightFogComponent* FogComp = HeightFogActor->GetComponent();
		
		const FZoneVisualSettings* TargetSettings = nullptr;
		for (const FZoneVisualSettings& Settings : ZoneSettings)
		{
			if (Settings.ZoneId == TargetZone)
			{
				TargetSettings = &Settings;
				break;
			}
		}

		if (TargetSettings)
		{
			// Interpolate density
			FogComp->FogDensity = FMath::FInterpTo(FogComp->FogDensity, TargetSettings->FogDensity, DeltaTime, InterpSpeed);
			
			// Interpolate inscattering color
			FLinearColor NewColor = FMath::CInterpTo(FogComp->FogInscatteringLuminance, TargetSettings->FogInscatteringColor, DeltaTime, InterpSpeed);
			FogComp->SetFogInscatteringColor(NewColor);
			
			// Toggle and interpolate volumetric fog properties
			FogComp->bEnableVolumetricFog = TargetSettings->bEnableVolumetricFog;
			if (FogComp->bEnableVolumetricFog)
			{
				FogComp->VolumetricFogExtinctionScale = FMath::FInterpTo(FogComp->VolumetricFogExtinctionScale, TargetSettings->VolumetricFogExtinctionScale, DeltaTime, InterpSpeed);
				
				FLinearColor CurrentAlbedo(FogComp->VolumetricFogAlbedo);
				FLinearColor TargetAlbedo(TargetSettings->VolumetricFogAlbedo);
				FLinearColor NewAlbedo = FMath::CInterpTo(CurrentAlbedo, TargetAlbedo, DeltaTime, InterpSpeed);
				FogComp->SetVolumetricFogAlbedo(NewAlbedo.ToFColor(true));
			}
		}
	}
}

void AZonalVisualsManager::FindFogActor()
{
	for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
	{
		HeightFogActor = *It;
		break;
	}

	if (!HeightFogActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ZonalVisuals] No ExponentialHeightFog actor found in level. Fog adjustments will not apply."));
	}
}

void AZonalVisualsManager::FindStreamingManager()
{
	for (TActorIterator<AZonalStreamingManager> It(GetWorld()); It; ++It)
	{
		StreamingManager = *It;
		break;
	}

	if (!StreamingManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ZonalVisuals] No AZonalStreamingManager actor found in level. Visuals will not switch automatically."));
	}
}

void AZonalVisualsManager::OnActiveZoneChanged(EZoneId NewZoneId, EZoneId OldZoneId)
{
	TargetZone = NewZoneId;
	ApplyConsoleVariables(NewZoneId);
	UE_LOG(LogTemp, Log, TEXT("[ZonalVisuals] Visuals target zone changed: %d -> %d"), (int32)OldZoneId, (int32)NewZoneId);
}

void AZonalVisualsManager::ApplyConsoleVariables(EZoneId ZoneId)
{
	const FZoneVisualSettings* TargetSettings = nullptr;
	for (const FZoneVisualSettings& Settings : ZoneSettings)
	{
		if (Settings.ZoneId == ZoneId)
		{
			TargetSettings = &Settings;
			break;
		}
	}

	if (!TargetSettings || !GEngine) return;

	for (const auto& Kvp : TargetSettings->ConsoleVariables)
	{
		FString FullCmd = FString::Printf(TEXT("%s %s"), *Kvp.Key, *Kvp.Value);
		GEngine->Exec(GetWorld(), *FullCmd);
		UE_LOG(LogTemp, Log, TEXT("[ZonalVisuals] Applied Console Variable: %s"), *FullCmd);
	}
}
