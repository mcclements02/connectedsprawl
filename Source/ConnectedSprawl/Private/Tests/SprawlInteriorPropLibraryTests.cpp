// The Connected Sprawl - realistic interior prop catalog and layout tests.

#include "World/SprawlInteriorPropLibrary.h"

#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
#include "UObject/SoftObjectPath.h"
#include "World/SprawlEnterableInteriors.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlInteriorPropLibraryTest,
	"ConnectedSprawl.World.InteriorPropLibrary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlInteriorPropLibraryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const TArray<FSprawlInteriorPropDefinition>& Definitions =
		FSprawlInteriorPropLibrary::GetDefinitions();
	TestTrue(TEXT("The catalog exposes at least ten real prop silhouettes"),
		Definitions.Num() >= 10);

	TSet<FName> DefinitionIds;
	int32 InstalledPrimaryAssets = 0;
	for (const FSprawlInteriorPropDefinition& Definition : Definitions)
	{
		TestTrue(*FString::Printf(TEXT("%s is a valid prop definition"),
			*Definition.Id.ToString()), Definition.IsValid());
		TestFalse(*FString::Printf(TEXT("%s is uniquely named"),
			*Definition.Id.ToString()), DefinitionIds.Contains(Definition.Id));
		DefinitionIds.Add(Definition.Id);
		if (!Definition.AssetPaths.IsEmpty())
		{
			const FSoftObjectPath ObjectPath(Definition.AssetPaths[0]);
			TestTrue(*FString::Printf(TEXT("%s has a valid object path"),
				*Definition.Id.ToString()), ObjectPath.IsValid());
			InstalledPrimaryAssets += FPackageName::DoesPackageExist(
				ObjectPath.GetLongPackageName()) ? 1 : 0;
		}
	}
	TestTrue(TEXT("The installed Downtown West pack supplies every primary mesh"),
		InstalledPrimaryAssets == Definitions.Num());

	const FSprawlEnterableInteriorsLayout Interiors =
		FSprawlEnterableInteriorsLayout::Build();
	for (const FSprawlEnterableBuildingSpec& Building : Interiors.Buildings)
	{
		const TArray<FSprawlInteriorPropPlacement> Props =
			FSprawlInteriorPropLibrary::BuildForBuilding(
				Building.Kind, Building.InteriorCenter);
		const FString Label = Building.DisplayName.ToString();
		TestTrue(*FString::Printf(TEXT("%s real-prop layout is valid"), *Label),
			FSprawlInteriorPropLibrary::IsValidForRoom(
				Building.Kind, Building.InteriorCenter,
				Building.InteriorHalfExtent, Props));
		TestTrue(*FString::Printf(TEXT("%s meets its prop density"), *Label),
			Props.Num() >= FSprawlInteriorPropLibrary::MinimumPropCount(
				Building.Kind));
	}
	return true;
}

#endif
