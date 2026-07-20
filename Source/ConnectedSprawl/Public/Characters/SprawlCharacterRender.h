// The Connected Sprawl - Keep the street's grime on the street.

#pragma once

#include "CoreMinimal.h"

class AActor;
class UPrimitiveComponent;

/**
 * FSprawlCharacterRender
 * ----------------------
 * The city dresses its roads with deferred decals — crosswalk paint, oil
 * grime, posters. A decal projects onto every primitive inside its box unless
 * that primitive opts out, so a character walking across a painted crossing
 * wears the paint: stripes smeared up Zarri's legs and across his shirt.
 *
 * Characters are never a valid projection surface here, so this applies the
 * opt-out uniformly — to the hero (body, face, hair and clothing all live on
 * separate components of the assembled MetaHuman), to pedestrians, and to
 * seated drivers.
 */
struct CONNECTEDSPRAWL_API FSprawlCharacterRender
{
	/** Stop world decals projecting onto one component. */
	static void DisableDecalProjection(UPrimitiveComponent* Component);

	/** Same for every primitive under an actor; returns how many were changed. */
	static int32 DisableDecalProjection(AActor* Actor);
};
