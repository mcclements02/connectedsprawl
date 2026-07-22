// The Connected Sprawl - Real clothing physics module tests.

#include "Characters/SprawlClothPhysicsModule.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlClothPhysicsModuleTest,
	"ConnectedSprawl.Characters.ClothPhysicsModule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlClothPhysicsModuleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FString Error;

	// 1. Validate Cotton Fleece default profile
	const FSprawlClothPhysicsProfile FleeceProfile =
		USprawlClothPhysicsModule::DevelopPhysicsProfile(ESprawlClothFabricType::CottonFleece);
	TestTrue(TEXT("Cotton Fleece profile validates cleanly"),
		USprawlClothPhysicsModule::ValidatePhysicsProfile(FleeceProfile, Error));
	TestEqual(TEXT("Cotton Fleece has 420 gsm density"), FleeceProfile.MassDensityGsm, 420.f);
	TestEqual(TEXT("Cotton Fleece maps to COTTON_FLEECE Blender preset"), FleeceProfile.BlenderPresetName, TEXT("COTTON_FLEECE"));

	// 2. Validate Tech Nylon profile
	const FSprawlClothPhysicsProfile NylonProfile =
		USprawlClothPhysicsModule::DevelopPhysicsProfile(ESprawlClothFabricType::TechNylon);
	TestTrue(TEXT("Tech Nylon profile validates cleanly"),
		USprawlClothPhysicsModule::ValidatePhysicsProfile(NylonProfile, Error));
	TestEqual(TEXT("Tech Nylon has high wind response scale"), NylonProfile.WindResponseScale, 2.10f);

	// 3. Test invalid profile parameters reject validation
	FSprawlClothPhysicsProfile InvalidProfile = FleeceProfile;
	InvalidProfile.MassDensityGsm = 1500.f; // Exceeds 1200.0 limit
	TestFalse(TEXT("Mass density out of bounds is rejected"),
		USprawlClothPhysicsModule::ValidatePhysicsProfile(InvalidProfile, Error));

	InvalidProfile = FleeceProfile;
	InvalidProfile.StretchingStiffness = 1.5f; // Exceeds 1.0 limit
	TestFalse(TEXT("Stiffness > 1.0 is rejected"),
		USprawlClothPhysicsModule::ValidatePhysicsProfile(InvalidProfile, Error));

	// 4. Test profile description formatting
	const FString Description = USprawlClothPhysicsModule::DescribePhysicsProfile(FleeceProfile);
	TestTrue(TEXT("Description contains density"), Description.Contains(TEXT("420gsm")));
	TestTrue(TEXT("Description contains Blender preset name"), Description.Contains(TEXT("COTTON_FLEECE")));

	// 5. Test fabric resolvers
	TestEqual(TEXT("Hoodie top resolves to CottonFleece"),
		USprawlClothPhysicsModule::ResolveTopFabric(ESprawlWardrobeTop::Hoodie),
		ESprawlClothFabricType::CottonFleece);

	TestEqual(TEXT("Jeans bottom resolves to Denim"),
		USprawlClothPhysicsModule::ResolveBottomFabric(ESprawlWardrobeBottom::Jeans),
		ESprawlClothFabricType::Denim);

	TestEqual(TEXT("Bomber jacket outerwear resolves to TechNylon"),
		USprawlClothPhysicsModule::ResolveOuterwearFabric(ESprawlWardrobeOuterwear::BomberJacket),
		ESprawlClothFabricType::TechNylon);

	return true;
}

#endif
