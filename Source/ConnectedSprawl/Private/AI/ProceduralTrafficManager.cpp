// The Connected Sprawl - Procedural traffic.

#include "AI/ProceduralTrafficManager.h"
#include "AI/SprawlPedestrian.h"
#include "Characters/ZarriCharacter.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Vehicles/SprawlCar.h"
#include "World/SprawlCityGridSubsystem.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

DEFINE_LOG_CATEGORY_STATIC(LogSprawlTrafficAudit, Log, All);

namespace
{
bool IsTrafficSpawnClear(UWorld* World, const FVector& Location,
	const FQuat& Rotation, const AActor* IgnoredActor)
{
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
	return !World->OverlapAnyTestByObjectType(Location, Rotation, Objects,
		FCollisionShape::MakeBox(FVector(330.f, 125.f, 90.f)), Params);
}
}

// File-scope alias (matches the other Sprawl translation units so unity
// builds don't see a local declaration shadowing the global one).
using Grid = USprawlCityGridSubsystem;

namespace
{
ESprawlHeading OppositeHeading(ESprawlHeading Heading)
{
	switch (Heading)
	{
	case ESprawlHeading::East: return ESprawlHeading::West;
	case ESprawlHeading::North: return ESprawlHeading::South;
	case ESprawlHeading::West: return ESprawlHeading::East;
	default: return ESprawlHeading::North;
	}
}

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
			SideAttempt == 0 ? Heading : OppositeHeading(Heading));
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
	int32 VisibleDriversThisSample = 0;
	int32 DriverReadyCarsThisSample = 0;
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
			if (Car->HasVisibleDriver())
			{
				++VisibleDriversThisSample;
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
		const ESprawlHeading Heading = Grid::HeadingFromYaw(Car->GetActorRotation().Yaw);
		const bool bNorthSouth = Grid::IsNorthSouth(Heading);
		const int32 RoadIndex = Grid::NearestRoadIndex(bNorthSouth ? Loc.X : Loc.Y);
		// Every lane on this side of the centreline is legal, so measure the
		// departure from the nearest one rather than from lane 0 alone.
		const float LaneCoord = bNorthSouth ? Loc.X : Loc.Y;
		float LaneError = TNumericLimits<float>::Max();
		for (int32 Lane = 0; Lane < Grid::LanesPerDirection; ++Lane)
		{
			LaneError = FMath::Min(LaneError, FMath::Abs(
				LaneCoord - Grid::LaneCenter(RoadIndex, Heading, Lane)));
		}
		const float TravelCoord = bNorthSouth ? Loc.Y : Loc.X;
		const float NearestCrossingDistance = [&]()
		{
			float Distance = TNumericLimits<float>::Max();
			for (int32 Index = 0; Index < Grid::NumRoads; ++Index)
			{
				Distance = FMath::Min(Distance,
					FMath::Abs(TravelCoord - Grid::RoadCenter(Index)));
			}
			return Distance;
		}();
		float& LaneViolationDuration = TrafficAuditLaneViolationDurations.FindOrAdd(CarKey);
		// Clear of the junction box and its turn-in arc, so only a genuine
		// straight-line lane departure is measured.
		if (NearestCrossingDistance > Grid::RoadWidth * 0.5f + 450.f
			&& LaneError > 90.f)
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
					*Car->GetName(), Loc.X, Loc.Y, LaneError, NearestCrossingDistance);
			}
		}
		else
		{
			LaneViolationDuration = 0.f;
		}

		const float TravelDirection = (Heading == ESprawlHeading::North
			|| Heading == ESprawlHeading::East) ? 1.f : -1.f;
		int32 NextCrossing = -1;
		float DistanceAhead = TNumericLimits<float>::Max();
		for (int32 Index = 0; Index < Grid::NumRoads; ++Index)
		{
			const float Distance = (Grid::RoadCenter(Index) - TravelCoord) * TravelDirection;
			if (Distance >= 0.f && Distance < DistanceAhead)
			{
				DistanceAhead = Distance;
				NextCrossing = Index;
			}
		}
		// Hold window derived from the grid rather than hard-coded, so this
		// gate follows the street layout instead of a past road width.
		constexpr float SignalHoldTolerance = 70.f;
		if (NextCrossing >= 0
			&& DistanceAhead >= Grid::VehicleStopDistance - SignalHoldTolerance
			&& DistanceAhead <= Grid::VehicleStopDistance + SignalHoldTolerance
			&& Car->GetVelocity().Size2D() < 80.f)
		{
			const int32 IntersectionX = bNorthSouth ? RoadIndex : NextCrossing;
			const int32 IntersectionY = bNorthSouth ? NextCrossing : RoadIndex;
			if (GridSub->GetSignal(IntersectionX, IntersectionY, bNorthSouth)
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
	TrafficAuditMaxVisibleDrivers = FMath::Max(
		TrafficAuditMaxVisibleDrivers, VisibleDriversThisSample);
	if (TrafficAuditElapsed >= 5.f)
	{
		TrafficAuditMaxMissingDriversAfterWarmup = FMath::Max(
			TrafficAuditMaxMissingDriversAfterWarmup,
			DriverReadyCarsThisSample - VisibleDriversThisSample);
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
	for (TActorIterator<ASprawlPedestrian> It(World); It; ++It)
	{
		ASprawlPedestrian* Pedestrian = *It;
		++PedestrianCount;
		if (Pedestrian->HasRealAvatar())
		{
			++RealAvatarCount;
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

	if (TrafficAuditElapsed < 30.f)
	{
		return;
	}
	bTrafficAuditComplete = true;
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
		&& TrafficAuditMaxPedestrians >= 20
		&& TrafficAuditMaxRealAvatars >= 20
		&& TrafficAuditMaxVisibleDrivers >= 10
		&& TrafficAuditMaxMissingDriversAfterWarmup == 0
		&& TrafficAuditPedOffsideViolators.IsEmpty();
	bTrafficAuditPassed = bPassed;
	const TCHAR* Result = bPassed ? TEXT("PASS") : TEXT("FAIL");
	if (bPassed)
	{
		UE_LOG(LogSprawlTrafficAudit, Display,
			TEXT("[TrafficAudit] %s cars=%d total_cars=%d enterable_cars=%d "
				"boundary_violators=%d upright_violators=%d moved=%d signal_stops=%d wheel_cars=%d "
				"max_box_occupancy=%d unauthorized_entries=%d min_spacing=%.1f "
				"lane_violators=%d pedestrians=%d real_avatars=%d visible_drivers=%d "
				"missing_drivers=%d ped_offside=%d"),
			Result, TrafficAuditPeakCars, TrafficAuditPeakTotalCars,
			TrafficAuditMaxEnterableCars, TrafficAuditMaxBoundaryViolations,
			TrafficAuditMaxUprightViolations, TrafficAuditMovedCars.Num(),
			TrafficAuditSignalStops.Num(), TrafficAuditWheelMotionCars.Num(),
			TrafficAuditMaxIntersectionOccupancy, TrafficAuditUnauthorizedEntries,
			TrafficAuditMinSpacing, TrafficAuditLaneViolators.Num(),
			TrafficAuditMaxPedestrians, TrafficAuditMaxRealAvatars,
			TrafficAuditMaxVisibleDrivers, TrafficAuditMaxMissingDriversAfterWarmup,
			TrafficAuditPedOffsideViolators.Num());
	}
	else
	{
		UE_LOG(LogSprawlTrafficAudit, Error,
			TEXT("[TrafficAudit] %s cars=%d total_cars=%d enterable_cars=%d "
				"boundary_violators=%d upright_violators=%d moved=%d signal_stops=%d wheel_cars=%d "
				"max_box_occupancy=%d unauthorized_entries=%d min_spacing=%.1f "
				"lane_violators=%d pedestrians=%d real_avatars=%d visible_drivers=%d "
				"missing_drivers=%d ped_offside=%d"),
			Result, TrafficAuditPeakCars, TrafficAuditPeakTotalCars,
			TrafficAuditMaxEnterableCars, TrafficAuditMaxBoundaryViolations,
			TrafficAuditMaxUprightViolations, TrafficAuditMovedCars.Num(),
			TrafficAuditSignalStops.Num(), TrafficAuditWheelMotionCars.Num(),
			TrafficAuditMaxIntersectionOccupancy, TrafficAuditUnauthorizedEntries,
			TrafficAuditMinSpacing, TrafficAuditLaneViolators.Num(),
			TrafficAuditMaxPedestrians, TrafficAuditMaxRealAvatars,
			TrafficAuditMaxVisibleDrivers, TrafficAuditMaxMissingDriversAfterWarmup,
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
			return; // allow lazy driver visuals to initialize
		}

		AZarriCharacter* Zarri = Cast<AZarriCharacter>(PC->GetPawn());
		ASprawlCar* Candidate = nullptr;
		for (APawn* Pawn : ActiveTraffic)
		{
			ASprawlCar* Car = Cast<ASprawlCar>(Pawn);
			if (Car && Car->HasAIDriver() && Car->HasVisibleDriver()
				&& !Car->HasActiveIntersectionReservation())
			{
				Candidate = Car;
				break;
			}
		}

		if (!Zarri || !Candidate)
		{
			if (CarjackAuditElapsed >= 20.f)
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
			if (CarjackAuditElapsed >= 20.f)
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
			if (CarjackAuditElapsed >= 20.f)
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
		&& EjectedDriver->GetActiveVariant() == CarjackAuditDriverVariant)
	{
		bCarjackAuditSawFleeingDriver = true;
	}
	const bool bFleeingDriverFound = bCarjackAuditSawFleeingDriver;

	const bool bPossessed = ClaimedCar && PC->GetPawn() == ClaimedCar;
	const bool bRetired = ClaimedCar && !ActiveTraffic.Contains(ClaimedCar)
		&& RetiredTraffic.Contains(ClaimedCar);
	const bool bLeaseFree = ClaimedCar
		&& !ClaimedCar->HasActiveIntersectionReservation();
	const bool bBackfilled = ActiveTraffic.Num() >= TargetActiveCount;
	const bool bPedestrianStable = PedestrianCountAfter
		>= CarjackAuditPedestrianCountBefore;
	const bool bPassed = bPossessed && bRetired && bLeaseFree && bBackfilled
		&& bFleeingDriverFound && bPedestrianStable
		&& ClaimedCar && !ClaimedCar->bAutoDrive && !ClaimedCar->HasAIDriver();

	if (!bPassed && CarjackAuditElapsed - CarjackAuditTriggerTime < 20.f)
	{
		return;
	}

	bCarjackAuditComplete = true;
	bCarjackAuditPassed = bPassed;
	if (bPassed)
	{
		UE_LOG(LogSprawlTrafficAudit, Display,
			TEXT("[CarjackAudit] PASS possessed=%d fleeing_driver=%d lease_free=%d "
				"retired=%d backfilled=%d active=%d target=%d peds_before=%d peds_after=%d"),
			bPossessed, bFleeingDriverFound, bLeaseFree, bRetired, bBackfilled,
			ActiveTraffic.Num(), TargetActiveCount,
			CarjackAuditPedestrianCountBefore, PedestrianCountAfter);
	}
	else
	{
		UE_LOG(LogSprawlTrafficAudit, Error,
			TEXT("[CarjackAudit] FAIL possessed=%d fleeing_driver=%d lease_free=%d "
				"retired=%d backfilled=%d active=%d target=%d peds_before=%d peds_after=%d"),
			bPossessed, bFleeingDriverFound, bLeaseFree, bRetired, bBackfilled,
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
	for (int32 i = ActiveTraffic.Num() - 1; i >= 0; --i)
	{
		APawn* P = ActiveTraffic[i];
		if (!IsValid(P))
		{
			ActiveTraffic.RemoveAtSwap(i);
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
		if (D > SpawnRadius)
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
	if (!World) return;

	int32 Attempts = 0;
	while (ActiveTraffic.Num() < TargetActiveCount && Attempts++ < 40)
	{
		// Random point in the outer annulus (InterestRadius..SpawnRadius),
		// snapped onto a road lane so the car starts mid-traffic-flow.
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		const float Dist  = FMath::FRandRange(InterestRadius, SpawnRadius);
		FVector SpawnLoc = PlayerLoc + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.f);

		// Snap to the nearest road on a random axis and pick a travel direction.
		const bool bVerticalRoad = FMath::RandBool();
		ESprawlHeading Heading;
		if (bVerticalRoad)
		{
			const int32 Road = Grid::NearestRoadIndex(SpawnLoc.X);
			Heading = FMath::RandBool() ? ESprawlHeading::North : ESprawlHeading::South;
			SpawnLoc.X = Grid::LaneCenter(Road, Heading);
		}
		else
		{
			const int32 Road = Grid::NearestRoadIndex(SpawnLoc.Y);
			Heading = FMath::RandBool() ? ESprawlHeading::East : ESprawlHeading::West;
			SpawnLoc.Y = Grid::LaneCenter(Road, Heading);
		}
		SpawnLoc.Z = 180.f;

		// Never materialise inside a junction box. The car would hold no
		// right-of-way lease while sitting in the middle of an intersection,
		// blocking every approach — and with a wide carriageway the boxes
		// cover a large share of each road's length.
		const float AlongCoord = bVerticalRoad ? SpawnLoc.Y : SpawnLoc.X;
		bool bInsideJunction = false;
		for (int32 Road = 0; Road < Grid::NumRoads; ++Road)
		{
			if (FMath::Abs(AlongCoord - Grid::RoadCenter(Road))
				< Grid::RoadWidth * 0.5f + 400.f)
			{
				bInsideJunction = true;
				break;
			}
		}
		if (bInsideJunction)
		{
			continue;
		}

		if (Grid::IsOverLake(SpawnLoc.X, SpawnLoc.Y) ||
			!Grid::IsInsideRoadGrid(SpawnLoc.X, SpawnLoc.Y))
		{
			continue;
		}

		const FQuat SpawnRotation = FRotator(
			0.f, Grid::HeadingYaw(Heading), 0.f).Quaternion();
		if (!IsTrafficSpawnClear(World, SpawnLoc, SpawnRotation, this))
		{
			continue;
		}

		TSubclassOf<APawn> Cls = ASprawlCar::StaticClass();
		if (TrafficPawnClasses.Num() > 0)
		{
			Cls = TrafficPawnClasses[FMath::RandRange(0, TrafficPawnClasses.Num() - 1)];
		}
		if (!Cls) break;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
		APawn* Spawned = World->SpawnActor<APawn>(
			Cls, SpawnLoc, FRotator(0.f, Grid::HeadingYaw(Heading), 0.f), Params);
		if (!Spawned)
		{
			continue;
		}

		if (ASprawlCar* Car = Cast<ASprawlCar>(Spawned))
		{
			Car->bAutoDrive = true;
		}
		ActiveTraffic.Add(Spawned);
	}
}
