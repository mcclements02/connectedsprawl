// The Connected Sprawl - lightweight world marker for repeatable city gigs.

#include "Missions/SprawlGigBeacon.h"

#include "Missions/SprawlGigSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ASprawlGigBeacon::ASprawlGigBeacon()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	PillarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GigPillar"));
	PillarMesh->SetupAttachment(SceneRoot);
	PillarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PillarMesh->SetCastShadow(false);
	PillarMesh->SetRelativeLocation(FVector(0.f, 0.f, 600.f));
	PillarMesh->SetRelativeScale3D(FVector(0.22f, 0.22f, 12.f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		PillarMesh->SetStaticMesh(Cylinder.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> LampGlow(
		TEXT("/Game/CityArt/M_LampGlow.M_LampGlow"));
	if (LampGlow.Succeeded())
	{
		UMaterialInstanceDynamic* GlowMID =
			UMaterialInstanceDynamic::Create(LampGlow.Object, this);
		if (GlowMID)
		{
			GlowMID->SetVectorParameterValue(
				TEXT("GlowColor"), FLinearColor(0.08f, 2.8f, 5.5f, 1.f));
			PillarMesh->SetMaterial(0, GlowMID);
		}
	}

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("GigTrigger"));
	Trigger->SetupAttachment(SceneRoot);
	Trigger->SetSphereRadius(600.f);
	Trigger->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
	Trigger->OnComponentBeginOverlap.AddDynamic(
		this, &ASprawlGigBeacon::HandleTriggerOverlap);
}

void ASprawlGigBeacon::Configure(USprawlGigSubsystem* InOwner)
{
	OwnerSubsystem = InOwner;
}

void ASprawlGigBeacon::HandleTriggerOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	(void)OverlappedComponent;
	(void)OtherComponent;
	(void)OtherBodyIndex;
	(void)bFromSweep;
	(void)SweepResult;

	UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (OwnerSubsystem && PC && OtherActor == PC->GetPawn())
	{
		OwnerSubsystem->CompleteActiveGig(this);
	}
}
