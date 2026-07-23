// The Connected Sprawl - Reference clothing mapping tests.

#include "Characters/SprawlReferenceClothingModule.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlReferenceClothingModuleTest,
	"ConnectedSprawl.Characters.ReferenceClothingModule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlReferenceClothingModuleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FSprawlReferenceClothingKit Kit =
		USprawlReferenceClothingModule::CreateZarriReferenceKit();
	FString Error;
	TestEqual(TEXT("Reference kit contains all authored pieces"),
		Kit.Pieces.Num(), 17);
	TestTrue(TEXT("Reference shirt and pants contract validates"),
		USprawlReferenceClothingModule::ValidateKit(Kit, Error));
	TestTrue(TEXT("Description identifies Blender mapping"),
		USprawlReferenceClothingModule::DescribeKit(Kit).Contains(TEXT("Blender")));

	int32 ShirtCount = 0;
	int32 PantsCount = 0;
	int32 LiveFitCount = 0;
	for (const FSprawlReferenceClothingPiece& Piece : Kit.Pieces)
	{
		TestTrue(TEXT("Every garment resolves inside the project clothing root"),
			Piece.MeshPath.ToString().StartsWith(
				TEXT("/Game/Import/Characters/ReferenceClothing/")));
		if (Piece.Layer == ESprawlReferenceClothingLayer::Shirt)
		{
			++ShirtCount;
		}
		else
		{
			++PantsCount;
		}
		if (Piece.bFollowBoneSegment)
		{
			++LiveFitCount;
			TestTrue(TEXT("Live fits have a normalized export axis"),
				Piece.AuthoredDirection.IsNormalized());
			TestTrue(TEXT("Live fits have a positive authored length"),
				Piece.AuthoredLengthCm > 0.f);
		}
	}
	TestEqual(TEXT("Nine shirt sections"), ShirtCount, 9);
	TestEqual(TEXT("Eight pants sections"), PantsCount, 8);
	TestEqual(TEXT("Twelve pieces refit to live limb segments"),
		LiveFitCount, 12);

	USprawlReferenceClothingModule* Module =
		NewObject<USprawlReferenceClothingModule>();
	TestNotNull(TEXT("Reference clothing module constructs"), Module);
	if (Module)
	{
		TestFalse(TEXT("Invalid runtime inputs fail without side effects"),
			Module->ApplyToMetaHuman(nullptr, nullptr));
		TestEqual(TEXT("Fresh module has no spawned pieces"),
			Module->GetPieceCount(), 0);
	}
	return true;
}

#endif
