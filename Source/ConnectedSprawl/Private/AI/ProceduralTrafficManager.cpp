// The Connected Sprawl - Procedural traffic.

#include "AI/ProceduralTrafficManager.h"
#include "AI/SprawlPedestrian.h"
#include "AI/SprawlTrafficLaneDiscipline.h"
#include "AI/SprawlTrafficRoute.h"
#include "Characters/SprawlCrowdMetaHuman.h"
#include "Characters/ZarriCharacter.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Vehicles/SprawlCar.h"
#include "World/SprawlCityGridSubsystem.h"
#include "World/SprawlParkingGarage.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

DEFINE_LOG_CATEGORY_STATIC(LogSprawlTrafficAudit, Log, All);

namespace
{
constexpr float CarjackAuditTimeoutSeconds = 40.f;

bool IsTrafficSpawnClear(UWorld* World, const FVector& Location,
	const FQuat& Rotation, const AActor* IgnoredActor,
	AActor** OutBlockingActor = nullptr)
{
	if (OutBlockingActor)
	{
		*OutBlockingActor = nullptr;
	}
	if (!World)
	{
		return false;
	}
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	Objects.AddObjectTypesToQuery(ECC_PhysicsBody);
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	FCollisionQueryParams Params(FName(TEXT("SprawlTrafficSpawnClearance")), false);
	if (IgnoredActor)
	{
		Params.AddIgnoredActor(IgnoredActor);
	}
	TArray<FOverlapResult> Overlaps;
	const bool bBlocked = World->OverlapMultiByObjectType(
		Overlaps, Location, Rotation, Objects,
		FCollisionShape::MakeBox(FVector(330.f, 125.f, 90.f)), Params);
	if (bBlocked && OutBlockingActor)
	{
		for (const FOverlapResult& Overlap : Overlaps)
		{
			if (AActor* Actor = Overlap.GetActor())
			{
				*OutBlockingActor = Actor;
				break;
			}
		}
	}
	return !bBlocked;
}

bool IsGarageExitClear(UWorld* World, const FSprawlParkingGarageExit& Exit,
	const ASprawlParkingGarage* Garage)
{
	if (!World || !Garage || !Exit.IsValid())
	{
		return false;
	}
	AActor* Blocker = nullptr;
	auto ReportBlocker = [&](const TCHAR* Phase, int32 PointIndex,
		const FVector& Location)
	{
		static TSet<FString> Reported;
		const FString Key = FString::Printf(TEXT("%s:%s:%d"),
			*Exit.Id.ToString(), Phase, PointIndex);
		if (!Reported.Contains(Key))
		{
			Reported.Add(Key);
			UE_LOG(LogTemp, Warning,
				TEXT("[TrafficGarage] exit=%s blocked_at=%s[%d] location=(%.0f,%.0f) by=%s"),
				*Exit.Id.ToString(), Phase, PointIndex, Location.X, Location.Y,
				Blocker ? *Blocker->GetName() : TEXT("<component>"));
		}
	};
	if (!IsTrafficSpawnClear(World, Exit.SpawnTransform.GetLocation(),
		Exit.SpawnTransform.GetRotation(), Garage, &Blocker))
	{
		ReportBlocker(TEXT("spawn"), 0, Exit.SpawnTransform.GetLocation());
		return false;
	}

	FVector Previous = Exit.SpawnTransform.GetLocation();
	for (int32 PointIndex = 0; PointIndex < Exit.DeparturePath.Num(); ++PointIndex)
	{
		const FVector& Point = Exit.DeparturePath[PointIndex];
		FVector Direction = Point - Previous;
		Direction.Z = 0.f;
		const FQuat Rotation = Direction.IsNearlyZero()
			? Exit.SpawnTransform.GetRotation()
			: Direction.Rotation().Quaternion();
		Blocker = nullptr;
		if (!IsTrafficSpawnClear(World, Point, Rotation, Garage, &Blocker))
		{
			ReportBlocker(TEXT("path"), PointIndex, Point);
			return false;
		}
		Previous = Point;
	}
	return true;
}
}

// File-scope alias (matches the other Sprawl translation units so unity
// builds don't see a local declaration shadowing the global one).
using Grid = USprawlCityGridSubsystem;

namespace
{
bool TryParkRetiredCar(UWorld* World, ASprawlCar* Car)
{
	if (!World || !Car)
	{
		return false;
	}

	const FVector Current = Car->GetActorLocation();
	const ESprawlHeading Heading = Grid::HeadingFromYaw(Car->GetActorRotation().Yaw);
	const bool bNorthSouth = Grid::IsNorthSouth(Heading);
	const int32 RoadIndex = Grid::NearestRoadIndex(bNorthSouth ? Current.X : Current.Y);
	const int32 BlockIndex = Grid::NearestBlockIndex(bNorthSouth ? Current.Y : Current.X);
	const float BlockCenter = Grid::BlockCenter(BlockIndex);
	const float Along = FMath::Clamp(bNorthSouth ? Current.Y : Current.X,
		BlockCenter - 600.f, BlockCenter + 600.f);
	const float PreferredSide = (Heading == ESprawlHeading::North
		|| Heading == ESprawlHeading::West) ? -1.f : 1.f;

	TArray<TPair<FVector, ESprawlHeading>> Candidates;
	for (int32 SideAttempt = 0; SideAttempt < 2; ++SideAttempt)
	{
		const float Side = SideAttempt == 0 ? PreferredSide : -PreferredSide;
		FVector Candidate = Current;
		if (bNorthSouth)
		{
			Candidate.X = Grid::RoadCenter(RoadIndex) + Side * Grid::ParkingOffset;
			Candidate.Y = Along;
		}
		else
		{
			Candidate.X = Along;
			Candidate.Y = Grid::RoadCenter(RoadIndex) + Side * Grid::ParkingOffset;
		}
		Candidate.Z = 180.f; // overlap-test above the ground, then let physics settle
		Candidates.Emplace(Candidate,
			SideAttempt == 0 ? Heading
				: FSprawlTrafficRoute::OppositeHeading(Heading));
	}

	for (const TPair<FVector, ESprawlHeading>& Candidate : Candidates)
	{
		const FVector& Location = Candidate.Key;
		const FQuat Rotation = FRotator(
			0.f, Grid::HeadingYaw(Candidate.Value), 0.f).Quaternion();
		if (!Grid::IsInsideCityBounds(Location.X, Location.Y)
			|| Grid::IsOverLake(Location.X, Location.Y, 50.f)
			|| Grid::IsOnRoadSurface(Location.X, Location.Y, -40.f)
			|| !IsTrafficSpawnClear(World, Location, Rotation, Car))
		{
			continue;
		}

		Car->SetActorLocationAndRotation(Location, Rotation, false, nullptr,
			ETeleportType::TeleportPhysics);
		if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(
			Car->GetRootComponent()))
		{
			RootPrimitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
			RootPrimitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}
		return true;
	}
	return false;
}
}

AProceduralTrafficManager::AProceduralTrafficManager()
{
	PrimaryActorTick.bCanEverTick     = true;
	PrimaryActorTick.TickInterval     = 0.5f;
}

void AProceduralTrafficManager::BeginPlay()
{
	Super::BeginPlay();
	bTrafficAuditEnabled = FParse::Param(
		FCommandLine::Get(), TEXT("SprawlTrafficAudit"));
	bCarjackAuditEnabled = FParse::Param(
		FCommandLine::Get(), TEXT("SprawlCarjackAudit"));

	// Adopt authored traffic instead of blindly spawning another full fleet on
	// top of it. Only cars already marked for auto-drive belong to this pool.
	TArray<ASprawlCar*> BlockedTraffic;
	for (TActorIterator<ASprawlCar> It(GetWorld()); It; ++It)
	{
		if (It->bAutoDrive)
		{
			if (IsTrafficSpawnClear(GetWorld(), It->GetActorLocation(),
				It->GetActorQuat(), *It))
			{
				ActiveTraffic.Add(*It);
			}
			else
			{
				BlockedTraffic.Add(*It);
			}
		}
	}
	for (ASprawlCar* Car : BlockedTraffic)
	{
		Car->Destroy();
	}
}

void AProceduralTrafficManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bTrafficAuditEnabled && !bTrafficAuditComplete)
	{
		RunTrafficAudit(DeltaSeconds);
	}
	if (bCarjackAuditEnabled && !bCarjackAuditComplete)
	{
		RunCarjackAudit(DeltaSeconds);
	}
	TimeSinceEval += DeltaSeconds;
	if (TimeSinceEval < EvaluateInterval) return;
	TimeSinceEval = 0.f;
	Evaluate();
}

void AProceduralTrafficManager::RequestAuditExitIfComplete()
{
	const bool bTrafficDone = !bTrafficAuditEnabled || bTrafficAuditComplete;
	const bool bCarjackDone = !bCarjackAuditEnabled || bCarjackAuditComplete;
	if (!bTrafficDone || !bCarjackDone)
	{
		return;
	}
	const bool bPassed = (!bTrafficAuditEnabled || bTrafficAuditPassed)
		&& (!bCarjackAuditEnabled || bCarjackAuditPassed);
	FPlatformMisc::RequestExitWithStatus(
		true, bPassed ? 0 : 1, TEXT("SprawlTrafficRuntimeAudit"));
}

void AProceduralTrafficManager::RunTrafficAudit(float DeltaSeconds)
{
	TrafficAuditElapsed += DeltaSeconds;
	UWorld* World = GetWorld();
	const USprawlCityGridSubsystem* GridSub = World
		? World->GetSubsystem<USprawlCityGridSubsystem>() : nullptr;
	if (!World || !GridSub)
	{
		return;
	}

	TArray<ASprawlCar*> Cars;
	TMap<int32, int32> IntersectionOccupancy;
	TSet<TWeakObjectPtr<APawn>> CarsInIntersectionsThisSample;
	int32 TotalCarsThisSample = 0;
	int32 EnterableCarsThisSample = 0;
	int32 BoundaryViolationsThisSample = 0;
	int32 UprightViolationsThisSample = 0;
	int32 HiddenDriversThisSample = 0;
	int32 VisibleDriverViolationsThisSample = 0;
	int32 DriverReadyCarsThisSample = 0;
	int32 CompletedTurnsThisSample = 0;
	int32 CompletedUTurnsThisSample = 0;
	for (TActorIterator<ASprawlCar> It(World); It; ++It)
	{
		ASprawlCar* Car = *It;
		++TotalCarsThisSample;
		if (Car->CanBeEntered())
		{
			++EnterableCarsThisSample;
		}
		if (!Car->IsWithinCityBounds())
		{
			++BoundaryViolationsThisSample;
		}
		if (!Car->IsCrashRecoveryActive()
			&& FVector::DotProduct(Car->GetActorUpVector(), FVector::UpVector) < 0.9f)
		{
			++UprightViolationsThisSample;
		}
		if (!Car->bAutoDrive)
		{
			continue;
		}
		if (Car->HasAIDriver())
		{
			++DriverReadyCarsThisSample;
			if (Car->HasHiddenSeatedDriver())
			{
				++HiddenDriversThisSample;
			}
			if (Car->HasVisibleDriver())
			{
				++VisibleDriverViolationsThisSample;
			}
		}
		Cars.Add(Car);
		const TWeakObjectPtr<APawn> CarKey(Car);
		FVector& Start = TrafficAuditStarts.FindOrAdd(CarKey, Car->GetActorLocation());
		if (FVector::Dist2D(Start, Car->GetActorLocation()) > 300.f)
		{
			TrafficAuditMovedCars.Add(CarKey);
		}
		if (Car->HasAnimatedWheelParts()
			&& FMath::Abs(Car->GetVisualWheelRotationDegrees()) > 5.f)
		{
			TrafficAuditWheelMotionCars.Add(CarKey);
		}

		const FVector Loc = Car->GetActorLocation();
		ESprawlHeading Heading = Grid::HeadingFromYaw(Car->GetActorRotation().Yaw);
		int32 RoadIndex = Grid::NearestRoadIndex(
			Grid::IsNorthSouth(Heading) ? Loc.X : Loc.Y);
		const bool bHasPlannedLane = Car->GetAITravelState(Heading, RoadIndex);
		const bool bNorthSouth = Grid::IsNorthSouth(Heading);
		const FSprawlTrafficLaneSample LaneSample =
			FSprawlTrafficLaneDiscipline::Measure(
				Loc, Car->GetActorForwardVector(), Car->GetVelocity(),
				Heading, RoadIndex);
		const float NearestCrossingDistance =
			FSprawlTrafficLaneDiscipline::DistanceToNearestIntersection(
				Loc, Heading);
		const bool bOnStraightEnvelope = bHasPlannedLane
			&& !Car->HasActiveIntersectionReservation()
			&& NearestCrossingDistance > Grid::RoadWidth * 0.5f + 450.f;
		float& LaneViolationDuration = TrafficAuditLaneViolationDurations.FindOrAdd(CarKey);
		if (bOnStraightEnvelope && LaneSample.LaneError > 90.f)
		{
			LaneViolationDuration += DeltaSeconds;
			// Do not fail a legal turn for briefly straddling the straight-lane
			// envelope. Beyond nine metres from the junction, a real departure
			// remains outside it for multiple consecutive samples.
			if (LaneViolationDuration >= 1.5f && !TrafficAuditLaneViolators.Contains(CarKey))
			{
				TrafficAuditLaneViolators.Add(CarKey);
				UE_LOG(LogSprawlTrafficAudit, Warning,
					TEXT("[TrafficAudit] persistent lane departure car=%s location=(%.1f,%.1f) "
						"lane_error=%.1f crossing_distance=%.1f"),
					*Car->GetName(), Loc.X, Loc.Y,
					LaneSample.LaneError, NearestCrossingDistance);
			}
		}
		else
		{
			LaneViolationDuration = 0.f;
		}

		float& WrongWayDuration = TrafficAuditWrongWayDurations.FindOrAdd(CarKey);
		if (bOnStraightEnvelope && LaneSample.bWrongWay)
		{
			WrongWayDuration += DeltaSeconds;
			if (WrongWayDuration >= 1.f
				&& !TrafficAuditWrongWayViolators.Contains(CarKey))
			{
				TrafficAuditWrongWayViolators.Add(CarKey);
				UE_LOG(LogSprawlTrafficAudit, Warning,
					TEXT("[TrafficAudit] persistent wrong-way travel car=%s "
						"location=(%.1f,%.1f) velocity_alignment=%.2f planned_heading=%d"),
					*Car->GetName(), Loc.X, Loc.Y,
					LaneSample.VelocityAlignment, static_cast<int32>(Heading));
			}
		}
		else
		{
			WrongWayDuration = 0.f;
		}

		CompletedTurnsThisSample += Car->GetAICompletedTurnCount();
		CompletedUTurnsThisSample += Car->GetAICompletedUTurnCount();

		FSprawlTrafficApproach NextApproach;
		const bool bHasNextApproach = bHasPlannedLane
			&& FSprawlTrafficRoute::TryGetNextApproach(
				Loc, Heading, RoadIndex, NextApproach, 0.f);
		// Hold window derived from the grid rather than hard-coded, so this
		// gate follows the street layout instead of a past road width.
		constexpr float SignalHoldTolerance = 70.f;
		if (bHasNextApproach
			&& NextApproach.DistanceAhead
				>= Grid::VehicleStopDistance - SignalHoldTolerance
			&& NextApproach.DistanceAhead
				<= Grid::VehicleStopDistance + SignalHoldTolerance
			&& Car->GetVelocity().Size2D() < 80.f)
		{
			if (GridSub->GetSignal(NextApproach.IntersectionX,
				NextApproach.IntersectionY, bNorthSouth)
				!= ESprawlSignal::Green)
			{
				TrafficAuditSignalStops.Add(CarKey);
			}
		}

		const int32 IntersectionX = Grid::NearestRoadIndex(Loc.X);
		const int32 IntersectionY = Grid::NearestRoadIndex(Loc.Y);
		// Inside the junction box itself, sized from the carriageway.
		constexpr float IntersectionHalf = Grid::RoadWidth * 0.5f;
		if (FMath::Abs(Loc.X - Grid::RoadCenter(IntersectionX)) < IntersectionHalf
			&& FMath::Abs(Loc.Y - Grid::RoadCenter(IntersectionY)) < IntersectionHalf)
		{
			const int32 Key = IntersectionX * Grid::NumRoads + IntersectionY;
			++IntersectionOccupancy.FindOrAdd(Key);
			CarsInIntersectionsThisSample.Add(CarKey);
			if (!TrafficAuditCarsInIntersections.Contains(CarKey)
				&& !GridSub->HasIntersectionReservation(
					IntersectionX, IntersectionY, Car))
			{
				++TrafficAuditUnauthorizedEntries;
				UE_LOG(LogSprawlTrafficAudit, Warning,
					TEXT("[TrafficAudit] car entered intersection without lease car=%s "
						"intersection=(%d,%d)"),
					*Car->GetName(), IntersectionX, IntersectionY);
			}
		}
	}
	TrafficAuditCarsInIntersections = MoveTemp(CarsInIntersectionsThisSample);

	TrafficAuditPeakCars = FMath::Max(TrafficAuditPeakCars, Cars.Num());
	TrafficAuditPeakTotalCars = FMath::Max(TrafficAuditPeakTotalCars, TotalCarsThisSample);
	TrafficAuditMaxEnterableCars = FMath::Max(
		TrafficAuditMaxEnterableCars, EnterableCarsThisSample);
	TrafficAuditMaxBoundaryViolations = FMath::Max(
		TrafficAuditMaxBoundaryViolations, BoundaryViolationsThisSample);
	TrafficAuditMaxUprightViolations = FMath::Max(
		TrafficAuditMaxUprightViolations, UprightViolationsThisSample);
	TrafficAuditMaxHiddenDrivers = FMath::Max(
		TrafficAuditMaxHiddenDrivers, HiddenDriversThisSample);
	TrafficAuditMaxVisibleDriverViolations = FMath::Max(
		TrafficAuditMaxVisibleDriverViolations, VisibleDriverViolationsThisSample);
	TrafficAuditMaxCompletedTurns = FMath::Max(
		TrafficAuditMaxCompletedTurns, CompletedTurnsThisSample);
	TrafficAuditMaxCompletedUTurns = FMath::Max(
		TrafficAuditMaxCompletedUTurns, CompletedUTurnsThisSample);
	if (TrafficAuditElapsed >= 5.f)
	{
		TrafficAuditMaxMissingHiddenDriversAfterWarmup = FMath::Max(
			TrafficAuditMaxMissingHiddenDriversAfterWarmup,
			DriverReadyCarsThisSample - HiddenDriversThisSample);
	}
	for (const TPair<int32, int32>& Pair : IntersectionOccupancy)
	{
		TrafficAuditMaxIntersectionOccupancy = FMath::Max(
			TrafficAuditMaxIntersectionOccupancy, Pair.Value);
	}
	for (int32 First = 0; First < Cars.Num(); ++First)
	{
		for (int32 Second = First + 1; Second < Cars.Num(); ++Second)
		{
			TrafficAuditMinSpacing = FMath::Min(TrafficAuditMinSpacing,
				FVector::Dist2D(Cars[First]->GetActorLocation(), Cars[Second]->GetActorLocation()));
		}
	}

	int32 PedestrianCount = 0;
	int32 RealAvatarCount = 0;
	TSet<FName> ActiveMetaHumanIdentities;
	for (TActorIterator<ASprawlPedestrian> It(World); It; ++It)
	{
		ASprawlPedestrian* Pedestrian = *It;
		++PedestrianCount;
		if (Pedestrian->HasRealAvatar())
		{
			++RealAvatarCount;
			ActiveMetaHumanIdentities.Add(Pedestrian->GetAppearanceId());
		}
		const TWeakObjectPtr<ASprawlPedestrian> PedKey(Pedestrian);
		float& OffRoadDuration = TrafficAuditPedOffRoadDurations.FindOrAdd(PedKey);
		const FVector PedLocation = Pedestrian->GetActorLocation();
		if (!Pedestrian->IsCrossingRoad() && !Pedestrian->IsFleeing()
			&& Grid::IsOnRoadSurface(PedLocation.X, PedLocation.Y, -40.f))
		{
			OffRoadDuration += DeltaSeconds;
			if (OffRoadDuration > 2.f)
			{
				TrafficAuditPedOffsideViolators.Add(PedKey);
			}
		}
		else
		{
			OffRoadDuration = 0.f;
		}
	}
	TrafficAuditMaxPedestrians = FMath::Max(TrafficAuditMaxPedestrians, PedestrianCount);
	TrafficAuditMaxRealAvatars = FMath::Max(TrafficAuditMaxRealAvatars, RealAvatarCount);
	TrafficAuditMaxDistinctMetaHumans = FMath::Max(
		TrafficAuditMaxDistinctMetaHumans, ActiveMetaHumanIdentities.Num());
	if (TrafficAuditElapsed >= 5.f)
	{
		TrafficAuditMaxNonMetaHumansAfterWarmup = FMath::Max(
			TrafficAuditMaxNonMetaHumansAfterWarmup,
			PedestrianCount - RealAvatarCount);
	}

	if (TrafficAuditElapsed < 30.f)
	{
		return;
	}
	bTrafficAuditComplete = true;
	const int32 MetaHumanCap =
		FSprawlCrowdMetaHuman::PopulationCap(PLATFORM_IOS != 0);
	const int32 RequiredPedestrians = PLATFORM_IOS
		? FMath::Min(2, MetaHumanCap)
		: FMath::Max(1, MetaHumanCap - 2);
	const int32 RequiredDistinctMetaHumans =
		FMath::Min(3, RequiredPedestrians);
	const bool bPassed = TrafficAuditPeakCars >= 10
		&& TrafficAuditPeakTotalCars >= 45
		&& TrafficAuditMaxEnterableCars >= 30
		&& TrafficAuditMaxBoundaryViolations == 0
		&& TrafficAuditMaxUprightViolations == 0
		&& TrafficAuditMovedCars.Num() >= 5
		&& TrafficAuditSignalStops.Num() >= 1
		&& TrafficAuditWheelMotionCars.Num() >= 5
		&& TrafficAuditMaxIntersectionOccupancy <= 1
		&& TrafficAuditUnauthorizedEntries == 0
		&& TrafficAuditMinSpacing >= 220.f
		&& TrafficAuditLaneViolators.IsEmpty()
		&& TrafficAuditWrongWayViolators.IsEmpty()
		&& TrafficAuditMaxPedestrians >= RequiredPedestrians
		&& TrafficAuditMaxRealAvatars >= RequiredPedestrians
		&& TrafficAuditMaxDistinctMetaHumans >= RequiredDistinctMetaHumans
		&& TrafficAuditMaxNonMetaHumansAfterWarmup == 0
		&& TrafficAuditMaxHiddenDrivers >= 10
		&& TrafficAuditMaxVisibleDriverViolations == 0
		&& TrafficAuditMaxMissingHiddenDriversAfterWarmup == 0
		&& TrafficAuditPedOffsideViolators.IsEmpty();
	bTrafficAuditPassed = bPassed;
	const TCHAR* Result = bPassed ? TEXT("PASS") : TEXT("FAIL");
	if (bPassed)
	{
		UE_LOG(LogSprawlTrafficAudit, Display,
			TEXT("[TrafficAudit] %s cars=%d total_cars=%d enterable_cars=%d "
				"boundary_violators=%d upright_violators=%d moved=%d signal_stops=%d wheel_cars=%d "
				"max_box_occupancy=%d unauthorized_entries=%d min_spacing=%.1f "
				"lane_violators=%d wrong_way=%d turns=%d uturns=%d "
				"route_recycles=%d spawn_route_rejects=%d garage_spawns=%d garage_blocked=%d "
				"pedestrians=%d metahumans=%d distinct_metahumans=%d non_metahumans=%d "
				"hidden_drivers=%d "
				"visible_driver_violations=%d missing_hidden_drivers=%d ped_offside=%d"),
			Result, TrafficAuditPeakCars, TrafficAuditPeakTotalCars,
			TrafficAuditMaxEnterableCars, TrafficAuditMaxBoundaryViolations,
			TrafficAuditMaxUprightViolations, TrafficAuditMovedCars.Num(),
			TrafficAuditSignalStops.Num(), TrafficAuditWheelMotionCars.Num(),
			TrafficAuditMaxIntersectionOccupancy, TrafficAuditUnauthorizedEntries,
			TrafficAuditMinSpacing, TrafficAuditLaneViolators.Num(),
			TrafficAuditWrongWayViolators.Num(), TrafficAuditMaxCompletedTurns,
			TrafficAuditMaxCompletedUTurns, TrafficRouteRecycles,
			TrafficSpawnRouteRejects, TrafficGarageSpawnSuccesses,
			TrafficGarageBlockedExits,
			TrafficAuditMaxPedestrians, TrafficAuditMaxRealAvatars,
			TrafficAuditMaxDistinctMetaHumans,
			TrafficAuditMaxNonMetaHumansAfterWarmup,
			TrafficAuditMaxHiddenDrivers, TrafficAuditMaxVisibleDriverViolations,
			TrafficAuditMaxMissingHiddenDriversAfterWarmup,
			TrafficAuditPedOffsideViolators.Num());
	}
	else
	{
		UE_LOG(LogSprawlTrafficAudit, Error,
			TEXT("[TrafficAudit] %s cars=%d total_cars=%d enterable_cars=%d "
				"boundary_violators=%d upright_violators=%d moved=%d signal_stops=%d wheel_cars=%d "
				"max_box_occupancy=%d unauthorized_entries=%d min_spacing=%.1f "
				"lane_violators=%d wrong_way=%d turns=%d uturns=%d "
				"route_recycles=%d spawn_route_rejects=%d garage_spawns=%d garage_blocked=%d "
				"pedestrians=%d metahumans=%d distinct_metahumans=%d non_metahumans=%d "
				"hidden_drivers=%d "
				"visible_driver_violations=%d missing_hidden_drivers=%d ped_offside=%d"),
			Result, TrafficAuditPeakCars, TrafficAuditPeakTotalCars,
			TrafficAuditMaxEnterableCars, TrafficAuditMaxBoundaryViolations,
			TrafficAuditMaxUprightViolations, TrafficAuditMovedCars.Num(),
			TrafficAuditSignalStops.Num(), TrafficAuditWheelMotionCars.Num(),
			TrafficAuditMaxIntersectionOccupancy, TrafficAuditUnauthorizedEntries,
			TrafficAuditMinSpacing, TrafficAuditLaneViolators.Num(),
			TrafficAuditWrongWayViolators.Num(), TrafficAuditMaxCompletedTurns,
			TrafficAuditMaxCompletedUTurns, TrafficRouteRecycles,
			TrafficSpawnRouteRejects, TrafficGarageSpawnSuccesses,
			TrafficGarageBlockedExits,
			TrafficAuditMaxPedestrians, TrafficAuditMaxRealAvatars,
			TrafficAuditMaxDistinctMetaHumans,
			TrafficAuditMaxNonMetaHumansAfterWarmup,
			TrafficAuditMaxHiddenDrivers, TrafficAuditMaxVisibleDriverViolations,
			TrafficAuditMaxMissingHiddenDriversAfterWarmup,
			TrafficAuditPedOffsideViolators.Num());
	}
	RequestAuditExitIfComplete();
}

void AProceduralTrafficManager::RunCarjackAudit(float DeltaSeconds)
{
	CarjackAuditElapsed += DeltaSeconds;
	UWorld* World = GetWorld();
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!World || !PC)
	{
		return;
	}

	if (!bCarjackAuditTriggered)
	{
		if (CarjackAuditElapsed < 2.f)
		{
			return; // allow lazy logical driver state to initialize
		}

		AZarriCharacter* Zarri = Cast<AZarriCharacter>(PC->GetPawn());
		ASprawlCar* Candidate = nullptr;
		for (APawn* Pawn : ActiveTraffic)
		{
			ASprawlCar* Car = Cast<ASprawlCar>(Pawn);
			if (Car && Car->HasAIDriver() && Car->HasHiddenSeatedDriver()
				&& !Car->HasActiveIntersectionReservation())
			{
				Candidate = Car;
				break;
			}
		}

		if (!Zarri || !Candidate)
		{
			if (CarjackAuditElapsed >= CarjackAuditTimeoutSeconds)
			{
				bCarjackAuditComplete = true;
				UE_LOG(LogSprawlTrafficAudit, Error,
					TEXT("[CarjackAudit] FAIL setup zarri=%d occupied_car=%d"),
					Zarri != nullptr, Candidate != nullptr);
				RequestAuditExitIfComplete();
			}
			return;
		}

		if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(
			Candidate->GetRootComponent()))
		{
			RootPrimitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
			RootPrimitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		}
		FVector EntryPoint = Candidate->GetActorLocation()
			- Candidate->GetActorRightVector() * 190.f + FVector(0.f, 0.f, 100.f);
		if (!Candidate->FindClearSideExit(40.f, 92.f, EntryPoint, Zarri, true))
		{
			if (CarjackAuditElapsed >= CarjackAuditTimeoutSeconds)
			{
				bCarjackAuditComplete = true;
				UE_LOG(LogSprawlTrafficAudit, Error,
					TEXT("[CarjackAudit] FAIL no_clear_entry_point"));
				RequestAuditExitIfComplete();
			}
			return;
		}
		Zarri->SetActorLocation(EntryPoint, false, nullptr, ETeleportType::TeleportPhysics);

		CarjackAuditDriverVariant = Candidate->GetAIDriverVariant();
		CarjackAuditPedestrianCountBefore = 0;
		for (TActorIterator<ASprawlPedestrian> It(World); It; ++It)
		{
			++CarjackAuditPedestrianCountBefore;
		}

		if (!Candidate->CanBeCarjacked() || !Zarri->EnterVehicle(Candidate))
		{
			if (CarjackAuditElapsed >= CarjackAuditTimeoutSeconds)
			{
				bCarjackAuditComplete = true;
				UE_LOG(LogSprawlTrafficAudit, Error,
					TEXT("[CarjackAudit] FAIL takeover speed_or_entry_gate"));
				RequestAuditExitIfComplete();
			}
			return;
		}

		CarjackAuditCar = Candidate;
		CarjackAuditTriggerTime = CarjackAuditElapsed;
		bCarjackAuditTriggered = true;
		return;
	}

	if (CarjackAuditElapsed - CarjackAuditTriggerTime < 2.f)
	{
		return; // give Evaluate() time to retire/backfill the claimed car
	}

	ASprawlCar* ClaimedCar = CarjackAuditCar.Get();
	int32 PedestrianCountAfter = 0;
	for (TActorIterator<ASprawlPedestrian> It(World); It; ++It)
	{
		++PedestrianCountAfter;
	}
	ASprawlPedestrian* EjectedDriver = ClaimedCar
		? ClaimedCar->GetLastEjectedDriver() : nullptr;
	if (EjectedDriver && EjectedDriver->IsFleeing()
		&& EjectedDriver->GetActiveVariant() == CarjackAuditDriverVariant
		&& EjectedDriver->IsUsingMetaHumanVisual()
		&& !EjectedDriver->GetRequestedAppearanceId().IsNone()
		&& EjectedDriver->GetAppearanceId()
			== EjectedDriver->GetRequestedAppearanceId())
	{
		bCarjackAuditSawFleeingDriver = true;
	}
	const bool bFleeingDriverFound = bCarjackAuditSawFleeingDriver;

	const bool bPossessed = ClaimedCar && PC->GetPawn() == ClaimedCar;
	const bool bRetired = ClaimedCar && !ActiveTraffic.Contains(ClaimedCar)
		&& RetiredTraffic.Contains(ClaimedCar);
	const bool bLeaseFree = ClaimedCar
		&& !ClaimedCar->HasActiveIntersectionReservation();
	bool bGarageDeparturePending = false;
	for (const APawn* Pawn : ActiveTraffic)
	{
		if (const ASprawlCar* Car = Cast<ASprawlCar>(Pawn);
			Car && Car->IsGarageEgressActive())
		{
			bGarageDeparturePending = true;
			break;
		}
	}
	const bool bBackfilled = ActiveTraffic.Num() >= TargetActiveCount
		&& TrafficGarageSpawnSuccesses > 0 && !bGarageDeparturePending;
	const bool bPedestrianStable = PedestrianCountAfter
		>= CarjackAuditPedestrianCountBefore;
	const bool bPassed = bPossessed && bRetired && bLeaseFree && bBackfilled
		&& bFleeingDriverFound && bPedestrianStable
		&& ClaimedCar && !ClaimedCar->bAutoDrive && !ClaimedCar->HasAIDriver();

	if (!bPassed
		&& CarjackAuditElapsed - CarjackAuditTriggerTime
			< CarjackAuditTimeoutSeconds)
	{
		return;
	}

	bCarjackAuditComplete = true;
	bCarjackAuditPassed = bPassed;
	if (bPassed)
	{
		UE_LOG(LogSprawlTrafficAudit, Display,
			TEXT("[CarjackAudit] PASS possessed=%d fleeing_driver=%d lease_free=%d "
				"retired=%d backfilled=%d garage_spawns=%d garage_pending=%d "
				"active=%d target=%d peds_before=%d peds_after=%d"),
			bPossessed, bFleeingDriverFound, bLeaseFree, bRetired, bBackfilled,
			TrafficGarageSpawnSuccesses, bGarageDeparturePending,
			ActiveTraffic.Num(), TargetActiveCount,
			CarjackAuditPedestrianCountBefore, PedestrianCountAfter);
	}
	else
	{
		UE_LOG(LogSprawlTrafficAudit, Error,
			TEXT("[CarjackAudit] FAIL possessed=%d fleeing_driver=%d lease_free=%d "
				"retired=%d backfilled=%d garage_spawns=%d garage_pending=%d "
				"active=%d target=%d peds_before=%d peds_after=%d"),
			bPossessed, bFleeingDriverFound, bLeaseFree, bRetired, bBackfilled,
			TrafficGarageSpawnSuccesses, bGarageDeparturePending,
			ActiveTraffic.Num(), TargetActiveCount,
			CarjackAuditPedestrianCountBefore, PedestrianCountAfter);
	}
	RequestAuditExitIfComplete();
}

void AProceduralTrafficManager::Evaluate()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !PC->GetPawn()) return;

	const FVector PlayerLoc = PC->GetPawn()->GetActorLocation();
	CullDistant(PlayerLoc);
	SpawnNeeded(PlayerLoc);
}

void AProceduralTrafficManager::CullDistant(const FVector& PlayerLoc)
{
	// A replacement now has one physical source instead of teleporting into an
	// annulus. Keep the fixed-size traffic pool alive across the whole playable
	// city so a garage car is not deleted merely because Zarri is across town.
	// This changes retention distance, never the TargetActiveCount actor budget.
	const float ManagedTrafficCullRadius = FMath::Max(
		SpawnRadius, Grid::CityBoundaryHalfExtent * 2.85f);
	for (int32 i = ActiveTraffic.Num() - 1; i >= 0; --i)
	{
		APawn* P = ActiveTraffic[i];
		if (!IsValid(P))
		{
			ActiveTraffic.RemoveAtSwap(i);
			continue;
		}
		if (const ASprawlCar* Car = Cast<ASprawlCar>(P);
			Car && Car->NeedsTrafficRecycle())
		{
			P->Destroy();
			ActiveTraffic.RemoveAtSwap(i);
			++TrafficRouteRecycles;
			continue;
		}

		// A claimed traffic car leaves the managed lane/spacing population
		// immediately. Keep it as a bounded abandoned-car object so stealing a
		// car never destroys the vehicle under the player.
		if (const ASprawlCar* Car = Cast<ASprawlCar>(P);
			Car && (!Car->bAutoDrive || Cast<APlayerController>(Car->GetController())))
		{
			RetiredTraffic.AddUnique(P);
			ActiveTraffic.RemoveAtSwap(i);
			continue;
		}

		const float D = FVector::Dist(P->GetActorLocation(), PlayerLoc);
		if (D > ManagedTrafficCullRadius)
		{
			P->Destroy();
			ActiveTraffic.RemoveAtSwap(i);
		}
	}

	for (int32 Index = RetiredTraffic.Num() - 1; Index >= 0; --Index)
	{
		APawn* Retired = RetiredTraffic[Index];
		if (!IsValid(Retired))
		{
			RetiredTraffic.RemoveAtSwap(Index);
			continue;
		}
		if (Cast<APlayerController>(Retired->GetController()))
		{
			continue;
		}
		const bool bTooFar = FVector::Dist(
			Retired->GetActorLocation(), PlayerLoc) > SpawnRadius;
		if (bTooFar)
		{
			Retired->Destroy();
			RetiredTraffic.RemoveAtSwap(Index);
			continue;
		}

		// Once the player leaves a stolen car, move it out of the live lane and
		// align it with the nearest curb. If both legal shoulders are blocked,
		// remove the abandoned car instead of letting it obstruct traffic.
		const FVector Location = Retired->GetActorLocation();
		if (Grid::IsOnRoadSurface(Location.X, Location.Y, -40.f))
		{
			ASprawlCar* RetiredCar = Cast<ASprawlCar>(Retired);
			if (!RetiredCar || !TryParkRetiredCar(GetWorld(), RetiredCar))
			{
				Retired->Destroy();
				RetiredTraffic.RemoveAtSwap(Index);
			}
		}
	}

	// Enforce the abandoned-car budget by removing the farthest unpossessed
	// car first. The player's current car is never selected or destroyed.
	const int32 RetiredBudget = FMath::Max(0, MaxRetiredTraffic);
	int32 AbandonedCount = 0;
	for (APawn* Retired : RetiredTraffic)
	{
		if (IsValid(Retired)
			&& !Cast<APlayerController>(Retired->GetController()))
		{
			++AbandonedCount;
		}
	}
	while (AbandonedCount > RetiredBudget)
	{
		int32 FarthestIndex = INDEX_NONE;
		float FarthestDistanceSq = -1.f;
		for (int32 Index = 0; Index < RetiredTraffic.Num(); ++Index)
		{
			APawn* Retired = RetiredTraffic[Index];
			if (!IsValid(Retired)
				|| Cast<APlayerController>(Retired->GetController()))
			{
				continue;
			}
			const float DistanceSq = FVector::DistSquared2D(
				Retired->GetActorLocation(), PlayerLoc);
			if (DistanceSq > FarthestDistanceSq)
			{
				FarthestDistanceSq = DistanceSq;
				FarthestIndex = Index;
			}
		}
		if (!RetiredTraffic.IsValidIndex(FarthestIndex))
		{
			break;
		}
		RetiredTraffic[FarthestIndex]->Destroy();
		RetiredTraffic.RemoveAtSwap(FarthestIndex);
		--AbandonedCount;
	}
}

void AProceduralTrafficManager::SpawnNeeded(const FVector& PlayerLoc)
{
	UWorld* World = GetWorld();
	if (!World || ActiveTraffic.Num() >= TargetActiveCount)
	{
		return;
	}

	ASprawlParkingGarage* Garage =
		ASprawlParkingGarage::EnsureForWorld(World);
	if (!Garage || !Garage->IsReadyForTraffic())
	{
		return;
	}

	const TArray<FSprawlParkingGarageExit>& Exits = Garage->GetVehicleExits();
	constexpr float MinimumPlayerDistance = 900.f;
	for (int32 Attempt = 0; Attempt < Exits.Num(); ++Attempt)
	{
		const int32 ExitIndex = (NextGarageExitIndex + Attempt) % Exits.Num();
		const FSprawlParkingGarageExit& Exit = Exits[ExitIndex];
		++TrafficGarageSpawnAttempts;
		if (!Exit.IsValid())
		{
			++TrafficSpawnRouteRejects;
			continue;
		}
		if (FVector::DistSquared2D(
			PlayerLoc, Exit.SpawnTransform.GetLocation())
			< FMath::Square(MinimumPlayerDistance)
			|| !IsGarageExitClear(World, Exit, Garage))
		{
			++TrafficGarageBlockedExits;
			continue;
		}

		TSubclassOf<APawn> SpawnClass = ASprawlCar::StaticClass();
		if (!TrafficPawnClasses.IsEmpty())
		{
			SpawnClass = TrafficPawnClasses[
				FMath::RandRange(0, TrafficPawnClasses.Num() - 1)];
		}
		if (!SpawnClass || !SpawnClass->IsChildOf(ASprawlCar::StaticClass()))
		{
			// Garage egress requires the shared car contract. A stale generic pawn
			// entry must not reintroduce a lane-pop fallback.
			SpawnClass = ASprawlCar::StaticClass();
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
		APawn* Spawned = World->SpawnActor<APawn>(SpawnClass,
			Exit.SpawnTransform.GetLocation(),
			Exit.SpawnTransform.Rotator(), Params);
		ASprawlCar* Car = Cast<ASprawlCar>(Spawned);
		if (!Car || !Car->BeginGarageEgress(
			Exit.Id, Exit.DeparturePath, Exit.MergeHeading, Exit.RoadIndex))
		{
			if (Spawned)
			{
				Spawned->Destroy();
			}
			++TrafficGarageBlockedExits;
			continue;
		}

		ActiveTraffic.Add(Car);
		++TrafficGarageSpawnSuccesses;
		NextGarageExitIndex = (ExitIndex + 1) % Exits.Num();
		UE_LOG(LogTemp, Display,
			TEXT("[TrafficGarage] spawned %s inside exit=%s active=%d target=%d"),
			*Car->GetName(), *Exit.Id.ToString(), ActiveTraffic.Num(),
			TargetActiveCount);
		return; // one realistic departure per evaluation pass
	}
}
