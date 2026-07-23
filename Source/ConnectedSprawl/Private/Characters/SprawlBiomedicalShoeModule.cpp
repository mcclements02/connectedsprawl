// The Connected Sprawl - Cordero Biomedical high-top development profile.

#include "Characters/SprawlBiomedicalShoeModule.h"

namespace
{
bool BiomedicalShoeIsFiniteColor(const FLinearColor& Color)
{
	return FMath::IsFinite(Color.R)
		&& FMath::IsFinite(Color.G)
		&& FMath::IsFinite(Color.B)
		&& FMath::IsFinite(Color.A);
}
}

FSprawlBiomedicalShoeDefinition
USprawlBiomedicalShoeModule::DevelopBioCircuitHighTop()
{
	FSprawlBiomedicalShoeDefinition Result;
	Result.DisplayName = NSLOCTEXT(
		"SprawlBiomedicalShoes",
		"CorderoBioCircuitHighTop",
		"Cordero Biomedical Bio-Circuit High-Top");
	return Result;
}

bool USprawlBiomedicalShoeModule::ValidateDefinition(
	const FSprawlBiomedicalShoeDefinition& Definition,
	FString& OutError)
{
	if (Definition.PresetId.IsNone() || Definition.DisplayName.IsEmpty())
	{
		OutError = TEXT("Biomedical shoe definitions require an ID and display name");
		return false;
	}
	if (Definition.Footwear != ESprawlWardrobeFootwear::HighTopSneakers)
	{
		OutError = TEXT("The Bio-Circuit profile requires fitted high-top sneakers");
		return false;
	}
	if (!BiomedicalShoeIsFiniteColor(Definition.NanofiberUpperColor)
		|| !BiomedicalShoeIsFiniteColor(Definition.SupportColor)
		|| !BiomedicalShoeIsFiniteColor(Definition.CircuitColor)
		|| !BiomedicalShoeIsFiniteColor(Definition.SockColor))
	{
		OutError = TEXT("Biomedical shoe colors must be finite");
		return false;
	}
	if (Definition.PresentationMesh.IsNull() || Definition.BlenderSource.IsEmpty())
	{
		OutError = TEXT("Biomedical shoe definitions require mesh and Blender source paths");
		return false;
	}
	if (Definition.ReferenceLengthCm < 20.f
		|| Definition.ReferenceLengthCm > 36.f
		|| Definition.TractionPodCountPerShoe < 4)
	{
		OutError = TEXT("Biomedical shoe dimensions or traction layout are invalid");
		return false;
	}
	if (!Definition.bHasCircuitWindow || !Definition.bHasHeelAirUnit)
	{
		OutError = TEXT("The Bio-Circuit profile requires its circuit window and heel air unit");
		return false;
	}

	OutError.Reset();
	return true;
}

FString USprawlBiomedicalShoeModule::DescribeDefinition(
	const FSprawlBiomedicalShoeDefinition& Definition)
{
	return FString::Printf(
		TEXT("%s | %.1f cm | %d traction pods per shoe | nanofiber upper | circuit window | heel air unit"),
		*Definition.DisplayName.ToString(),
		Definition.ReferenceLengthCm,
		Definition.TractionPodCountPerShoe);
}

bool USprawlBiomedicalShoeModule::SwapToBioCircuitHighTop(
	USprawlFootwearModule* FootwearModule)
{
	if (!FootwearModule)
	{
		return false;
	}

	const FSprawlBiomedicalShoeDefinition Definition =
		DevelopBioCircuitHighTop();
	FString Error;
	if (!ValidateDefinition(Definition, Error))
	{
		return false;
	}

	return FootwearModule->SwapFootwear(
		Definition.Footwear,
		Definition.Socks,
		Definition.NanofiberUpperColor,
		Definition.SockColor,
		Definition.SupportColor);
}
