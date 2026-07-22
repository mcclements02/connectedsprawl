// The Connected Sprawl - Strict MetaHuman crowd roster and budget tests.

#include "Characters/SprawlCrowdMetaHuman.h"

#include "Characters/SprawlHumanCharacterModule.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlCrowdMetaHumanTest,
	"ConnectedSprawl.Characters.CrowdMetaHuman",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlCrowdMetaHumanTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const TArray<FSprawlCrowdMetaHumanEntry>& Roster =
		FSprawlCrowdMetaHuman::Roster();
	TestTrue(TEXT("The first authored crowd ships at least three real identities"),
		Roster.Num() >= 3);

	TSet<FName> UniqueIds;
	TSet<FSoftObjectPath> UniqueFaceMeshes;
	for (const FSprawlCrowdMetaHumanEntry& Entry : Roster)
	{
		UniqueIds.Add(Entry.AppearanceId);
		const FString Path = Entry.VisualClassPath.ToString();
		TestTrue(*FString::Printf(TEXT("%s is an assembled project MetaHuman"),
			*Entry.AppearanceId.ToString()),
			Path.StartsWith(TEXT("/Game/MetaHumans/"))
			&& Path.EndsWith(TEXT("_C")));
		TestFalse(TEXT("Legacy toy pedestrians never enter the roster"),
			Path.Contains(TEXT("/Game/Pedestrians/")));
		TestTrue(TEXT("Every authored stature is plausible"),
			Entry.AuthoredHeightCm >= 150.f && Entry.AuthoredHeightCm <= 205.f);

		const FString BlueprintObjectPath = Path.LeftChop(2);
		const FString BlueprintPackage =
			FPackageName::ObjectPathToPackageName(BlueprintObjectPath);
		TestTrue(*FString::Printf(TEXT("%s runtime Blueprint package exists"),
			*Entry.AppearanceId.ToString()),
			FPackageName::DoesPackageExist(BlueprintPackage));
		TestTrue(*FString::Printf(TEXT("%s assembled face package exists"),
			*Entry.AppearanceId.ToString()),
			Entry.FaceMeshPath.IsValid()
			&& FPackageName::DoesPackageExist(
				Entry.FaceMeshPath.GetLongPackageName()));
		if (Entry.FaceMeshPath.IsValid())
		{
			UniqueFaceMeshes.Add(Entry.FaceMeshPath);
		}
	}
	TestEqual(TEXT("Roster IDs are unique"), UniqueIds.Num(), Roster.Num());
	TestEqual(TEXT("Roster identities use distinct authored face meshes"),
		UniqueFaceMeshes.Num(), Roster.Num());
	TSet<FName> BalancedCycle;
	for (int32 Index = 0; Index < Roster.Num(); ++Index)
	{
		BalancedCycle.Add(
			FSprawlCrowdMetaHuman::AppearanceIdForPopulationIndex(Index));
	}
	TestEqual(TEXT("One population cycle guarantees every authored identity"),
		BalancedCycle.Num(), Roster.Num());

	TSet<FName> MasculineChoices;
	TSet<FName> FeminineChoices;
	TSet<FName> AndrogynousChoices;
	for (int32 Seed = 0; Seed < 128; ++Seed)
	{
		MasculineChoices.Add(FSprawlCrowdMetaHuman::ChooseAppearanceId(
			Seed, ESprawlHumanPresentation::Masculine));
		FeminineChoices.Add(FSprawlCrowdMetaHuman::ChooseAppearanceId(
			Seed, ESprawlHumanPresentation::Feminine));
		AndrogynousChoices.Add(FSprawlCrowdMetaHuman::ChooseAppearanceId(
			Seed, ESprawlHumanPresentation::Androgynous));
	}
	TestTrue(TEXT("Masculine residents alternate between authored faces"),
		MasculineChoices.Num() >= 2);
	TestTrue(TEXT("The feminine resident base is available"),
		FeminineChoices.Contains(TEXT("Amina")));
	TestTrue(TEXT("Unconstrained identities span the complete launch roster"),
		AndrogynousChoices.Num() >= 3);

	FSprawlHumanCustomization StyleInput;
	StyleInput.Seed = 7719;
	StyleInput.AppearanceId = TEXT("Amina");
	StyleInput.HeightCm = 176.f;
	StyleInput.BodyBuild = ESprawlHumanBuild::Broad;
	const FSprawlCrowdMetaHumanStyle FirstStyle =
		FSprawlCrowdMetaHuman::BuildStyle(StyleInput, 168.f);
	const FSprawlCrowdMetaHumanStyle RepeatStyle =
		FSprawlCrowdMetaHuman::BuildStyle(StyleInput, 168.f);
	TestTrue(TEXT("Copy styling is deterministic"),
		FirstStyle.VisualScale.Equals(RepeatStyle.VisualScale)
		&& FirstStyle.PrimaryWardrobe.Equals(RepeatStyle.PrimaryWardrobe)
		&& FirstStyle.SecondaryWardrobe.Equals(RepeatStyle.SecondaryWardrobe));
	TestTrue(TEXT("Height changes the visual without replacing the face"),
		FirstStyle.VisualScale.Z > 1.f);
	TestTrue(TEXT("Uniform stature preserves authored facial proportions"),
		FMath::IsNearlyEqual(FirstStyle.VisualScale.X, FirstStyle.VisualScale.Z));

	TestEqual(TEXT("iPhone uses a conservative full-MetaHuman cap"),
		FSprawlCrowdMetaHuman::PopulationCap(true), 3);
	TestEqual(TEXT("Mac retains a small realistic crowd"),
		FSprawlCrowdMetaHuman::PopulationCap(false), 8);
	TestEqual(TEXT("iPhone keeps one nearby resident at higher detail"),
		FSprawlCrowdMetaHuman::HighDetailBudget(true), 1);
	TestEqual(TEXT("Mac keeps three nearby residents at higher detail"),
		FSprawlCrowdMetaHuman::HighDetailBudget(false), 3);

	const TArray<int32> Nearest =
		FSprawlCrowdMetaHuman::SelectHighDetailIndices(
			{900.f, 100.f, 400.f, 100.f}, 2);
	TestEqual(TEXT("LOD selection honors its exact budget"), Nearest.Num(), 2);
	TestEqual(TEXT("Equal-distance selection is stable"), Nearest[0], 1);
	TestEqual(TEXT("The next equal-distance resident is selected next"),
		Nearest[1], 3);
	return true;
}

#endif
