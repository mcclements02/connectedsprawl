// The Connected Sprawl - Real-human visual roster for the ambient city crowd.

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"

class AActor;
class UAnimSequence;
class UChildActorComponent;
class USkeletalMeshComponent;
enum class ESprawlHumanPresentation : uint8;
struct FSprawlHumanCustomization;

/** One authored Optimized/Low MetaHuman identity available to the crowd. */
struct CONNECTEDSPRAWL_API FSprawlCrowdMetaHumanEntry
{
	FName AppearanceId;
	FSoftObjectPath VisualClassPath;
	FSoftObjectPath FaceMeshPath;
	ESprawlHumanPresentation Presentation;
	float AuthoredHeightCm = 170.f;
};

/** Safe per-copy variation that preserves the authored face and rig. */
struct CONNECTEDSPRAWL_API FSprawlCrowdMetaHumanStyle
{
	FVector VisualScale = FVector::OneVector;
	FLinearColor PrimaryWardrobe = FLinearColor::White;
	FLinearColor SecondaryWardrobe = FLinearColor::Gray;
};

/**
 * Strict ambient-character gateway.
 *
 * Pedestrians may only render Blueprint classes in this roster.  Legacy
 * /Game/Pedestrians meshes remain available to old tooling, but are never a
 * runtime fallback here: a missing resident falls through to another assembled
 * MetaHuman, with Zarri as the final known-good visual.
 */
struct CONNECTEDSPRAWL_API FSprawlCrowdMetaHuman
{
	static const TArray<FSprawlCrowdMetaHumanEntry>& Roster();
	static const FSprawlCrowdMetaHumanEntry* FindEntry(FName AppearanceId);

	/** Stable server/client choice constrained by the requested presentation. */
	static FName ChooseAppearanceId(
		int32 Seed, ESprawlHumanPresentation Presentation);

	/** Balanced deterministic cycle used by the authority-owned crowd spawner. */
	static FName AppearanceIdForPopulationIndex(int32 PopulationIndex);

	/** Deterministic stature/build/wardrobe variation for copies of one face. */
	static FSprawlCrowdMetaHumanStyle BuildStyle(
		const FSprawlHumanCustomization& Customization,
		float AuthoredHeightCm);

	/**
	 * Spawn the preferred resident visual (or a real-human roster fallback),
	 * locate its Body mesh, disable collision/decals, and apply copy styling.
	 */
	static bool Activate(
		UChildActorComponent* VisualComponent,
		const FSprawlHumanCustomization& Customization,
		USkeletalMeshComponent*& OutBodyMesh,
		FName& OutResolvedAppearanceId);

	/** MetaHuman Creator Core Data clips shared by every roster body. */
	static bool LoadLocomotion(
		UAnimSequence*& OutIdle,
		UAnimSequence*& OutWalk,
		UAnimSequence*& OutRun);

	/** Set the complete assembled actor to a bounded crowd LOD. */
	static void SetHighDetail(
		UChildActorComponent* VisualComponent, bool bHighDetail);

	/** Full MetaHuman population budgets for the project's two target tiers. */
	static int32 PopulationCap(bool bIOS);
	static int32 HighDetailBudget(bool bIOS);

	/** Stable nearest-first helper, kept pure for automation coverage. */
	static TArray<int32> SelectHighDetailIndices(
		const TArray<float>& DistanceSquared, int32 Budget);
};
