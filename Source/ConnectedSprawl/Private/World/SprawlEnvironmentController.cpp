// The Connected Sprawl - Living environment controller.

#include "World/SprawlEnvironmentController.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

ASprawlEnvironmentController::ASprawlEnvironmentController()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ASprawlEnvironmentController::BeginPlay()
{
	Super::BeginPlay();
	Hour = FMath::Fmod(StartHour, 24.f);
	FindSceneActors();
	BuildLampPool();
	UpdateSunAndSky();
}

void ASprawlEnvironmentController::FindSceneActors()
{
	UWorld* World = GetWorld();
	for (TActorIterator<ADirectionalLight> It(World); It; ++It) { Sun = *It; break; }
	for (TActorIterator<ASkyLight> It(World); It; ++It)         { Sky = *It; break; }
	for (TActorIterator<AExponentialHeightFog> It(World); It; ++It) { Fog = *It; break; }

	// The cycle needs to move the sun every frame.
	if (Sun && Sun->GetLightComponent())
	{
		Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	}
	if (Sky && Sky->GetLightComponent())
	{
		Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	}
}

void ASprawlEnvironmentController::BuildLampPool()
{
	LampPositions.Reset();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("Streetlight")))
		{
			LampPositions.Add(It->GetActorLocation());
		}
	}

	const int32 PoolSize = FMath::Min(MaxActiveStreetlights, LampPositions.Num());
	for (int32 i = 0; i < PoolSize; ++i)
	{
		UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetIntensity(9000.f);                       // lumens-ish; warm sodium
		Light->SetLightColor(FLinearColor(1.0f, 0.72f, 0.38f));
		Light->SetAttenuationRadius(2400.f);
		Light->SetCastShadows(false);                      // dozens of these — keep them cheap
		Light->SetVisibility(false);
		Light->SetupAttachment(RootComponent);
		Light->SetUsingAbsoluteLocation(true);
		Light->RegisterComponent();
		LampLights.Add(Light);
	}

	UE_LOG(LogTemp, Log, TEXT("[Environment] %d streetlights found, %d pooled lights"),
		LampPositions.Num(), LampLights.Num());
}

void ASprawlEnvironmentController::UpdateSunAndSky()
{
	// Solar elevation: peaks at noon, dips below the horizon at night.
	const float SunElevation = FMath::Sin((Hour - 6.f) / 12.f * PI) * MaxSunElevation;
	bNight = SunElevation < 2.f;

	if (Sun)
	{
		if (SunElevation > 0.f)
		{
			// Daytime sun: warm white at altitude, deep orange when low.
			Sun->SetActorRotation(FRotator(-SunElevation, SunYaw, 0.f));
			const float LowSun = FMath::Clamp(SunElevation / 18.f, 0.f, 1.f);
			const FLinearColor Color = FMath::Lerp(
				FLinearColor(1.0f, 0.45f, 0.18f),   // horizon
				FLinearColor(1.0f, 0.95f, 0.86f),   // noon
				LowSun);
			const float Intensity = FMath::Lerp(2.2f, 9.5f, FMath::Clamp(SunElevation / 30.f, 0.f, 1.f));
			if (ULightComponent* L = Sun->GetLightComponent())
			{
				L->SetIntensity(Intensity);
				L->SetLightColor(Color);
			}
		}
		else
		{
			// Night: the directional light plays a dim blue moon opposite the sun.
			const float MoonElevation = FMath::Clamp(-SunElevation, 8.f, 45.f);
			Sun->SetActorRotation(FRotator(-MoonElevation, SunYaw + 180.f, 0.f));
			if (ULightComponent* L = Sun->GetLightComponent())
			{
				L->SetIntensity(0.6f);
				L->SetLightColor(FLinearColor(0.55f, 0.65f, 0.95f));
			}
		}
	}

	if (Sky)
	{
		if (USkyLightComponent* SkyComp = Cast<USkyLightComponent>(Sky->GetLightComponent()))
		{
			const float DayAmount = FMath::Clamp(SunElevation / 12.f, 0.f, 1.f);
			SkyComp->SetIntensity(FMath::Lerp(0.25f, 1.4f, DayAmount));
		}
	}

	if (Fog && Fog->GetComponent())
	{
		UExponentialHeightFogComponent* FogComp = Fog->GetComponent();
		const float DayAmount = FMath::Clamp(SunElevation / 12.f, 0.f, 1.f);
		// Golden-hour haze: thickest right around sunrise/sunset.
		const float Twilight = FMath::Clamp(1.f - FMath::Abs(SunElevation) / 10.f, 0.f, 1.f);
		FogComp->SetFogDensity(0.010f + DayAmount * 0.002f + Twilight * 0.014f);

		const FLinearColor DayFog(0.45f, 0.55f, 0.72f);
		const FLinearColor DuskFog(0.85f, 0.48f, 0.28f);
		const FLinearColor NightFog(0.02f, 0.035f, 0.075f);
		FLinearColor FogColor = FMath::Lerp(NightFog, DayFog, DayAmount);
		FogColor = FMath::Lerp(FogColor, DuskFog, Twilight * 0.7f);
		FogComp->SetFogInscatteringColor(FogColor);
	}
}

void ASprawlEnvironmentController::UpdateStreetlights(const FVector& PlayerLoc)
{
	if (LampLights.Num() == 0)
	{
		return;
	}

	if (!bNight)
	{
		for (UPointLightComponent* Light : LampLights)
		{
			if (Light) { Light->SetVisibility(false); }
		}
		return;
	}

	// Pick the lamps nearest the player and park the pooled lights on them.
	TArray<int32> Order;
	Order.Reserve(LampPositions.Num());
	for (int32 i = 0; i < LampPositions.Num(); ++i)
	{
		Order.Add(i);
	}
	Order.Sort([&](int32 A, int32 B)
	{
		return FVector::DistSquared2D(LampPositions[A], PlayerLoc)
		     < FVector::DistSquared2D(LampPositions[B], PlayerLoc);
	});

	for (int32 i = 0; i < LampLights.Num(); ++i)
	{
		UPointLightComponent* Light = LampLights[i];
		if (!Light) continue;
		if (i < Order.Num())
		{
			Light->SetWorldLocation(LampPositions[Order[i]] + FVector(0, 0, -40.f));
			Light->SetVisibility(true);
		}
		else
		{
			Light->SetVisibility(false);
		}
	}
}

void ASprawlEnvironmentController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bPaused && DayLengthMinutes > 0.1f)
	{
		Hour = FMath::Fmod(Hour + DeltaSeconds * 24.f / (DayLengthMinutes * 60.f), 24.f);
	}

	UpdateSunAndSky();

	LampUpdateTimer -= DeltaSeconds;
	if (LampUpdateTimer <= 0.f)
	{
		LampUpdateTimer = 1.0f;
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				UpdateStreetlights(Pawn->GetActorLocation());
			}
		}
	}
}
