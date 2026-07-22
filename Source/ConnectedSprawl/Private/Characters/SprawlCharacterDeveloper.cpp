// The Connected Sprawl - Data-driven city character and MetaHuman briefs.

#include "Characters/SprawlCharacterDeveloper.h"

#include "Characters/SprawlHumanCharacterModule.h"
#include "EngineUtils.h"
#include "World/SprawlCityGridSubsystem.h"
#include "World/SprawlEnvironmentController.h"

namespace
{
using Grid = USprawlCityGridSubsystem;

struct FWeightedRole
{
	ESprawlCharacterRole Role;
	int32 Weight;
};

float NormalizeHour(float Hour)
{
	const float Wrapped = FMath::Fmod(Hour, 24.f);
	return Wrapped < 0.f ? Wrapped + 24.f : Wrapped;
}

bool IsNightHour(float Hour)
{
	const float H = NormalizeHour(Hour);
	return H >= 21.f || H < 6.f;
}

bool IsRushHour(float Hour)
{
	const float H = NormalizeHour(Hour);
	return (H >= 7.f && H < 10.f) || (H >= 16.f && H < 19.f);
}

ESprawlCharacterRole PickWeightedRole(
	FRandomStream& Stream, const TArray<FWeightedRole>& Choices)
{
	int32 Total = 0;
	for (const FWeightedRole& Choice : Choices)
	{
		Total += FMath::Max(0, Choice.Weight);
	}
	if (Total <= 0)
	{
		return ESprawlCharacterRole::Resident;
	}

	int32 Roll = Stream.RandRange(1, Total);
	for (const FWeightedRole& Choice : Choices)
	{
		Roll -= FMath::Max(0, Choice.Weight);
		if (Roll <= 0)
		{
			return Choice.Role;
		}
	}
	return Choices.Last().Role;
}

ESprawlCharacterRole PickRole(
	FRandomStream& Stream, ESprawlCharacterDistrict District, float Hour)
{
	if (IsNightHour(Hour))
	{
		switch (District)
		{
		case ESprawlCharacterDistrict::IronForest:
			return PickWeightedRole(Stream, {
				{ ESprawlCharacterRole::NightWorker, 45 },
				{ ESprawlCharacterRole::TechWorker, 15 },
				{ ESprawlCharacterRole::Courier, 10 },
				{ ESprawlCharacterRole::Resident, 30 }});
		case ESprawlCharacterDistrict::RailYards:
			return PickWeightedRole(Stream, {
				{ ESprawlCharacterRole::WarehouseWorker, 35 },
				{ ESprawlCharacterRole::NightWorker, 25 },
				{ ESprawlCharacterRole::Courier, 20 },
				{ ESprawlCharacterRole::Resident, 20 }});
		case ESprawlCharacterDistrict::Junction:
			return PickWeightedRole(Stream, {
				{ ESprawlCharacterRole::Resident, 30 },
				{ ESprawlCharacterRole::ServiceWorker, 25 },
				{ ESprawlCharacterRole::NightWorker, 20 },
				{ ESprawlCharacterRole::Courier, 15 },
				{ ESprawlCharacterRole::Student, 10 }});
		default:
			return PickWeightedRole(Stream, {
				{ ESprawlCharacterRole::Resident, 35 },
				{ ESprawlCharacterRole::NightWorker, 25 },
				{ ESprawlCharacterRole::Courier, 25 },
				{ ESprawlCharacterRole::Commuter, 15 }});
		}
	}

	const int32 CommuteWeight = IsRushHour(Hour) ? 30 : 12;
	switch (District)
	{
	case ESprawlCharacterDistrict::IronForest:
		return PickWeightedRole(Stream, {
			{ ESprawlCharacterRole::TechWorker, 38 },
			{ ESprawlCharacterRole::Commuter, CommuteWeight },
			{ ESprawlCharacterRole::Founder, 12 },
			{ ESprawlCharacterRole::ServiceWorker, 10 },
			{ ESprawlCharacterRole::Resident, 10 }});
	case ESprawlCharacterDistrict::RailYards:
		return PickWeightedRole(Stream, {
			{ ESprawlCharacterRole::WarehouseWorker, 35 },
			{ ESprawlCharacterRole::Courier, 20 },
			{ ESprawlCharacterRole::Commuter, CommuteWeight },
			{ ESprawlCharacterRole::Resident, 20 },
			{ ESprawlCharacterRole::ServiceWorker, 10 }});
	case ESprawlCharacterDistrict::Junction:
		return PickWeightedRole(Stream, {
			{ ESprawlCharacterRole::Resident, 20 },
			{ ESprawlCharacterRole::Commuter, CommuteWeight },
			{ ESprawlCharacterRole::ServiceWorker, 20 },
			{ ESprawlCharacterRole::Student, 15 },
			{ ESprawlCharacterRole::Founder, 10 },
			{ ESprawlCharacterRole::Courier, 10 }});
	default:
		return PickWeightedRole(Stream, {
			{ ESprawlCharacterRole::Commuter, CommuteWeight },
			{ ESprawlCharacterRole::Resident, 25 },
			{ ESprawlCharacterRole::Courier, 18 },
			{ ESprawlCharacterRole::ServiceWorker, 15 },
			{ ESprawlCharacterRole::Student, 12 }});
	}
}

ESprawlCharacterDistrict WorkDistrictFor(
	ESprawlCharacterRole Role, ESprawlCharacterDistrict CurrentDistrict,
	FRandomStream& Stream)
{
	switch (Role)
	{
	case ESprawlCharacterRole::TechWorker:
	case ESprawlCharacterRole::Founder:
		return ESprawlCharacterDistrict::IronForest;
	case ESprawlCharacterRole::WarehouseWorker:
		return ESprawlCharacterDistrict::RailYards;
	case ESprawlCharacterRole::ServiceWorker:
	case ESprawlCharacterRole::Student:
		return ESprawlCharacterDistrict::Junction;
	case ESprawlCharacterRole::Courier:
	case ESprawlCharacterRole::Commuter:
		return static_cast<ESprawlCharacterDistrict>(
			Stream.RandRange(0, static_cast<int32>(ESprawlCharacterDistrict::Arteries)));
	default:
		return CurrentDistrict;
	}
}

ESprawlWardrobeStyle WardrobeFor(ESprawlCharacterRole Role)
{
	switch (Role)
	{
	case ESprawlCharacterRole::TechWorker:
		return ESprawlWardrobeStyle::SmartCasual;
	case ESprawlCharacterRole::Commuter:
		return ESprawlWardrobeStyle::Corporate;
	case ESprawlCharacterRole::WarehouseWorker:
	case ESprawlCharacterRole::NightWorker:
		return ESprawlWardrobeStyle::Workwear;
	case ESprawlCharacterRole::Courier:
		return ESprawlWardrobeStyle::Athletic;
	default:
		return ESprawlWardrobeStyle::Streetwear;
	}
}

const TCHAR* DistrictName(ESprawlCharacterDistrict District)
{
	switch (District)
	{
	case ESprawlCharacterDistrict::Junction: return TEXT("The Junction");
	case ESprawlCharacterDistrict::IronForest: return TEXT("Iron Forest");
	case ESprawlCharacterDistrict::RailYards: return TEXT("Rail Yards");
	default: return TEXT("the 85-Express arteries");
	}
}

const TCHAR* RoleName(ESprawlCharacterRole Role)
{
	switch (Role)
	{
	case ESprawlCharacterRole::Resident: return TEXT("local resident");
	case ESprawlCharacterRole::Commuter: return TEXT("cross-city commuter");
	case ESprawlCharacterRole::TechWorker: return TEXT("technology worker");
	case ESprawlCharacterRole::ServiceWorker: return TEXT("service worker");
	case ESprawlCharacterRole::WarehouseWorker: return TEXT("warehouse worker");
	case ESprawlCharacterRole::Courier: return TEXT("bike and foot courier");
	case ESprawlCharacterRole::Founder: return TEXT("early-stage founder");
	case ESprawlCharacterRole::Student: return TEXT("student");
	default: return TEXT("night-shift worker");
	}
}

const TCHAR* WardrobeName(ESprawlWardrobeStyle Wardrobe)
{
	switch (Wardrobe)
	{
	case ESprawlWardrobeStyle::SmartCasual: return TEXT("practical smart-casual layers");
	case ESprawlWardrobeStyle::Corporate: return TEXT("clean office separates");
	case ESprawlWardrobeStyle::Workwear: return TEXT("worn, functional workwear");
	case ESprawlWardrobeStyle::Athletic: return TEXT("weather-ready athletic layers");
	default: return TEXT("contemporary everyday streetwear");
	}
}

ESprawlCharacterActivity ActivityFor(
	ESprawlCharacterRole Role, float Hour, FRandomStream& Stream)
{
	const float H = NormalizeHour(Hour);
	if (IsRushHour(H))
	{
		return ESprawlCharacterActivity::Commuting;
	}
	if (IsNightHour(H))
	{
		if (Role == ESprawlCharacterRole::NightWorker
			|| Role == ESprawlCharacterRole::WarehouseWorker
			|| Role == ESprawlCharacterRole::Courier)
		{
			return ESprawlCharacterActivity::Working;
		}
		return Stream.FRand() < 0.45f
			? ESprawlCharacterActivity::Socializing
			: ESprawlCharacterActivity::HeadingHome;
	}
	if (H >= 10.f && H < 16.f)
	{
		if (Role == ESprawlCharacterRole::Resident || Role == ESprawlCharacterRole::Student)
		{
			return Stream.FRand() < 0.35f
				? ESprawlCharacterActivity::Socializing
				: ESprawlCharacterActivity::RunningErrand;
		}
		return ESprawlCharacterActivity::Working;
	}
	if (H >= 19.f)
	{
		return Stream.FRand() < 0.35f
			? ESprawlCharacterActivity::Exercising
			: ESprawlCharacterActivity::Socializing;
	}
	return ESprawlCharacterActivity::RunningErrand;
}

const TCHAR* DestinationFor(
	ESprawlCharacterActivity Activity, ESprawlCharacterDistrict WorkDistrict)
{
	switch (Activity)
	{
	case ESprawlCharacterActivity::Commuting: return DistrictName(WorkDistrict);
	case ESprawlCharacterActivity::Working: return TEXT("their current shift");
	case ESprawlCharacterActivity::RunningErrand: return TEXT("a nearby shop or service");
	case ESprawlCharacterActivity::Socializing: return TEXT("a cafe, stoop, or public corner");
	case ESprawlCharacterActivity::Exercising: return TEXT("a park or waterfront route");
	default: return TEXT("home");
	}
}

void ScheduleFor(ESprawlCharacterRole Role, float& OutStart, float& OutEnd)
{
	switch (Role)
	{
	case ESprawlCharacterRole::NightWorker:
		OutStart = 20.f; OutEnd = 6.f; break;
	case ESprawlCharacterRole::WarehouseWorker:
		OutStart = 6.f; OutEnd = 18.f; break;
	case ESprawlCharacterRole::Courier:
		OutStart = 8.f; OutEnd = 22.f; break;
	case ESprawlCharacterRole::Student:
		OutStart = 8.f; OutEnd = 19.f; break;
	case ESprawlCharacterRole::ServiceWorker:
		OutStart = 7.f; OutEnd = 23.f; break;
	default:
		OutStart = 7.f; OutEnd = 20.f; break;
	}
}

FString PickName(FRandomStream& Stream)
{
	static const TCHAR* FirstNames[] = {
		TEXT("Amina"), TEXT("Andre"), TEXT("Camila"), TEXT("Darius"),
		TEXT("Elena"), TEXT("Imani"), TEXT("Jae"), TEXT("Jordan"),
		TEXT("Luis"), TEXT("Maya"), TEXT("Nia"), TEXT("Omar"),
		TEXT("Priya"), TEXT("Rafael"), TEXT("Sam"), TEXT("Tasha")};
	static const TCHAR* LastInitials[] = {
		TEXT("A."), TEXT("B."), TEXT("C."), TEXT("D."), TEXT("G."),
		TEXT("H."), TEXT("K."), TEXT("M."), TEXT("P."), TEXT("R."),
		TEXT("S."), TEXT("T."), TEXT("V."), TEXT("W.")};
	return FString::Printf(TEXT("%s %s"),
		FirstNames[Stream.RandRange(0, UE_ARRAY_COUNT(FirstNames) - 1)],
		LastInitials[Stream.RandRange(0, UE_ARRAY_COUNT(LastInitials) - 1)]);
}

FString PickAvatar(ESprawlWardrobeStyle Wardrobe, FRandomStream& Stream)
{
	static const TCHAR* Streetwear[] = {
		TEXT("Chill"), TEXT("Kyle"), TEXT("Samuela"), TEXT("Bruno"),
		TEXT("Erika")};
	static const TCHAR* SmartCasual[] = {
		TEXT("Erika"), TEXT("Lydia"), TEXT("BizDude"), TEXT("Bruno")};
	static const TCHAR* Corporate[] = {
		TEXT("BizDude"), TEXT("Erika"), TEXT("Lydia")};
	static const TCHAR* Workwear[] = {
		TEXT("Baldman"), TEXT("Bruno"), TEXT("Samuela"), TEXT("Kyle")};
	static const TCHAR* Athletic[] = {
		TEXT("Chill"), TEXT("Kyle"), TEXT("Samuela"), TEXT("Bruno")};

	auto Pick = [&Stream](const TCHAR* const* Values, int32 Count)
	{
		return FString(Values[Stream.RandRange(0, Count - 1)]);
	};
	switch (Wardrobe)
	{
	case ESprawlWardrobeStyle::SmartCasual:
		return Pick(SmartCasual, UE_ARRAY_COUNT(SmartCasual));
	case ESprawlWardrobeStyle::Corporate:
		return Pick(Corporate, UE_ARRAY_COUNT(Corporate));
	case ESprawlWardrobeStyle::Workwear:
		return Pick(Workwear, UE_ARRAY_COUNT(Workwear));
	case ESprawlWardrobeStyle::Athletic:
		return Pick(Athletic, UE_ARRAY_COUNT(Athletic));
	default:
		return Pick(Streetwear, UE_ARRAY_COUNT(Streetwear));
	}
}
}

FPrimaryAssetId USprawlCharacterDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("SprawlCharacter"), GetFName());
}

ESprawlCharacterDistrict USprawlCharacterDeveloper::DistrictForLocation(
	const FVector& WorldLocation)
{
	const int32 Gx = Grid::NearestBlockIndex(WorldLocation.X);
	const int32 Gy = Grid::NearestBlockIndex(WorldLocation.Y);
	if (Gy <= 1)
	{
		return ESprawlCharacterDistrict::RailYards;
	}
	if (Gy >= 5)
	{
		return ESprawlCharacterDistrict::IronForest;
	}
	if (Gx >= 2 && Gx <= 4 && Gy >= 2 && Gy <= 4)
	{
		return ESprawlCharacterDistrict::Junction;
	}
	return ESprawlCharacterDistrict::Arteries;
}

FSprawlCharacterProfile USprawlCharacterDeveloper::DevelopCharacter(
	int32 Seed, const FVector& WorldLocation, float CityHour)
{
	FRandomStream Stream(Seed);
	FSprawlCharacterProfile Profile;
	Profile.Seed = Seed;
	Profile.CharacterId = FName(*FString::Printf(
		TEXT("City_%08X"), static_cast<uint32>(Seed)));
	Profile.DisplayName = PickName(Stream);
	Profile.HomeDistrict = DistrictForLocation(WorldLocation);
	Profile.Role = PickRole(Stream, Profile.HomeDistrict, CityHour);
	Profile.WorkDistrict = WorkDistrictFor(Profile.Role, Profile.HomeDistrict, Stream);
	Profile.Activity = ActivityFor(Profile.Role, CityHour, Stream);
	Profile.Destination = DestinationFor(Profile.Activity, Profile.WorkDistrict);
	Profile.Wardrobe = WardrobeFor(Profile.Role);
	Profile.PreferredAvatarVariant = PickAvatar(Profile.Wardrobe, Stream);
	// Stay inside the existing 176 cm crowd capsule. Named MetaHumans authored
	// through USprawlCharacterDefinition may use the wider validated range.
	Profile.HeightCm = Stream.FRandRange(158.f, 176.f);
	Profile.WalkSpeedScale = Stream.FRandRange(0.88f, 1.12f);
	Profile.CrossChance = 0.30f;
	Profile.IdleTalkChance = 0.18f;

	switch (Profile.Role)
	{
	case ESprawlCharacterRole::Courier:
		Profile.WalkSpeedScale *= 1.10f;
		Profile.CrossChance = 0.42f;
		Profile.IdleTalkChance = 0.08f;
		break;
	case ESprawlCharacterRole::WarehouseWorker:
		Profile.CrossChance = 0.18f;
		Profile.IdleTalkChance = 0.08f;
		break;
	case ESprawlCharacterRole::TechWorker:
	case ESprawlCharacterRole::Founder:
		Profile.IdleTalkChance = 0.34f;
		break;
	case ESprawlCharacterRole::ServiceWorker:
		Profile.IdleTalkChance = 0.30f;
		break;
	case ESprawlCharacterRole::Student:
		Profile.CrossChance = 0.36f;
		Profile.IdleTalkChance = 0.28f;
		break;
	default:
		break;
	}

	Profile.WalkSpeedScale = FMath::Clamp(Profile.WalkSpeedScale, 0.75f, 1.30f);
	ScheduleFor(Profile.Role, Profile.ActiveStartHour, Profile.ActiveEndHour);
	Profile.MetaHumanCreatorBrief = BuildMetaHumanCreatorBrief(Profile);
	Profile.ReferenceImagePrompt = BuildReferenceImagePrompt(Profile);
	return Profile;
}

float USprawlCharacterDeveloper::PopulationMultiplier(
	ESprawlCharacterDistrict District, float CityHour)
{
	const float H = NormalizeHour(CityHour);
	if (H >= 1.f && H < 6.f)
	{
		switch (District)
		{
		case ESprawlCharacterDistrict::Junction: return 0.42f;
		case ESprawlCharacterDistrict::RailYards: return 0.34f;
		case ESprawlCharacterDistrict::IronForest: return 0.22f;
		default: return 0.28f;
		}
	}
	if (H >= 21.f || H < 1.f)
	{
		switch (District)
		{
		case ESprawlCharacterDistrict::Junction: return 0.72f;
		case ESprawlCharacterDistrict::RailYards: return 0.50f;
		case ESprawlCharacterDistrict::IronForest: return 0.32f;
		default: return 0.45f;
		}
	}
	switch (District)
	{
	case ESprawlCharacterDistrict::Junction: return 1.0f;
	case ESprawlCharacterDistrict::RailYards: return 0.68f;
	case ESprawlCharacterDistrict::IronForest: return 0.56f;
	default: return 0.76f;
	}
}

FString USprawlCharacterDeveloper::BuildMetaHumanCreatorBrief(
	const FSprawlCharacterProfile& Profile)
{
	const FSprawlHumanCustomization Customization =
		USprawlHumanCharacterModule::DevelopDiverseCustomization(Profile);
	const FString Appearance =
		USprawlHumanCharacterModule::DescribeCustomization(Customization);
	return FString::Printf(
		TEXT("Create %s, a %s based in %s. Appearance direction: %s. Wardrobe: %s, ")
		TEXT("layered for believable daily use with no visible brand logos. Favor natural ")
		TEXT("skin detail, subtle asymmetry, practical grooming, and a rested neutral expression. ")
		TEXT("Use Zarri's joints-only body and six-action animation contract as a technical ")
		TEXT("baseline, but make the face and identity visibly unique rather than a Zarri clone. ")
		TEXT("Assemble with Optimized/Low quality, hair cards, joints-only rig, and 2K source ")
		TEXT("textures so the character remains suitable for the iPhone crowd budget."),
		*Profile.DisplayName, RoleName(Profile.Role), DistrictName(Profile.HomeDistrict),
		*Appearance, WardrobeName(Profile.Wardrobe));
}

FString USprawlCharacterDeveloper::BuildReferenceImagePrompt(
	const FSprawlCharacterProfile& Profile)
{
	const FSprawlHumanCustomization Customization =
		USprawlHumanCharacterModule::DevelopDiverseCustomization(Profile);
	const FString Appearance =
		USprawlHumanCharacterModule::DescribeCustomization(Customization);
	return FString::Printf(
		TEXT("Realistic full-body character turnaround sheet for %s, %s from %s: %s, wearing %s. ")
		TEXT("Front, left profile, and back views at matching scale; neutral A-pose; 85mm lens ")
		TEXT("look; even studio light; plain light-gray background; natural human proportions; ")
		TEXT("clear garment seams and footwear; no logos, text, props, dramatic pose, or environment."),
		*Profile.DisplayName, RoleName(Profile.Role), DistrictName(Profile.HomeDistrict),
		*Appearance, WardrobeName(Profile.Wardrobe));
}

bool USprawlCharacterDeveloper::ValidateProfile(
	const FSprawlCharacterProfile& Profile, FString& OutError)
{
	if (Profile.CharacterId.IsNone())
	{
		OutError = TEXT("CharacterId is required");
		return false;
	}
	if (Profile.DisplayName.TrimStartAndEnd().IsEmpty())
	{
		OutError = TEXT("DisplayName is required");
		return false;
	}
	if (Profile.HeightCm < 150.f || Profile.HeightCm > 205.f)
	{
		OutError = TEXT("HeightCm must stay between 150 and 205");
		return false;
	}
	if (Profile.WalkSpeedScale < 0.6f || Profile.WalkSpeedScale > 1.5f)
	{
		OutError = TEXT("WalkSpeedScale must stay between 0.6 and 1.5");
		return false;
	}
	if (Profile.CrossChance < 0.f || Profile.CrossChance > 1.f
		|| Profile.IdleTalkChance < 0.f || Profile.IdleTalkChance > 1.f)
	{
		OutError = TEXT("Behavior chances must stay between 0 and 1");
		return false;
	}
	if (Profile.PreferredAvatarVariant.IsEmpty())
	{
		OutError = TEXT("PreferredAvatarVariant is required for the crowd fallback");
		return false;
	}
	if (Profile.MetaHumanCreatorBrief.IsEmpty() || Profile.ReferenceImagePrompt.IsEmpty())
	{
		OutError = TEXT("MetaHuman and reference-image authoring briefs are required");
		return false;
	}
	OutError.Reset();
	return true;
}

float USprawlCharacterDeveloper::ResolveCityHour(const UWorld* World)
{
	if (World)
	{
		TActorIterator<ASprawlEnvironmentController> It(World);
		if (It)
		{
			return It->GetHour();
		}
	}
	return 8.071530f;
}
