// The Connected Sprawl - Parking-deck layout and traffic-egress tests.

#include "World/SprawlParkingGarage.h"

#include "AI/SprawlTrafficRoute.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

using Grid = USprawlCityGridSubsystem;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlParkingGarageLayoutTest,
	"ConnectedSprawl.World.ParkingGarageLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlParkingGarageLayoutTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FSprawlParkingGarageLayout Layout =
		FSprawlParkingGarageLayout::Build();
	TestTrue(TEXT("The garage layout is internally valid"), Layout.IsValid());
	TestEqual(TEXT("The garage provides four parking levels"),
		Layout.DeckCount, FSprawlParkingGarageLayout::ExpectedDeckCount);
	TestEqual(TEXT("Every level has one deck slab"),
		Layout.Decks.Num(), Layout.DeckCount);
	TestEqual(TEXT("Adjacent levels have a visible ramp"),
		Layout.Ramps.Num(), Layout.DeckCount - 1);
	TestTrue(TEXT("Instancing supplies substantial parking detail"),
		Layout.Rails.Num() + Layout.Markings.Num()
			+ Layout.ParkedVehicles.Num() >= 50);
	TestEqual(TEXT("The deck has two independent covered exits"),
		Layout.Exits.Num(), 2);

	TSet<FName> ExitIds;
	for (const FSprawlParkingGarageExit& Exit : Layout.Exits)
	{
		const FString Label = Exit.Id.ToString();
		TestTrue(*FString::Printf(TEXT("%s exit is valid"), *Label),
			Exit.IsValid());
		TestFalse(*FString::Printf(TEXT("%s car is born off-road"), *Label),
			Grid::IsOnRoadSurface(
				Exit.SpawnTransform.GetLocation().X,
				Exit.SpawnTransform.GetLocation().Y));
		TestTrue(*FString::Printf(TEXT("%s finishes on a viable route"), *Label),
			FSprawlTrafficRoute::IsSpawnRouteViable(
				Exit.MergeTransform.GetLocation(), Exit.MergeHeading, Exit.RoadIndex));
		const float Lane = Grid::LaneCenter(Exit.RoadIndex, Exit.MergeHeading);
		const float LaneCoordinate = Grid::IsNorthSouth(Exit.MergeHeading)
			? Exit.MergeTransform.GetLocation().X
			: Exit.MergeTransform.GetLocation().Y;
		TestNearlyEqual(*FString::Printf(TEXT("%s merge lane is exact"), *Label),
			LaneCoordinate, Lane, 0.01f);
		TestFalse(*FString::Printf(TEXT("%s id is unique"), *Label),
			ExitIds.Contains(Exit.Id));
		ExitIds.Add(Exit.Id);
	}

	const FVector2D Center = FSprawlParkingGarageLayout::GarageCenter();
	TestFalse(TEXT("The garage replaces an ordinary dry block"),
		Grid::IsOverLake(Center.X, Center.Y));
	TestFalse(TEXT("The garage footprint never occupies a road"),
		Grid::IsOnRoadSurface(Center.X, Center.Y));
	return true;
}

#endif
