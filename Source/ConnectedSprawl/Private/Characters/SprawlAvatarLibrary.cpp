// The Connected Sprawl - Shared helpers for the imported human avatars.

#include "Characters/SprawlAvatarLibrary.h"
#include "Characters/SprawlCharacterRender.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "ReferenceSkeleton.h"

namespace
{
/** Bone pairs that straddle the body's centreline, best-first. */
struct FSprawlBonePair
{
	const TCHAR* Left;
	const TCHAR* Right;
};

const FSprawlBonePair SprawlSideBonePairs[] = {
	// Hips first: legs are always planted in a reference pose, whereas arms
	// are sometimes authored in an A-pose that skews a shoulder-based read.
	{ TEXT("thigh_l"), TEXT("thigh_r") },
	{ TEXT("leftupleg"), TEXT("rightupleg") },
	{ TEXT("upperleg_l"), TEXT("upperleg_r") },
	{ TEXT("leg_l"), TEXT("leg_r") },
	{ TEXT("calf_l"), TEXT("calf_r") },
	{ TEXT("clavicle_l"), TEXT("clavicle_r") },
	{ TEXT("leftshoulder"), TEXT("rightshoulder") },
	{ TEXT("upperarm_l"), TEXT("upperarm_r") },
	{ TEXT("leftarm"), TEXT("rightarm") },
};

/** Bone name without any rig prefix ("mixamorig:LeftUpLeg" -> "leftupleg"). */
FString NormalizedBoneName(const FName BoneName)
{
	FString Name = BoneName.ToString().ToLower();
	int32 ColonIndex = INDEX_NONE;
	if (Name.FindLastChar(TEXT(':'), ColonIndex))
	{
		Name.MidInline(ColonIndex + 1);
	}
	return Name;
}

/** Exact match wins over a substring so "head" beats "head_end". */
int32 FindBoneIndex(const FReferenceSkeleton& RefSkeleton, const TCHAR* Token)
{
	const FString Needle(Token);
	int32 Fallback = INDEX_NONE;
	for (int32 BoneIndex = 0; BoneIndex < RefSkeleton.GetNum(); ++BoneIndex)
	{
		const FString Name = NormalizedBoneName(RefSkeleton.GetBoneName(BoneIndex));
		if (Name == Needle)
		{
			return BoneIndex;
		}
		if (Fallback == INDEX_NONE && Name.Contains(Needle))
		{
			Fallback = BoneIndex;
		}
	}
	return Fallback;
}

/** Reference-pose transform of a bone in component space. */
FTransform RefPoseComponentSpace(const FReferenceSkeleton& RefSkeleton, int32 BoneIndex)
{
	const TArray<FTransform>& BonePose = RefSkeleton.GetRefBonePose();
	FTransform Result = FTransform::Identity;
	for (int32 Index = BoneIndex; Index != INDEX_NONE;
		Index = RefSkeleton.GetParentIndex(Index))
	{
		if (!BonePose.IsValidIndex(Index))
		{
			return FTransform::Identity;
		}
		Result = Result * BonePose[Index];
	}
	return Result;
}
}

const TArray<FString>& FSprawlAvatarLibrary::PedestrianVariants()
{
	// Must match the folders created by Content/Python/import_artwork.py.
	// Zarri is intentionally omitted so the hero remains unique on the street.
	static const TArray<FString> Variants = {
		TEXT("Baldman"), TEXT("BizDude"), TEXT("Cappy"), TEXT("Chill"), TEXT("Erika"),
		TEXT("Jennifer"), TEXT("Jimmy"), TEXT("Kate"), TEXT("Kyle"),
		TEXT("Lydia"), TEXT("Mafiossini"), TEXT("OldMoustache"),
		TEXT("Olivia"), TEXT("Rose"), TEXT("Samuela"), TEXT("Shiro"),
		TEXT("Bruno"), TEXT("CursedAmy"), TEXT("Eugenia"),
		TEXT("Jenny"), TEXT("Juanita"),
	};
	return Variants;
}

bool FSprawlAvatarLibrary::UsesFormalWalk(const FString& VariantName)
{
	return VariantName == TEXT("BizDude") || VariantName == TEXT("Mafiossini");
}

USkeletalMesh* FSprawlAvatarLibrary::LoadAvatarMesh(const FString& VariantName)
{
	const FString Path = FString::Printf(
		TEXT("/Game/Pedestrians/%s/SK_%s.SK_%s"), *VariantName, *VariantName, *VariantName);
	return LoadObject<USkeletalMesh>(nullptr, *Path);
}

UAnimSequence* FSprawlAvatarLibrary::LoadAvatarAnim(const FString& VariantName, const TCHAR* AnimSuffix)
{
	const FString Path = FString::Printf(
		TEXT("/Game/Pedestrians/%s/%s_%s.%s_%s"),
		*VariantName, *VariantName, AnimSuffix, *VariantName, AnimSuffix);
	return LoadObject<UAnimSequence>(nullptr, *Path);
}

bool FSprawlAvatarLibrary::ApplyAvatar(USkeletalMeshComponent* MeshComp, USkeletalMesh* Mesh,
	float DesiredHeight, float CapsuleHalfHeight)
{
	if (!MeshComp || !Mesh)
	{
		return false;
	}

	MeshComp->SetAnimInstanceClass(nullptr);
	MeshComp->SetSkeletalMeshAsset(Mesh);
	MeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	// Road paint and grime are decals; they must not smear onto people.
	FSprawlCharacterRender::DisableDecalProjection(MeshComp);

	// The avatars stand ~1.2m in source units; size them to the capsule.
	const FBoxSphereBounds Bounds = Mesh->GetImportedBounds();
	const float MeshHeight = FMath::Max(1.f, Bounds.BoxExtent.Z * 2.f);
	const float Scale = DesiredHeight / MeshHeight;
	MeshComp->SetRelativeScale3D(FVector(Scale));

	// Feet (mesh origin) on the capsule bottom; Mixamo-style imports face +Y,
	// so yaw them -90 onto the pawn's +X forward — same as the old mannequin.
	MeshComp->SetRelativeLocationAndRotation(
		FVector(0.f, 0.f, -CapsuleHalfHeight), FRotator(0.f, -90.f, 0.f));
	return true;
}

bool FSprawlAvatarLibrary::ComputeMeshForwardYaw(
	const USkeletalMesh* Mesh, float& OutYawDegrees)
{
	if (!Mesh)
	{
		return false;
	}
	const FReferenceSkeleton& RefSkeleton = Mesh->GetRefSkeleton();
	if (RefSkeleton.GetNum() <= 0)
	{
		return false;
	}

	for (const FSprawlBonePair& Pair : SprawlSideBonePairs)
	{
		const int32 LeftIndex = FindBoneIndex(RefSkeleton, Pair.Left);
		const int32 RightIndex = FindBoneIndex(RefSkeleton, Pair.Right);
		if (LeftIndex == INDEX_NONE || RightIndex == INDEX_NONE
			|| LeftIndex == RightIndex)
		{
			continue;
		}

		FVector RightDirection =
			RefPoseComponentSpace(RefSkeleton, RightIndex).GetLocation()
			- RefPoseComponentSpace(RefSkeleton, LeftIndex).GetLocation();
		RightDirection.Z = 0.f;
		if (!RightDirection.Normalize())
		{
			continue;
		}

		// UE is left-handed: forward = right x up.
		const FVector Forward =
			FVector::CrossProduct(RightDirection, FVector::UpVector);
		if (Forward.IsNearlyZero())
		{
			continue;
		}
		OutYawDegrees = Forward.Rotation().Yaw;
		return true;
	}
	return false;
}

bool FSprawlAvatarLibrary::ComputeHeadHeightRatio(
	const USkeletalMesh* Mesh, float& OutRatio)
{
	if (!Mesh)
	{
		return false;
	}
	const FReferenceSkeleton& RefSkeleton = Mesh->GetRefSkeleton();
	if (RefSkeleton.GetNum() <= 0)
	{
		return false;
	}
	const int32 HeadIndex = FindBoneIndex(RefSkeleton, TEXT("head"));
	if (HeadIndex == INDEX_NONE)
	{
		return false;
	}

	const FBoxSphereBounds Bounds = Mesh->GetImportedBounds();
	const float Height = Bounds.BoxExtent.Z * 2.f;
	if (Height <= 1.f)
	{
		return false;
	}
	// Skull base to the crown, over standing height.
	const float HeadBoneZ =
		RefPoseComponentSpace(RefSkeleton, HeadIndex).GetLocation().Z;
	const float CrownZ = Bounds.Origin.Z + Bounds.BoxExtent.Z;
	OutRatio = (CrownZ - HeadBoneZ) / Height;
	return true;
}

void FSprawlAvatarLibrary::PlayLoop(USkeletalMeshComponent* MeshComp, UAnimSequence* Anim,
	TObjectPtr<UAnimSequence>& CurrentAnim, float PlayRate)
{
	if (!MeshComp || !Anim)
	{
		return;
	}
	if (Anim != CurrentAnim)
	{
		MeshComp->PlayAnimation(Anim, /*bLooping=*/true);
		CurrentAnim = Anim;
	}
	if (UAnimSingleNodeInstance* Node = MeshComp->GetSingleNodeInstance())
	{
		Node->SetPlayRate(PlayRate);
	}
}
