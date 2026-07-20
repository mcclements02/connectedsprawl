// The Connected Sprawl - GPU-animated coastal water surface configuration.

#include "World/SprawlOceanSurface.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

FSprawlOceanWaveProfile FSprawlOceanWaveProfile::CoastalOcean()
{
	return FSprawlOceanWaveProfile();
}

bool FSprawlOceanWaveProfile::IsValid() const
{
	return WaterColor.A > 0.f
		&& FlowSpeed > 0.f
		&& MaxDepthCm > 0.f
		&& WaveSizeCm > 0.f
		&& WaveStrength > 0.f;
}

void FSprawlOceanWaveProfile::ApplyTo(UMaterialInstanceDynamic& Material) const
{
	if (!IsValid())
	{
		return;
	}
	// These names are exposed by DatasmithContent's M_Water parent. BaseColor,
	// Metallic, and Roughness retain a safe appearance if a simpler fallback
	// material is substituted in a stripped target build.
	Material.SetVectorParameterValue(TEXT("WaterColor"), WaterColor);
	Material.SetScalarParameterValue(TEXT("FlowSpeed"), FlowSpeed);
	Material.SetScalarParameterValue(TEXT("MaxDepth (cm)"), MaxDepthCm);
	Material.SetScalarParameterValue(TEXT("WaveSize"), WaveSizeCm);
	Material.SetScalarParameterValue(TEXT("WaveStrength"), WaveStrength);
	Material.SetVectorParameterValue(TEXT("BaseColor"), WaterColor);
	Material.SetScalarParameterValue(TEXT("Metallic"), 0.f);
	Material.SetScalarParameterValue(TEXT("Roughness"), 0.08f);
}

FVector FSprawlOceanSurface::CalculateScale(const FVector2D& SurfaceSize,
	const FBoxSphereBounds& MeshBounds)
{
	const FVector MeshSize = MeshBounds.BoxExtent * 2.f;
	if (SurfaceSize.X <= 0.f || SurfaceSize.Y <= 0.f
		|| MeshSize.X <= UE_SMALL_NUMBER || MeshSize.Y <= UE_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}
	return FVector(SurfaceSize.X / MeshSize.X, SurfaceSize.Y / MeshSize.Y, 1.f);
}

FVector FSprawlOceanSurface::CalculateLocation(const FVector& SurfaceCenter,
	const FBoxSphereBounds& MeshBounds, const FVector& Scale)
{
	return SurfaceCenter - MeshBounds.Origin * Scale;
}

bool FSprawlOceanSurface::FitMesh(UStaticMeshComponent& Component,
	const FVector2D& SurfaceSize, const FVector& SurfaceCenter)
{
	const UStaticMesh* Mesh = Component.GetStaticMesh();
	if (!Mesh)
	{
		return false;
	}
	const FBoxSphereBounds Bounds = Mesh->GetBounds();
	const FVector Scale = CalculateScale(SurfaceSize, Bounds);
	if (Scale.IsNearlyZero())
	{
		return false;
	}
	Component.SetRelativeScale3D(Scale);
	Component.SetRelativeLocation(CalculateLocation(SurfaceCenter, Bounds, Scale));
	// The authored shader displaces vertices above its static mesh bounds.
	Component.SetBoundsScale(1.5f);
	return true;
}

UMaterialInstanceDynamic* FSprawlOceanSurface::BuildWaveMaterial(
	UStaticMeshComponent& Component, UObject& Owner,
	UMaterialInterface& MaterialBase, const FSprawlOceanWaveProfile& Profile)
{
	if (!Profile.IsValid())
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* Material =
		UMaterialInstanceDynamic::Create(&MaterialBase, &Owner);
	if (!Material)
	{
		return nullptr;
	}
	Profile.ApplyTo(*Material);
	Component.SetMaterial(0, Material);
	Component.SetTranslucentSortPriority(-5);
	return Material;
}
