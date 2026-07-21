// The Connected Sprawl - City-limit gates where streets meet the boundary.

#include "World/SprawlCityGates.h"

#include "World/SprawlCityGridSubsystem.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

using Grid = USprawlCityGridSubsystem;

namespace
{
// Gates stand on the city floor's outer margin, at the mouth of each street.
constexpr float GateDistance = 11350.f;
constexpr float PillarClearance = 90.f;   // beyond the kerb line
constexpr float PillarSide = 180.f;
constexpr float PillarHeight = 620.f;
constexpr float BeamHeight = 150.f;
constexpr float BeamDepth = 140.f;
constexpr float StubLength = 700.f;
constexpr float StubHeight = 240.f;

// The lake meets the south boundary across this X range and the east boundary
// below this Y; streets ending in water get no gate.
constexpr float BayGapMinX = 600.f;
constexpr float BayGapMaxX = 11800.f;
constexpr float LakeEastMaxY = -4300.f;

const FLinearColor ConcreteTint(0.24f, 0.235f, 0.22f, 1.f);
}

FSprawlCityGateLayout FSprawlCityGateLayout::Build()
{
	FSprawlCityGateLayout Layout;
	const float PillarOffset = Grid::RoadWidth * 0.5f + PillarClearance;
	const float BeamSpan = 2.f * PillarOffset + PillarSide;
	const float StubCenter = PillarOffset + PillarSide * 0.5f
		+ StubLength * 0.5f + 40.f;

	// bAlongX: the street runs along Y and crosses a north/south boundary, so
	// the gate's span axis is X. Otherwise the span axis is Y.
	auto AddGate = [&](float AlongCenter, float BoundarySign, bool bNorthSouth)
	{
		auto Piece = [&](float Along, float Z, const FVector& SpanSize)
		{
			// SpanSize.X runs along the gate span; swap for east/west gates.
			const FVector Size = bNorthSouth
				? SpanSize
				: FVector(SpanSize.Y, SpanSize.X, SpanSize.Z);
			const float X = bNorthSouth
				? AlongCenter + Along : BoundarySign * GateDistance;
			const float Y = bNorthSouth
				? BoundarySign * GateDistance : AlongCenter + Along;
			Layout.Pieces.Emplace(FRotator::ZeroRotator,
				FVector(X, Y, Z), Size / 100.f);
		};
		Piece(-PillarOffset, PillarHeight * 0.5f,
			FVector(PillarSide, PillarSide, PillarHeight));
		Piece(PillarOffset, PillarHeight * 0.5f,
			FVector(PillarSide, PillarSide, PillarHeight));
		Piece(0.f, PillarHeight + BeamHeight * 0.5f,
			FVector(BeamSpan, BeamDepth, BeamHeight));
		Piece(-StubCenter, StubHeight * 0.5f,
			FVector(StubLength, BeamDepth, StubHeight));
		Piece(StubCenter, StubHeight * 0.5f,
			FVector(StubLength, BeamDepth, StubHeight));
		++Layout.GateCount;
	};

	for (int32 Road = 0; Road < Grid::NumRoads; ++Road)
	{
		const float Along = Grid::RoadCenter(Road);
		// North boundary: every street gets a gate.
		AddGate(Along, 1.f, true);
		// South boundary: skip streets ending in the bay.
		if (Along < BayGapMinX || Along > BayGapMaxX)
		{
			AddGate(Along, -1.f, true);
		}
		// West boundary: every street.
		AddGate(Along, -1.f, false);
		// East boundary: skip streets ending in the lake's east face.
		if (Along > LakeEastMaxY)
		{
			AddGate(Along, 1.f, false);
		}
	}
	return Layout;
}

bool FSprawlCityGateLayout::IsValid() const
{
	if (GateCount < 12 || Pieces.Num() != GateCount * 5)
	{
		return false;
	}
	for (const FTransform& Piece : Pieces)
	{
		const FVector Loc = Piece.GetLocation();
		if (Piece.GetScale3D().GetMin() <= 0.f
			|| FMath::Max(FMath::Abs(Loc.X), FMath::Abs(Loc.Y))
				< Grid::CityBoundaryHalfExtent - 400.f)
		{
			return false;
		}
	}
	return true;
}

ASprawlCityGates::ASprawlCityGates()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Cube(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> Concrete(
		TEXT("/Game/Import/CityKit/MI_ConcreteWorn.MI_ConcreteWorn"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> Opaque(
		TEXT("/Game/Materials/Master/M_Simple_Opaque.M_Simple_Opaque"));
	CubeMesh = Cube.Get();
	ConcreteMaterial = Concrete.Get();
	OpaqueBase = Opaque.Get();

	Pieces = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("GatePieces"));
	Pieces->SetupAttachment(RootComponent);
	// Pillars block like street furniture; the beam is far above traffic.
	Pieces->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Pieces->SetGenerateOverlapEvents(false);
	Pieces->SetCanEverAffectNavigation(false);
	Pieces->SetCastShadow(false);
	Pieces->SetReceivesDecals(false);
	if (CubeMesh)
	{
		Pieces->SetStaticMesh(CubeMesh);
	}
}

void ASprawlCityGates::BeginPlay()
{
	Super::BeginPlay();
	const FSprawlCityGateLayout Layout = FSprawlCityGateLayout::Build();
	if (!Layout.IsValid() || !Pieces)
	{
		return;
	}
	UMaterialInterface* Surface = ConcreteMaterial;
	if (!Surface && OpaqueBase)
	{
		FallbackMaterial = UMaterialInstanceDynamic::Create(OpaqueBase, this);
		FallbackMaterial->SetVectorParameterValue(TEXT("BaseColor"), ConcreteTint);
		FallbackMaterial->SetVectorParameterValue(TEXT("Color"), ConcreteTint);
		FallbackMaterial->SetScalarParameterValue(TEXT("Roughness"), 0.85f);
		Surface = FallbackMaterial;
	}
	if (Surface)
	{
		Pieces->SetMaterial(0, Surface);
	}
	Pieces->ClearInstances();
	for (const FTransform& Piece : Layout.Pieces)
	{
		Pieces->AddInstance(Piece);
	}
	UE_LOG(LogTemp, Display,
		TEXT("[CityGates] %d gates (%d pieces) at the street mouths"),
		Layout.GateCount, Layout.Pieces.Num());
}

ASprawlCityGates* ASprawlCityGates::EnsureForWorld(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	TActorIterator<ASprawlCityGates> Existing(World);
	if (Existing)
	{
		return *Existing;
	}
	FActorSpawnParameters Params;
	Params.Name = TEXT("SprawlCityGates");
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<ASprawlCityGates>(
		FVector::ZeroVector, FRotator::ZeroRotator, Params);
}
