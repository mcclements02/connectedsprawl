// The Connected Sprawl - Human-scale fitted footwear policy tests.

#include "Characters/SprawlFootwearModule.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlFootwearModuleTest,
	"ConnectedSprawl.Characters.FootwearModule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlFootwearModuleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const ESprawlWardrobeFootwear Styles[] = {
		ESprawlWardrobeFootwear::LowTopSneakers,
		ESprawlWardrobeFootwear::HighTopSneakers,
		ESprawlWardrobeFootwear::DressShoes,
		ESprawlWardrobeFootwear::WorkBoots,
		ESprawlWardrobeFootwear::RunningShoes,
		ESprawlWardrobeFootwear::AthleticTrainers,
	};
	FString Error;
	for (const ESprawlWardrobeFootwear Style : Styles)
	{
		for (const float HeelToBall : {10.5f, 14.f, 16.5f})
		{
			const FSprawlFootwearDimensions Dimensions =
				USprawlFootwearModule::DevelopDimensions(
					Style, ESprawlWardrobeSocks::Crew, HeelToBall);
			TestTrue(*FString::Printf(
				TEXT("Style %d at %.1f cm remains human scale"),
				static_cast<int32>(Style), HeelToBall),
				USprawlFootwearModule::ValidateDimensions(Dimensions, Error));
		}
	}

	const FSprawlFootwearDimensions LowTop =
		USprawlFootwearModule::DevelopDimensions(
			ESprawlWardrobeFootwear::LowTopSneakers,
			ESprawlWardrobeSocks::Crew, 14.f);
	const FSprawlFootwearDimensions HighTop =
		USprawlFootwearModule::DevelopDimensions(
			ESprawlWardrobeFootwear::HighTopSneakers,
			ESprawlWardrobeSocks::Crew, 14.f);
	const FSprawlFootwearDimensions Dress =
		USprawlFootwearModule::DevelopDimensions(
			ESprawlWardrobeFootwear::DressShoes,
			ESprawlWardrobeSocks::Dress, 14.f);
	const FSprawlFootwearDimensions WorkBoot =
		USprawlFootwearModule::DevelopDimensions(
			ESprawlWardrobeFootwear::WorkBoots,
			ESprawlWardrobeSocks::Crew, 14.f);
	const FSprawlFootwearDimensions Running =
		USprawlFootwearModule::DevelopDimensions(
			ESprawlWardrobeFootwear::RunningShoes,
			ESprawlWardrobeSocks::Ankle, 14.f);
	const FSprawlFootwearDimensions Athletic =
		USprawlFootwearModule::DevelopDimensions(
			ESprawlWardrobeFootwear::AthleticTrainers,
			ESprawlWardrobeSocks::Crew, 14.f);

	TestTrue(TEXT("High-top collars are taller than low-top collars"),
		HighTop.UpperHeightCm > LowTop.UpperHeightCm);
	TestTrue(TEXT("Work boots have the tallest collar"),
		WorkBoot.UpperHeightCm > HighTop.UpperHeightCm);
	TestTrue(TEXT("Dress shoes use a narrower last"),
		Dress.ShoeWidthCm < LowTop.ShoeWidthCm);
	TestTrue(TEXT("Running shoes have more sole cushion than dress shoes"),
		Running.SoleThicknessCm > Dress.SoleThicknessCm);
	TestTrue(TEXT("Athletic trainers have more cushion than running shoes"),
		Athletic.SoleThicknessCm > Running.SoleThicknessCm);
	TestTrue(TEXT("Athletic trainers use a stable forefoot width"),
		Athletic.ShoeWidthCm > LowTop.ShoeWidthCm);
	TestTrue(TEXT("Dress socks are longer than crew socks"),
		Dress.SockLengthCm > LowTop.SockLengthCm);
	TestTrue(TEXT("Crew socks are longer than ankle socks"),
		LowTop.SockLengthCm > Running.SockLengthCm);

	FSprawlFootwearDimensions Invalid = LowTop;
	Invalid.ShoeLengthCm = 75.f;
	TestFalse(TEXT("Oversized paddle footwear is rejected"),
		USprawlFootwearModule::ValidateDimensions(Invalid, Error));

	const FTransform AnimatedCalfAnchor(
		FRotator(8.f, 25.f, -4.f), FVector(30.f, -12.f, 8.f),
		FVector::ZeroVector);
	const FTransform ShoeOffset(
		FRotator(2.f, -3.f, 1.f), FVector(-6.f, 0.8f, -4.2f),
		FVector::OneVector);
	const FTransform FirstFollow =
		USprawlFootwearModule::ResolveBoneFollowTransform(
			AnimatedCalfAnchor, ShoeOffset);
	TestTrue(TEXT("A zero-scale skeletal anchor cannot collapse its shoe"),
		FirstFollow.GetScale3D().Equals(FVector::OneVector));

	const FVector StepDelta(18.f, 7.f, 5.f);
	FTransform MovedCalfAnchor = AnimatedCalfAnchor;
	MovedCalfAnchor.AddToTranslation(StepDelta);
	const FTransform MovedFollow =
		USprawlFootwearModule::ResolveBoneFollowTransform(
			MovedCalfAnchor, ShoeOffset);
	TestTrue(TEXT("The shoe follows animated skeletal translation exactly"),
		(MovedFollow.GetLocation() - FirstFollow.GetLocation()).Equals(
			StepDelta, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("The shoe retains the animated skeletal rotation"),
		MovedFollow.GetRotation().Equals(FirstFollow.GetRotation(), KINDA_SMALL_NUMBER));

	const FTransform BodyWorld(
		FRotator(0.f, 45.f, 0.f), FVector(200.f, -80.f, 12.f),
		FVector::OneVector);
	FTransform ShoeBodyFacing(
		FRotator(0.f, 12.f, 0.f), FVector::ZeroVector,
		FVector::OneVector);
	const FTransform GroundedFirst =
		USprawlFootwearModule::ResolveAnimatedShoeTransform(
			AnimatedCalfAnchor, ShoeOffset, BodyWorld, ShoeBodyFacing);
	FTransform PitchedCalfAnchor = AnimatedCalfAnchor;
	PitchedCalfAnchor.SetRotation(FRotator(68.f, 25.f, -4.f).Quaternion());
	const FTransform GroundedPitched =
		USprawlFootwearModule::ResolveAnimatedShoeTransform(
			PitchedCalfAnchor, ShoeOffset, BodyWorld, ShoeBodyFacing);
	TestTrue(TEXT("A running calf pitch does not point the shoe vertically"),
		GroundedPitched.GetRotation().Equals(
			GroundedFirst.GetRotation(), KINDA_SMALL_NUMBER));
	TestFalse(TEXT("The shoe position still responds to the running calf pose"),
		GroundedPitched.GetLocation().Equals(
			GroundedFirst.GetLocation(), KINDA_SMALL_NUMBER));
	FTransform TurnedBody = BodyWorld;
	TurnedBody.SetRotation(FRotator(0.f, 90.f, 0.f).Quaternion());
	const FTransform GroundedTurned =
		USprawlFootwearModule::ResolveAnimatedShoeTransform(
			AnimatedCalfAnchor, ShoeOffset, TurnedBody, ShoeBodyFacing);
	TestFalse(TEXT("The shoe rotates with the character's facing"),
		GroundedTurned.GetRotation().Equals(
			GroundedFirst.GetRotation(), KINDA_SMALL_NUMBER));
	return true;
}

#endif
