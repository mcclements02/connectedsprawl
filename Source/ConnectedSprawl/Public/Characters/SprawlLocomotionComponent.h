// The Connected Sprawl - Standalone on-foot locomotion visual.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SprawlLocomotionComponent.generated.h"

class UAnimSequence;
class USceneComponent;
class USkeletalMesh;
class USkeletalMeshComponent;

/**
 * One gait band. Bands are evaluated fastest-first: the first band whose
 * MinSpeed the pawn has reached wins. ReferenceSpeed is the ground speed the
 * clip was authored for, so the play rate is Speed/ReferenceSpeed clamped to
 * [MinPlayRate, MaxPlayRate]. ReferenceSpeed 0 means "always play at 1x".
 */
USTRUCT(BlueprintType)
struct FSprawlGait
{
	GENERATED_BODY()

	FSprawlGait() = default;
	FSprawlGait(UAnimSequence* InClip, float InMinSpeed, float InReferenceSpeed,
		float InMinPlayRate, float InMaxPlayRate)
		: Clip(InClip)
		, MinSpeed(InMinSpeed)
		, ReferenceSpeed(InReferenceSpeed)
		, MinPlayRate(InMinPlayRate)
		, MaxPlayRate(InMaxPlayRate)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gait")
	TObjectPtr<UAnimSequence> Clip = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gait")
	float MinSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gait")
	float ReferenceSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gait")
	float MinPlayRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gait")
	float MaxPlayRate = 1.f;
};

/**
 * USprawlLocomotionComponent
 * --------------------------
 * Self-contained on-foot locomotion visual, independent of which body is
 * driving it (imported Mixamo-style avatar, assembled MetaHuman, or engine
 * mannequin). It owns two jobs:
 *
 *   1. Facing. Different pipelines author their characters down different
 *      local axes — Mixamo faces +Y, so the old code hard-coded a -90 yaw,
 *      and anything that did not share that convention walked sideways. This
 *      component reads the mesh's own reference skeleton (hip/shoulder pair)
 *      to find where the body actually faces, then rotates the visual so it
 *      matches the pawn's forward. No per-asset magic numbers.
 *
 *   2. Gaits. A fastest-first band list picks the clip and scales its play
 *      rate to ground speed, so feet keep pace instead of skating.
 *
 * Owning pawns feed it a ground speed each tick and nothing else.
 */
UCLASS(ClassGroup=(Sprawl), meta=(BlueprintSpawnableComponent))
class CONNECTEDSPRAWL_API USprawlLocomotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USprawlLocomotionComponent();

	/**
	 * Point the component at a visual.
	 * @param InVisualMesh  Skeletal mesh that plays the clips.
	 * @param InYawTarget   Component rotated to correct facing; pass the mesh
	 *                      itself, or an outer component (e.g. a child actor)
	 *                      when the mesh is owned by another actor.
	 */
	UFUNCTION(BlueprintCallable, Category="Locomotion")
	void SetVisual(USkeletalMeshComponent* InVisualMesh, USceneComponent* InYawTarget);

	/** Replace the gait bands; sorted fastest-first internally. */
	UFUNCTION(BlueprintCallable, Category="Locomotion")
	void SetGaits(const TArray<FSprawlGait>& InGaits);

	/** Convenience for the common idle/walk/jog/sprint set; null clips are skipped. */
	void SetStandardGaits(UAnimSequence* Idle, UAnimSequence* Walk,
		UAnimSequence* Jog, UAnimSequence* Sprint);

	/** Pick and play the band matching this ground speed (cm/s). */
	UFUNCTION(BlueprintCallable, Category="Locomotion")
	void UpdateLocomotion(float GroundSpeed);

	/** Rotate the yaw target so the body faces the owner's forward. */
	UFUNCTION(BlueprintCallable, Category="Locomotion")
	bool AlignVisualToOwnerForward();

	/** World direction the body is visually facing right now. */
	UFUNCTION(BlueprintPure, Category="Locomotion")
	FVector GetVisualForward() const;

	/** Yaw (deg) this component applied to the target; 0 when already aligned. */
	UFUNCTION(BlueprintPure, Category="Locomotion")
	float GetAppliedYawCorrection() const { return AppliedYawCorrection; }

	UFUNCTION(BlueprintPure, Category="Locomotion")
	bool HasVisual() const { return VisualMesh != nullptr; }

	/** Forget the current visual (e.g. before swapping bodies). */
	UFUNCTION(BlueprintCallable, Category="Locomotion")
	void ClearVisual();

	/**
	 * Yaw (deg) the mesh's authored forward sits at in its own local space:
	 * ~90 for a Mixamo/MetaHuman body that faces +Y, ~0 for one facing +X.
	 * Derived from the reference skeleton's left/right bone pair.
	 */
	static bool ComputeMeshForwardYaw(const USkeletalMesh* Mesh, float& OutYawDegrees);

protected:
	UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> VisualMesh;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> YawTarget;
	UPROPERTY(Transient) TObjectPtr<UAnimSequence> CurrentClip;
	UPROPERTY(EditAnywhere, Category="Locomotion") TArray<FSprawlGait> Gaits;

	/** Where the mesh asset faces in its own space; folded into GetVisualForward. */
	float MeshForwardYaw = 0.f;
	bool bMeshForwardYawValid = false;
	float AppliedYawCorrection = 0.f;
};
