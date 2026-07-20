// The Connected Sprawl - Crowd casting: who is allowed on Zarri's streets.

#include "Characters/SprawlCrowdAppearance.h"

#include "Characters/SprawlAvatarLibrary.h"
#include "Engine/SkeletalMesh.h"

namespace
{
TArray<FString> GHumanVariants;
bool bGHumanVariantsBuilt = false;

void BuildHumanVariants()
{
	bGHumanVariantsBuilt = true;
	GHumanVariants.Reset();

	const TArray<FString>& AllVariants = FSprawlAvatarLibrary::PedestrianVariants();
	TArray<FString> Rejected;
	TArray<FString> Unmeasured;

	for (const FString& Variant : AllVariants)
	{
		USkeletalMesh* Mesh = FSprawlAvatarLibrary::LoadAvatarMesh(Variant);
		float HeadRatio = 0.f;
		if (!Mesh || !FSprawlAvatarLibrary::ComputeHeadHeightRatio(Mesh, HeadRatio))
		{
			// Unmeasurable art is kept: silently emptying the streets would be
			// a worse failure than an occasional mismatched extra.
			Unmeasured.Add(Variant);
			GHumanVariants.Add(Variant);
			continue;
		}

		UE_LOG(LogTemp, Display, TEXT("[Crowd] %-14s head/height=%.3f  %s"),
			*Variant, HeadRatio,
			HeadRatio <= FSprawlCrowdAppearance::MaxHumanHeadRatio
				? TEXT("cast") : TEXT("cut (stylised proportions)"));

		if (HeadRatio <= FSprawlCrowdAppearance::MaxHumanHeadRatio)
		{
			GHumanVariants.Add(Variant);
		}
		else
		{
			Rejected.Add(Variant);
		}
	}

	if (GHumanVariants.IsEmpty())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Crowd] proportion cut rejected every variant; using the "
				"unfiltered pool so the city stays populated"));
		GHumanVariants = AllVariants;
		return;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[Crowd] cast %d/%d variants (cut %d, unmeasured %d kept)"),
		GHumanVariants.Num(), AllVariants.Num(), Rejected.Num(), Unmeasured.Num());
}
}

const TArray<FString>& FSprawlCrowdAppearance::HumanVariants()
{
	if (!bGHumanVariantsBuilt)
	{
		BuildHumanVariants();
	}
	return GHumanVariants;
}

bool FSprawlCrowdAppearance::IsHumanProportioned(const FString& VariantName)
{
	return HumanVariants().Contains(VariantName);
}
