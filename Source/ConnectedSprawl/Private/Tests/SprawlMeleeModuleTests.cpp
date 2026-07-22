// The Connected Sprawl - Punch/kick module contract tests.

#include "Characters/SprawlMeleeInput.h"
#include "Characters/SprawlMeleeModule.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSprawlMeleeModuleTest,
	"ConnectedSprawl.Characters.MeleeModule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSprawlMeleeModuleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const TArray<FKey>& DirectKeys = FSprawlMeleeInput::GetOnFootKeys();
	TestEqual(TEXT("Pawn-level melee has exactly keyboard and gamepad routes"),
		DirectKeys.Num(), 2);
	TestTrue(TEXT("Keyboard X is a direct on-foot melee key"),
		FSprawlMeleeInput::IsOnFootKey(EKeys::X));
	TestTrue(TEXT("The gamepad X/left face button is a direct melee key"),
		FSprawlMeleeInput::IsOnFootKey(EKeys::Gamepad_FaceButton_Left));
	TestFalse(TEXT("Unrelated movement keys cannot trigger melee"),
		FSprawlMeleeInput::IsOnFootKey(EKeys::W));
	TestNotEqual(TEXT("Direct melee routes do not duplicate one another"),
		DirectKeys[0], DirectKeys[1]);

	FString Error;
	const FSprawlMeleeAttackSpec Punch =
		USprawlMeleeModule::DefaultAttackSpec(ESprawlMeleeAttack::Punch);
	const FSprawlMeleeAttackSpec Kick =
		USprawlMeleeModule::DefaultAttackSpec(ESprawlMeleeAttack::Kick);
	TestTrue(TEXT("The default punch spec validates"),
		USprawlMeleeModule::ValidateAttackSpec(Punch, Error));
	TestTrue(TEXT("The default kick spec validates"),
		USprawlMeleeModule::ValidateAttackSpec(Kick, Error));
	TestTrue(TEXT("A kick has more impact than a punch"),
		Kick.Damage > Punch.Damage
		&& Kick.KnockbackStrength > Punch.KnockbackStrength);
	TestTrue(TEXT("A kick trades cadence for reach"),
		Kick.Range > Punch.Range
		&& Kick.CooldownSeconds > Punch.CooldownSeconds);

	const FVector Origin = FVector::ZeroVector;
	const FVector Forward = FVector::ForwardVector;
	TestTrue(TEXT("A character directly in front is targetable"),
		USprawlMeleeModule::IsTargetInsideAttackArc(
			Origin, Forward, FVector(150.f, 0.f, 0.f), Punch));
	TestTrue(TEXT("Mild aim error remains targetable"),
		USprawlMeleeModule::IsTargetInsideAttackArc(
			Origin, Forward, FVector(115.f, 65.f, 0.f), Punch));
	TestFalse(TEXT("A character behind Zarri is never targetable"),
		USprawlMeleeModule::IsTargetInsideAttackArc(
			Origin, Forward, FVector(-60.f, 0.f, 0.f), Punch));
	TestFalse(TEXT("A character outside the cone is rejected"),
		USprawlMeleeModule::IsTargetInsideAttackArc(
			Origin, Forward, FVector(30.f, 145.f, 0.f), Punch));
	TestFalse(TEXT("A character beyond assisted reach is rejected"),
		USprawlMeleeModule::IsTargetInsideAttackArc(
			Origin, Forward,
			FVector(Punch.Range + Punch.SweepRadius + 1.f, 0.f, 0.f), Punch));
	TestFalse(TEXT("A different floor is rejected"),
		USprawlMeleeModule::IsTargetInsideAttackArc(
			Origin, Forward,
			FVector(50.f, 0.f, Punch.VerticalTolerance + 1.f), Punch));
	TestFalse(TEXT("A zero facing vector cannot target a bystander"),
		USprawlMeleeModule::IsTargetInsideAttackArc(
			Origin, FVector::ZeroVector, FVector(50.f, 0.f, 0.f), Punch));

	FSprawlMeleeAttackSpec Invalid = Punch;
	Invalid.CooldownSeconds = 0.f;
	TestFalse(TEXT("An unsafe zero cooldown is rejected"),
		USprawlMeleeModule::ValidateAttackSpec(Invalid, Error));
	Invalid = Punch;
	Invalid.Range = 500.f;
	TestFalse(TEXT("An implausible reach is rejected"),
		USprawlMeleeModule::ValidateAttackSpec(Invalid, Error));

	TestEqual(TEXT("Punch alternates to kick"),
		USprawlMeleeModule::AlternateAttack(ESprawlMeleeAttack::Punch),
		ESprawlMeleeAttack::Kick);
	TestEqual(TEXT("Kick alternates back to punch"),
		USprawlMeleeModule::AlternateAttack(ESprawlMeleeAttack::Kick),
		ESprawlMeleeAttack::Punch);

	USprawlMeleeModule* Module = NewObject<USprawlMeleeModule>();
	TestTrue(TEXT("Melee modules opt into component replication"),
		Module->GetIsReplicated());
	TestEqual(TEXT("The single action button starts with a punch"),
		Module->GetNextAlternatingAttack(), ESprawlMeleeAttack::Punch);
	TestFalse(TEXT("An unowned module cannot produce an attack"),
		Module->TryAlternatingAttack());
	TestEqual(TEXT("Rejected attacks do not advance alternation"),
		Module->GetNextAlternatingAttack(), ESprawlMeleeAttack::Punch);
	TestEqual(TEXT("Rejected attacks do not create replicated state"),
		Module->GetRuntimeState().Revision, 0);
	return true;
}

#endif
