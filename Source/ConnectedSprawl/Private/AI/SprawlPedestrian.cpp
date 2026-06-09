// The Connected Sprawl - Pedestrian NPC.

#include "AI/SprawlPedestrian.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/SprawlCar.h"
#include "World/SprawlCityGridSubsystem.h"

using Grid = USprawlCityGridSubsystem;

ASprawlPedestrian::ASprawlPedestrian()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate              = FRotator(0.f, 320.f, 0.f);
	Move->MaxWalkSpeed              = 150.f;
	Move->bUseRVOAvoidance          = true;  // don't clip through each other

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

FIntPoint ASprawlPedestrian::CornerSigns(int32 Index)
{
	switch ((Index % 4 + 4) % 4)
	{
	case 0:  return FIntPoint(+1, +1);
	case 1:  return FIntPoint(-1, +1);
	case 2:  return FIntPoint(-1, -1);
	default: return FIntPoint(+1, -1);
	}
}

FVector ASprawlPedestrian::CornerPoint(int32 Gx, int32 Gy, int32 Index) const
{
	const FIntPoint Signs = CornerSigns(Index);
	const float Reach = Grid::BlockSize * 0.5f - SidewalkInset;
	return FVector(
		Grid::BlockCenter(Gx) + Signs.X * Reach,
		Grid::BlockCenter(Gy) + Signs.Y * Reach,
		GetActorLocation().Z);
}

bool ASprawlPedestrian::IsWalkableBlock(int32 Gx, int32 Gy)
{
	if (Gx < 0 || Gx >= Grid::NumBlocks || Gy < 0 || Gy >= Grid::NumBlocks)
	{
		return false;
	}
	return !Grid::IsOverLake(Grid::BlockCenter(Gx), Grid::BlockCenter(Gy), 0.f);
}

void ASprawlPedestrian::AnchorToNearestCorner()
{
	const FVector Loc = GetActorLocation();
	BlockX = Grid::NearestBlockIndex(Loc.X);
	BlockY = Grid::NearestBlockIndex(Loc.Y);

	// If we somehow ended up over the lake, snap to the nearest dry block.
	if (!IsWalkableBlock(BlockX, BlockY))
	{
		float Best = TNumericLimits<float>::Max();
		for (int32 Gx = 0; Gx < Grid::NumBlocks; ++Gx)
		{
			for (int32 Gy = 0; Gy < Grid::NumBlocks; ++Gy)
			{
				if (!IsWalkableBlock(Gx, Gy)) continue;
				const float D = FVector2D::DistSquared(
					FVector2D(Loc.X, Loc.Y),
					FVector2D(Grid::BlockCenter(Gx), Grid::BlockCenter(Gy)));
				if (D < Best) { Best = D; BlockX = Gx; BlockY = Gy; }
			}
		}
	}

	float Best = TNumericLimits<float>::Max();
	for (int32 i = 0; i < 4; ++i)
	{
		const float D = FVector::DistSquared2D(Loc, CornerPoint(BlockX, BlockY, i));
		if (D < Best) { Best = D; CornerIndex = i; }
	}
	TargetPoint = CornerPoint(BlockX, BlockY, CornerIndex);
	State = EPedState::WalkEdge;
}

void ASprawlPedestrian::BeginPlay()
{
	Super::BeginPlay();

	BaseSpeed = FMath::FRandRange(MinWalkSpeed, MaxWalkSpeed);
	GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;

	// Subtle build variety so a crowd doesn't read as clones.
	const float Stature = FMath::FRandRange(0.93f, 1.04f);
	SetActorScale3D(FVector(Stature));

	WalkDir = FMath::RandBool() ? 1 : -1;
	AnchorToNearestCorner();
}

void ASprawlPedestrian::OnReachedCorner()
{
	// Sometimes cross the street here; otherwise continue around the block,
	// occasionally reversing for variety.
	if (FMath::FRand() < CrossChance && TryStartCrossing())
	{
		return;
	}

	if (FMath::FRand() < 0.12f)
	{
		WalkDir = -WalkDir;
	}
	CornerIndex = (CornerIndex + WalkDir + 4) % 4;
	TargetPoint = CornerPoint(BlockX, BlockY, CornerIndex);
	State = EPedState::WalkEdge;
}

bool ASprawlPedestrian::TryStartCrossing()
{
	const FIntPoint Signs = CornerSigns(CornerIndex);

	// From this corner we can cross along X or along Y; try both in a
	// random order and take the first walkable neighbour.
	const bool bTryXFirst = FMath::RandBool();
	for (int32 Attempt = 0; Attempt < 2; ++Attempt)
	{
		const bool bAlongX = (Attempt == 0) ? bTryXFirst : !bTryXFirst;
		const int32 NewBx = BlockX + (bAlongX ? Signs.X : 0);
		const int32 NewBy = BlockY + (bAlongX ? 0 : Signs.Y);
		if (NewBx == BlockX && NewBy == BlockY) continue;
		if (!IsWalkableBlock(NewBx, NewBy)) continue;

		// The matching corner on the far side of the road.
		int32 FarCorner = CornerIndex;
		for (int32 i = 0; i < 4; ++i)
		{
			const FIntPoint S = CornerSigns(i);
			const FIntPoint Want = bAlongX ? FIntPoint(-Signs.X, Signs.Y)
			                               : FIntPoint(Signs.X, -Signs.Y);
			if (S == Want) { FarCorner = i; break; }
		}

		BlockX = NewBx;
		BlockY = NewBy;
		CornerIndex = FarCorner;
		TargetPoint = CornerPoint(BlockX, BlockY, CornerIndex);
		State = EPedState::WaitToCross;
		CurbScanTimer = 0.f;
		return true;
	}
	return false;
}

bool ASprawlPedestrian::IsTrafficApproaching() const
{
	const FVector Loc = GetActorLocation();
	const FVector2D CrossMid(
		(Loc.X + TargetPoint.X) * 0.5f,
		(Loc.Y + TargetPoint.Y) * 0.5f);

	for (TActorIterator<ASprawlCar> It(GetWorld()); It; ++It)
	{
		const ASprawlCar* Car = *It;
		const FVector CarLoc = Car->GetActorLocation();
		const float Dist = FVector2D::Distance(FVector2D(CarLoc.X, CarLoc.Y), CrossMid);
		if (Dist > 1400.f)
		{
			continue;
		}
		const FVector Vel = Car->GetVelocity();
		if (Vel.Size2D() < 90.f)
		{
			continue; // parked or stopped at the light
		}
		// Moving roughly toward the crossing?
		const FVector ToMid(CrossMid.X - CarLoc.X, CrossMid.Y - CarLoc.Y, 0.f);
		if (FVector::DotProduct(Vel.GetSafeNormal2D(), ToMid.GetSafeNormal2D()) > 0.f)
		{
			return true;
		}
	}
	return false;
}

void ASprawlPedestrian::CheckForDanger()
{
	const FVector Loc = GetActorLocation();
	for (TActorIterator<ASprawlCar> It(GetWorld()); It; ++It)
	{
		const ASprawlCar* Car = *It;
		const FVector CarLoc = Car->GetActorLocation();
		if (FVector::Dist2D(Loc, CarLoc) > 420.f)
		{
			continue;
		}
		const FVector Vel = Car->GetVelocity();
		if (Vel.Size2D() < 480.f)
		{
			continue;
		}
		// Sprint away, biased perpendicular to the car's path.
		const FVector Away = (Loc - CarLoc).GetSafeNormal2D();
		const FVector Side = FVector::CrossProduct(Vel.GetSafeNormal2D(), FVector::UpVector);
		FleeDir = (Away * 0.6f + Side * (FVector::DotProduct(Away, Side) >= 0.f ? 0.4f : -0.4f))
			.GetSafeNormal2D();
		if (FleeDir.IsNearlyZero())
		{
			FleeDir = Away.IsNearlyZero() ? FVector::ForwardVector : Away;
		}
		State = EPedState::Flee;
		StateTimer = 2.2f;
		GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
		return;
	}
}

void ASprawlPedestrian::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Danger scan at ~5Hz keeps a big crowd cheap.
	DangerScanTimer -= DeltaSeconds;
	if (State != EPedState::Flee && DangerScanTimer <= 0.f)
	{
		DangerScanTimer = 0.2f;
		CheckForDanger();
	}

	switch (State)
	{
	case EPedState::Flee:
	{
		AddMovementInput(FleeDir, 1.f);
		StateTimer -= DeltaSeconds;
		if (StateTimer <= 0.f)
		{
			GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
			AnchorToNearestCorner();
		}
		return;
	}

	case EPedState::WaitToCross:
	{
		// Stand at the curb until the road is clear, then commit.
		CurbScanTimer -= DeltaSeconds;
		if (CurbScanTimer <= 0.f)
		{
			CurbScanTimer = 0.4f;
			if (!IsTrafficApproaching())
			{
				State = EPedState::Cross;
				// Cross briskly — nobody strolls across a live road.
				GetCharacterMovement()->MaxWalkSpeed = BaseSpeed * 1.45f;
			}
		}
		return;
	}

	case EPedState::Cross:
	case EPedState::WalkEdge:
	{
		const FVector Loc = GetActorLocation();
		FVector ToTarget = TargetPoint - Loc;
		ToTarget.Z = 0.f;

		if (ToTarget.SizeSquared() < FMath::Square(70.f))
		{
			if (State == EPedState::Cross)
			{
				GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
			}
			OnReachedCorner();
			return;
		}

		AddMovementInput(ToTarget.GetSafeNormal(), 1.f);

		// Wedged against something? Re-anchor and try a different path.
		if (GetVelocity().Size2D() < 14.f)
		{
			StuckTimer += DeltaSeconds;
			if (StuckTimer > 1.2f)
			{
				StuckTimer = 0.f;
				WalkDir = -WalkDir;
				GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
				AnchorToNearestCorner();
				OnReachedCorner();
			}
		}
		else
		{
			StuckTimer = 0.f;
		}
		return;
	}
	}
}
