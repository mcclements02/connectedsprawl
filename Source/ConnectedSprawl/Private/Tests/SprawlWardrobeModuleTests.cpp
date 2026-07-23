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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlJacketProfileTest,
	"ConnectedSprawl.Characters.JacketProfile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlJacketProfileTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const ESprawlWardrobeOuterwear Jackets[] = {
		ESprawlWardrobeOuterwear::UtilityJacket,
		ESprawlWardrobeOuterwear::BomberJacket,
		ESprawlWardrobeOuterwear::Blazer,
		ESprawlWardrobeOuterwear::TrackJacket,
	};
	FString Error;
	// Every jacket must stay human-scale across the full range of bodies the
	// crowd generator produces, not just the average one.
	for (const ESprawlWardrobeOuterwear Jacket : Jackets)
	{
		for (const float Shoulder : {13.f, 18.f, 26.f})
		{
			for (const float Torso : {38.f, 50.f, 68.f})
			{
				const FSprawlJacketProfile Profile =
					USprawlWardrobeModule::DevelopJacketProfile(
						Jacket, Shoulder, Torso);
				TestTrue(*FString::Printf(
					TEXT("Jacket %d fits a %.0f/%.0f cm body"),
					static_cast<int32>(Jacket), Shoulder, Torso),
					USprawlWardrobeModule::ValidateJacketProfile(Profile, Error));
				// A shell cut to the joint span vanishes inside the torso, so
				// every jacket must flare past the deltoids it covers.
				TestTrue(TEXT("A jacket always clears the shoulders it covers"),
					Profile.ShoulderHalfWidthCm > Shoulder * 1.3f);
				TestTrue(TEXT("A jacket chest clears the ribcage"),
					Profile.ChestHalfWidthCm > Shoulder * 1.1f);
				TestTrue(TEXT("Sleeves clear the arm inside them"),
					Profile.SleeveUpperRadiusCm > Shoulder * 0.4f);
			}
		}
	}

	const FSprawlJacketProfile Bomber =
		USprawlWardrobeModule::DevelopJacketProfile(
			ESprawlWardrobeOuterwear::BomberJacket, 18.f, 50.f);
	const FSprawlJacketProfile Blazer =
		USprawlWardrobeModule::DevelopJacketProfile(
			ESprawlWardrobeOuterwear::Blazer, 18.f, 50.f);
	const FSprawlJacketProfile Utility =
		USprawlWardrobeModule::DevelopJacketProfile(
			ESprawlWardrobeOuterwear::UtilityJacket, 18.f, 50.f);
	const FSprawlJacketProfile Track =
		USprawlWardrobeModule::DevelopJacketProfile(
			ESprawlWardrobeOuterwear::TrackJacket, 18.f, 50.f);

	TestTrue(TEXT("A blazer hangs lower than a cropped bomber"),
		Blazer.HemDropCm > Bomber.HemDropCm);
	TestTrue(TEXT("A bomber gathers into a hem tighter than its chest"),
		Bomber.HemHalfWidthCm < Bomber.ChestHalfWidthCm);
	TestTrue(TEXT("A utility jacket is roomier than a track jacket"),
		Utility.ChestHalfWidthCm > Track.ChestHalfWidthCm);
	TestTrue(TEXT("A blazer carries the tallest lapel"),
		Blazer.CollarHeightCm > Bomber.CollarHeightCm
		&& Blazer.CollarHeightCm > Track.CollarHeightCm);

	FSprawlJacketProfile Barrel = Bomber;
	Barrel.ChestHalfDepthCm = Barrel.ChestHalfWidthCm + 1.f;
	TestFalse(TEXT("A jacket deeper than it is wide is rejected"),
		USprawlWardrobeModule::ValidateJacketProfile(Barrel, Error));

	FSprawlJacketProfile Broken = Bomber;
	Broken.ShellHeightCm = NAN;
	TestFalse(TEXT("A non-finite jacket profile is rejected"),
		USprawlWardrobeModule::ValidateJacketProfile(Broken, Error));

	const FSprawlJacketProfile Degenerate =
		USprawlWardrobeModule::DevelopJacketProfile(
			ESprawlWardrobeOuterwear::BomberJacket, NAN, -0.f);
	TestTrue(TEXT("Unusable measurements still cut a wearable jacket"),
		USprawlWardrobeModule::ValidateJacketProfile(Degenerate, Error));
	return true;
}

#endif
