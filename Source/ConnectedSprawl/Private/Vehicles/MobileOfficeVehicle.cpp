// The Connected Sprawl - Zarri's mobile office.

#include "Vehicles/MobileOfficeVehicle.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"

AMobileOfficeVehicle::AMobileOfficeVehicle()
{
	PrimaryActorTick.bCanEverTick = true;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(GetMesh());
	SpringArm->TargetArmLength = 560.f;
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 9.f;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraRotationLagSpeed = 10.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(SpringArm);
	FollowCamera->SetFieldOfView(84.f);

	RadioAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("RadioAudio"));
	RadioAudio->SetupAttachment(GetMesh());
	RadioAudio->bAutoActivate = false;
}

void AMobileOfficeVehicle::BeginPlay()
{
	Super::BeginPlay();
}

void AMobileOfficeVehicle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateHighwayFeel(DeltaSeconds);
}

bool AMobileOfficeVehicle::CanTakeCall() const
{
	// Don't trigger dialogue if the player is doing something intense (>100mph, in combat).
	return GetSpeedMph() < 100.f;
}

void AMobileOfficeVehicle::TakeCall(FName ContactId)
{
	UE_LOG(LogTemp, Log, TEXT("[MobileOffice] Incoming call from: %s"), *ContactId.ToString());
	// Actual dialogue/UI handled by PhoneSubsystem; this is the vehicle-side hook.
}

float AMobileOfficeVehicle::GetSpeedMph() const
{
	const FVector Velocity = GetVelocity();
	// cm/s -> mph : *0.0223694
	return Velocity.Size() * 0.0223694f;
}

void AMobileOfficeVehicle::UpdateHighwayFeel(float DeltaSeconds)
{
	if (!FollowCamera || !SpringArm) return;

	const float Speed = GetSpeedMph();

	// Ramp from 84 FoV -> 100 FoV as speed crosses 60->120mph ("Artery Mode").
	const float TargetFoV = FMath::GetMappedRangeValueClamped(
		FVector2D(60.f, 120.f), FVector2D(84.f, 100.f), Speed);
	const float NewFoV = FMath::FInterpTo(FollowCamera->FieldOfView, TargetFoV, DeltaSeconds, 3.f);
	FollowCamera->SetFieldOfView(NewFoV);

	// Pull camera back at speed for the "cinematic highway" feel.
	const float TargetArm = FMath::GetMappedRangeValueClamped(
		FVector2D(60.f, 120.f), FVector2D(560.f, 760.f), Speed);
	SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, TargetArm, DeltaSeconds, 2.f);
}
