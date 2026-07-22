// The Connected Sprawl - Complete, varied wardrobe policy tests.

#include "Characters/SprawlWardrobeModule.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlWardrobeModuleTest,
	"ConnectedSprawl.Characters.WardrobeModule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlWardrobeModuleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const ESprawlWardrobeStyle Styles[] = {
		ESprawlWardrobeStyle::Streetwear,
		ESprawlWardrobeStyle::SmartCasual,
		ESprawlWardrobeStyle::Corporate,
		ESprawlWardrobeStyle::Workwear,
		ESprawlWardrobeStyle::Athletic,
	};

	TSet<uint8> Footwear;
	int32 HatCount = 0;
	int32 NoHatCount = 0;
	int32 JacketCount = 0;
	int32 NoJacketCount = 0;
	FString Error;
	for (const ESprawlWardrobeStyle Style : Styles)
	{
		for (int32 Seed = 0; Seed < 64; ++Seed)
		{
			const FSprawlWardrobeOutfit First =
				USprawlWardrobeModule::DevelopOutfit(Style, Seed);
			const FSprawlWardrobeOutfit Repeat =
				USprawlWardrobeModule::DevelopOutfit(Style, Seed);
			TestTrue(*FString::Printf(
				TEXT("Style %d seed %d is deterministic"),
				static_cast<int32>(Style), Seed), First == Repeat);
			TestTrue(*FString::Printf(
				TEXT("Style %d seed %d is a complete valid outfit"),
				static_cast<int32>(Style), Seed),
				USprawlWardrobeModule::ValidateOutfit(First, Error));
			TestFalse(TEXT("A generated outfit always has an ID"),
				First.OutfitId.IsNone());
			Footwear.Add(static_cast<uint8>(First.Footwear));
			First.Headwear == ESprawlWardrobeHeadwear::None
				? ++NoHatCount : ++HatCount;
			First.Outerwear == ESprawlWardrobeOuterwear::None
				? ++NoJacketCount : ++JacketCount;
		}
	}

	TestTrue(TEXT("The city wardrobe includes several shoe types"),
		Footwear.Num() >= 5);
	TestTrue(TEXT("Some residents receive hats"), HatCount > 0);
	TestTrue(TEXT("Some residents keep their hair uncovered"), NoHatCount > 0);
	TestTrue(TEXT("Some residents receive jackets"), JacketCount > 0);
	TestTrue(TEXT("Some residents use a lighter outfit"), NoJacketCount > 0);

	const FSprawlWardrobeOutfit Zarri =
		USprawlWardrobeModule::CreateZarriOutfit();
	TestEqual(TEXT("Zarri wears founder bomber styling"),
		Zarri.Outerwear, ESprawlWardrobeOuterwear::BomberJacket);
	TestEqual(TEXT("Zarri's approved hair remains visible"),
		Zarri.Headwear, ESprawlWardrobeHeadwear::None);
	TestEqual(TEXT("Zarri has athletic trainers"),
		Zarri.Footwear, ESprawlWardrobeFootwear::AthleticTrainers);
	TestEqual(TEXT("Zarri has socks"),
		Zarri.Socks, ESprawlWardrobeSocks::Crew);
	TestTrue(TEXT("Wardrobe descriptions call out footwear"),
		USprawlWardrobeModule::DescribeOutfit(Zarri).Contains(TEXT("athletic trainers")));
	TestTrue(TEXT("Wardrobe descriptions call out socks"),
		USprawlWardrobeModule::DescribeOutfit(Zarri).Contains(TEXT("socks")));

	const FSprawlWardrobeOutfit Nanobanana =
		USprawlWardrobeModule::CreateNanobananaOutfit();
	TestEqual(TEXT("Nanobanana wears tech bomber outerwear"),
		Nanobanana.Outerwear, ESprawlWardrobeOuterwear::BomberJacket);
	TestEqual(TEXT("Nanobanana wears oversized hoodie top"),
		Nanobanana.Top, ESprawlWardrobeTop::Hoodie);
	TestEqual(TEXT("Nanobanana wears cargo joggers bottom"),
		Nanobanana.Bottom, ESprawlWardrobeBottom::Cargo);
	TestTrue(TEXT("Nanobanana outfit validates cleanly"),
		USprawlWardrobeModule::ValidateOutfit(Nanobanana, Error));

	FSprawlWardrobeOutfit Invalid = Zarri;
	Invalid.OutfitId = NAME_None;
	TestFalse(TEXT("An outfit without an ID is rejected"),
		USprawlWardrobeModule::ValidateOutfit(Invalid, Error));
	return true;
}

#endif
