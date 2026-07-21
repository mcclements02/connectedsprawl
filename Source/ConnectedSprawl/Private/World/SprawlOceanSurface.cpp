// The Connected Sprawl - GPU-animated coastal water surface configuration.

#include "World/SprawlOceanSurface.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
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
	// Project-owned translucent fallback (M_Simple_Glass) parameters. The
	// DatasmithContent water path simply ignores the names it does not expose,
	// so setting both keeps the coastal look correct whichever base is active.
	Material.SetVectorParameterValue(TEXT("Color"), WaterColor);
	Material.SetScalarParameterValue(TEXT("Opacity"), 0.72f);
	Material.SetScalarParameterValue(TEXT("Refraction"), 1.033f);
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
	// Inflate the fit slightly so the water edge tucks under the surrounding
	// shore ground rather than meeting it in a hard, z-fighting seam.
	constexpr float EdgeOverlap = 1.04f;
	const FVector Scale = CalculateScale(SurfaceSize * EdgeOverlap, Bounds);
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
	UMaterialInterface& MaterialBase, const FSprawlOceanWaveProfile& Profile,
	UMaterialInterface* SafeFallbackBase)
{
	if (!Profile.IsValid())
	{
		return nullptr;
	}
	UMaterialInterface* Base = &MaterialBase;
	// If the authored water parent (DatasmithContent's M_Water) did not resolve,
	// the instance collapses to the engine default material and would render as
	// a flat opaque surface (or the checker). Swap to the project-owned
	// translucent master so the lake is always water on every cook target.
	const UMaterial* Root = MaterialBase.GetMaterial();
	const bool bBaseBroken =
		!Root || Root == UMaterial::GetDefaultMaterial(MD_Surface);
	if (bBaseBroken && SafeFallbackBase)
	{
		Base = SafeFallbackBase;
	}
	UMaterialInstanceDynamic* Material =
		UMaterialInstanceDynamic::Create(Base, &Owner);
	if (!Material)
	{
		return nullptr;
	}
	Profile.ApplyTo(*Material);
	Component.SetMaterial(0, Material);
	Component.SetTranslucentSortPriority(-5);
	return Material;
}
