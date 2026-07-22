// The Connected Sprawl - Authored Zarri streetwear catalog tests.

#include "Characters/SprawlStreetwearModule.h"

#include "Engine/StaticMesh.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlStreetwearModuleTest,
	"ConnectedSprawl.Characters.StreetwearModule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlStreetwearModuleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FSprawlStreetwearKit Kit =
		USprawlStreetwearModule::CreateZarriNanobananaKit();
	FString Error;
	TestTrue(TEXT("Zarri's authored streetwear catalog validates"),
		USprawlStreetwearModule::ValidateKit(Kit, Error));
	TestEqual(TEXT("The complete authored kit contains all modular pieces"),
		Kit.Pieces.Num(), 45);
	TestTrue(TEXT("The catalog description reports the piece count"),
		USprawlStreetwearModule::DescribeKit(Kit).Contains(TEXT("45")));

	int32 LayerCounts[5] = {0, 0, 0, 0, 0};
	TSet<FName> PieceIds;
	for (const FSprawlStreetwearPieceDefinition& Piece : Kit.Pieces)
	{
		++LayerCounts[static_cast<int32>(Piece.Layer)];
		TestFalse(TEXT("Every streetwear piece has a unique ID"),
			PieceIds.Contains(Piece.PieceId));
		PieceIds.Add(Piece.PieceId);
		TestNotNull(*FString::Printf(TEXT("Authored mesh resolves: %s"),
			*Piece.PieceId.ToString()),
			Cast<UStaticMesh>(Piece.MeshPath.TryLoad()));
	}
	TestEqual(TEXT("Hoodie geometry is complete"), LayerCounts[0], 10);
	TestEqual(TEXT("Bomber geometry is complete"), LayerCounts[1], 9);
	TestEqual(TEXT("Cargo geometry is complete"), LayerCounts[2], 8);
	TestEqual(TEXT("Beanie geometry is complete"), LayerCounts[3], 2);
	TestEqual(TEXT("Hero trainer geometry is complete"), LayerCounts[4], 16);

	const FSprawlWardrobeOutfit Nanobanana =
		USprawlWardrobeModule::CreateNanobananaOutfit();
	TestTrue(TEXT("The authored module accepts Zarri's Nanobanana outfit"),
		USprawlStreetwearModule::SupportsOutfit(Nanobanana));
	TestFalse(TEXT("The authored module does not replace an unrelated outfit"),
		USprawlStreetwearModule::SupportsOutfit(
			USprawlWardrobeModule::CreateZarriOutfit()));

	FSprawlStreetwearKit Duplicate = Kit;
	Duplicate.Pieces[1].PieceId = Duplicate.Pieces[0].PieceId;
	TestFalse(TEXT("Duplicate garment IDs are rejected"),
		USprawlStreetwearModule::ValidateKit(Duplicate, Error));
	return true;
}

#endif
