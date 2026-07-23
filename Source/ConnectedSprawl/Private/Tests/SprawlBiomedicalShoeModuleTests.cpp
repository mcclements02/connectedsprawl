// The Connected Sprawl - Cordero Biomedical shoe profile tests.

#include "Characters/SprawlBiomedicalShoeModule.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlBiomedicalShoeModuleTest,
	"ConnectedSprawl.Characters.BiomedicalShoeModule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlBiomedicalShoeModuleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FSprawlBiomedicalShoeDefinition Definition =
		USprawlBiomedicalShoeModule::DevelopBioCircuitHighTop();
	FString Error;
	TestTrue(TEXT("The authored Bio-Circuit profile validates"),
		USprawlBiomedicalShoeModule::ValidateDefinition(Definition, Error));
	TestEqual(TEXT("The profile keeps its stable catalog ID"),
		Definition.PresetId, FName(TEXT("CorderoBioCircuitHighTop")));
	TestEqual(TEXT("The runtime fallback is an animated fitted high-top"),
		Definition.Footwear, ESprawlWardrobeFootwear::HighTopSneakers);
	TestTrue(TEXT("The authored mesh resolves under the project content root"),
		Definition.PresentationMesh.ToString().StartsWith(TEXT("/Game/")));
	TestTrue(TEXT("The source scene remains reproducible"),
		Definition.BlenderSource.EndsWith(TEXT(".blend")));
	TestTrue(TEXT("The signature circuit window is retained"),
		Definition.bHasCircuitWindow);
	TestTrue(TEXT("The signature heel air unit is retained"),
		Definition.bHasHeelAirUnit);
	TestTrue(TEXT("The description reports the defining construction"),
		USprawlBiomedicalShoeModule::DescribeDefinition(Definition).Contains(
			TEXT("circuit window")));
	TestFalse(TEXT("A swap safely rejects an unbound footwear module"),
		USprawlBiomedicalShoeModule::SwapToBioCircuitHighTop(nullptr));
	return true;
}

#endif
