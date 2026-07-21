// The Connected Sprawl - Runtime repair of checker/default ground surfaces.

#include "World/SprawlSurfaceRepair.h"

#include "World/SprawlWaterfrontScenery.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
// A surface within this band of the lake reads as a waterfront promenade
// rather than street asphalt. Roughly one grid Step (block + road).
constexpr float PromenadeMargin = 3200.f;

// Only large surfaces (a ground plane) are repaired when their material is
// merely missing; small prop slots with an empty material are left untouched.
constexpr float LargeSurfaceRadius = 1500.f;

// Warm light stone for the waterfront; dry dark asphalt for the streets. Both
// keep enough luminance to stay clear of the visual-audit crush gate.
const FLinearColor PromenadeStone(0.145f, 0.135f, 0.120f, 1.f);
const FLinearColor StreetAsphalt(0.050f, 0.050f, 0.055f, 1.f);
}

ASprawlSurfaceRepair::ASprawlSurfaceRepair()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorEnableCollision(false);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> Opaque(
		TEXT("/Game/Materials/Master/M_Simple_Opaque.M_Simple_Opaque"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> Asphalt(
		TEXT("/Game/Import/CityKit/MI_AsphaltWorn.MI_AsphaltWorn"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> Concrete(
		TEXT("/Game/Import/CityKit/MI_ConcreteWorn.MI_ConcreteWorn"));
	OpaqueBase = Opaque.Get();
	AsphaltMaterial = Asphalt.Get();
	ConcreteMaterial = Concrete.Get();
}

bool ASprawlSurfaceRepair::IsCheckerMaterial(const UMaterialInterface* Material)
{
	if (!Material)
	{
		return false;
	}
	const UMaterial* Root = Material->GetMaterial();
	// Match by name so the check needs no live pointer to the engine asset and
	// survives whether the checker was assigned directly or reached via a
	// parent that failed to resolve.
	return Root && Root->GetFName() == TEXT("WorldGridMaterial");
}

bool ASprawlSurfaceRepair::IsMissingMaterial(const UMaterialInterface* Material)
{
	if (!Material)
	{
		return true;
	}
	const UMaterial* Root = Material->GetMaterial();
	return !Root || Root == UMaterial::GetDefaultMaterial(MD_Surface);
}

UMaterialInstanceDynamic* ASprawlSurfaceRepair::MakeRepairMaterial(
	const FLinearColor& Color, float Roughness)
{
	if (!OpaqueBase)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* Mat =
		UMaterialInstanceDynamic::Create(OpaqueBase, this);
	if (!Mat)
	{
		return nullptr;
	}
	// M_Simple_Opaque exposes both BaseColor and Color; set both so the tint
	// lands regardless of which one the master wired to its base-colour input.
	Mat->SetVectorParameterValue(TEXT("BaseColor"), Color);
	Mat->SetVectorParameterValue(TEXT("Color"), Color);
	Mat->SetScalarParameterValue(TEXT("Metallic"), 0.f);
	Mat->SetScalarParameterValue(TEXT("Roughness"), Roughness);
	RepairMaterials.Add(Mat);
	return Mat;
}

int32 ASprawlSurfaceRepair::RepairWorldSurfaces()
{
	UWorld* World = GetWorld();
	if (!World || !OpaqueBase)
	{
		return 0;
	}

	// Lake footprint (identical to the waterfront scenery layout) drives the
	// promenade-vs-asphalt role tint.
	const FSprawlWaterfrontSceneryLayout Layout =
		FSprawlWaterfrontSceneryLayout::Build();
	const FVector2D LakeHalf = Layout.WaterSize * 0.5f;

	int32 Repaired = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		// The waterfront actor owns the translucent water surface; never rebind
		// it to an opaque material here.
		if (!Actor || Actor == this || Actor->IsA<ASprawlWaterfrontScenery>())
		{
			continue;
		}

		TArray<UStaticMeshComponent*> Meshes;
		Actor->GetComponents(Meshes);
		for (UStaticMeshComponent* Mesh : Meshes)
		{
			if (!Mesh)
			{
				continue;
			}
			const bool bLarge = Mesh->Bounds.SphereRadius > LargeSurfaceRadius;
			const int32 SlotCount = Mesh->GetNumMaterials();
			for (int32 Slot = 0; Slot < SlotCount; ++Slot)
			{
				const UMaterialInterface* Current = Mesh->GetMaterial(Slot);
				// The checker is never intentional, so repair it anywhere. A
				// merely-missing material is only repaired on a large surface,
				// so authored props with an empty slot are left alone.
				const bool bBroken = IsCheckerMaterial(Current)
					|| (bLarge && IsMissingMaterial(Current));
				if (!bBroken)
				{
					continue;
				}

				const FVector Loc = Actor->GetActorLocation();
				// The four legacy world-edge planes are ~95 m cards floating
				// 10.5 m off the ground well outside the city. Any repaint
				// leaves them as a shelf slicing through the skyline ring, so
				// hide them outright: the ring ground, proxy skyline,
				// mountains, and distance fog close the horizon now.
				if (Mesh->Bounds.SphereRadius > 8000.f
					&& Loc.Size2D() > 8000.f)
				{
					Mesh->SetVisibility(false, true);
					++Repaired;
					UE_LOG(LogTemp, Display,
						TEXT("[SurfaceRepair] hid legacy edge card '%s' z=%.0f r=%.0f"),
						*Actor->GetName(), Loc.Z, Mesh->Bounds.SphereRadius);
					continue;
				}
				const bool bNearLake =
					FMath::Abs(Loc.X - Layout.WaterCenter.X)
						< LakeHalf.X + PromenadeMargin
					&& FMath::Abs(Loc.Y - Layout.WaterCenter.Y)
						< LakeHalf.Y + PromenadeMargin;
				// Prefer the Blender-baked world-projected sets; the flat
				// tinted MID remains the last-resort repair when the kit
				// import has not run on this clone.
				UMaterialInterface* Textured =
					bNearLake ? ConcreteMaterial.Get() : AsphaltMaterial.Get();
				if (Textured)
				{
					Mesh->SetMaterial(Slot, Textured);
					++Repaired;
					continue;
				}
				UMaterialInstanceDynamic* Fixed = MakeRepairMaterial(
					bNearLake ? PromenadeStone : StreetAsphalt,
					bNearLake ? 0.75f : 0.9f);
				if (Fixed)
				{
					Mesh->SetMaterial(Slot, Fixed);
					++Repaired;
				}
			}
		}
	}
	return Repaired;
}

void ASprawlSurfaceRepair::BeginPlay()
{
	Super::BeginPlay();
	const int32 Repaired = RepairWorldSurfaces();

	// Verify no checker survived: re-scan and count any large surface still on
	// the engine checker or default material after the repair pass.
	int32 Remaining = 0;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor || Actor == this || Actor->IsA<ASprawlWaterfrontScenery>())
			{
				continue;
			}
			TArray<UStaticMeshComponent*> Meshes;
			Actor->GetComponents(Meshes);
			for (const UStaticMeshComponent* Mesh : Meshes)
			{
				if (!Mesh || Mesh->Bounds.SphereRadius <= LargeSurfaceRadius)
				{
					continue;
				}
				for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
				{
					const UMaterialInterface* M = Mesh->GetMaterial(Slot);
					Remaining += (IsCheckerMaterial(M) || IsMissingMaterial(M)) ? 1 : 0;
				}
			}
		}
	}

	// Display verbosity: LogTemp/Log is filtered out of -game runs in this
	// project, so anything that must be verifiable headlessly uses Display.
	UE_LOG(LogTemp, Display,
		TEXT("[SurfaceRepair] rebound %d checker/default surface slot(s); remaining large checker/missing = %d"),
		Repaired, Remaining);
}

ASprawlSurfaceRepair* ASprawlSurfaceRepair::EnsureForWorld(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	TActorIterator<ASprawlSurfaceRepair> Existing(World);
	if (Existing)
	{
		return *Existing;
	}
	FActorSpawnParameters Params;
	Params.Name = TEXT("SprawlSurfaceRepair");
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<ASprawlSurfaceRepair>(
		FVector::ZeroVector, FRotator::ZeroRotator, Params);
}
