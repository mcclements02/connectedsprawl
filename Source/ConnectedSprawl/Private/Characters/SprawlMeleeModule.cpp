// The Connected Sprawl - Reusable punch/kick gameplay module.

#include "Characters/SprawlMeleeModule.h"

#include "AI/SprawlPedestrian.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

USprawlMeleeModule::USprawlMeleeModule()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);

	PunchSpec = DefaultAttackSpec(ESprawlMeleeAttack::Punch);
	KickSpec = DefaultAttackSpec(ESprawlMeleeAttack::Kick);
}

void USprawlMeleeModule::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USprawlMeleeModule, RuntimeState);
}

FSprawlMeleeAttackSpec USprawlMeleeModule::DefaultAttackSpec(
	ESprawlMeleeAttack Attack)
{
	FSprawlMeleeAttackSpec Spec;
	if (Attack == ESprawlMeleeAttack::Kick)
	{
		Spec.Damage = 14.f;
		Spec.Range = 145.f;
		Spec.SweepRadius = 55.f;
		Spec.HalfAngleDegrees = 52.f;
		Spec.VerticalTolerance = 125.f;
		Spec.CooldownSeconds = 0.68f;
		Spec.RecoverySeconds = 0.46f;
		Spec.KnockbackStrength = 265.f;
		Spec.KnockbackLift = 90.f;
	}
	return Spec;
}

FSprawlMeleeAttackSpec USprawlMeleeModule::GetAttackSpec(
	ESprawlMeleeAttack Attack) const
{
	return Attack == ESprawlMeleeAttack::Kick ? KickSpec : PunchSpec;
}

bool USprawlMeleeModule::ValidateAttackSpec(
	const FSprawlMeleeAttackSpec& Spec, FString& OutError)
{
	auto Reject = [&OutError](const TCHAR* Message)
	{
		OutError = Message;
		return false;
	};
	if (!FMath::IsFinite(Spec.Damage) || Spec.Damage < 0.f || Spec.Damage > 100.f)
	{
		return Reject(TEXT("Damage must be finite and between 0 and 100"));
	}
	if (!FMath::IsFinite(Spec.Range) || Spec.Range < 20.f || Spec.Range > 300.f)
	{
		return Reject(TEXT("Range must be finite and between 20 and 300 cm"));
	}
	if (!FMath::IsFinite(Spec.SweepRadius)
		|| Spec.SweepRadius < 1.f || Spec.SweepRadius > 100.f)
	{
		return Reject(TEXT("Sweep radius must be finite and between 1 and 100 cm"));
	}
	if (!FMath::IsFinite(Spec.HalfAngleDegrees)
		|| Spec.HalfAngleDegrees < 5.f || Spec.HalfAngleDegrees > 90.f)
	{
		return Reject(TEXT("Half angle must be finite and between 5 and 90 degrees"));
	}
	if (!FMath::IsFinite(Spec.VerticalTolerance)
		|| Spec.VerticalTolerance < 20.f || Spec.VerticalTolerance > 250.f)
	{
		return Reject(TEXT("Vertical tolerance must be finite and between 20 and 250 cm"));
	}
	if (!FMath::IsFinite(Spec.CooldownSeconds)
		|| Spec.CooldownSeconds < 0.05f || Spec.CooldownSeconds > 3.f)
	{
		return Reject(TEXT("Cooldown must be finite and between 0.05 and 3 seconds"));
	}
	if (!FMath::IsFinite(Spec.RecoverySeconds)
		|| Spec.RecoverySeconds < 0.05f || Spec.RecoverySeconds > 2.f)
	{
		return Reject(TEXT("Recovery must be finite and between 0.05 and 2 seconds"));
	}
	if (!FMath::IsFinite(Spec.KnockbackStrength) || Spec.KnockbackStrength < 0.f
		|| Spec.KnockbackStrength > 600.f || !FMath::IsFinite(Spec.KnockbackLift)
		|| Spec.KnockbackLift < 0.f || Spec.KnockbackLift > 300.f)
	{
		return Reject(TEXT("Knockback must stay inside the supported mobile-safe range"));
	}
	OutError.Reset();
	return true;
}

bool USprawlMeleeModule::IsTargetInsideAttackArc(
	const FVector& Origin, const FVector& Forward,
	const FVector& Target, const FSprawlMeleeAttackSpec& Spec)
{
	FString Error;
	if (!ValidateAttackSpec(Spec, Error))
	{
		return false;
	}
	if (FMath::Abs(Target.Z - Origin.Z) > Spec.VerticalTolerance)
	{
		return false;
	}

	FVector2D ToTarget(Target.X - Origin.X, Target.Y - Origin.Y);
	const float Distance = ToTarget.Size();
	if (Distance > Spec.Range + Spec.SweepRadius)
	{
		return false;
	}
	if (Distance <= KINDA_SMALL_NUMBER)
	{
		return true;
	}
	ToTarget /= Distance;
	FVector2D Forward2D(Forward.X, Forward.Y);
	if (!Forward2D.Normalize())
	{
		return false;
	}
	const float MinimumDot = FMath::Cos(FMath::DegreesToRadians(
		Spec.HalfAngleDegrees));
	return FVector2D::DotProduct(Forward2D, ToTarget) >= MinimumDot;
}

ESprawlMeleeAttack USprawlMeleeModule::AlternateAttack(
	ESprawlMeleeAttack Attack)
{
	return Attack == ESprawlMeleeAttack::Punch
		? ESprawlMeleeAttack::Kick : ESprawlMeleeAttack::Punch;
}

float USprawlMeleeModule::CurrentWorldTime() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.f;
}

bool USprawlMeleeModule::IsOwnerReadyToAttack() const
{
	const AActor* Owner = GetOwner();
	return Owner && !Owner->IsActorBeingDestroyed() && !Owner->IsHidden()
		&& Owner->GetActorEnableCollision();
}

bool USprawlMeleeModule::CanStartAttackAtTime(float CurrentTime) const
{
	return IsOwnerReadyToAttack() && !RuntimeState.bAttackActive
		&& CurrentTime + KINDA_SMALL_NUMBER >= NextAttackAllowedTime;
}

bool USprawlMeleeModule::CanAttack() const
{
	return CanStartAttackAtTime(CurrentWorldTime());
}

float USprawlMeleeModule::GetCooldownRemaining() const
{
	return FMath::Max(0.f, NextAttackAllowedTime - CurrentWorldTime());
}

bool USprawlMeleeModule::TryPunch()
{
	return TryAttack(ESprawlMeleeAttack::Punch);
}

bool USprawlMeleeModule::TryKick()
{
	return TryAttack(ESprawlMeleeAttack::Kick);
}

bool USprawlMeleeModule::TryAlternatingAttack()
{
	return TryAttack(NextAlternatingAttack);
}

bool USprawlMeleeModule::TryAttack(ESprawlMeleeAttack Attack)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}
	const FSprawlMeleeAttackSpec Spec = GetAttackSpec(Attack);
	FString Error;
	if (!ValidateAttackSpec(Spec, Error))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Melee] %s rejected invalid attack: %s"),
			*Owner->GetName(), *Error);
		return false;
	}
	const float Now = CurrentWorldTime();
	if (!CanStartAttackAtTime(Now))
	{
		return false;
	}

	if (Owner->GetNetMode() != NM_Standalone && !Owner->HasAuthority())
	{
		// Predict only cadence/alternation; the server remains the sole hit and
		// damage authority, and its replicated state triggers presentation.
		NextAttackAllowedTime = Now + Spec.CooldownSeconds;
		NextAlternatingAttack = AlternateAttack(Attack);
		ServerTryAttack(Attack);
		return true;
	}
	return PerformAuthorityAttack(Attack);
}

void USprawlMeleeModule::ServerTryAttack_Implementation(
	ESprawlMeleeAttack Attack)
{
	PerformAuthorityAttack(Attack);
}

AActor* USprawlMeleeModule::FindAttackTarget(
	const FSprawlMeleeAttackSpec& Spec, FHitResult& OutHit) const
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return nullptr;
	}

	FVector Forward = Owner->GetActorForwardVector();
	Forward.Z = 0.f;
	if (!Forward.Normalize())
	{
		Forward = FVector::ForwardVector;
	}
	const FVector Origin = Owner->GetActorLocation();
	const FVector End = Origin + Forward * Spec.Range;

	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SprawlMeleeSweep), false, Owner);
	TArray<FHitResult> Hits;
	World->SweepMultiByObjectType(Hits, Origin, End, FQuat::Identity,
		ObjectQuery, FCollisionShape::MakeSphere(Spec.SweepRadius), QueryParams);

	AActor* BestTarget = nullptr;
	float BestDistanceSquared = BIG_NUMBER;
	TSet<AActor*> Considered;
	for (const FHitResult& Hit : Hits)
	{
		AActor* Candidate = Hit.GetActor();
		if (!Candidate || Candidate == Owner || Considered.Contains(Candidate)
			|| Candidate->IsActorBeingDestroyed() || Candidate->IsHidden()
			|| !Candidate->CanBeDamaged() || !Cast<ACharacter>(Candidate))
		{
			continue;
		}
		Considered.Add(Candidate);
		if (!IsTargetInsideAttackArc(
			Origin, Forward, Candidate->GetActorLocation(), Spec))
		{
			continue;
		}

		// The pawn-only sweep supplies aim assist; a separate visibility ray
		// keeps that assist from reaching through cars, walls, or street props.
		FCollisionQueryParams SightParams(
			SCENE_QUERY_STAT(SprawlMeleeSight), true, Owner);
		FHitResult SightHit;
		if (World->LineTraceSingleByChannel(SightHit, Origin,
			Candidate->GetActorLocation(), ECC_Visibility, SightParams)
			&& SightHit.GetActor() != Candidate)
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(
			Origin, Candidate->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestTarget = Candidate;
			OutHit = Hit;
		}
	}
	return BestTarget;
}

void USprawlMeleeModule::ApplyAttackHit(
	ESprawlMeleeAttack Attack, const FSprawlMeleeAttackSpec& Spec,
	AActor* Target, const FHitResult& Hit) const
{
	AActor* Owner = GetOwner();
	if (!Owner || !Target)
	{
		return;
	}
	FVector Forward = Owner->GetActorForwardVector();
	Forward.Z = 0.f;
	Forward = Forward.GetSafeNormal(SMALL_NUMBER, FVector::ForwardVector);
	const APawn* OwnerPawn = Cast<APawn>(Owner);
	UGameplayStatics::ApplyPointDamage(Target, Spec.Damage, Forward, Hit,
		OwnerPawn ? OwnerPawn->GetController() : nullptr, Owner,
		UDamageType::StaticClass());

	if (ASprawlPedestrian* Pedestrian = Cast<ASprawlPedestrian>(Target))
	{
		Pedestrian->StartFleeFrom(Owner, 4.f);
	}
	if (ACharacter* Character = Cast<ACharacter>(Target))
	{
		Character->LaunchCharacter(
			Forward * Spec.KnockbackStrength
				+ FVector::UpVector * Spec.KnockbackLift,
			false, false);
	}
	UE_LOG(LogTemp, Display, TEXT("[Melee] %s %s %s for %.0f damage"),
		*Owner->GetName(),
		Attack == ESprawlMeleeAttack::Punch ? TEXT("punched") : TEXT("kicked"),
		*Target->GetName(), Spec.Damage);
}

void USprawlMeleeModule::CommitRuntimeState(
	FSprawlMeleeRuntimeState NewState)
{
	NewState.Revision = RuntimeState.Revision >= MAX_int32
		? 1 : RuntimeState.Revision + 1;
	RuntimeState = MoveTemp(NewState);
	LastNotifiedRevision = RuntimeState.Revision;
	if (AActor* Owner = GetOwner())
	{
		Owner->ForceNetUpdate();
	}
}

bool USprawlMeleeModule::PerformAuthorityAttack(
	ESprawlMeleeAttack Attack)
{
	AActor* Owner = GetOwner();
	if (!Owner || (Owner->GetNetMode() != NM_Standalone && !Owner->HasAuthority()))
	{
		return false;
	}
	const FSprawlMeleeAttackSpec Spec = GetAttackSpec(Attack);
	FString Error;
	const float Now = CurrentWorldTime();
	if (!ValidateAttackSpec(Spec, Error) || !CanStartAttackAtTime(Now))
	{
		return false;
	}

	NextAttackAllowedTime = Now + Spec.CooldownSeconds;
	AttackEndsAtTime = Now + Spec.RecoverySeconds;
	NextAlternatingAttack = AlternateAttack(Attack);

	FHitResult Hit;
	AActor* Target = FindAttackTarget(Spec, Hit);
	if (Target)
	{
		ApplyAttackHit(Attack, Spec, Target, Hit);
	}

	FSprawlMeleeRuntimeState NewState = RuntimeState;
	NewState.LastAttack = Attack;
	NewState.bAttackActive = true;
	NewState.bHitCharacter = Target != nullptr;
	CommitRuntimeState(MoveTemp(NewState));
	SetComponentTickEnabled(true);
	OnAttackStarted.Broadcast(Attack, Spec.RecoverySeconds, Target != nullptr);
	if (Target)
	{
		OnAttackHit.Broadcast(Attack, Target);
	}
	return true;
}

void USprawlMeleeModule::FinishAttack()
{
	if (!RuntimeState.bAttackActive)
	{
		SetComponentTickEnabled(false);
		return;
	}
	const ESprawlMeleeAttack FinishedAttack = RuntimeState.LastAttack;
	FSprawlMeleeRuntimeState NewState = RuntimeState;
	NewState.bAttackActive = false;
	CommitRuntimeState(MoveTemp(NewState));
	SetComponentTickEnabled(false);
	OnAttackFinished.Broadcast(FinishedAttack);
}

void USprawlMeleeModule::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const AActor* Owner = GetOwner();
	if (RuntimeState.bAttackActive && Owner
		&& (Owner->GetNetMode() == NM_Standalone || Owner->HasAuthority())
		&& CurrentWorldTime() >= AttackEndsAtTime)
	{
		FinishAttack();
	}
}

void USprawlMeleeModule::OnRep_RuntimeState()
{
	if (RuntimeState.Revision == LastNotifiedRevision)
	{
		return;
	}
	LastNotifiedRevision = RuntimeState.Revision;
	if (RuntimeState.bAttackActive)
	{
		OnAttackStarted.Broadcast(RuntimeState.LastAttack,
			GetAttackSpec(RuntimeState.LastAttack).RecoverySeconds,
			RuntimeState.bHitCharacter);
	}
	else
	{
		OnAttackFinished.Broadcast(RuntimeState.LastAttack);
	}
}
