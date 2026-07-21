// The Connected Sprawl - GPU-animated coastal water surface configuration.

#pragma once

#include "CoreMinimal.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UObject;
class UStaticMeshComponent;

/** Mobile-conscious ocean parameters consumed by the authored water shader. */
struct CONNECTEDSPRAWL_API FSprawlOceanWaveProfile
{
	FLinearColor WaterColor = FLinearColor(0.010f, 0.055f, 0.085f, 1.f);
	float FlowSpeed = 0.40f;
	float MaxDepthCm = 900.f;
	float WaveSizeCm = 900.f;
	float WaveStrength = 2.6f;

	static FSprawlOceanWaveProfile CoastalOcean();
	bool IsValid() const;
	void ApplyTo(UMaterialInstanceDynamic& Material) const;
};

/**
 * Fits an authored plane to the live lake and equips it with GPU-driven waves.
 * It performs no per-frame CPU work and adds no collision or simulation body.
 */
struct CONNECTEDSPRAWL_API FSprawlOceanSurface
{
	static FVector CalculateScale(const FVector2D& SurfaceSize,
		const FBoxSphereBounds& MeshBounds);
	static FVector CalculateLocation(const FVector& SurfaceCenter,
		const FBoxSphereBounds& MeshBounds, const FVector& Scale);
	static bool FitMesh(UStaticMeshComponent& Component,
		const FVector2D& SurfaceSize, const FVector& SurfaceCenter);
	// SafeFallbackBase is a project-owned translucent master used when the
	// authored water parent (DatasmithContent's M_Water) fails to resolve, so
	// the lake never renders as the flat default material or the checker.
	static UMaterialInstanceDynamic* BuildWaveMaterial(
		UStaticMeshComponent& Component, UObject& Owner,
		UMaterialInterface& MaterialBase,
		const FSprawlOceanWaveProfile& Profile,
		UMaterialInterface* SafeFallbackBase = nullptr);
};
