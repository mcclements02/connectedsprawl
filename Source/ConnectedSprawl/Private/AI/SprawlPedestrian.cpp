// The Connected Sprawl - Pedestrian NPC.

#include "AI/SprawlPedestrian.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"
#include "UObject/ConstructorHelpers.h"

ASprawlPedestrian::ASprawlPedestrian()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate              = FRotator(0.f, 300.f, 0.f);
	Move->MaxWalkSpeed              = 150.f;          // a walking pace
	Move->bUseRVOAvoidance         = true;           // don't clip through each other

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetRelativeLocationAndRotation(
			FVector(0.f, 0.f, -89.f), FRotator(0.f, -90.f, 0.f));

		static ConstructorHelpers::FObjectFinder<USkeletalMesh> PedMesh(
			TEXT("/Game/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin"));
		if (PedMesh.Succeeded())
		{
			MeshComp->SetSkeletalMeshAsset(PedMesh.Object);
		}

		static ConstructorHelpers::FClassFinder<UAnimInstance> PedAnim(
			TEXT("/Game/Mannequin/Animations/ThirdPerson_AnimBP"));
		if (PedAnim.Succeeded())
		{
			MeshComp->SetAnimInstanceClass(PedAnim.Class);
		}
	}

	// Self-driven — no controller / nav mesh needed.
	AutoPossessAI = EAutoPossessAI::Disabled;
}

void ASprawlPedestrian::PickNewDirection()
{
	const float Yaw = FMath::FRandRange(0.f, 360.f);
	WanderDir = FRotationMatrix(FRotator(0.f, Yaw, 0.f)).GetUnitAxis(EAxis::X);
}

void ASprawlPedestrian::BeginPlay()
{
	Super::BeginPlay();
	PickNewDirection();
	RepickTimer = FMath::FRandRange(1.5f, 6.0f);
}

void ASprawlPedestrian::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AddMovementInput(WanderDir, 1.f);

	RepickTimer -= DeltaSeconds;
	if (RepickTimer <= 0.f)
	{
		PickNewDirection();
		RepickTimer = FMath::FRandRange(3.0f, 7.0f);
	}

	// Bumped a wall / car? turn away.
	if (GetVelocity().Size2D() < 14.f)
	{
		StuckTimer += DeltaSeconds;
		if (StuckTimer > 0.35f)
		{
			PickNewDirection();
			StuckTimer = 0.f;
			RepickTimer = FMath::FRandRange(3.0f, 7.0f);
		}
	}
	else
	{
		StuckTimer = 0.f;
	}
}
