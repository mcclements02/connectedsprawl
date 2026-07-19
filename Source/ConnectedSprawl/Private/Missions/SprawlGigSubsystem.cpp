// The Connected Sprawl - repeatable clean-income city gigs.

#include "Missions/SprawlGigSubsystem.h"

#include "Founder/FounderSubsystem.h"
#include "Founder/SprawlGameFlowSubsystem.h"
#include "Missions/SprawlGigBeacon.h"
#include "World/SprawlCityGridSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"

using Grid = USprawlCityGridSubsystem;

bool USprawlGigSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void USprawlGigSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (!IsRuntimeAudit())
	{
		ScheduleNextGig(FirstGigDelaySeconds);
	}
}

void USprawlGigSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GigTimer);
	}
	DestroyActiveBeacon();
	Super::Deinitialize();
}

void USprawlGigSubsystem::ScheduleNextGig(float DelaySeconds)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			GigTimer, this, &USprawlGigSubsystem::StartNextGig,
			FMath::Max(0.05f, DelaySeconds), false);
	}
}

void USprawlGigSubsystem::StartNextGig()
{
	if (ActiveGig.bActive || !GetWorld())
	{
		return;
	}

	UGameInstance* GI = GetWorld()->GetGameInstance();
	USprawlGameFlowSubsystem* Flow =
		GI ? GI->GetSubsystem<USprawlGameFlowSubsystem>() : nullptr;
	if (Flow && Flow->IsGameOver())
	{
		return;
	}

	FVector Target;
	if (!ChooseTarget(Target))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Gig] No dry sidewalk target available; retrying"));
		ScheduleNextGig(10.f);
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ActiveBeacon = GetWorld()->SpawnActor<ASprawlGigBeacon>(
		ASprawlGigBeacon::StaticClass(), Target, FRotator::ZeroRotator, Params);
	if (!ActiveBeacon)
	{
		ScheduleNextGig(10.f);
		return;
	}
	ActiveBeacon->Configure(this);

	FRandomStream Random(0x5A17 + CompletedGigCount * 7919);
	ActiveGig.bActive = true;
	ActiveGig.GigId = FName(*FString::Printf(TEXT("CityGig_%d"), CompletedGigCount + 1));
	ActiveGig.TargetLocation = Target;
	ActiveGig.Payout = Random.FRandRange(
		FMath::Min(MinimumPayout, MaximumPayout),
		FMath::Max(MinimumPayout, MaximumPayout));
	ActiveGig.Objective = FText::Format(
		NSLOCTEXT("Sprawl", "GigObjective", "Reach the glowing city marker · ${0}"),
		FText::AsNumber(FMath::RoundToInt(ActiveGig.Payout)));
	OnGigStarted.Broadcast(ActiveGig);
	UE_LOG(LogTemp, Log, TEXT("[Gig] Started %s payout=$%.0f target=(%.0f,%.0f)"),
		*ActiveGig.GigId.ToString(), ActiveGig.Payout, Target.X, Target.Y);
}

bool USprawlGigSubsystem::ChooseTarget(FVector& OutTarget) const
{
	UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	const FVector PlayerLocation = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
	FCollisionQueryParams QueryParams(FName(TEXT("SprawlGigTarget")), false, Pawn);
	if (Pawn)
	{
		QueryParams.AddIgnoredActor(Pawn);
	}
	// Radius must not exceed half-height: Chaos otherwise normalizes the shape
	// into ground contact and rejects every valid sidewalk point.
	const FCollisionShape Clearance = FCollisionShape::MakeCapsule(75.f, 120.f);
	int32 RejectedBounds = 0;
	int32 RejectedLake = 0;
	int32 RejectedRoad = 0;
	int32 RejectedNoGround = 0;
	int32 RejectedRoof = 0;
	int32 RejectedOverlap = 0;
	auto ResolveCandidate = [&](const FVector& Raw, FVector& Resolved) -> bool
	{
		if (!World || !Grid::IsInsideCityBounds(Raw.X, Raw.Y))
		{
			++RejectedBounds;
			return false;
		}
		if (Grid::IsOverLake(Raw.X, Raw.Y, 80.f))
		{
			++RejectedLake;
			return false;
		}
		if (Grid::IsOnRoadSurface(Raw.X, Raw.Y, -40.f))
		{
			++RejectedRoad;
			return false;
		}
		FHitResult GroundHit;
		const FVector TraceStart(Raw.X, Raw.Y, 1200.f);
		const FVector TraceEnd(Raw.X, Raw.Y, -400.f);
		if (!World->LineTraceSingleByChannel(
			GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
		{
			++RejectedNoGround;
			return false;
		}
		if (FMath::Abs(GroundHit.ImpactPoint.Z) > 120.f)
		{
			++RejectedRoof;
			return false; // reject rooftops, missing ground, and hidden geometry
		}
		const FVector CapsuleCenter(
			Raw.X, Raw.Y, GroundHit.ImpactPoint.Z + 145.f);
		if (World->OverlapBlockingTestByChannel(
			CapsuleCenter, FQuat::Identity, ECC_Pawn, Clearance, QueryParams))
		{
			++RejectedOverlap;
			return false;
		}
		Resolved = FVector(Raw.X, Raw.Y, GroundHit.ImpactPoint.Z + 15.f);
		return true;
	};

	float BestDistanceSq = -1.f;
	FVector Best = FVector::ZeroVector;
	// Building footprints vary by block. Probe the pedestrian ring plus two
	// progressively curb-side sidewalk rings instead of assuming one inset is
	// clear everywhere.
	constexpr int32 CandidateRingCount = 3;
	constexpr int32 CornersPerBlock = 4;
	const int32 PointsPerBlock = CandidateRingCount * CornersPerBlock;
	const int32 TotalCandidates = Grid::NumBlocks * Grid::NumBlocks * PointsPerBlock;
	const int32 Offset = (CompletedGigCount * 37 + 11) % TotalCandidates;
	for (int32 Visit = 0; Visit < TotalCandidates; ++Visit)
	{
		const int32 Flat = (Offset + Visit * 53) % TotalCandidates;
		const int32 PointInBlock = Flat % PointsPerBlock;
		const int32 Corner = PointInBlock % CornersPerBlock;
		const int32 Ring = PointInBlock / CornersPerBlock;
		const int32 BlockFlat = Flat / PointsPerBlock;
		const int32 Gx = BlockFlat % Grid::NumBlocks;
		const int32 Gy = BlockFlat / Grid::NumBlocks;
		const float CenterX = Grid::BlockCenter(Gx);
		const float CenterY = Grid::BlockCenter(Gy);
		if (Grid::IsOverLake(CenterX, CenterY, 0.f))
		{
			continue;
		}

		const float SignX = (Corner == 0 || Corner == 3) ? 1.f : -1.f;
		const float SignY = (Corner == 0 || Corner == 1) ? 1.f : -1.f;
		const float SidewalkInset = Ring == 0 ? 360.f : (Ring == 1 ? 200.f : 100.f);
		const float Reach = Grid::BlockSize * 0.5f - SidewalkInset;
		const FVector RawCandidate(
			CenterX + SignX * Reach, CenterY + SignY * Reach, 15.f);
		FVector Candidate;
		if (!ResolveCandidate(RawCandidate, Candidate))
		{
			continue;
		}
		const float DistanceSq = FVector::DistSquared2D(PlayerLocation, Candidate);
		if (DistanceSq > BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			Best = Candidate;
		}
		if (DistanceSq >= FMath::Square(MinimumTargetDistance))
		{
			OutTarget = Candidate;
			return true;
		}
	}

	if (BestDistanceSq >= 0.f)
	{
		OutTarget = Best;
		return true;
	}
	UE_LOG(LogTemp, Warning,
		TEXT("[Gig] Target search rejected bounds=%d lake=%d road=%d no_ground=%d roof=%d overlap=%d"),
		RejectedBounds, RejectedLake, RejectedRoad, RejectedNoGround,
		RejectedRoof, RejectedOverlap);
	return false;
}

bool USprawlGigSubsystem::CompleteActiveGig(ASprawlGigBeacon* SourceBeacon)
{
	if (!ActiveGig.bActive || SourceBeacon != ActiveBeacon)
	{
		return false;
	}

	UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	USprawlGameFlowSubsystem* Flow =
		GI ? GI->GetSubsystem<USprawlGameFlowSubsystem>() : nullptr;
	if (Flow && Flow->IsGameOver())
	{
		DestroyActiveBeacon();
		ActiveGig = FSprawlGigStatus();
		return false;
	}

	const FSprawlGigStatus Completed = ActiveGig;
	DestroyActiveBeacon();
	ActiveGig = FSprawlGigStatus();
	++CompletedGigCount;
	if (Flow)
	{
		// Victory can fire synchronously from AddIncome, so update run stats first.
		Flow->NotifyGigCompleted();
	}

	if (UFounderSubsystem* Founder = GI ? GI->GetSubsystem<UFounderSubsystem>() : nullptr)
	{
		Founder->AddIncome(Completed.Payout, ECashFlowSource::CleanContract,
			FText::FromString(TEXT("Completed city gig")), false);
	}
	OnGigCompleted.Broadcast(Completed);
	ScheduleNextGig(GigCooldownSeconds);
	UE_LOG(LogTemp, Log, TEXT("[Gig] Completed %s +$%.0f"),
		*Completed.GigId.ToString(), Completed.Payout);
	return true;
}

bool USprawlGigSubsystem::ForceStartGigForAudit()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GigTimer);
	}
	DestroyActiveBeacon();
	ActiveGig = FSprawlGigStatus();
	StartNextGig();
	return ActiveGig.bActive && ActiveBeacon != nullptr;
}

bool USprawlGigSubsystem::CompleteActiveGigForAudit()
{
	return CompleteActiveGig(ActiveBeacon);
}

void USprawlGigSubsystem::DestroyActiveBeacon()
{
	if (IsValid(ActiveBeacon))
	{
		ActiveBeacon->Destroy();
	}
	ActiveBeacon = nullptr;
}

bool USprawlGigSubsystem::IsRuntimeAudit() const
{
	return FParse::Param(FCommandLine::Get(), TEXT("SprawlTrafficAudit")) ||
		FParse::Param(FCommandLine::Get(), TEXT("SprawlCarjackAudit")) ||
		FParse::Param(FCommandLine::Get(), TEXT("SprawlProgressionAudit")) ||
		FParse::Param(FCommandLine::Get(), TEXT("SprawlVisualAudit"));
}
