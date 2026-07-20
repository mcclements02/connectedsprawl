// The Connected Sprawl - Deterministic waterfront scenery layout tests.

#include "World/SprawlWaterfrontScenery.h"

#include "World/SprawlCityGridSubsystem.h"
#include "World/SprawlOceanSurface.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

using Grid = USprawlCityGridSubsystem;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlWaterfrontSceneryLayoutTest,
	"ConnectedSprawl.World.WaterfrontSceneryLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlWaterfrontSceneryLayoutTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FSprawlWaterfrontSceneryLayout Layout =
		FSprawlWaterfrontSceneryLayout::Build();
	TestTrue(TEXT("The waterfront layout is internally valid"), Layout.IsValid());

	const float LakeMinX = Grid::BlockCenter(4) - Grid::Step * 0.5f;
	const float LakeMaxX = Grid::BlockCenter(6) + Grid::Step * 0.5f;
	const float LakeMinY = Grid::BlockCenter(0) - Grid::Step * 0.5f;
	const float LakeMaxY = Grid::BlockCenter(1) + Grid::Step * 0.5f;
	TestTrue(TEXT("Water center X follows the live grid"), FMath::IsNearlyEqual(
		Layout.WaterCenter.X, (LakeMinX + LakeMaxX) * 0.5f));
	TestTrue(TEXT("Water center Y follows the live grid"), FMath::IsNearlyEqual(
		Layout.WaterCenter.Y, (LakeMinY + LakeMaxY) * 0.5f));
	TestTrue(TEXT("Water width covers all three lake blocks"), FMath::IsNearlyEqual(
		Layout.WaterSize.X, LakeMaxX - LakeMinX));
	TestTrue(TEXT("Water depth covers both lake blocks"), FMath::IsNearlyEqual(
		Layout.WaterSize.Y, LakeMaxY - LakeMinY));

	const FSprawlOceanWaveProfile Waves =
		FSprawlOceanWaveProfile::CoastalOcean();
	TestTrue(TEXT("The ocean wave profile is valid"), Waves.IsValid());
	TestTrue(TEXT("Ocean flow animates instead of remaining still"),
		Waves.FlowSpeed > 0.f);
	TestTrue(TEXT("Ocean displacement is visibly enabled"),
		Waves.WaveStrength >= 1.f);
	const FBoxSphereBounds ExampleBounds(
		FVector(100.f, -50.f, 0.f), FVector(500.f, 250.f, 1.f), 600.f);
	const FVector ExampleScale = FSprawlOceanSurface::CalculateScale(
		FVector2D(4000.f, 1000.f), ExampleBounds);
	TestTrue(TEXT("Authored water meshes fit the requested width"),
		FMath::IsNearlyEqual(ExampleScale.X, 4.f));
	TestTrue(TEXT("Authored water meshes fit the requested depth"),
		FMath::IsNearlyEqual(ExampleScale.Y, 2.f));
	const FVector ExampleCenter(800.f, 900.f, 20.f);
	const FVector ExampleLocation = FSprawlOceanSurface::CalculateLocation(
		ExampleCenter, ExampleBounds, ExampleScale);
	TestTrue(TEXT("Off-centre source meshes align to the lake center"),
		(ExampleLocation + ExampleBounds.Origin * ExampleScale)
			.Equals(ExampleCenter));

	TestEqual(TEXT("Two far banks close the open horizon"),
		Layout.ShoreTransforms.Num(), 2);
	TestEqual(TEXT("Four broad mountain ranges cover the south/east horizon"),
		Layout.MountainTransforms.Num(), 4);
	TestEqual(TEXT("The horizon has a separate foothill layer"),
		Layout.FoothillTransforms.Num(), 7);
	for (const FTransform& Transform : Layout.ShoreTransforms)
	{
		const FVector Location = Transform.GetLocation();
		TestFalse(TEXT("Far banks stay outside the playable city"),
			Grid::IsInsideCityBounds(Location.X, Location.Y));
	}
	for (const FTransform& Transform : Layout.MountainTransforms)
	{
		const FVector Location = Transform.GetLocation();
		TestFalse(TEXT("Mountains stay outside gameplay space"),
			Grid::IsInsideCityBounds(Location.X, Location.Y));
		TestTrue(TEXT("Mountain transforms have positive scale"),
			Transform.GetScale3D().GetMin() > 0.f);
	}
	return true;
}

#endif
