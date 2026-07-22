// The Connected Sprawl - enterable interior layout and portal tests.

#include "World/SprawlEnterableInteriors.h"

#include "Misc/AutomationTest.h"
#include "World/SprawlCityGridSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

using Grid = USprawlCityGridSubsystem;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlEnterableInteriorsLayoutTest,
	"ConnectedSprawl.World.EnterableInteriorsLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlEnterableInteriorsLayoutTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FSprawlEnterableInteriorsLayout Layout =
		FSprawlEnterableInteriorsLayout::Build();
	TestTrue(TEXT("The complete interior layout is valid"), Layout.IsValid());
	TestEqual(TEXT("Store, office, and condo are all present"),
		Layout.Buildings.Num(),
		FSprawlEnterableInteriorsLayout::ExpectedBuildingCount);

	TSet<FName> Ids;
	TSet<ESprawlBuildingKind> Kinds;
	for (const FSprawlEnterableBuildingSpec& Building : Layout.Buildings)
	{
		const FString Label = Building.DisplayName.ToString();
		TestTrue(*FString::Printf(TEXT("%s has a valid authored contract"), *Label),
			Building.IsValid());
		TestFalse(*FString::Printf(TEXT("%s id is unique"), *Label),
			Ids.Contains(Building.Id));
		TestFalse(*FString::Printf(TEXT("%s type is unique"), *Label),
			Kinds.Contains(Building.Kind));
		Ids.Add(Building.Id);
		Kinds.Add(Building.Kind);

		TestTrue(*FString::Printf(TEXT("%s doorway is inside the city"), *Label),
			Grid::IsInsideCityBounds(
				Building.ExteriorReturnLocation.X,
				Building.ExteriorReturnLocation.Y));
		TestFalse(*FString::Printf(TEXT("%s doorway stays off asphalt"), *Label),
			Grid::IsOnRoadSurface(
				Building.ExteriorReturnLocation.X,
				Building.ExteriorReturnLocation.Y));
		TestFalse(*FString::Printf(TEXT("%s doorway stays off water"), *Label),
			Grid::IsOverLake(
				Building.ExteriorReturnLocation.X,
				Building.ExteriorReturnLocation.Y, 50.f));
		TestTrue(*FString::Printf(TEXT("%s has recognizable furnishing"), *Label),
			Building.Furniture.Num() >= 5);

		FSprawlInteriorTransition Entry;
		TestTrue(*FString::Printf(TEXT("%s exterior enters"), *Label),
			Layout.FindTransition(Building.ExteriorReturnLocation, Entry));
		TestTrue(*FString::Printf(TEXT("%s entry transition is entering"), *Label),
			Entry.bEntering);
		TestEqual(*FString::Printf(TEXT("%s entry keeps its id"), *Label),
			Entry.BuildingId, Building.Id);
		TestTrue(*FString::Printf(TEXT("%s entry lands inside"), *Label),
			Building.GetInteriorBounds().IsInsideOrOn(Entry.Target.GetLocation()));

		FSprawlInteriorTransition Exit;
		TestTrue(*FString::Printf(TEXT("%s interior exit works"), *Label),
			Layout.FindTransition(Building.InteriorExitLocation, Exit));
		TestFalse(*FString::Printf(TEXT("%s exit transition is leaving"), *Label),
			Exit.bEntering);
		TestTrue(*FString::Printf(TEXT("%s exits to its sidewalk door"), *Label),
			Exit.Target.GetLocation().Equals(
				Building.ExteriorReturnLocation, 0.01f));
		TestTrue(*FString::Printf(TEXT("%s maps back to its exterior"), *Label),
			Layout.ResolveMapLocation(Building.InteriorEntryLocation).Equals(
				Building.ExteriorReturnLocation, 0.01f));
	}

	FSprawlInteriorTransition FarAway;
	TestFalse(TEXT("Ordinary city space does not trigger a portal"),
		Layout.FindTransition(FVector(0.f, 0.f, 120.f), FarAway));
	return true;
}

#endif
