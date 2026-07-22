// The Connected Sprawl - Street dressing repair: signs, junction props, and
// signal poles standing correctly on the sidewalks.

#include "World/SprawlStreetDressing.h"

#include "World/SprawlCityGridSubsystem.h"
#include "World/SprawlTrafficLight.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"

using Grid = USprawlCityGridSubsystem;

namespace
{
// Junction dressing authored by rpg_city_realism.py; a block-features pass
// later recentred these onto block centres (ledger: 25 props displaced).
const TCHAR* JunctionPropMeshes[] = {
	TEXT("SM_food_cart"),
	TEXT("SM_prop_fountain_full"),
	TEXT("SM_lamps_light_sign_a"),
	TEXT("SM_lamps_light_sign_b"),
	TEXT("SM_prop_poster_pillar_with_posters"),
};

// Park blocks own their centre dressing legitimately (fountain plaza).
const FIntPoint ParkBlocks[] = { {1, 5}, {2, 2}, {5, 5} };

bool MeshNameContains(const AStaticMeshActor& Actor, const TCHAR* Fragment)
{
	const UStaticMeshComponent* Mesh = Actor.GetStaticMeshComponent();
	const UStaticMesh* Asset = Mesh ? Mesh->GetStaticMesh() : nullptr;
	return Asset && Asset->GetName().Contains(Fragment);
}

bool IsTilted(const AActor& Actor)
{
	const FRotator R = Actor.GetActorRotation();
	return FMath::Abs(R.Pitch) > 4.f || FMath::Abs(R.Roll) > 4.f;
}

bool IsFloating(const AActor& Actor)
{
	const float Z = Actor.GetActorLocation().Z;
	return Z < FSprawlKerbPlacement::SidewalkTopZ - 20.f
		|| Z > FSprawlKerbPlacement::SidewalkTopZ + 40.f;
}

/** Yaw that faces the point toward a target (visual front = +X). */
float YawToward(const FVector2D& From, const FVector2D& To)
{
	const FVector2D D = To - From;
	return FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X));
}
}

FVector2D FSprawlKerbPlacement::KerbPointFor(const FVector2D& Position,
	float Offset)
{
	const float RoadX = Grid::RoadCenter(Grid::NearestRoadIndex(Position.X));
	const float RoadY = Grid::RoadCenter(Grid::NearestRoadIndex(Position.Y));
	// Attach to whichever road axis is closer, keeping the actor's side.
	if (FMath::Abs(Position.X - RoadX) <= FMath::Abs(Position.Y - RoadY))
	{
		const float Side = Position.X >= RoadX ? 1.f : -1.f;
		return FVector2D(RoadX + Side * Offset, Position.Y);
	}
	const float Side = Position.Y >= RoadY ? 1.f : -1.f;
	return FVector2D(Position.X, RoadY + Side * Offset);
}

FVector2D FSprawlKerbPlacement::JunctionCornerFor(const FVector2D& Position,
	float Offset)
{
	const float RoadX = Grid::RoadCenter(Grid::NearestRoadIndex(Position.X));
	const float RoadY = Grid::RoadCenter(Grid::NearestRoadIndex(Position.Y));
	const float SX = Position.X >= RoadX ? 1.f : -1.f;
	const float SY = Position.Y >= RoadY ? 1.f : -1.f;
	return FVector2D(RoadX + SX * Offset, RoadY + SY * Offset);
}

bool FSprawlKerbPlacement::IsInCarriageway(const FVector2D& Position)
{
	const float RoadX = Grid::RoadCenter(Grid::NearestRoadIndex(Position.X));
	const float RoadY = Grid::RoadCenter(Grid::NearestRoadIndex(Position.Y));
	const float Half = Grid::RoadWidth * 0.5f + 20.f;
	return FMath::Abs(Position.X - RoadX) < Half
		|| FMath::Abs(Position.Y - RoadY) < Half;
}

bool FSprawlKerbPlacement::IsOnSidewalk(const FVector2D& Position)
{
	const float RoadX = Grid::RoadCenter(Grid::NearestRoadIndex(Position.X));
	const float RoadY = Grid::RoadCenter(Grid::NearestRoadIndex(Position.Y));
	const float DeltaX = FMath::Abs(Position.X - RoadX);
	const float DeltaY = FMath::Abs(Position.Y - RoadY);
	const float RoadHalf = Grid::RoadWidth * 0.5f;
	const auto IsSidewalkOffset = [](float Distance)
	{
		return Distance >= SidewalkInnerOffset
			&& Distance <= SidewalkOuterOffset;
	};

	// A sidewalk follows one road beyond its kerb, but must also be clear of
	// any crossing road. This rejects parking bays and asphalt aprons that are
	// road-free only because they sit outside the live travel lane.
	return (IsSidewalkOffset(DeltaX) && DeltaY >= RoadHalf)
		|| (IsSidewalkOffset(DeltaY) && DeltaX >= RoadHalf);
}

bool FSprawlKerbPlacement::IsAtStrandedBlockCentre(const FVector2D& Position)
{
	for (int32 Bx = 0; Bx < Grid::NumBlocks; ++Bx)
	{
		for (int32 By = 0; By < Grid::NumBlocks; ++By)
		{
			bool bPark = false;
			for (const FIntPoint& Park : ParkBlocks)
			{
				bPark |= Park.X == Bx && Park.Y == By;
			}
			if (bPark)
			{
				continue;
			}
			if (FMath::Abs(Position.X - Grid::BlockCenter(Bx)) < 350.f
				&& FMath::Abs(Position.Y - Grid::BlockCenter(By)) < 350.f)
			{
				return true;
			}
		}
	}
	return false;
}

ASprawlStreetDressing::ASprawlStreetDressing()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorEnableCollision(false);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

int32 ASprawlStreetDressing::RepairFoldoutSigns()
{
	int32 Fixed = 0;
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		if (!MeshNameContains(**It, TEXT("sign_foldout")))
		{
			continue;
		}
		const FVector Loc = It->GetActorLocation();
		const FVector2D P(Loc.X, Loc.Y);
		const bool bWrong = IsTilted(**It) || IsFloating(**It)
			|| !FSprawlKerbPlacement::IsOnSidewalk(P);
		if (!bWrong)
		{
			continue;
		}
		const FVector2D Kerb = FSprawlKerbPlacement::KerbPointFor(
			P, FSprawlKerbPlacement::SignKerbOffset);
		const float RoadX = Grid::RoadCenter(Grid::NearestRoadIndex(Kerb.X));
		const float RoadY = Grid::RoadCenter(Grid::NearestRoadIndex(Kerb.Y));
		// Face the nearer road so the board reads toward passing traffic.
		const FVector2D Facing =
			FMath::Abs(Kerb.X - RoadX) <= FMath::Abs(Kerb.Y - RoadY)
				? FVector2D(RoadX, Kerb.Y)
				: FVector2D(Kerb.X, RoadY);
		It->SetActorLocationAndRotation(
			FVector(Kerb.X, Kerb.Y, FSprawlKerbPlacement::SidewalkTopZ),
			FRotator(0.f, YawToward(Kerb, Facing), 0.f));
		++Fixed;
	}
	return Fixed;
}

int32 ASprawlStreetDressing::RepairStrandedJunctionProps()
{
	int32 Fixed = 0;
	int32 CornerSpread = 0;
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		bool bJunctionProp = false;
		for (const TCHAR* MeshName : JunctionPropMeshes)
		{
			bJunctionProp |= MeshNameContains(**It, MeshName);
		}
		if (!bJunctionProp)
		{
			continue;
		}
		const FVector Loc = It->GetActorLocation();
		const FVector2D P(Loc.X, Loc.Y);
		const bool bWrong = FSprawlKerbPlacement::IsAtStrandedBlockCentre(P)
			|| FSprawlKerbPlacement::IsInCarriageway(P) || IsTilted(**It);
		if (!bWrong)
		{
			continue;
		}
		FVector2D Corner = FSprawlKerbPlacement::JunctionCornerFor(
			P, FSprawlKerbPlacement::PropCornerOffset);
		// Spread multiple props claiming one corner along its diagonal.
		const float Extra = 150.f * (CornerSpread++ % 3);
		const FVector2D Out =
			(Corner - FSprawlKerbPlacement::JunctionCornerFor(P, 0.f))
				.GetSafeNormal();
		Corner += Out * Extra;
		const FVector2D JunctionCentre =
			FSprawlKerbPlacement::JunctionCornerFor(P, 0.f);
		It->SetActorLocationAndRotation(
			FVector(Corner.X, Corner.Y, FSprawlKerbPlacement::SidewalkTopZ),
			FRotator(0.f, YawToward(Corner, JunctionCentre), 0.f));
		++Fixed;
	}
	return Fixed;
}

int32 ASprawlStreetDressing::RepairWallSignsInCarriageway()
{
	int32 Fixed = 0;
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		// Wall-mounted shop signs live on building faces by design; only one
		// that ended up inside the carriageway (or floating there) is wrong.
		if (!MeshNameContains(**It, TEXT("signs_sign"))
			|| MeshNameContains(**It, TEXT("sign_foldout")))
		{
			continue;
		}
		const FVector Loc = It->GetActorLocation();
		const FVector2D P(Loc.X, Loc.Y);
		if (!FSprawlKerbPlacement::IsInCarriageway(P))
		{
			continue;
		}
		const FVector2D Kerb = FSprawlKerbPlacement::KerbPointFor(
			P, FSprawlKerbPlacement::SignKerbOffset);
		It->SetActorLocationAndRotation(
			FVector(Kerb.X, Kerb.Y,
				FMath::Max(Loc.Z, FSprawlKerbPlacement::SidewalkTopZ)),
			FRotator(0.f, It->GetActorRotation().Yaw, 0.f));
		++Fixed;
	}
	return Fixed;
}

int32 ASprawlStreetDressing::EnsureIntersectionSignals(int32& OutSpawned)
{
	OutSpawned = 0;
	int32 Snapped = 0;
	TSet<FIntPoint> ExistingIntersections;
	for (TActorIterator<ASprawlTrafficLight> It(GetWorld()); It; ++It)
	{
		const FVector Previous = It->GetActorLocation();
		It->SnapToIntersection(FSprawlKerbPlacement::SidewalkTopZ);
		if (!Previous.Equals(It->GetActorLocation(), 1.f))
		{
			++Snapped;
		}
		ExistingIntersections.Add(FIntPoint(It->IntersectionX, It->IntersectionY));
	}

	for (int32 IntersectionX = 0; IntersectionX < Grid::NumRoads; ++IntersectionX)
	{
		for (int32 IntersectionY = 0; IntersectionY < Grid::NumRoads; ++IntersectionY)
		{
			const FVector2D Center = Grid::IntersectionCenter(IntersectionX, IntersectionY);
			if (Grid::IsOverLake(Center.X, Center.Y, 100.f)
				|| ExistingIntersections.Contains(FIntPoint(IntersectionX, IntersectionY)))
			{
				continue;
			}

			FActorSpawnParameters Params;
			Params.Name = *FString::Printf(TEXT("SprawlIntersectionSignal_%d_%d"),
				IntersectionX, IntersectionY);
			Params.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ASprawlTrafficLight* Signal = GetWorld()->SpawnActor<ASprawlTrafficLight>(
				FVector(Center.X, Center.Y, FSprawlKerbPlacement::SidewalkTopZ),
				FRotator::ZeroRotator, Params);
			if (!Signal)
			{
				continue;
			}
			Signal->IntersectionX = IntersectionX;
			Signal->IntersectionY = IntersectionY;
			++OutSpawned;
		}
	}
	return Snapped;
}

void ASprawlStreetDressing::BeginPlay()
{
	Super::BeginPlay();
	const int32 Foldouts = RepairFoldoutSigns();
	const int32 Stranded = RepairStrandedJunctionProps();
	const int32 WallSigns = RepairWallSignsInCarriageway();
	int32 SpawnedSignals = 0;
	const int32 Signals = EnsureIntersectionSignals(SpawnedSignals);
	UE_LOG(LogTemp, Display,
		TEXT("[StreetDressing] foldout_signs_sidewalk=%d stranded_junction_props=%d wall_signs_off_road=%d signal_models_snapped=%d signal_models_spawned=%d"),
		Foldouts, Stranded, WallSigns, Signals, SpawnedSignals);
}

ASprawlStreetDressing* ASprawlStreetDressing::EnsureForWorld(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	TActorIterator<ASprawlStreetDressing> Existing(World);
	if (Existing)
	{
		return *Existing;
	}
	FActorSpawnParameters Params;
	Params.Name = TEXT("SprawlStreetDressing");
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<ASprawlStreetDressing>(
		FVector::ZeroVector, FRotator::ZeroRotator, Params);
}
