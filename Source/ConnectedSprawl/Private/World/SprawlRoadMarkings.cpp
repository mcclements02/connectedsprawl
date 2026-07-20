// The Connected Sprawl - Procedural road markings.

#include "World/SprawlRoadMarkings.h"
#include "World/SprawlCityGridSubsystem.h"
#include "World/SprawlRoadMarkingLayout.h"
#include "World/SprawlRoadPaintOcclusion.h"
#include "Components/BoxComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

using Grid = USprawlCityGridSubsystem;

namespace
{
constexpr float IntersectionHalf = Grid::RoadWidth * 0.5f;
constexpr float BoundaryThickness = 120.f;
constexpr float BoundaryHalfHeight = 700.f;

// Lane and bay striping.
constexpr float CenterlineWidth = 18.f;
constexpr float LaneDividerWidth = 14.f;
constexpr float EdgeLineWidth = 12.f;
/** Offset to the solid line separating the outer lane from the parking bay. */
constexpr float ParkingEdgeOffset =
	Grid::LanesPerDirection * Grid::LaneWidth;                          // 700
/** Length of one parallel-parking bay, and the tick that separates them. */
constexpr float ParkingBayLength = 700.f;
constexpr float ParkingTickWidth = 12.f;
}

ASprawlRoadMarkings::ASprawlRoadMarkings()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> PaintMat(
		TEXT("/Game/Materials/MI_RoadPaint.MI_RoadPaint"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> OpaquePaintMat(
		TEXT("/Game/CityArt/M_RoadWhite.M_RoadWhite"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FallbackMat(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	PaintMesh = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Paint"));
	PaintMesh->SetupAttachment(RootComponent);
	FSprawlRoadPaintOcclusion::Configure(PaintMesh);
	if (CubeMesh.Succeeded())
	{
		PaintMesh->SetStaticMesh(CubeMesh.Object);
	}
	if (UMaterialInterface* RoadPaint = FSprawlRoadPaintOcclusion::SelectMaterial(
		PaintMat.Get(), OpaquePaintMat.Get()))
	{
		PaintMesh->SetMaterial(0, RoadPaint);
	}
	else if (FallbackMat.Succeeded())
	{
		PaintMesh->SetMaterial(0, FallbackMat.Object);
	}

	const float WallCenter = Grid::CityBoundaryHalfExtent + BoundaryThickness;
	const float WallLength = Grid::CityBoundaryHalfExtent + BoundaryThickness * 2.f;
	NorthBoundary = CreateBoundary(TEXT("CityBoundaryNorth"),
		FVector(0.f, WallCenter, BoundaryHalfHeight),
		FVector(WallLength, BoundaryThickness, BoundaryHalfHeight));
	SouthBoundary = CreateBoundary(TEXT("CityBoundarySouth"),
		FVector(0.f, -WallCenter, BoundaryHalfHeight),
		FVector(WallLength, BoundaryThickness, BoundaryHalfHeight));
	EastBoundary = CreateBoundary(TEXT("CityBoundaryEast"),
		FVector(WallCenter, 0.f, BoundaryHalfHeight),
		FVector(BoundaryThickness, WallLength, BoundaryHalfHeight));
	WestBoundary = CreateBoundary(TEXT("CityBoundaryWest"),
		FVector(-WallCenter, 0.f, BoundaryHalfHeight),
		FVector(BoundaryThickness, WallLength, BoundaryHalfHeight));
}

UBoxComponent* ASprawlRoadMarkings::CreateBoundary(const TCHAR* Name,
	const FVector& Location, const FVector& HalfExtent)
{
	UBoxComponent* Boundary = CreateDefaultSubobject<UBoxComponent>(Name);
	Boundary->SetupAttachment(RootComponent);
	Boundary->SetRelativeLocation(Location);
	Boundary->SetBoxExtent(HalfExtent);
	Boundary->SetCollisionProfileName(TEXT("BlockAll"));
	Boundary->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Boundary->SetGenerateOverlapEvents(false);
	Boundary->SetHiddenInGame(true);
	Boundary->SetCanEverAffectNavigation(false);
	Boundary->ComponentTags.Add(TEXT("Sprawl.CityBoundary"));
	return Boundary;
}

void ASprawlRoadMarkings::BeginPlay()
{
	Super::BeginPlay();
	BuildMarkings();
}

void ASprawlRoadMarkings::AddStripe(const FVector& Center, float LengthX, float LengthY)
{
	const FSprawlRoadMarkingFit Fit = FSprawlRoadMarkingLayout::GetFit();
	const FVector Scale(LengthX / 100.f, LengthY / 100.f, Fit.MeshThickness);
	PaintMesh->AddInstance(FTransform(FRotator::ZeroRotator, Center, Scale), /*bWorldSpace=*/true);
}

void ASprawlRoadMarkings::BuildMarkings()
{
	if (!PaintMesh || !PaintMesh->GetStaticMesh())
	{
		return;
	}

	const FSprawlRoadMarkingFit Fit = FSprawlRoadMarkingLayout::GetFit();
	TArray<FSprawlRoadMarkingSegment> RoadIntervals;
	FSprawlRoadMarkingLayout::BuildRoadIntervals(RoadIntervals);

	// --- Lane striping down every road, both axes ---
	// Painted per road: a dashed centreline, a dashed divider between the two
	// travel lanes on each side, and a near-solid edge line separating the
	// outer lane from the parallel-parking bay.
	//
	// Lateral offsets are signed so the same loop serves both sides; the
	// stripe is laid across (Offset) and along (Along) the road's axis.
	struct FLateralStripe
	{
		float Offset;
		float Width;
		float Length;
		float Spacing;
	};
	TArray<FLateralStripe> LaneStripes;
	LaneStripes.Add({ 0.f, CenterlineWidth, DashLength, DashSpacing });
	// Dividers only sit between travel lanes, so a two-way street has none.
	for (int32 Boundary = 1; Boundary < Grid::LanesPerDirection; ++Boundary)
	{
		const float Offset = Boundary * Grid::LaneWidth;
		LaneStripes.Add({ -Offset, LaneDividerWidth, DashLength, DashSpacing });
		LaneStripes.Add({ +Offset, LaneDividerWidth, DashLength, DashSpacing });
	}
	// Near-continuous: dash length matches spacing so it reads as solid.
	LaneStripes.Add({ -ParkingEdgeOffset, EdgeLineWidth, DashSpacing, DashSpacing });
	LaneStripes.Add({ +ParkingEdgeOffset, EdgeLineWidth, DashSpacing, DashSpacing });

	for (int32 R = 0; R < Grid::NumRoads; ++R)
	{
		const float RoadCoord = Grid::RoadCenter(R);
		for (const FLateralStripe& Stripe : LaneStripes)
		{
			const float Lateral = RoadCoord + Stripe.Offset;
			TArray<float> DashCenters;
			FSprawlRoadMarkingLayout::AppendCenteredDashCenters(
				RoadIntervals, Stripe.Length, Stripe.Spacing, DashCenters);
			for (const float Along : DashCenters)
			{
				// Vertical road: stripe runs along Y at x = Lateral.
				if (!Grid::IsOverLake(Lateral, Along))
				{
					AddStripe(FVector(Lateral, Along, Fit.SurfaceZ),
						Stripe.Width, Stripe.Length);
				}
				// Horizontal road: stripe runs along X at y = Lateral.
				if (!Grid::IsOverLake(Along, Lateral))
				{
					AddStripe(FVector(Along, Lateral, Fit.SurfaceZ),
						Stripe.Length, Stripe.Width);
				}
			}
		}
	}

	// --- Parallel-parking bays outboard of the travel lanes ---
	// One tick between neighbouring bays, so drivers can see where a car is
	// meant to sit rather than guessing at the kerb.
	int32 ParkingBays = 0;
	for (int32 R = 0; R < Grid::NumRoads; ++R)
	{
		const float RoadCoord = Grid::RoadCenter(R);
		for (const float Side : { -1.f, 1.f })
		{
			const float BayCenter = RoadCoord + Side * Grid::ParkingOffset;
			TArray<float> BayCenters;
			FSprawlRoadMarkingLayout::AppendCenteredDashCenters(
				RoadIntervals, ParkingTickWidth, ParkingBayLength, BayCenters);
			for (const float Along : BayCenters)
			{
				if (!Grid::IsOverLake(BayCenter, Along))
				{
					AddStripe(FVector(BayCenter, Along, Fit.SurfaceZ),
						Grid::ParkingBayWidth, ParkingTickWidth);
					++ParkingBays;
				}
				if (!Grid::IsOverLake(Along, BayCenter))
				{
					AddStripe(FVector(Along, BayCenter, Fit.SurfaceZ),
						ParkingTickWidth, Grid::ParkingBayWidth);
					++ParkingBays;
				}
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
			// travel, repeated the full width of the carriageway.
			const float BandCenter = IntersectionHalf + 110.f;
			const float BandHalfWidth = IntersectionHalf - 80.f;
			for (float Across = -BandHalfWidth; Across <= BandHalfWidth; Across += 90.f)
			{
				// North + south approaches (vertical road, stripes along Y).
				AddStripe(FVector(C.X + Across, C.Y + BandCenter, Fit.SurfaceZ), 40.f, 150.f);
				AddStripe(FVector(C.X + Across, C.Y - BandCenter, Fit.SurfaceZ), 40.f, 150.f);
				// East + west approaches (horizontal road, stripes along X).
				AddStripe(FVector(C.X + BandCenter, C.Y + Across, Fit.SurfaceZ), 150.f, 40.f);
				AddStripe(FVector(C.X - BandCenter, C.Y + Across, Fit.SurfaceZ), 150.f, 40.f);
			}

			// Stop lines: across both inbound travel lanes, just before the
			// crosswalk. Right-hand traffic (see Grid::LaneCenter).
			const float StopDist = Grid::StopLineDistance;
			// Centre of the inbound carriageway, and how wide it is.
			const float ApproachCenter = ParkingEdgeOffset * 0.5f;
			const float ApproachWidth = ParkingEdgeOffset;
			// Northbound (lanes on -X side) stops south of the intersection.
			AddStripe(FVector(C.X - ApproachCenter, C.Y - StopDist, Fit.SurfaceZ),
				ApproachWidth, 30.f);
			// Southbound (lanes on +X side) stops north of the intersection.
			AddStripe(FVector(C.X + ApproachCenter, C.Y + StopDist, Fit.SurfaceZ),
				ApproachWidth, 30.f);
			// Eastbound (lanes on +Y side) stops west of the intersection.
			AddStripe(FVector(C.X - StopDist, C.Y + ApproachCenter, Fit.SurfaceZ),
				30.f, ApproachWidth);
			// Westbound (lanes on -Y side) stops east of the intersection.
			AddStripe(FVector(C.X + StopDist, C.Y - ApproachCenter, Fit.SurfaceZ),
				30.f, ApproachWidth);
		}
	}

	UE_LOG(LogTemp, Display,
		TEXT("[RoadMarkings] painted %d instances: %d intersections, %d parking bays, "
			"%d lanes/direction on a %.0fcm carriageway"),
		PaintMesh->GetInstanceCount(), Intersections, ParkingBays,
		Grid::LanesPerDirection, Grid::RoadWidth);
}
