// The Connected Sprawl - Panel Cloth runtime bridge tests.

#include "Characters/SprawlPanelClothModule.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlPanelClothModuleTest,
	"ConnectedSprawl.Characters.PanelClothModule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlPanelClothModuleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	USprawlPanelClothModule* Module = NewObject<USprawlPanelClothModule>();
	TestNotNull(TEXT("Panel Cloth runtime module constructs"), Module);
	if (!Module)
	{
		return false;
	}

	TestEqual(TEXT("Runtime loads the graph-free baked hoodie path"),
		Module->GetConfiguredAssetPath().ToString(),
		FString(TEXT(
			"/Game/Import/Characters/Streetwear/PanelCloth/CA_Zarri_Hoodie_Runtime.CA_Zarri_Hoodie_Runtime")));
	TestFalse(TEXT("A fresh module has no active solver component"),
		Module->IsPanelClothActive());
	TestFalse(TEXT("Invalid visual inputs preserve the rigid fallback"),
		Module->ApplyToMetaHuman(nullptr, nullptr));
	return true;
}

#endif
