// The Connected Sprawl - Procedural road markings.

#include "World/SprawlRoadMarkings.h"
#include "World/SprawlCityGridSubsystem.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

using Grid = USprawlCityGridSubsystem;

namespace
{
// Markings sit just above the asphalt (road surface tops out at z = 0).
constexpr float PaintZ = 1.5f;
constexpr float PaintThickness = 0.03f; // cube scale -> 3cm
constexpr float IntersectionHalf = Grid::RoadWidth * 0.5f; // 300
}

ASprawlRoadMarkings::ASprawlRoadMarkings()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> PaintMat(
		TEXT("/Game/Materials/MI_RoadPaint.MI_RoadPaint"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FallbackMat(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	PaintMesh = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Paint"));
	PaintMesh->SetupAttachment(RootComponent);
	PaintMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PaintMesh->SetCastShadow(false);
	if (CubeMesh.Succeeded())
	{
		PaintMesh->SetStaticMesh(CubeMesh.Object);
	}
	if (PaintMat.Get())
	{
		PaintMesh->SetMaterial(0, PaintMat.Get());
	}
	else if (FallbackMat.Succeeded())
	{
		PaintMesh->SetMaterial(0, FallbackMat.Object);
	}
}

void ASprawlRoadMarkings::BeginPlay()
{
	Super::BeginPlay();
	BuildMarkings();
}

void ASprawlRoadMarkings::AddStripe(const FVector& Center, float LengthX, float LengthY)
{
	const FVector Scale(LengthX / 100.f, LengthY / 100.f, PaintThickness);
	PaintMesh->AddInstance(FTransform(FRotator::ZeroRotator, Center, Scale), /*bWorldSpace=*/true);
}

void ASprawlRoadMarkings::BuildMarkings()
{
	if (!PaintMesh || !PaintMesh->GetStaticMesh())
	{
		return;
	}

	const float GridLo = Grid::RoadCenter(0) - Grid::Step * 0.5f;
	const float GridHi = Grid::RoadCenter(Grid::NumRoads - 1) + Grid::Step * 0.5f;

	// True when a centerline point is inside any intersection box.
	auto InAnyIntersection = [](float Along) -> bool
	{
		for (int32 R = 0; R < Grid::NumRoads; ++R)
		{
			if (FMath::Abs(Along - Grid::RoadCenter(R)) < IntersectionHalf + 120.f)
			{
				return true;
			}
		}
		return false;
	};

	// --- Dashed centerlines down every road, both axes ---
	for (int32 R = 0; R < Grid::NumRoads; ++R)
	{
		const float RoadCoord = Grid::RoadCenter(R);
		for (float Along = GridLo; Along < GridHi; Along += DashSpacing)
		{
			if (InAnyIntersection(Along))
			{
				continue;
			}
			// Vertical road: dash runs along Y at x = RoadCoord.
			if (!Grid::IsOverLake(RoadCoord, Along))
			{
				AddStripe(FVector(RoadCoord, Along, PaintZ), 18.f, DashLength);
			}
			// Horizontal road: dash runs along X at y = RoadCoord.
			if (!Grid::IsOverLake(Along, RoadCoord))
			{
				AddStripe(FVector(Along, RoadCoord, PaintZ), DashLength, 18.f);
			}
		}
	}

	// --- Crosswalks + stop lines at every dry intersection ---
	int32 Intersections = 0;
	for (int32 Ix = 0; Ix < Grid::NumRoads; ++Ix)
	{
		for (int32 Iy = 0; Iy < Grid::NumRoads; ++Iy)
		{
			const FVector2D C = Grid::IntersectionCenter(Ix, Iy);
			if (Grid::IsOverLake(C.X, C.Y, 100.f))
			{
				continue;
			}
			++Intersections;

			// Zebra bands across each approach: stripes run parallel to
			// travel, repeated across the road width.
			const float BandCenter = IntersectionHalf + 110.f; // 410 from center
			for (float Across = -240.f; Across <= 240.f; Across += 80.f)
			{
				// North + south approaches (vertical road, stripes along Y).
				AddStripe(FVector(C.X + Across, C.Y + BandCenter, PaintZ), 35.f, 130.f);
				AddStripe(FVector(C.X + Across, C.Y - BandCenter, PaintZ), 35.f, 130.f);
				// East + west approaches (horizontal road, stripes along X).
				AddStripe(FVector(C.X + BandCenter, C.Y + Across, PaintZ), 130.f, 35.f);
				AddStripe(FVector(C.X - BandCenter, C.Y + Across, PaintZ), 130.f, 35.f);
			}

			// Stop lines: across the inbound lane only, just before the
			// crosswalk. Right-hand traffic (see Grid::LaneCenter).
			const float StopDist = BandCenter + 130.f; // 540 from center
			// Northbound (lane on -X side) stops south of the intersection.
			AddStripe(FVector(C.X - Grid::LaneOffset, C.Y - StopDist, PaintZ), 270.f, 30.f);
			// Southbound (lane on +X side) stops north of the intersection.
			AddStripe(FVector(C.X + Grid::LaneOffset, C.Y + StopDist, PaintZ), 270.f, 30.f);
			// Eastbound (lane on +Y side) stops west of the intersection.
			AddStripe(FVector(C.X - StopDist, C.Y + Grid::LaneOffset, PaintZ), 30.f, 270.f);
			// Westbound (lane on -Y side) stops east of the intersection.
			AddStripe(FVector(C.X + StopDist, C.Y - Grid::LaneOffset, PaintZ), 30.f, 270.f);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[RoadMarkings] painted %d instances across %d intersections"),
		PaintMesh->GetInstanceCount(), Intersections);
}
