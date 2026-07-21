// The Connected Sprawl - Deterministic skyline layout tests.

#include "World/SprawlCitySkyline.h"

#include "World/SprawlCityGridSubsystem.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

using Grid = USprawlCityGridSubsystem;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlCitySkylineLayoutTest,
	"ConnectedSprawl.World.CitySkylineLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlCitySkylineLayoutTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FSprawlSkylineLayout Layout = FSprawlSkylineLayout::Build();
	TestTrue(TEXT("The skyline layout is internally valid"), Layout.IsValid());
	TestTrue(TEXT("The ring holds a substantial number of proxies"),
		Layout.BuildingTransforms.Num() >= 40);
	TestEqual(TEXT("Every building carries a roof cap"),
		Layout.RoofTransforms.Num(), Layout.BuildingTransforms.Num());

	const float Boundary = Grid::CityBoundaryHalfExtent;
	for (int32 i = 0; i < Layout.BuildingTransforms.Num(); ++i)
	{
		const FVector Loc = Layout.BuildingTransforms[i].GetLocation();
		TestFalse(TEXT("Proxies never intrude into the playable city"),
			Grid::IsInsideCityBounds(Loc.X, Loc.Y));

		// Band matches the layout tune: near row 2400-4400, far row 4200-7800.
		const float Height = Layout.BuildingTransforms[i].GetScale3D().Z * 100.f;
		TestTrue(TEXT("Proxy heights stay within the silhouette band"),
			Height >= 2300.f && Height <= 8200.f);

		// The bay must stay open: nothing on the south run across the lake's
		// outward arc, so the water still meets the mountain horizon.
		const bool bSouthBand = Loc.Y < -(Boundary + 900.f);
		TestFalse(TEXT("The lake's south face stays free of proxies"),
			bSouthBand && Loc.X > 1600.f && Loc.X < 10800.f);

		const FVector RoofLoc = Layout.RoofTransforms[i].GetLocation();
		TestTrue(TEXT("Roof caps sit atop their buildings"),
			RoofLoc.Z > Loc.Z);
	}
	return true;
}

#endif
