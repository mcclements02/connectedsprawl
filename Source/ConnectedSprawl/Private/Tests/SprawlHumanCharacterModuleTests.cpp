// The Connected Sprawl - Human-character diversity, actions, and state tests.

#include "Characters/SprawlHumanCharacterModule.h"

#include "Animation/AnimSequence.h"
#include "Characters/SprawlCrowdMetaHuman.h"
#include "Misc/AutomationTest.h"
#include "World/SprawlCityGridSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlHumanCharacterModuleTest,
	"ConnectedSprawl.Characters.HumanCharacterModule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlHumanCharacterModuleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using Grid = USprawlCityGridSubsystem;

	const FVector Junction(
		Grid::BlockCenter(3), Grid::BlockCenter(3), 0.f);
	const FSprawlCharacterProfile Profile =
		USprawlCharacterDeveloper::DevelopCharacter(41277, Junction, 12.f);
	const FSprawlHumanCustomization First =
		USprawlHumanCharacterModule::DevelopDiverseCustomization(Profile);
	const FSprawlHumanCustomization Repeat =
		USprawlHumanCharacterModule::DevelopDiverseCustomization(Profile);
	TestTrue(TEXT("Customization is deterministic for a developed identity"),
		First == Repeat);
	TestEqual(TEXT("Every generated person uses Zarri's technical rig contract"),
		First.RigTemplateId, FName(TEXT("Zarri_JointsOnly")));
	TestEqual(TEXT("Legacy provenance follows the developed profile"),
		First.LightweightAvatarVariant, Profile.PreferredAvatarVariant);
	TestTrue(TEXT("A city resident selects an authored real-human identity"),
		FSprawlCrowdMetaHuman::FindEntry(First.AppearanceId) != nullptr);

	FString Error;
	TestTrue(TEXT("Generated customization validates"),
		USprawlHumanCharacterModule::ValidateCustomization(First, Error));
	TestTrue(TEXT("Generated customization retains the mobile MetaHuman tier"),
		First.bOptimizedMetaHuman);
	TestTrue(TEXT("Generated customization includes a complete outfit"),
		USprawlWardrobeModule::ValidateOutfit(First.Outfit, Error));
	TestTrue(TEXT("Authoring description contains a concrete human direction"),
		USprawlHumanCharacterModule::DescribeCustomization(First).Contains(
			TEXT("skin tone")));
	TestTrue(TEXT("Authoring description includes footwear"),
		USprawlHumanCharacterModule::DescribeCustomization(First).Contains(
			TEXT("shoe")));

	TSet<uint8> AgeBands;
	TSet<uint8> Presentations;
	TSet<uint8> BodyBuilds;
	TSet<uint8> SkinTones;
	TSet<uint8> HairTextures;
	TSet<FName> AppearanceIds;
	for (int32 Seed = 0; Seed < 256; ++Seed)
	{
		const FSprawlCharacterProfile Resident =
			USprawlCharacterDeveloper::DevelopCharacter(
				Seed, Junction, static_cast<float>(Seed % 24));
		const FSprawlHumanCustomization Look =
			USprawlHumanCharacterModule::DevelopDiverseCustomization(Resident);
		AgeBands.Add(static_cast<uint8>(Look.AgeBand));
		Presentations.Add(static_cast<uint8>(Look.Presentation));
		BodyBuilds.Add(static_cast<uint8>(Look.BodyBuild));
		SkinTones.Add(static_cast<uint8>(Look.SkinTone));
		HairTextures.Add(static_cast<uint8>(Look.HairTexture));
		AppearanceIds.Add(Look.AppearanceId);
		TestTrue(*FString::Printf(TEXT("Customization seed %d validates"), Seed),
			USprawlHumanCharacterModule::ValidateCustomization(Look, Error));
	}
	TestTrue(TEXT("The generated cast spans every age band"), AgeBands.Num() >= 4);
	TestTrue(TEXT("The generated cast spans multiple presentations"),
		Presentations.Num() >= 2);
	TestTrue(TEXT("The generated cast spans every body build"), BodyBuilds.Num() >= 4);
	TestTrue(TEXT("The generated cast spans every skin-tone direction"),
		SkinTones.Num() >= 6);
	TestTrue(TEXT("The generated cast spans varied hair textures"),
		HairTextures.Num() >= 4);
	TestTrue(TEXT("The generated cast uses every launch MetaHuman identity"),
		AppearanceIds.Num() >= 3);

	const FSprawlHumanCustomization Zarri =
		USprawlHumanCharacterModule::CreateZarriCustomization();
	TestEqual(TEXT("Zarri remains a young adult"),
		Zarri.AgeBand, ESprawlHumanAgeBand::YoungAdult);
	TestEqual(TEXT("Zarri retains his athletic direction"),
		Zarri.BodyBuild, ESprawlHumanBuild::Athletic);
	TestEqual(TEXT("Zarri retains his medium-deep complexion direction"),
		Zarri.SkinTone, ESprawlHumanSkinTone::MediumDeep);
	TestEqual(TEXT("Zarri keeps his unique lightweight fallback"),
		Zarri.LightweightAvatarVariant, FString(TEXT("Zarri")));
	TestEqual(TEXT("Zarri keeps his assembled hero identity"),
		Zarri.AppearanceId, FName(TEXT("Zarri")));
	TestEqual(TEXT("Zarri wears Nanobanana techwear outfit by default"),
		Zarri.Outfit, USprawlWardrobeModule::CreateNanobananaOutfit());

	TestEqual(TEXT("Zero speed stands"),
		USprawlHumanCharacterModule::ResolveAction(0.f, false, false, false),
		ESprawlHumanAction::Stand);
	TestEqual(TEXT("Idle conversation talks"),
		USprawlHumanCharacterModule::ResolveAction(0.f, true, false, false),
		ESprawlHumanAction::Talk);
	TestEqual(TEXT("Ground movement walks"),
		USprawlHumanCharacterModule::ResolveAction(140.f, false, false, false),
		ESprawlHumanAction::Walk);
	TestEqual(TEXT("Fast movement runs"),
		USprawlHumanCharacterModule::ResolveAction(470.f, false, false, false),
		ESprawlHumanAction::Run);
	TestEqual(TEXT("A seated person sits"),
		USprawlHumanCharacterModule::ResolveAction(0.f, false, true, false),
		ESprawlHumanAction::Sit);
	TestEqual(TEXT("Driving outranks seated state"),
		USprawlHumanCharacterModule::ResolveAction(0.f, false, true, true),
		ESprawlHumanAction::Drive);

	TestEqual(TEXT("Stand maps to the imported Idle clip"),
		USprawlHumanCharacterModule::FallbackAnimationSuffix(
			ESprawlHumanAction::Stand), FName(TEXT("Idle")));
	TestEqual(TEXT("Run maps to the imported Jog clip"),
		USprawlHumanCharacterModule::FallbackAnimationSuffix(
			ESprawlHumanAction::Run), FName(TEXT("Jog")));
	TestEqual(TEXT("Drive has a safe imported seated fallback"),
		USprawlHumanCharacterModule::FallbackAnimationSuffix(
			ESprawlHumanAction::Drive), FName(TEXT("Sit")));

	USprawlCharacterDefinition* Definition =
		NewObject<USprawlCharacterDefinition>();
	Definition->SitAnimation = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(
		TEXT("/Game/Test/AS_Sit.AS_Sit")));
	TestTrue(TEXT("A named driver falls back to its MetaHuman sit clip"),
		USprawlHumanCharacterModule::AnimationForAction(
			Definition, ESprawlHumanAction::Drive).ToSoftObjectPath()
		== Definition->SitAnimation.ToSoftObjectPath());
	Definition->DriveAnimation = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(
		TEXT("/Game/Test/AS_Drive.AS_Drive")));
	TestTrue(TEXT("A named driver prefers its dedicated MetaHuman drive clip"),
		USprawlHumanCharacterModule::AnimationForAction(
			Definition, ESprawlHumanAction::Drive).ToSoftObjectPath()
		== Definition->DriveAnimation.ToSoftObjectPath());

	USprawlHumanCharacterModule* Module =
		NewObject<USprawlHumanCharacterModule>();
	TestTrue(TEXT("Human modules opt into component replication"),
		Module->GetIsReplicated());
	TestTrue(TEXT("A module configures directly from the developed profile"),
		Module->ConfigureFromProfile(Profile));
	const int32 ConfigRevision = Module->GetRuntimeState().Revision;
	TestEqual(TEXT("Configured modules begin standing"),
		Module->GetRuntimeState().Action, ESprawlHumanAction::Stand);
	TestEqual(TEXT("Automatic movement changes the action"),
		Module->UpdateActionFromMovement(140.f, false, false, false),
		ESprawlHumanAction::Walk);
	TestTrue(TEXT("A real state transition increments the revision"),
		Module->GetRuntimeState().Revision > ConfigRevision);
	TestTrue(TEXT("A scripted sit can be held"),
		Module->SetAction(ESprawlHumanAction::Sit, true));
	TestEqual(TEXT("Movement cannot replace a held sit"),
		Module->UpdateActionFromMovement(470.f, false, false, false),
		ESprawlHumanAction::Sit);
	TestTrue(TEXT("A held action can be released"), Module->ClearHeldAction());
	TestEqual(TEXT("Running resumes after the held action is released"),
		Module->UpdateActionFromMovement(470.f, false, false, false),
		ESprawlHumanAction::Run);
	TestTrue(TEXT("The same module can be configured as the Zarri baseline"),
		Module->ConfigureZarri());
	TestTrue(TEXT("Zarri exposes a complete valid source profile"),
		USprawlCharacterDeveloper::ValidateProfile(
			Module->GetSourceProfile(), Error));
	TestTrue(TEXT("Zarri's source and replicated customization stay aligned"),
		Module->GetRuntimeState().Customization == Zarri);

	FSprawlHumanCustomization Invalid = First;
	Invalid.HeightCm = 220.f;
	TestFalse(TEXT("Unsafe stature is rejected"),
		USprawlHumanCharacterModule::ValidateCustomization(Invalid, Error));
	Invalid = First;
	Invalid.bOptimizedMetaHuman = false;
	TestFalse(TEXT("Non-optimized city MetaHumans are rejected"),
		USprawlHumanCharacterModule::ValidateCustomization(Invalid, Error));
	Invalid = First;
	Invalid.AppearanceId = TEXT("BizDude");
	TestFalse(TEXT("Legacy toy identity cannot enter replicated customization"),
		USprawlHumanCharacterModule::ValidateCustomization(Invalid, Error));
	return true;
}

#endif
