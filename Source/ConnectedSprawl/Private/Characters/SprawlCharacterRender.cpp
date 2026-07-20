// The Connected Sprawl - Keep the street's grime on the street.

#include "Characters/SprawlCharacterRender.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

void FSprawlCharacterRender::DisableDecalProjection(UPrimitiveComponent* Component)
{
	if (Component)
	{
		Component->SetReceivesDecals(false);
	}
}

int32 FSprawlCharacterRender::DisableDecalProjection(AActor* Actor)
{
	if (!Actor)
	{
		return 0;
	}

	int32 Changed = 0;
	TInlineComponentArray<UPrimitiveComponent*> Primitives(Actor);
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		if (Primitive)
		{
			Primitive->SetReceivesDecals(false);
			++Changed;
		}
	}
	return Changed;
}
