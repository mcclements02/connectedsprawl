// The Connected Sprawl - Driver visibility policy tests.

#include "Vehicles/SprawlDriverVisibility.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlDriverVisibilityTest,
	"ConnectedSprawl.Vehicles.DriverVisibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlDriverVisibilityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FSprawlDriverVisibilityDecision Empty =
		FSprawlDriverVisibility::Resolve(false);
	TestFalse(TEXT("An empty car has no logical occupant"),
		Empty.bKeepLogicalOccupancy);
	TestFalse(TEXT("An empty car renders no mounted driver"),
		Empty.bRenderMountedMesh);

	const FSprawlDriverVisibilityDecision Seated =
		FSprawlDriverVisibility::Resolve(true);
	TestTrue(TEXT("A seated driver remains a logical occupant"),
		Seated.bKeepLogicalOccupancy);
	TestFalse(TEXT("A seated driver is hidden inside the car"),
		Seated.bRenderMountedMesh);
	TestFalse(TEXT("A hidden seated driver loads no avatar assets"),
		Seated.bLoadAvatarAssets);
	TestFalse(TEXT("A hidden seated driver does not tick a pose"),
		Seated.bTickPose);
	return true;
}

#endif
