// The Connected Sprawl - Procedural traffic.

#include "AI/ProceduralTrafficManager.h"
#include "AI/SprawlPedestrian.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Vehicles/SprawlCar.h"
#include "World/SprawlCityGridSubsystem.h"
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
	TimeSinceEval += DeltaSeconds;
	if (TimeSinceEval < EvaluateInterval) return;
	TimeSinceEval = 0.f;
	Evaluate();
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
		const float LaneError = FMath::Abs(
			(bNorthSouth ? Loc.X : Loc.Y) - Grid::LaneCenter(RoadIndex, Heading));
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
		if (NearestCrossingDistance > 900.f && LaneError > 90.f)
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
		if (NextCrossing >= 0 && DistanceAhead >= 740.f && DistanceAhead <= 840.f
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
		if (FMath::Abs(Loc.X - Grid::RoadCenter(IntersectionX)) < 300.f
			&& FMath::Abs(Loc.Y - Grid::RoadCenter(IntersectionY)) < 300.f)
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
		++PedestrianCount;
		if (It->HasRealAvatar())
		{
			++RealAvatarCount;
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
		&& TrafficAuditMaxRealAvatars >= 20;
	const TCHAR* Result = bPassed ? TEXT("PASS") : TEXT("FAIL");
	if (bPassed)
	{
		UE_LOG(LogSprawlTrafficAudit, Display,
			TEXT("[TrafficAudit] %s cars=%d total_cars=%d enterable_cars=%d "
				"boundary_violators=%d upright_violators=%d moved=%d signal_stops=%d wheel_cars=%d "
				"max_box_occupancy=%d unauthorized_entries=%d min_spacing=%.1f "
				"lane_violators=%d pedestrians=%d real_avatars=%d"),
			Result, TrafficAuditPeakCars, TrafficAuditPeakTotalCars,
			TrafficAuditMaxEnterableCars, TrafficAuditMaxBoundaryViolations,
			TrafficAuditMaxUprightViolations, TrafficAuditMovedCars.Num(),
			TrafficAuditSignalStops.Num(), TrafficAuditWheelMotionCars.Num(),
			TrafficAuditMaxIntersectionOccupancy, TrafficAuditUnauthorizedEntries,
			TrafficAuditMinSpacing, TrafficAuditLaneViolators.Num(),
			TrafficAuditMaxPedestrians, TrafficAuditMaxRealAvatars);
	}
	else
	{
		UE_LOG(LogSprawlTrafficAudit, Error,
			TEXT("[TrafficAudit] %s cars=%d total_cars=%d enterable_cars=%d "
				"boundary_violators=%d upright_violators=%d moved=%d signal_stops=%d wheel_cars=%d "
				"max_box_occupancy=%d unauthorized_entries=%d min_spacing=%.1f "
				"lane_violators=%d pedestrians=%d real_avatars=%d"),
			Result, TrafficAuditPeakCars, TrafficAuditPeakTotalCars,
			TrafficAuditMaxEnterableCars, TrafficAuditMaxBoundaryViolations,
			TrafficAuditMaxUprightViolations, TrafficAuditMovedCars.Num(),
			TrafficAuditSignalStops.Num(), TrafficAuditWheelMotionCars.Num(),
			TrafficAuditMaxIntersectionOccupancy, TrafficAuditUnauthorizedEntries,
			TrafficAuditMinSpacing, TrafficAuditLaneViolators.Num(),
			TrafficAuditMaxPedestrians, TrafficAuditMaxRealAvatars);
	}
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

		const float D = FVector::Dist(P->GetActorLocation(), PlayerLoc);
		if (D > SpawnRadius)
		{
			P->Destroy();
			ActiveTraffic.RemoveAtSwap(i);
		}
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
