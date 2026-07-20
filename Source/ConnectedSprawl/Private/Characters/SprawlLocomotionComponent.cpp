// The Connected Sprawl - Standalone on-foot locomotion visual.

#include "Characters/SprawlLocomotionComponent.h"

#include "Animation/AnimSequence.h"
#include "Characters/SprawlAvatarLibrary.h"
#include "Characters/SprawlCharacterRender.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Actor.h"

USprawlLocomotionComponent::USprawlLocomotionComponent()
{
	// The owning pawn drives updates from its own tick; nothing to do here.
	PrimaryComponentTick.bCanEverTick = false;
}

bool USprawlLocomotionComponent::ComputeMeshForwardYaw(
	const USkeletalMesh* Mesh, float& OutYawDegrees)
{
	// Shared with the crowd-casting module; both read the same skeleton.
	return FSprawlAvatarLibrary::ComputeMeshForwardYaw(Mesh, OutYawDegrees);
}

void USprawlLocomotionComponent::SetVisual(
	USkeletalMeshComponent* InVisualMesh, USceneComponent* InYawTarget)
{
	VisualMesh = InVisualMesh;
	YawTarget = InYawTarget ? InYawTarget : InVisualMesh;
	FSprawlCharacterRender::DisableDecalProjection(VisualMesh);
	CurrentClip = nullptr;
	AppliedYawCorrection = 0.f;
	MeshForwardYaw = 0.f;
	bMeshForwardYawValid = VisualMesh
		&& ComputeMeshForwardYaw(VisualMesh->GetSkeletalMeshAsset(), MeshForwardYaw);

	if (VisualMesh && !bMeshForwardYawValid)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Locomotion] %s: could not read a facing bone pair; leaving the "
				"authored rotation alone"),
			*VisualMesh->GetName());
	}
}

void USprawlLocomotionComponent::ClearVisual()
{
	VisualMesh = nullptr;
	YawTarget = nullptr;
	CurrentClip = nullptr;
	AppliedYawCorrection = 0.f;
	MeshForwardYaw = 0.f;
	bMeshForwardYawValid = false;
}

void USprawlLocomotionComponent::SetGaits(const TArray<FSprawlGait>& InGaits)
{
	Gaits = InGaits;
	Gaits.RemoveAll([](const FSprawlGait& Gait) { return Gait.Clip == nullptr; });
	Gaits.Sort([](const FSprawlGait& A, const FSprawlGait& B)
	{
		return A.MinSpeed > B.MinSpeed;
	});
	CurrentClip = nullptr;
}

void USprawlLocomotionComponent::SetStandardGaits(UAnimSequence* Idle,
	UAnimSequence* Walk, UAnimSequence* Jog, UAnimSequence* Sprint)
{
	TArray<FSprawlGait> NewGaits;
	if (Sprint)
	{
		NewGaits.Add(FSprawlGait(Sprint, 560.f, 610.f, 0.85f, 1.35f));
	}
	if (Jog)
	{
		// Without a dedicated sprint clip the jog covers the top band, so it
		// keeps stretching rather than capping at a jog cadence.
		NewGaits.Add(FSprawlGait(Jog, 320.f, 330.f, 0.9f, Sprint ? 1.6f : 1.8f));
	}
	if (Walk)
	{
		NewGaits.Add(FSprawlGait(Walk, 25.f, 130.f, 0.7f, 2.f));
	}
	if (Idle)
	{
		NewGaits.Add(FSprawlGait(Idle, 0.f, 0.f, 1.f, 1.f));
	}
	SetGaits(NewGaits);
}

void USprawlLocomotionComponent::UpdateLocomotion(float GroundSpeed)
{
	if (!VisualMesh || Gaits.IsEmpty())
	{
		return;
	}

	for (const FSprawlGait& Gait : Gaits)
	{
		if (GroundSpeed < Gait.MinSpeed)
		{
			continue;
		}
		const float PlayRate = Gait.ReferenceSpeed > 1.f
			? FMath::Clamp(GroundSpeed / Gait.ReferenceSpeed,
				Gait.MinPlayRate, Gait.MaxPlayRate)
			: 1.f;
		FSprawlAvatarLibrary::PlayLoop(VisualMesh, Gait.Clip, CurrentClip, PlayRate);
		return;
	}
}

bool USprawlLocomotionComponent::AlignVisualToOwnerForward()
{
	const AActor* Owner = GetOwner();
	if (!VisualMesh || !YawTarget || !Owner || !bMeshForwardYawValid)
	{
		return false;
	}

	// Where the body currently faces in the world = the mesh component's world
	// yaw plus the asset's own authored facing. Measuring the live transform
	// (rather than assuming a convention) means any rotation already baked into
	// a Blueprint or applied by the avatar library is respected, never doubled.
	const float VisualWorldYaw =
		VisualMesh->GetComponentRotation().Yaw + MeshForwardYaw;
	const float Correction = FMath::FindDeltaAngleDegrees(
		VisualWorldYaw, Owner->GetActorRotation().Yaw);
	if (FMath::IsNearlyZero(Correction, 0.05f))
	{
		AppliedYawCorrection = 0.f;
		return true;
	}

	FRotator Relative = YawTarget->GetRelativeRotation();
	Relative.Yaw += Correction;
	YawTarget->SetRelativeRotation(Relative);
	AppliedYawCorrection = Correction;

	UE_LOG(LogTemp, Display,
		TEXT("[Locomotion] Aligned %s by %.1f deg (mesh faces %.1f deg locally)"),
		*VisualMesh->GetName(), Correction, MeshForwardYaw);
	return true;
}

FVector USprawlLocomotionComponent::GetVisualForward() const
{
	if (!VisualMesh)
	{
		const AActor* Owner = GetOwner();
		return Owner ? Owner->GetActorForwardVector() : FVector::ForwardVector;
	}
	const float WorldYaw = VisualMesh->GetComponentRotation().Yaw
		+ (bMeshForwardYawValid ? MeshForwardYaw : 0.f);
	return FRotator(0.f, WorldYaw, 0.f).Vector();
}
