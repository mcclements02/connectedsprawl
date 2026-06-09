// The Connected Sprawl - Instanced ground cover.

#include "World/SprawlGroundCover.h"
#include "World/SprawlCityGridSubsystem.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

using Grid = USprawlCityGridSubsystem;

ASprawlGroundCover::ASprawlGroundCover()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(
		TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GrassMat(
		TEXT("/Game/CityArt/MI_Leaves.MI_Leaves"));

	GrassMesh = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Grass"));
	GrassMesh->SetupAttachment(RootComponent);
	GrassMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GrassMesh->SetCastShadow(false); // thousands of blades; shadows would be pure cost
	if (ConeMesh.Succeeded())
	{
		GrassMesh->SetStaticMesh(ConeMesh.Object);
	}
	if (GrassMat.Succeeded())
	{
		GrassMesh->SetMaterial(0, GrassMat.Object);
	}

	// A few flower meshes ship with the sample scene; use whichever exist.
	const TCHAR* FlowerPaths[] = {
		TEXT("/Game/Geometry/SM_Flower_0.SM_Flower_0"),
		TEXT("/Game/Geometry/SM_Flower_1.SM_Flower_1"),
		TEXT("/Game/Geometry/SM_Flower_2.SM_Flower_2"),
	};
	for (int32 i = 0; i < 3; ++i)
	{
		ConstructorHelpers::FObjectFinder<UStaticMesh> Flower(FlowerPaths[i]);
		if (!Flower.Succeeded())
		{
			continue;
		}
		const FString Name = FString::Printf(TEXT("Flowers%d"), i);
		UHierarchicalInstancedStaticMeshComponent* Comp =
			CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(*Name);
		Comp->SetupAttachment(RootComponent);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCastShadow(false);
		Comp->SetStaticMesh(Flower.Object);
		FlowerMeshes.Add(Comp);
	}
}

void ASprawlGroundCover::BeginPlay()
{
	Super::BeginPlay();
	PlantParks();
}

void ASprawlGroundCover::PlantParks()
{
	if (!GrassMesh || !GrassMesh->GetStaticMesh())
	{
		return;
	}

	GrassMesh->SetCullDistances(static_cast<int32>(CullDistance * 0.8f),
	                            static_cast<int32>(CullDistance));

	// Deterministic scatter so the meadow looks the same every session.
	FRandomStream Rand(20260609);

	const float SurfaceTop = 14.f; // top of the raised block slab (build_city.py)
	const float Reach = Grid::BlockSize * 0.5f - 200.f;

	for (const FIntPoint& Park : ParkBlocks)
	{
		const float Bx = Grid::BlockCenter(Park.X);
		const float By = Grid::BlockCenter(Park.Y);

		for (int32 i = 0; i < GrassPerPark; ++i)
		{
			const FVector Pos(
				Bx + Rand.FRandRange(-Reach, Reach),
				By + Rand.FRandRange(-Reach, Reach),
				SurfaceTop);
			const FRotator Rot(
				Rand.FRandRange(-7.f, 7.f),
				Rand.FRandRange(0.f, 360.f),
				Rand.FRandRange(-7.f, 7.f));
			// Engine cone is 100x100; squash into a thin 25-55cm blade tuft.
			const FVector Scale(
				Rand.FRandRange(0.05f, 0.11f),
				Rand.FRandRange(0.05f, 0.11f),
				Rand.FRandRange(0.25f, 0.55f));
			GrassMesh->AddInstance(FTransform(Rot, Pos, Scale), /*bWorldSpace=*/true);
		}

		for (int32 i = 0; i < FlowersPerPark && FlowerMeshes.Num() > 0; ++i)
		{
			UHierarchicalInstancedStaticMeshComponent* Comp =
				FlowerMeshes[Rand.RandRange(0, FlowerMeshes.Num() - 1)];
			if (!Comp || !Comp->GetStaticMesh())
			{
				continue;
			}
			const FVector Pos(
				Bx + Rand.FRandRange(-Reach, Reach),
				By + Rand.FRandRange(-Reach, Reach),
				SurfaceTop);
			const FRotator Rot(0.f, Rand.FRandRange(0.f, 360.f), 0.f);
			const float S = Rand.FRandRange(0.8f, 1.3f);
			Comp->AddInstance(FTransform(Rot, Pos, FVector(S)), /*bWorldSpace=*/true);
		}
	}

	for (UHierarchicalInstancedStaticMeshComponent* Comp : FlowerMeshes)
	{
		if (Comp)
		{
			Comp->SetCullDistances(static_cast<int32>(CullDistance * 0.8f),
			                       static_cast<int32>(CullDistance));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[GroundCover] planted %d parks with %d grass instances each"),
		ParkBlocks.Num(), GrassPerPark);
}
