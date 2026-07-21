// The Connected Sprawl - Deterministic city-gate layout tests.

#include "World/SprawlCityGates.h"

#include "World/SprawlCityGridSubsystem.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

using Grid = USprawlCityGridSubsystem;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlCityGateLayoutTest,
	"ConnectedSprawl.World.CityGateLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlCityGateLayoutTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FSprawlCityGateLayout Layout = FSprawlCityGateLayout::Build();
	TestTrue(TEXT("The gate layout is internally valid"), Layout.IsValid());

	// 6 north + 3 south (bay skipped) + 6 west + 4 east (lake face skipped).
	TestEqual(TEXT("Every dry street mouth carries exactly one gate"),
		Layout.GateCount, 19);
	TestEqual(TEXT("Each gate is two pillars, a beam, and two stubs"),
		Layout.Pieces.Num(), Layout.GateCount * 5);

	int32 BeamsAboveTraffic = 0;
	for (const FTransform& Piece : Layout.Pieces)
	{
		const FVector Loc = Piece.GetLocation();
		const FVector Size = Piece.GetScale3D() * 100.f;
		// No gate piece may stand in the bay's open-water arc.
		const bool bSouthBand = Loc.Y < -(Grid::CityBoundaryHalfExtent - 300.f);
		TestFalse(TEXT("No gate stands in the bay"),
			bSouthBand && Loc.X > 1000.f && Loc.X < 11500.f);
		// Beams clear a tall vehicle comfortably.
		if (Size.Z <= 200.f && Loc.Z > 400.f)
		{
			TestTrue(TEXT("Beams sit well above the carriageway"),
				Loc.Z - Size.Z * 0.5f > 500.f);
			++BeamsAboveTraffic;
		}
	}
	TestEqual(TEXT("Every gate carries one beam"),
		BeamsAboveTraffic, Layout.GateCount);
	return true;
}

#endif
