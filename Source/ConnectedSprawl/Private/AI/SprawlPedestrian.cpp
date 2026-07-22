// The Connected Sprawl - Pedestrian NPC.

#include "AI/SprawlPedestrian.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Characters/SprawlCharacterDeveloper.h"
#include "Characters/SprawlCrowdMetaHuman.h"
#include "Characters/SprawlHumanCharacterModule.h"
#include "Characters/SprawlLocomotionComponent.h"
#include "Characters/SprawlWardrobeModule.h"
#include "Vehicles/SprawlCar.h"
#include "World/SprawlCityGridSubsystem.h"

using Grid = USprawlCityGridSubsystem;

ASprawlPedestrian::ASprawlPedestrian()
{
	PrimaryActorTick.bCanEverTick = true;
	HumanCharacter = CreateDefaultSubobject<USprawlHumanCharacterModule>(
		TEXT("HumanCharacter"));
	Wardrobe = CreateDefaultSubobject<USprawlWardrobeModule>(TEXT("Wardrobe"));
	Locomotion = CreateDefaultSubobject<USprawlLocomotionComponent>(
		TEXT("Locomotion"));
	MetaHumanVisualComponent = CreateDefaultSubobject<UChildActorComponent>(
		TEXT("MetaHumanVisual"));
	MetaHumanVisualComponent->SetupAttachment(RootComponent);
	MetaHumanVisualComponent->SetRelativeLocation(FVector(0.f, 0.f, -92.f));
	MetaHumanVisualComponent->SetChildActorOwnerOnCreation(true);

	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate              = FRotator(0.f, 320.f, 0.f);
	Move->MaxWalkSpeed              = 150.f;
	Move->bUseRVOAvoidance          = true;  // don't clip through each other

	// Keep ACharacter's inherited mesh hidden as a debug attachment only. No
	// mannequin or legacy pedestrian asset is loaded for an ambient citizen.
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetRelativeLocationAndRotation(
			FVector(0.f, 0.f, -89.f), FRotator(0.f, -90.f, 0.f));
		MeshComp->SetHiddenInGame(true, true);
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
	if (!bHasDevelopedProfile
		&& (GetNetMode() == NM_Standalone || HasAuthority()))
	{
		const int32 Seed = static_cast<int32>(HashCombineFast(
			GetTypeHash(GetName()), GetTypeHash(FMath::RoundToInt(GetActorLocation().X))));
		FSprawlCharacterProfile GeneratedProfile =
			USprawlCharacterDeveloper::DevelopCharacter(
				Seed, GetActorLocation(),
				USprawlCharacterDeveloper::ResolveCityHour(GetWorld()));
		if (!ForcedVariant.IsEmpty())
		{
			GeneratedProfile.PreferredAvatarVariant = ForcedVariant;
		}
		SetDevelopedProfile(GeneratedProfile);
	}
	if (!ForcedVariant.IsEmpty() && HumanCharacter
		&& HumanCharacter->IsConfigured()
		&& (GetNetMode() == NM_Standalone || HasAuthority()))
	{
		FSprawlHumanCustomization DriverCustomization =
			HumanCharacter->GetRuntimeState().Customization;
		DriverCustomization.AppearanceId =
			FSprawlCrowdMetaHuman::ChooseAppearanceId(
				static_cast<int32>(GetTypeHash(ForcedVariant)),
				DriverCustomization.Presentation);
		FString CustomizationError;
		if (!HumanCharacter->SetCustomization(
			DriverCustomization, CustomizationError))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[CrowdMetaHuman] driver identity rejected: %s"),
				*CustomizationError);
		}
	}
	// Subscribe only after deferred/profile customization is complete. This
	// avoids constructing an intermediate MetaHuman when an ejected driver's
	// stable variant immediately changes its requested roster identity.
	if (HumanCharacter)
	{
		HumanCharacter->OnRuntimeStateChanged.AddDynamic(
			this, &ASprawlPedestrian::HandleHumanRuntimeStateChanged);
	}

	const float SpeedScale = bHasDevelopedProfile
		? CharacterProfile.WalkSpeedScale : 1.f;
	FRandomStream MovementStream(bHasDevelopedProfile
		? CharacterProfile.Seed ^ 0x4D4F5645 : FMath::Rand());
	BaseSpeed = MovementStream.FRandRange(MinWalkSpeed, MaxWalkSpeed) * SpeedScale;
	GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;

	if (bHasDevelopedProfile)
	{
		DesiredHeight = CharacterProfile.HeightCm;
		CrossChance = CharacterProfile.CrossChance;
	}
	WalkDir = MovementStream.RandRange(0, 1) == 1 ? 1 : -1;
	if (HumanCharacter && HumanCharacter->IsConfigured())
	{
		InitializeAppearance();
	}
	AnchorToNearestCorner();
}

void ASprawlPedestrian::SetDevelopedProfile(
	const FSprawlCharacterProfile& InProfile)
{
	FString Error;
	if (!USprawlCharacterDeveloper::ValidateProfile(InProfile, Error))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[CharacterDeveloper] rejected pedestrian profile: %s"), *Error);
		return;
	}
	CharacterProfile = InProfile;
	bHasDevelopedProfile = true;
	if (HumanCharacter)
	{
		HumanCharacter->ConfigureFromProfile(CharacterProfile);
	}
}

bool ASprawlPedestrian::SetHumanAction(
	ESprawlHumanAction Action, bool bHoldAction)
{
	if (!HumanCharacter || !HumanCharacter->SetAction(Action, bHoldAction))
	{
		return false;
	}
	if (bHoldAction && (Action == ESprawlHumanAction::Stand
		|| Action == ESprawlHumanAction::Talk
		|| Action == ESprawlHumanAction::Sit
		|| Action == ESprawlHumanAction::Drive))
	{
		GetCharacterMovement()->StopMovementImmediately();
	}
	return true;
}

void ASprawlPedestrian::ResumeCityActivity()
{
	if (HumanCharacter)
	{
		HumanCharacter->ClearHeldAction();
		HumanCharacter->SetAction(ESprawlHumanAction::Stand, false);
	}
	GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
	AnchorToNearestCorner();
}

void ASprawlPedestrian::InitializeAppearance()
{
	if (!HumanCharacter || !HumanCharacter->IsConfigured())
	{
		return;
	}
	const FSprawlHumanCustomization& Customization =
		HumanCharacter->GetRuntimeState().Customization;
	// Semantic Talk is deterministic and authority-authored even on a dedicated
	// server; only the expensive assembled visual is skipped there.
	const float TalkChance = bHasDevelopedProfile
		? CharacterProfile.IdleTalkChance : 0.25f;
	FRandomStream PoseStream(Customization.Seed ^ 0x17A1C710);
	bIdleTalkPose = PoseStream.FRand() < TalkChance;
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (!MetaHumanVisualComponent || !Locomotion)
	{
		return;
	}
	if (bUsingMetaHumanVisual
		&& bHasActiveVisualCustomization
		&& ActiveVisualCustomization == Customization)
	{
		return;
	}

	UAnimSequence* LoadedIdle = nullptr;
	UAnimSequence* LoadedWalk = nullptr;
	UAnimSequence* LoadedRun = nullptr;
	if (!FSprawlCrowdMetaHuman::LoadLocomotion(
		LoadedIdle, LoadedWalk, LoadedRun))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[CrowdMetaHuman] locomotion is unavailable for %s"),
			*GetName());
		return;
	}

	USkeletalMeshComponent* LoadedBody = nullptr;
	FName ResolvedAppearanceId;
	if (Wardrobe)
	{
		Wardrobe->ClearWardrobe();
	}
	if (!FSprawlCrowdMetaHuman::Activate(
		MetaHumanVisualComponent, Customization,
		LoadedBody, ResolvedAppearanceId))
	{
		Locomotion->ClearVisual();
		bUsingMetaHumanVisual = false;
		bHasActiveVisualCustomization = false;
		UE_LOG(LogTemp, Error,
			TEXT("[CrowdMetaHuman] no assembled real-human visual could be activated "
				"for %s (requested=%s)"),
			*GetName(), *Customization.AppearanceId.ToString());
		return;
	}

	MetaHumanBodyMesh = LoadedBody;
	IdleAnim = LoadedIdle;
	WalkAnim = LoadedWalk;
	JogAnim = LoadedRun;
	Locomotion->SetVisual(MetaHumanBodyMesh, MetaHumanVisualComponent);
	Locomotion->SetGaits({
		FSprawlGait(JogAnim, 315.f, 520.f, 0.8f, 1.55f),
		FSprawlGait(WalkAnim, 25.f, 150.f, 0.7f, 1.75f),
		FSprawlGait(IdleAnim, 0.f, 0.f, 1.f, 1.f),
	});
	Locomotion->UpdateLocomotion(0.f);
	const UAnimSingleNodeInstance* SingleNode =
		MetaHumanBodyMesh->GetSingleNodeInstance();
	if (!SingleNode || SingleNode->GetAnimationAsset() != IdleAnim)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[CrowdMetaHuman] %s rejected MetaHuman locomotion; hiding resident"),
			*GetName());
		MetaHumanVisualComponent->SetChildActorClass(nullptr);
		if (Wardrobe)
		{
			Wardrobe->ClearWardrobe();
		}
		Locomotion->ClearVisual();
		MetaHumanBodyMesh = nullptr;
		bUsingMetaHumanVisual = false;
		bHasActiveVisualCustomization = false;
		return;
	}
	Locomotion->AlignVisualToOwnerForward();
	if (Wardrobe)
	{
		AActor* VisualActor = MetaHumanVisualComponent->GetChildActor();
		if (!Wardrobe->ApplyToMetaHuman(
			VisualActor, MetaHumanBodyMesh, Customization.Outfit))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[Wardrobe] %s kept its fitted base garment; an accessory layer was unavailable"),
				*GetName());
		}
	}

	RequestedAppearanceId = Customization.AppearanceId;
	ActiveAppearanceId = ResolvedAppearanceId;
	ActiveVisualCustomization = Customization;
	bHasActiveVisualCustomization = true;
	ActiveVariant = ForcedVariant.IsEmpty()
		? ResolvedAppearanceId.ToString() : ForcedVariant;

	bUsingMetaHumanVisual = true;
	if (USkeletalMeshComponent* DebugMesh = GetMesh())
	{
		DebugMesh->SetHiddenInGame(true, true);
	}
	UE_LOG(LogTemp, Display,
		TEXT("[CrowdMetaHuman] %s activated %s (requested=%s seed=%d)"),
		*GetName(), *ResolvedAppearanceId.ToString(),
		*Customization.AppearanceId.ToString(), Customization.Seed);
}

void ASprawlPedestrian::UpdateAnimation()
{
	const float Speed = GetVelocity().Size2D();
	const bool bTalkingAtRest = bIdleTalkPose
		&& State != EPedState::Flee && Speed < 25.f;
	const bool bCanAuthorAction =
		GetNetMode() == NM_Standalone || HasAuthority();
	ESprawlHumanAction Action = USprawlHumanCharacterModule::ResolveAction(
		Speed, bTalkingAtRest, false, false, State == EPedState::Flee);
	if (HumanCharacter)
	{
		Action = bCanAuthorAction
			? HumanCharacter->UpdateActionFromMovement(
				Speed, bTalkingAtRest, false, false, State == EPedState::Flee)
			: HumanCharacter->GetRuntimeState().Action;
	}

	if (!bUsingMetaHumanVisual || !Locomotion)
	{
		return;
	}
	// Talk/sit/drive are retained in replicated gameplay state.  Without a
	// compatible optional clip, stopping movement naturally resolves to the
	// neutral stand loop instead of ever falling back to a legacy avatar.
	if (Action == ESprawlHumanAction::Talk
		|| Action == ESprawlHumanAction::Sit
		|| Action == ESprawlHumanAction::Drive)
	{
		Locomotion->ClearActionAnimation();
	}
	Locomotion->UpdateLocomotion(Speed);
}

void ASprawlPedestrian::HandleHumanRuntimeStateChanged(
	FSprawlHumanRuntimeState RuntimeState)
{
	if (RuntimeState.CharacterId.IsNone())
	{
		return;
	}
	if (!bUsingMetaHumanVisual || !bHasActiveVisualCustomization
		|| ActiveVisualCustomization != RuntimeState.Customization)
	{
		InitializeAppearance();
	}
}

void ASprawlPedestrian::SetMetaHumanHighDetail(bool bHighDetail)
{
	FSprawlCrowdMetaHuman::SetHighDetail(
		MetaHumanVisualComponent, bHighDetail);
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

		// Keep the current block authoritative while waiting at the curb. A
		// danger response before the walk signal must flee inward, not toward
		// the destination block across live traffic.
		PendingBlockX = NewBx;
		PendingBlockY = NewBy;
		PendingCornerIndex = FarCorner;
		TargetPoint = CornerPoint(NewBx, NewBy, FarCorner);
		bCrossingAlongX = bAlongX;
		const FVector Loc = GetActorLocation();
		const FVector2D CrossMid(
			(Loc.X + TargetPoint.X) * 0.5f,
			(Loc.Y + TargetPoint.Y) * 0.5f);
		CrossingIntersectionX = Grid::NearestRoadIndex(CrossMid.X);
		CrossingIntersectionY = Grid::NearestRoadIndex(CrossMid.Y);
		State = EPedState::WaitToCross;
		CurbScanTimer = 0.f;
		StateTimer = 0.f;
		return true;
	}
	return false;
}

bool ASprawlPedestrian::HasPedestrianRightOfWay() const
{
	const USprawlCityGridSubsystem* GridSub = GetWorld()
		? GetWorld()->GetSubsystem<USprawlCityGridSubsystem>() : nullptr;
	if (!GridSub)
	{
		return false;
	}

	// Pedestrians move with the traffic axis parallel to their crossing. Requiring
	// that axis to be actively green avoids stepping off during amber/all-red and
	// leaves a complete, predictable window before conflicting traffic restarts.
	const bool bParallelNorthSouthTraffic = !bCrossingAlongX;
	if (GridSub->GetSignal(
		CrossingIntersectionX, CrossingIntersectionY,
		bParallelNorthSouthTraffic) != ESprawlSignal::Green)
	{
		return false;
	}

	const float CycleTime = GridSub->GetCycleTime(
		CrossingIntersectionX, CrossingIntersectionY);
	const float HalfCycle = Grid::SignalCycle * 0.5f;
	const float ServedTime = bParallelNorthSouthTraffic
		? CycleTime
		: CycleTime - HalfCycle;
	const float RemainingGreen = Grid::SignalGreenTime - ServedTime;
	const float CrossingSpeed = FMath::Max(230.f, BaseSpeed * 1.45f);
	const float RequiredTime = FVector::Dist2D(GetActorLocation(), TargetPoint)
		/ CrossingSpeed + 0.25f;
	return RemainingGreen >= RequiredTime;
}

void ASprawlPedestrian::StartFleeFrom(
	const AActor* DangerActor, float DurationSeconds)
{
	if (HumanCharacter)
	{
		HumanCharacter->ClearHeldAction();
	}
	const FVector DangerLocation = DangerActor
		? DangerActor->GetActorLocation()
		: GetActorLocation() - GetActorForwardVector();
	FleeDir = (GetActorLocation() - DangerLocation).GetSafeNormal2D();
	if (FleeDir.IsNearlyZero())
	{
		FleeDir = GetActorRightVector().GetSafeNormal2D();
	}
	State = EPedState::Flee;
	StateTimer = FMath::Max(0.25f, DurationSeconds);
	GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
	ContainFleeDirection();
}

void ASprawlPedestrian::ContainFleeDirection()
{
	const FVector Loc = GetActorLocation();
	const FVector Sample = Loc + FleeDir * 250.f;
	if (!Grid::IsOnRoadSurface(Sample.X, Sample.Y, -40.f))
	{
		return;
	}

	FVector TowardBlock(
		Grid::BlockCenter(BlockX) - Loc.X,
		Grid::BlockCenter(BlockY) - Loc.Y,
		0.f);
	TowardBlock = TowardBlock.GetSafeNormal2D();
	if (!TowardBlock.IsNearlyZero())
	{
		FleeDir = (FleeDir * 0.2f + TowardBlock * 0.8f).GetSafeNormal2D();
	}
}

void ASprawlPedestrian::EnforceSidewalkBoundary(float DeltaSeconds)
{
	if (State != EPedState::WalkEdge && State != EPedState::WaitToCross)
	{
		ContinuousOffRoadTime = 0.f;
		return;
	}

	const FVector Loc = GetActorLocation();
	if (!Grid::IsOnRoadSurface(Loc.X, Loc.Y, -40.f))
	{
		ContinuousOffRoadTime = 0.f;
		return;
	}

	ContinuousOffRoadTime += DeltaSeconds;
	if (ContinuousOffRoadTime <= 0.75f)
	{
		return;
	}

	AnchorToNearestCorner();
	SetActorLocation(TargetPoint, false, nullptr, ETeleportType::TeleportPhysics);
	GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
	ContinuousOffRoadTime = 0.f;
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
		if (HumanCharacter)
		{
			HumanCharacter->ClearHeldAction();
		}
		State = EPedState::Flee;
		StateTimer = 2.2f;
		GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
		ContainFleeDirection();
		return;
	}
}

void ASprawlPedestrian::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateAnimation();
	if (GetNetMode() != NM_Standalone && !HasAuthority())
	{
		return;
	}

	// Danger scan at ~5Hz keeps a big crowd cheap.
	DangerScanTimer -= DeltaSeconds;
	if (State != EPedState::Flee && DangerScanTimer <= 0.f)
	{
		DangerScanTimer = 0.2f;
		CheckForDanger();
	}

	if (HumanCharacter && HumanCharacter->IsActionHeld()
		&& State != EPedState::Flee)
	{
		const ESprawlHumanAction Action =
			HumanCharacter->GetRuntimeState().Action;
		if (Action == ESprawlHumanAction::Stand
			|| Action == ESprawlHumanAction::Talk
			|| Action == ESprawlHumanAction::Sit
			|| Action == ESprawlHumanAction::Drive)
		{
			GetCharacterMovement()->StopMovementImmediately();
			return;
		}
	}

	EnforceSidewalkBoundary(DeltaSeconds);

	switch (State)
	{
	case EPedState::Flee:
	{
		ContainFleeDirection();
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
		// Stand at the curb until the conflicting traffic has a red signal and
		// the physical approach is clear, then commit without stopping mid-road.
		StateTimer += DeltaSeconds;
		if (StateTimer >= Grid::SignalCycle + 2.f)
		{
			GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
			AnchorToNearestCorner();
			return;
		}
		CurbScanTimer -= DeltaSeconds;
		if (CurbScanTimer <= 0.f)
		{
			CurbScanTimer = 0.25f;
			if (HasPedestrianRightOfWay() && !IsTrafficApproaching())
			{
				BlockX = PendingBlockX;
				BlockY = PendingBlockY;
				CornerIndex = PendingCornerIndex;
				State = EPedState::Cross;
				// Cross briskly — nobody strolls across a live road.
				GetCharacterMovement()->MaxWalkSpeed = FMath::Max(230.f, BaseSpeed * 1.45f);
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
