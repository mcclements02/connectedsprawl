// The Connected Sprawl - Reusable punch/kick gameplay module.

#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "SprawlMeleeModule.generated.h"

class AActor;

/** The two unarmed attacks exposed to input, Blueprint, and animation. */
UENUM(BlueprintType)
enum class ESprawlMeleeAttack : uint8
{
	Punch,
	Kick
};

/** Designer-tunable reach, cadence, damage, and reaction for one attack. */
USTRUCT(BlueprintType)
struct CONNECTEDSPRAWL_API FSprawlMeleeAttackSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee",
		meta=(ClampMin="0.0", ClampMax="100.0"))
	float Damage = 8.f;

	/** Forward travel of the targeting sweep, in centimetres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee",
		meta=(ClampMin="20.0", ClampMax="300.0"))
	float Range = 125.f;

	/** Radius around the forward sweep, giving hands and feet humane aim assist. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee",
		meta=(ClampMin="1.0", ClampMax="100.0"))
	float SweepRadius = 55.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee",
		meta=(ClampMin="5.0", ClampMax="90.0"))
	float HalfAngleDegrees = 55.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee",
		meta=(ClampMin="20.0", ClampMax="250.0"))
	float VerticalTolerance = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee",
		meta=(ClampMin="0.05", ClampMax="3.0"))
	float CooldownSeconds = 0.38f;

	/** Presentation window; optional one-shot animation returns to locomotion after this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee",
		meta=(ClampMin="0.05", ClampMax="2.0"))
	float RecoverySeconds = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee",
		meta=(ClampMin="0.0", ClampMax="600.0"))
	float KnockbackStrength = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee",
		meta=(ClampMin="0.0", ClampMax="300.0"))
	float KnockbackLift = 45.f;
};

/** Compact attack state replicated for remote presentation and future AI use. */
USTRUCT(BlueprintType)
struct CONNECTEDSPRAWL_API FSprawlMeleeRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melee")
	ESprawlMeleeAttack LastAttack = ESprawlMeleeAttack::Punch;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melee")
	bool bAttackActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melee")
	bool bHitCharacter = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Melee|Replication")
	int32 Revision = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FSprawlMeleeAttackStarted, ESprawlMeleeAttack, Attack,
	float, RecoverySeconds, bool, bHitCharacter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FSprawlMeleeAttackHit, ESprawlMeleeAttack, Attack, AActor*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FSprawlMeleeAttackFinished, ESprawlMeleeAttack, Attack);

/**
 * Authority-safe, asset-independent unarmed combat.
 *
 * The module owns gameplay only: a short forward sweep selects one visible
 * character, emits UE's standard point-damage event, and applies a restrained
 * reaction. Presentation stays modular through delegates, so Zarri's assembled
 * MetaHuman or lightweight fallback can play a compatible one-shot without
 * making missing animation content a runtime dependency.
 */
UCLASS(ClassGroup=(Sprawl), meta=(BlueprintSpawnableComponent))
class CONNECTEDSPRAWL_API USprawlMeleeModule : public UActorComponent
{
	GENERATED_BODY()

public:
	USprawlMeleeModule();

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Melee")
	bool TryPunch();

	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Melee")
	bool TryKick();

	/** One-button control: successful presses alternate punch then kick. */
	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Melee")
	bool TryAlternatingAttack();

	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Melee")
	bool TryAttack(ESprawlMeleeAttack Attack);

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Melee")
	bool CanAttack() const;

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Melee")
	float GetCooldownRemaining() const;

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Melee")
	ESprawlMeleeAttack GetNextAlternatingAttack() const
	{
		return NextAlternatingAttack;
	}

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Melee")
	FSprawlMeleeRuntimeState GetRuntimeState() const { return RuntimeState; }

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Melee")
	FSprawlMeleeAttackSpec GetAttackSpec(ESprawlMeleeAttack Attack) const;

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Melee")
	static FSprawlMeleeAttackSpec DefaultAttackSpec(ESprawlMeleeAttack Attack);

	/** Pure targeting predicate shared by runtime selection and automation. */
	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Melee")
	static bool IsTargetInsideAttackArc(
		const FVector& Origin, const FVector& Forward,
		const FVector& Target, const FSprawlMeleeAttackSpec& Spec);

	UFUNCTION(BlueprintCallable, Category="Connected Sprawl|Melee")
	static bool ValidateAttackSpec(
		const FSprawlMeleeAttackSpec& Spec, FString& OutError);

	UFUNCTION(BlueprintPure, Category="Connected Sprawl|Melee")
	static ESprawlMeleeAttack AlternateAttack(ESprawlMeleeAttack Attack);

	UPROPERTY(BlueprintAssignable, Category="Connected Sprawl|Melee")
	FSprawlMeleeAttackStarted OnAttackStarted;

	UPROPERTY(BlueprintAssignable, Category="Connected Sprawl|Melee")
	FSprawlMeleeAttackHit OnAttackHit;

	UPROPERTY(BlueprintAssignable, Category="Connected Sprawl|Melee")
	FSprawlMeleeAttackFinished OnAttackFinished;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee|Punch")
	FSprawlMeleeAttackSpec PunchSpec;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Melee|Kick")
	FSprawlMeleeAttackSpec KickSpec;

	UPROPERTY(ReplicatedUsing=OnRep_RuntimeState, VisibleInstanceOnly,
		BlueprintReadOnly, Category="Melee")
	FSprawlMeleeRuntimeState RuntimeState;

private:
	UFUNCTION(Server, Reliable)
	void ServerTryAttack(ESprawlMeleeAttack Attack);

	UFUNCTION()
	void OnRep_RuntimeState();

	bool PerformAuthorityAttack(ESprawlMeleeAttack Attack);
	bool CanStartAttackAtTime(float CurrentTime) const;
	bool IsOwnerReadyToAttack() const;
	AActor* FindAttackTarget(
		const FSprawlMeleeAttackSpec& Spec, FHitResult& OutHit) const;
	void ApplyAttackHit(ESprawlMeleeAttack Attack,
		const FSprawlMeleeAttackSpec& Spec, AActor* Target,
		const FHitResult& Hit) const;
	void CommitRuntimeState(FSprawlMeleeRuntimeState NewState);
	void FinishAttack();
	float CurrentWorldTime() const;

	ESprawlMeleeAttack NextAlternatingAttack = ESprawlMeleeAttack::Punch;
	float NextAttackAllowedTime = -BIG_NUMBER;
	float AttackEndsAtTime = -BIG_NUMBER;
	int32 LastNotifiedRevision = 0;
};
