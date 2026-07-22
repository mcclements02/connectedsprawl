// The Connected Sprawl - Swappable athletic-shoe catalog tests.

#include "Characters/SprawlAthleticShoeModule.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlAthleticShoeModuleTest,
	"ConnectedSprawl.Characters.AthleticShoeModule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlAthleticShoeModuleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const ESprawlAthleticShoePreset Presets[] = {
		ESprawlAthleticShoePreset::ZarriVelocity,
		ESprawlAthleticShoePreset::MetroRunner,
		ESprawlAthleticShoePreset::CourtHighTop,
		ESprawlAthleticShoePreset::NightSprint,
	};
	TestEqual(TEXT("The runtime catalog reports every preset"),
		USprawlAthleticShoeModule::GetPresetCount(),
		static_cast<int32>(UE_ARRAY_COUNT(Presets)));

	TSet<FName> PresetIds;
	FString Error;
	for (const ESprawlAthleticShoePreset Preset : Presets)
	{
		const FSprawlAthleticShoeDefinition Definition =
			USprawlAthleticShoeModule::DevelopPreset(Preset);
		TestTrue(*FString::Printf(TEXT("Preset %d is complete"),
			static_cast<int32>(Preset)),
			USprawlAthleticShoeModule::ValidatePreset(Definition, Error));
		PresetIds.Add(Definition.PresetId);
	}
	TestEqual(TEXT("Every athletic preset has a stable unique ID"),
		PresetIds.Num(), static_cast<int32>(UE_ARRAY_COUNT(Presets)));
	TestEqual(TEXT("Preset cycling wraps to Zarri's pair"),
		USprawlAthleticShoeModule::NextPreset(
			ESprawlAthleticShoePreset::NightSprint),
		ESprawlAthleticShoePreset::ZarriVelocity);
	TestFalse(TEXT("A swap safely rejects an unbound footwear module"),
		USprawlAthleticShoeModule::SwapToPreset(
			nullptr, ESprawlAthleticShoePreset::ZarriVelocity));
	return true;
}

#endif
