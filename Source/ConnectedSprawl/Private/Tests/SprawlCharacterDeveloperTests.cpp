// The Connected Sprawl - Character Developer determinism and city-casting tests.

#include "Characters/SprawlCharacterDeveloper.h"

#include "Misc/AutomationTest.h"
#include "World/SprawlCityGridSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

using Grid = USprawlCityGridSubsystem;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlCharacterDeveloperTest,
	"ConnectedSprawl.Characters.CharacterDeveloper",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlCharacterDeveloperTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestEqual(TEXT("The center blocks are The Junction"),
		USprawlCharacterDeveloper::DistrictForLocation(FVector(
			Grid::BlockCenter(3), Grid::BlockCenter(3), 0.f)),
		ESprawlCharacterDistrict::Junction);
	TestEqual(TEXT("The north blocks are Iron Forest"),
		USprawlCharacterDeveloper::DistrictForLocation(FVector(
			Grid::BlockCenter(3), Grid::BlockCenter(6), 0.f)),
		ESprawlCharacterDistrict::IronForest);
	TestEqual(TEXT("The south blocks are Rail Yards"),
		USprawlCharacterDeveloper::DistrictForLocation(FVector(
			Grid::BlockCenter(1), Grid::BlockCenter(0), 0.f)),
		ESprawlCharacterDistrict::RailYards);
	TestEqual(TEXT("Outer middle blocks are arteries"),
		USprawlCharacterDeveloper::DistrictForLocation(FVector(
			Grid::BlockCenter(0), Grid::BlockCenter(3), 0.f)),
		ESprawlCharacterDistrict::Arteries);

	const FVector Junction(
		Grid::BlockCenter(3), Grid::BlockCenter(3), 0.f);
	const FSprawlCharacterProfile First =
		USprawlCharacterDeveloper::DevelopCharacter(8675309, Junction, 8.5f);
	const FSprawlCharacterProfile Repeat =
		USprawlCharacterDeveloper::DevelopCharacter(8675309, Junction, 8.5f);
	TestEqual(TEXT("A seed produces a stable identity"),
		First.CharacterId, Repeat.CharacterId);
	TestEqual(TEXT("A seed produces a stable name"),
		First.DisplayName, Repeat.DisplayName);
	TestEqual(TEXT("A seed produces a stable role"), First.Role, Repeat.Role);
	TestEqual(TEXT("A seed produces a stable avatar"),
		First.PreferredAvatarVariant, Repeat.PreferredAvatarVariant);
	TestTrue(TEXT("A seed produces a stable height"),
		FMath::IsNearlyEqual(First.HeightCm, Repeat.HeightCm));

	const FSprawlCharacterProfile Different =
		USprawlCharacterDeveloper::DevelopCharacter(8675310, Junction, 8.5f);
	TestNotEqual(TEXT("Different seeds produce different identities"),
		First.CharacterId, Different.CharacterId);

	for (int32 Seed = 0; Seed < 128; ++Seed)
	{
		const int32 Gx = Seed % Grid::NumBlocks;
		const int32 Gy = (Seed / Grid::NumBlocks) % Grid::NumBlocks;
		const FVector Location(
			Grid::BlockCenter(Gx), Grid::BlockCenter(Gy), 0.f);
		const float Hour = static_cast<float>(Seed % 24);
		const FSprawlCharacterProfile Profile =
			USprawlCharacterDeveloper::DevelopCharacter(Seed, Location, Hour);
		FString Error;
		TestTrue(*FString::Printf(TEXT("Seed %d validates: %s"), Seed, *Error),
			USprawlCharacterDeveloper::ValidateProfile(Profile, Error));
		TestTrue(TEXT("Generated crowd height fits the existing capsule"),
			Profile.HeightCm >= 158.f && Profile.HeightCm <= 176.f);
		TestTrue(TEXT("MetaHuman brief retains the mobile assembly contract"),
			Profile.MetaHumanCreatorBrief.Contains(TEXT("Optimized/Low"))
			&& Profile.MetaHumanCreatorBrief.Contains(TEXT("hair cards")));
		TestTrue(TEXT("Reference prompt asks for a usable turnaround"),
			Profile.ReferenceImagePrompt.Contains(TEXT("Front"))
			&& Profile.ReferenceImagePrompt.Contains(TEXT("back views")));
	}

	const float JunctionDay = USprawlCharacterDeveloper::PopulationMultiplier(
		ESprawlCharacterDistrict::Junction, 12.f);
	const float IronDay = USprawlCharacterDeveloper::PopulationMultiplier(
		ESprawlCharacterDistrict::IronForest, 12.f);
	const float JunctionLate = USprawlCharacterDeveloper::PopulationMultiplier(
		ESprawlCharacterDistrict::Junction, 3.f);
	TestTrue(TEXT("The Junction is busier than Iron Forest by day"),
		JunctionDay > IronDay);
	TestTrue(TEXT("The Junction naturally thins after midnight"),
		JunctionLate < JunctionDay);
	TestTrue(TEXT("Density never exceeds the mobile cap"),
		JunctionDay <= 1.f && IronDay <= 1.f && JunctionLate <= 1.f);
	return true;
}

#endif
