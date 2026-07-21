// The Connected Sprawl - Deterministic kerb/corner placement tests.

#include "World/SprawlStreetDressing.h"

#include "World/SprawlCityGridSubsystem.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

using Grid = USprawlCityGridSubsystem;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlKerbPlacementTest,
	"ConnectedSprawl.World.KerbPlacement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlKerbPlacementTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const float RoadX = Grid::RoadCenter(2);
	const float RoadY = Grid::RoadCenter(3);

	// A point drifted into the carriageway is detected and pushed to the
	// sidewalk band on its own side of its nearest road.
	const FVector2D InRoad(RoadX + 180.f, RoadY + Grid::BlockSize * 0.5f);
	TestTrue(TEXT("A point near a road centreline is in the carriageway"),
		FSprawlKerbPlacement::IsInCarriageway(InRoad));
	const FVector2D Kerb = FSprawlKerbPlacement::KerbPointFor(
		InRoad, FSprawlKerbPlacement::SignKerbOffset);
	TestFalse(TEXT("The kerb point leaves the carriageway"),
		FSprawlKerbPlacement::IsInCarriageway(Kerb));
	TestTrue(TEXT("The kerb point keeps the actor's side of the road"),
		Kerb.X > RoadX);
	TestTrue(TEXT("The kerb offset is exact"), FMath::IsNearlyEqual(
		Kerb.X, RoadX + FSprawlKerbPlacement::SignKerbOffset));

	// A stranded prop near a junction resolves to its own quadrant corner.
	const FVector2D NearJunction(RoadX - 300.f, RoadY + 260.f);
	const FVector2D Corner = FSprawlKerbPlacement::JunctionCornerFor(
		NearJunction, FSprawlKerbPlacement::PropCornerOffset);
	TestTrue(TEXT("The corner sits on the actor's quadrant"),
		Corner.X < RoadX && Corner.Y > RoadY);
	TestFalse(TEXT("The corner is clear of the carriageway"),
		FSprawlKerbPlacement::IsInCarriageway(Corner));

	// Block-centre detection: stranded on an ordinary block, legal in a park.
	const FVector2D Stranded(Grid::BlockCenter(3), Grid::BlockCenter(3));
	TestTrue(TEXT("An ordinary block centre counts as stranded"),
		FSprawlKerbPlacement::IsAtStrandedBlockCentre(Stranded));
	const FVector2D ParkCentre(Grid::BlockCenter(2), Grid::BlockCenter(2));
	TestFalse(TEXT("A park block centre is legitimate dressing ground"),
		FSprawlKerbPlacement::IsAtStrandedBlockCentre(ParkCentre));

	// Signal poles land on the sidewalk corner, outside the carriageway.
	const FVector2D Signal = FSprawlKerbPlacement::JunctionCornerFor(
		FVector2D(RoadX + 40.f, RoadY - 60.f),
		FSprawlKerbPlacement::SignalCornerOffset);
	TestFalse(TEXT("Signal corners stand clear of the carriageway"),
		FSprawlKerbPlacement::IsInCarriageway(Signal));
	return true;
}

#endif
