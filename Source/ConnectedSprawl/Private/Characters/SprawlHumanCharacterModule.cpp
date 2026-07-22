// The Connected Sprawl - Reusable, replicated human-character contract.

#include "Characters/SprawlHumanCharacterModule.h"

#include "Animation/AnimSequence.h"
#include "Characters/SprawlCrowdMetaHuman.h"
#include "Characters/SprawlWardrobeModule.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

namespace
{
bool SprawlHumanIsFeminineVariant(const FString& Variant)
{
	static const TSet<FString> Variants = {
		TEXT("Erika"), TEXT("Jennifer"), TEXT("Kate"), TEXT("Lydia"),
		TEXT("Olivia"), TEXT("Rose"), TEXT("Samuela"), TEXT("CursedAmy"),
		TEXT("Eugenia"), TEXT("Jenny"), TEXT("Juanita")};
	return Variants.Contains(Variant);
}

ESprawlHumanPresentation SprawlHumanPresentationForVariant(
	const FString& Variant)
{
	if (Variant == TEXT("Chill"))
	{
		return ESprawlHumanPresentation::Androgynous;
	}
	return SprawlHumanIsFeminineVariant(Variant)
		? ESprawlHumanPresentation::Feminine
		: ESprawlHumanPresentation::Masculine;
}

ESprawlHumanAgeBand SprawlHumanAgeForProfile(
	const FSprawlCharacterProfile& Profile, FRandomStream& Stream)
{
	if (Profile.Role == ESprawlCharacterRole::Student)
	{
		return ESprawlHumanAgeBand::YoungAdult;
	}
	if (Profile.PreferredAvatarVariant == TEXT("OldMoustache"))
	{
		return ESprawlHumanAgeBand::Elder;
	}
	if (Profile.PreferredAvatarVariant == TEXT("Baldman")
		|| Profile.PreferredAvatarVariant == TEXT("Mafiossini"))
	{
		return ESprawlHumanAgeBand::Mature;
	}

	const float Roll = Stream.FRand();
	if (Roll < 0.18f) return ESprawlHumanAgeBand::YoungAdult;
	if (Roll < 0.72f) return ESprawlHumanAgeBand::Adult;
	if (Roll < 0.94f) return ESprawlHumanAgeBand::Mature;
	return ESprawlHumanAgeBand::Elder;
}

ESprawlHumanBuild SprawlHumanBuildForProfile(
	const FSprawlCharacterProfile& Profile, FRandomStream& Stream)
{
	switch (Profile.Role)
	{
	case ESprawlCharacterRole::Courier:
		return Stream.FRand() < 0.7f
			? ESprawlHumanBuild::Athletic : ESprawlHumanBuild::Lean;
	case ESprawlCharacterRole::WarehouseWorker:
		return Stream.FRand() < 0.6f
			? ESprawlHumanBuild::Broad : ESprawlHumanBuild::Athletic;
	case ESprawlCharacterRole::Student:
		return Stream.FRand() < 0.65f
			? ESprawlHumanBuild::Lean : ESprawlHumanBuild::Average;
	default:
		return static_cast<ESprawlHumanBuild>(Stream.RandRange(
			0, static_cast<int32>(ESprawlHumanBuild::Broad)));
	}
}

const TCHAR* SprawlHumanAgeName(ESprawlHumanAgeBand Value)
{
	switch (Value)
	{
	case ESprawlHumanAgeBand::YoungAdult: return TEXT("young adult");
	case ESprawlHumanAgeBand::Mature: return TEXT("mature adult");
	case ESprawlHumanAgeBand::Elder: return TEXT("older adult");
	default: return TEXT("adult");
	}
}

const TCHAR* SprawlHumanPresentationName(ESprawlHumanPresentation Value)
{
	switch (Value)
	{
	case ESprawlHumanPresentation::Masculine: return TEXT("masculine-presenting");
	case ESprawlHumanPresentation::Feminine: return TEXT("feminine-presenting");
	default: return TEXT("androgynous-presenting");
	}
}

const TCHAR* SprawlHumanBuildName(ESprawlHumanBuild Value)
{
	switch (Value)
	{
	case ESprawlHumanBuild::Lean: return TEXT("lean");
	case ESprawlHumanBuild::Athletic: return TEXT("athletic");
	case ESprawlHumanBuild::Broad: return TEXT("broad");
	default: return TEXT("average-build");
	}
}

const TCHAR* SprawlHumanSkinName(ESprawlHumanSkinTone Value)
{
	switch (Value)
	{
	case ESprawlHumanSkinTone::Deep: return TEXT("deep skin tone");
	case ESprawlHumanSkinTone::Dark: return TEXT("dark skin tone");
	case ESprawlHumanSkinTone::MediumDeep: return TEXT("medium-deep skin tone");
	case ESprawlHumanSkinTone::Medium: return TEXT("medium skin tone");
	case ESprawlHumanSkinTone::MediumLight: return TEXT("medium-light skin tone");
	default: return TEXT("light skin tone");
	}
}

const TCHAR* SprawlHumanHairName(ESprawlHumanHairTexture Value)
{
	switch (Value)
	{
	case ESprawlHumanHairTexture::Bald: return TEXT("bald or closely shaved hair");
	case ESprawlHumanHairTexture::Straight: return TEXT("straight hair");
	case ESprawlHumanHairTexture::Wavy: return TEXT("wavy hair");
	case ESprawlHumanHairTexture::Curly: return TEXT("curly hair");
	default: return TEXT("coily hair");
	}
}
}

bool FSprawlHumanCustomization::operator==(
	const FSprawlHumanCustomization& Other) const
{
	return RigTemplateId == Other.RigTemplateId
		&& Seed == Other.Seed
		&& AppearanceId == Other.AppearanceId
		&& AgeBand == Other.AgeBand
		&& Presentation == Other.Presentation
		&& BodyBuild == Other.BodyBuild
		&& SkinTone == Other.SkinTone
		&& HairTexture == Other.HairTexture
		&& Wardrobe == Other.Wardrobe
		&& Outfit == Other.Outfit
		&& FMath::IsNearlyEqual(HeightCm, Other.HeightCm)
		&& LightweightAvatarVariant == Other.LightweightAvatarVariant
		&& bOptimizedMetaHuman == Other.bOptimizedMetaHuman;
}

USprawlHumanCharacterModule::USprawlHumanCharacterModule()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void USprawlHumanCharacterModule::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USprawlHumanCharacterModule, RuntimeState);
}

bool USprawlHumanCharacterModule::CanMutateState() const
{
	const AActor* Owner = GetOwner();
	return !Owner || Owner->GetNetMode() == NM_Standalone || Owner->HasAuthority();
}

void USprawlHumanCharacterModule::CommitState(
	FSprawlHumanRuntimeState NewState)
{
	NewState.Revision = RuntimeState.Revision >= MAX_int32
		? 1 : RuntimeState.Revision + 1;
	RuntimeState = MoveTemp(NewState);
	OnRuntimeStateChanged.Broadcast(RuntimeState);
	if (AActor* Owner = GetOwner())
	{
		Owner->ForceNetUpdate();
	}
}

bool USprawlHumanCharacterModule::ConfigureFromProfile(
	const FSprawlCharacterProfile& Profile)
{
	if (!CanMutateState())
	{
		return false;
	}

	FString Error;
	if (!USprawlCharacterDeveloper::ValidateProfile(Profile, Error))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[HumanCharacter] rejected %s: %s"),
			*Profile.CharacterId.ToString(), *Error);
		return false;
	}

	SourceProfile = Profile;
	FSprawlHumanRuntimeState NewState;
	NewState.CharacterId = Profile.CharacterId;
	NewState.Customization = DevelopDiverseCustomization(Profile);
	NewState.Action = ESprawlHumanAction::Stand;
	CommitState(MoveTemp(NewState));
	return true;
}

bool USprawlHumanCharacterModule::ConfigureFromDefinition(
	USprawlCharacterDefinition* Definition)
{
	return Definition && ConfigureFromProfile(Definition->Profile);
}

bool USprawlHumanCharacterModule::ConfigureZarri(int32 Seed)
{
	if (!CanMutateState())
	{
		return false;
	}
	SourceProfile = FSprawlCharacterProfile();
	SourceProfile.CharacterId = TEXT("Zarri");
	SourceProfile.DisplayName = TEXT("Zarri");
	SourceProfile.Seed = Seed;
	SourceProfile.HomeDistrict = ESprawlCharacterDistrict::RailYards;
	SourceProfile.WorkDistrict = ESprawlCharacterDistrict::IronForest;
	SourceProfile.Role = ESprawlCharacterRole::Founder;
	SourceProfile.Activity = ESprawlCharacterActivity::Working;
	SourceProfile.Destination = TEXT("ZarriWorks and the next meeting");
	SourceProfile.ActiveStartHour = 7.f;
	SourceProfile.ActiveEndHour = 23.f;
	SourceProfile.Wardrobe = ESprawlWardrobeStyle::Streetwear;
	SourceProfile.HeightCm = 178.f;
	SourceProfile.WalkSpeedScale = 1.f;
	SourceProfile.CrossChance = 0.30f;
	SourceProfile.IdleTalkChance = 0.35f;
	SourceProfile.PreferredAvatarVariant = TEXT("Zarri");
	SourceProfile.MetaHumanCreatorBrief =
		TEXT("Use the approved MHC_Zarri Optimized/Low joints-only assembly: ")
		TEXT("young Black founder, medium-deep skin, athletic build, Afro fade, ")
		TEXT("and tech-forward streetwear. Preserve Zarri's unique face.");
	SourceProfile.ReferenceImagePrompt =
		TEXT("Approved realistic Zarri turnaround: front, side, and back views; ")
		TEXT("neutral A-pose; medium-deep skin; Afro fade; athletic build; ")
		TEXT("tech-forward streetwear; plain studio background; no logos.");

	FSprawlHumanRuntimeState NewState;
	NewState.CharacterId = TEXT("Zarri");
	NewState.Customization = CreateZarriCustomization(Seed);
	NewState.Action = ESprawlHumanAction::Stand;
	CommitState(MoveTemp(NewState));
	return true;
}

bool USprawlHumanCharacterModule::SetCustomization(
	const FSprawlHumanCustomization& Customization, FString& OutError)
{
	if (!CanMutateState())
	{
		OutError = TEXT("Only the authority may change replicated customization");
		return false;
	}
	if (!ValidateCustomization(Customization, OutError))
	{
		return false;
	}
	if (RuntimeState.Customization == Customization)
	{
		OutError.Reset();
		return true;
	}

	FSprawlHumanRuntimeState NewState = RuntimeState;
	NewState.Customization = Customization;
	CommitState(MoveTemp(NewState));
	OutError.Reset();
	return true;
}

bool USprawlHumanCharacterModule::SetAction(
	ESprawlHumanAction NewAction, bool bHoldAction)
{
	if (!CanMutateState())
	{
		return false;
	}
	if (RuntimeState.Action == NewAction
		&& RuntimeState.bActionHeld == bHoldAction)
	{
		return true;
	}

	FSprawlHumanRuntimeState NewState = RuntimeState;
	NewState.Action = NewAction;
	NewState.bActionHeld = bHoldAction;
	CommitState(MoveTemp(NewState));
	return true;
}

bool USprawlHumanCharacterModule::ClearHeldAction()
{
	if (!CanMutateState())
	{
		return false;
	}
	if (!RuntimeState.bActionHeld)
	{
		return true;
	}
	FSprawlHumanRuntimeState NewState = RuntimeState;
	NewState.bActionHeld = false;
	CommitState(MoveTemp(NewState));
	return true;
}

ESprawlHumanAction USprawlHumanCharacterModule::UpdateActionFromMovement(
	float GroundSpeed, bool bTalking, bool bSeated,
	bool bDriving, bool bForceRun)
{
	if (RuntimeState.bActionHeld)
	{
		return RuntimeState.Action;
	}
	const ESprawlHumanAction Desired = ResolveAction(
		GroundSpeed, bTalking, bSeated, bDriving, bForceRun);
	SetAction(Desired, false);
	return RuntimeState.Action;
}

FSprawlHumanCustomization USprawlHumanCharacterModule::DevelopDiverseCustomization(
	const FSprawlCharacterProfile& Profile)
{
	FRandomStream Stream(Profile.Seed ^ 0x51A77E11);
	FSprawlHumanCustomization Result;
	Result.RigTemplateId = TEXT("Zarri_JointsOnly");
	Result.Seed = Profile.Seed;
	Result.AgeBand = SprawlHumanAgeForProfile(Profile, Stream);
	Result.Presentation = SprawlHumanPresentationForVariant(
		Profile.PreferredAvatarVariant);
	Result.AppearanceId = FSprawlCrowdMetaHuman::ChooseAppearanceId(
		Profile.Seed, Result.Presentation);
	Result.BodyBuild = SprawlHumanBuildForProfile(Profile, Stream);
	Result.SkinTone = static_cast<ESprawlHumanSkinTone>(Stream.RandRange(
		0, static_cast<int32>(ESprawlHumanSkinTone::Light)));
	Result.HairTexture = Profile.PreferredAvatarVariant == TEXT("Baldman")
		? ESprawlHumanHairTexture::Bald
		: static_cast<ESprawlHumanHairTexture>(Stream.RandRange(
			static_cast<int32>(ESprawlHumanHairTexture::Straight),
			static_cast<int32>(ESprawlHumanHairTexture::Coily)));
	Result.Wardrobe = Profile.Wardrobe;
	Result.Outfit = USprawlWardrobeModule::DevelopOutfit(
		Profile.Wardrobe, Profile.Seed);
	Result.HeightCm = Profile.HeightCm;
	Result.LightweightAvatarVariant = Profile.PreferredAvatarVariant;
	Result.bOptimizedMetaHuman = true;
	return Result;
}

FSprawlHumanCustomization USprawlHumanCharacterModule::CreateZarriCustomization(
	int32 Seed)
{
	FSprawlHumanCustomization Result;
	Result.RigTemplateId = TEXT("Zarri_JointsOnly");
	Result.Seed = Seed;
	Result.AppearanceId = TEXT("Zarri");
	Result.AgeBand = ESprawlHumanAgeBand::YoungAdult;
	Result.Presentation = ESprawlHumanPresentation::Masculine;
	Result.BodyBuild = ESprawlHumanBuild::Athletic;
	Result.SkinTone = ESprawlHumanSkinTone::MediumDeep;
	Result.HairTexture = ESprawlHumanHairTexture::Coily;
	Result.Wardrobe = ESprawlWardrobeStyle::Streetwear;
	Result.Outfit = USprawlWardrobeModule::CreateNanobananaOutfit(Seed);
	Result.HeightCm = 178.f;
	Result.LightweightAvatarVariant = TEXT("Zarri");
	Result.bOptimizedMetaHuman = true;
	return Result;
}

ESprawlHumanAction USprawlHumanCharacterModule::ResolveAction(
	float GroundSpeed, bool bTalking, bool bSeated,
	bool bDriving, bool bForceRun)
{
	if (bDriving) return ESprawlHumanAction::Drive;
	if (bSeated) return ESprawlHumanAction::Sit;
	if (bForceRun || GroundSpeed >= 315.f) return ESprawlHumanAction::Run;
	if (GroundSpeed >= 25.f) return ESprawlHumanAction::Walk;
	if (bTalking) return ESprawlHumanAction::Talk;
	return ESprawlHumanAction::Stand;
}

bool USprawlHumanCharacterModule::ValidateCustomization(
	const FSprawlHumanCustomization& Customization, FString& OutError)
{
	if (Customization.RigTemplateId.IsNone())
	{
		OutError = TEXT("RigTemplateId is required");
		return false;
	}
	if (!FSprawlCrowdMetaHuman::FindEntry(Customization.AppearanceId))
	{
		OutError = TEXT("AppearanceId must reference the real-human crowd roster");
		return false;
	}
	if (Customization.HeightCm < 150.f || Customization.HeightCm > 205.f)
	{
		OutError = TEXT("HeightCm must stay between 150 and 205");
		return false;
	}
	if (!USprawlWardrobeModule::ValidateOutfit(
		Customization.Outfit, OutError))
	{
		return false;
	}
	if (!Customization.bOptimizedMetaHuman)
	{
		OutError = TEXT("City characters must use the Optimized MetaHuman pipeline");
		return false;
	}
	OutError.Reset();
	return true;
}

FString USprawlHumanCharacterModule::DescribeCustomization(
	const FSprawlHumanCustomization& Customization)
{
	return FString::Printf(
		TEXT("%s [%s], %s, %s build, %s, %s, %.0f cm tall; %s"),
		*Customization.AppearanceId.ToString(),
		SprawlHumanAgeName(Customization.AgeBand),
		SprawlHumanPresentationName(Customization.Presentation),
		SprawlHumanBuildName(Customization.BodyBuild),
		SprawlHumanSkinName(Customization.SkinTone),
		SprawlHumanHairName(Customization.HairTexture),
		Customization.HeightCm,
		*USprawlWardrobeModule::DescribeOutfit(Customization.Outfit));
}

FName USprawlHumanCharacterModule::FallbackAnimationSuffix(
	ESprawlHumanAction Action)
{
	switch (Action)
	{
	case ESprawlHumanAction::Walk: return TEXT("Walk");
	case ESprawlHumanAction::Run: return TEXT("Jog");
	case ESprawlHumanAction::Talk: return TEXT("Talk");
	case ESprawlHumanAction::Sit:
	case ESprawlHumanAction::Drive: return TEXT("Sit");
	default: return TEXT("Idle");
	}
}

TSoftObjectPtr<UAnimSequence> USprawlHumanCharacterModule::AnimationForAction(
	const USprawlCharacterDefinition* Definition, ESprawlHumanAction Action)
{
	if (!Definition)
	{
		return nullptr;
	}

	switch (Action)
	{
	case ESprawlHumanAction::Walk: return Definition->WalkAnimation;
	case ESprawlHumanAction::Run: return Definition->RunAnimation;
	case ESprawlHumanAction::Talk: return Definition->TalkAnimation;
	case ESprawlHumanAction::Sit: return Definition->SitAnimation;
	case ESprawlHumanAction::Drive:
		return Definition->DriveAnimation.IsNull()
			? Definition->SitAnimation : Definition->DriveAnimation;
	default: return Definition->IdleAnimation;
	}
}

void USprawlHumanCharacterModule::OnRep_RuntimeState()
{
	OnRuntimeStateChanged.Broadcast(RuntimeState);
}
