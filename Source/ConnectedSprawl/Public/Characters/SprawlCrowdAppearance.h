// The Connected Sprawl - Crowd casting: who is allowed on Zarri's streets.

#pragma once

#include "CoreMinimal.h"

/**
 * FSprawlCrowdAppearance
 * ----------------------
 * Zarri is a realistically proportioned MetaHuman, but the imported avatar
 * pool mixes real-human scans with stylised big-head characters. Standing a
 * caricature next to him breaks the scene instantly, and hand-maintaining a
 * blocklist rots the moment new art lands.
 *
 * So the pool is cast by measurement instead: each candidate's head length is
 * read from its reference skeleton as a fraction of standing height. Adults
 * sit near an eighth (0.12-0.14); cartoon proportions run far larger. Only
 * variants inside the human band walk the city or ride in traffic.
 *
 * Measured once on first use and cached; the full table is logged so the cut
 * is auditable rather than a magic constant.
 */
struct CONNECTEDSPRAWL_API FSprawlCrowdAppearance
{
	/**
	 * Widest head-to-height ratio still read as a real adult. Set from the
	 * measured spread of the imported pool, comfortably clear of both the
	 * human cluster and the caricatures.
	 */
	static constexpr float MaxHumanHeadRatio = 0.20f;

	/**
	 * Pedestrian variants whose proportions match Zarri. Falls back to the
	 * unfiltered pool if measurement rejects everything, so a bad threshold or
	 * a missing art import can never empty the streets.
	 */
	static const TArray<FString>& HumanVariants();

	/** True when this variant passes the proportion cut. */
	static bool IsHumanProportioned(const FString& VariantName);
};
