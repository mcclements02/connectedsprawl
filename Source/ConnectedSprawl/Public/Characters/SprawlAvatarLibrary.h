// The Connected Sprawl - Shared helpers for the imported human avatars.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectPtr.h"

class USkeletalMesh;
class USkeletalMeshComponent;
class UAnimSequence;

/**
 * FSprawlAvatarLibrary
 * --------------------
 * The art pipeline (Tools/retarget_avatars.py + Content/Python/import_artwork.py)
 * produces one folder per character under /Game/Pedestrians/<Name>/ containing
 * a skeletal mesh SK_<Name> and AnimSequences <Name>_Idle/_Walk/_Jog/_Talk/
 * _WalkFormal. These helpers load them at runtime and fit the mesh onto a
 * character capsule. Every load is fallback-safe: if the art import hasn't run
 * yet, callers keep whatever mesh they already had (the UE4 mannequin).
 */
struct CONNECTEDSPRAWL_API FSprawlAvatarLibrary
{
	/** Names of the imported pedestrian avatar variants (sans Zarri's hero look). */
	static const TArray<FString>& PedestrianVariants();

	/** Variants that walk with the formal gait (suits). */
	static bool UsesFormalWalk(const FString& VariantName);

	static USkeletalMesh* LoadAvatarMesh(const FString& VariantName);
	static UAnimSequence* LoadAvatarAnim(const FString& VariantName, const TCHAR* AnimSuffix);

	/**
	 * Apply an avatar mesh to a character's mesh component in single-node
	 * animation mode: scaled to DesiredHeight, feet on the capsule bottom,
	 * facing the capsule's forward (+X). Returns false if Mesh is null.
	 */
	static bool ApplyAvatar(USkeletalMeshComponent* MeshComp, USkeletalMesh* Mesh,
		float DesiredHeight, float CapsuleHalfHeight);

	/** Swap the looping single-node animation if it changed; optional play rate. */
	static void PlayLoop(USkeletalMeshComponent* MeshComp, UAnimSequence* Anim,
		TObjectPtr<UAnimSequence>& CurrentAnim, float PlayRate = 1.f);
};
