// The Connected Sprawl - Swappable athletic-shoe preset catalog.

#include "Characters/SprawlAthleticShoeModule.h"

namespace
{
bool IsFiniteColor(const FLinearColor& Color)
{
	return FMath::IsFinite(Color.R)
		&& FMath::IsFinite(Color.G)
		&& FMath::IsFinite(Color.B)
		&& FMath::IsFinite(Color.A);
}
}

FSprawlAthleticShoeDefinition USprawlAthleticShoeModule::DevelopPreset(
	ESprawlAthleticShoePreset Preset)
{
	FSprawlAthleticShoeDefinition Result;
	switch (Preset)
	{
	case ESprawlAthleticShoePreset::MetroRunner:
		Result.PresetId = TEXT("MetroRunner");
		Result.DisplayName = NSLOCTEXT(
			"SprawlAthleticShoes", "MetroRunner", "Metro Runner");
		Result.Footwear = ESprawlWardrobeFootwear::RunningShoes;
		Result.Socks = ESprawlWardrobeSocks::Ankle;
		Result.UpperColor = FLinearColor(0.70f, 0.73f, 0.76f, 1.f);
		Result.SockColor = FLinearColor(0.82f, 0.82f, 0.84f, 1.f);
		Result.AccentColor = FLinearColor(0.84f, 0.13f, 0.08f, 1.f);
		break;
	case ESprawlAthleticShoePreset::CourtHighTop:
		Result.PresetId = TEXT("CourtHighTop");
		Result.DisplayName = NSLOCTEXT(
			"SprawlAthleticShoes", "CourtHighTop", "Court High-Top");
		Result.Footwear = ESprawlWardrobeFootwear::HighTopSneakers;
		Result.Socks = ESprawlWardrobeSocks::Crew;
		Result.UpperColor = FLinearColor(0.72f, 0.66f, 0.52f, 1.f);
		Result.SockColor = FLinearColor(0.76f, 0.72f, 0.62f, 1.f);
		Result.AccentColor = FLinearColor(0.38f, 0.055f, 0.035f, 1.f);
		break;
	case ESprawlAthleticShoePreset::NightSprint:
		Result.PresetId = TEXT("NightSprint");
		Result.DisplayName = NSLOCTEXT(
			"SprawlAthleticShoes", "NightSprint", "Night Sprint");
		Result.Footwear = ESprawlWardrobeFootwear::AthleticTrainers;
		Result.Socks = ESprawlWardrobeSocks::Ankle;
		Result.UpperColor = FLinearColor(0.018f, 0.022f, 0.030f, 1.f);
		Result.SockColor = FLinearColor(0.025f, 0.028f, 0.035f, 1.f);
		Result.AccentColor = FLinearColor(0.28f, 0.84f, 0.32f, 1.f);
		break;
	default:
		Result.PresetId = TEXT("ZarriVelocity");
		Result.DisplayName = NSLOCTEXT(
			"SprawlAthleticShoes", "ZarriVelocity", "Zarri Velocity");
		Result.Footwear = ESprawlWardrobeFootwear::AthleticTrainers;
		Result.Socks = ESprawlWardrobeSocks::Crew;
		Result.UpperColor = FLinearColor(0.035f, 0.09f, 0.28f, 1.f);
		Result.SockColor = FLinearColor(0.025f, 0.045f, 0.10f, 1.f);
		Result.AccentColor = FLinearColor(0.10f, 0.56f, 0.48f, 1.f);
		break;
	}
	return Result;
}

bool USprawlAthleticShoeModule::ValidatePreset(
	const FSprawlAthleticShoeDefinition& Definition,
	FString& OutError)
{
	if (Definition.PresetId.IsNone() || Definition.DisplayName.IsEmpty())
	{
		OutError = TEXT("Athletic shoe presets require an ID and display name");
		return false;
	}
	const bool bAthleticType =
		Definition.Footwear == ESprawlWardrobeFootwear::AthleticTrainers
		|| Definition.Footwear == ESprawlWardrobeFootwear::RunningShoes
		|| Definition.Footwear == ESprawlWardrobeFootwear::HighTopSneakers;
	if (!bAthleticType)
	{
		OutError = TEXT("Athletic shoe presets require a trainer, runner, or high-top");
		return false;
	}
	if (!IsFiniteColor(Definition.UpperColor)
		|| !IsFiniteColor(Definition.SockColor)
		|| !IsFiniteColor(Definition.AccentColor))
	{
		OutError = TEXT("Athletic shoe colors must be finite");
		return false;
	}
	OutError.Reset();
	return true;
}

ESprawlAthleticShoePreset USprawlAthleticShoeModule::NextPreset(
	ESprawlAthleticShoePreset Current)
{
	const int32 Next = (static_cast<int32>(Current) + 1) % GetPresetCount();
	return static_cast<ESprawlAthleticShoePreset>(Next);
}

bool USprawlAthleticShoeModule::SwapToPreset(
	USprawlFootwearModule* FootwearModule,
	ESprawlAthleticShoePreset Preset)
{
	if (!FootwearModule)
	{
		return false;
	}
	const FSprawlAthleticShoeDefinition Definition = DevelopPreset(Preset);
	FString Error;
	if (!ValidatePreset(Definition, Error))
	{
		return false;
	}
	return FootwearModule->SwapFootwear(
		Definition.Footwear,
		Definition.Socks,
		Definition.UpperColor,
		Definition.SockColor,
		Definition.AccentColor);
}
