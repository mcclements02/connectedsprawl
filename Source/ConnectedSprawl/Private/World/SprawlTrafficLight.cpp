// The Connected Sprawl - Complete visual traffic-signal model.

#include "World/SprawlTrafficLight.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
constexpr float SignalCornerOffset =
	USprawlCityGridSubsystem::RoadWidth * 0.5f + 160.f;
constexpr float PoleHeight = 500.f;
constexpr float HeadHeight = 430.f;

const FLinearColor RedLens(2.0f, 0.06f, 0.04f);
const FLinearColor AmberLens(1.8f, 0.9f, 0.05f);
const FLinearColor GreenLens(0.05f, 1.6f, 0.25f);
}

ASprawlTrafficLight::ASprawlTrafficLight()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f; // signal changes do not need frame-rate work
	SetActorEnableCollision(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> GlowMat(
		TEXT("/Game/Materials/M_SignalGlow.M_SignalGlow"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> MetalMat(
		TEXT("/Game/CityArt/MI_Metal.MI_Metal"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMat(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	UStaticMesh* Cube = CubeMesh.Succeeded() ? CubeMesh.Object.Get() : nullptr;
	UStaticMesh* Cylinder = CylinderMesh.Succeeded() ? CylinderMesh.Object.Get() : Cube;
	UStaticMesh* Sphere = SphereMesh.Succeeded() ? SphereMesh.Object.Get() : Cube;
	UMaterialInterface* Glow = GlowMat.Get()
		? GlowMat.Get()
		: (BasicMat.Succeeded() ? BasicMat.Object.Get() : nullptr);
	UMaterialInterface* Housing = MetalMat.Get()
		? MetalMat.Get()
		: (BasicMat.Succeeded() ? BasicMat.Object.Get() : nullptr);

	SignalRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SignalRoot"));
	RootComponent = SignalRoot;

	auto MakePart = [&](const FString& Name, UStaticMesh* Mesh,
		const FVector& RelativeLocation, const FVector& RelativeScale,
		UMaterialInterface* Material) -> UStaticMeshComponent*
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Part->SetupAttachment(SignalRoot);
		if (Mesh)
		{
			Part->SetStaticMesh(Mesh);
		}
		Part->SetRelativeLocation(RelativeLocation);
		Part->SetRelativeScale3D(RelativeScale);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCanEverAffectNavigation(false);
		if (Material)
		{
			Part->SetMaterial(0, Material);
		}
		return Part;
	};

	auto MakeLens = [&](const FString& Name, const FVector& RelativeLocation,
		TArray<TObjectPtr<UStaticMeshComponent>>& Lamps)
	{
		Lamps.Add(MakePart(Name, Sphere, RelativeLocation,
			FVector(0.16f), Glow));
	};

	// One low-cost model covers the whole junction. Four sidewalk-corner poles
	// make the phase readable from every approach without multiplying actors or
	// the 10 Hz signal tick budget.
	const FIntPoint CornerSigns[] = {
		FIntPoint(1, 1), FIntPoint(-1, 1),
		FIntPoint(-1, -1), FIntPoint(1, -1)
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(CornerSigns); ++Index)
	{
		const float SignX = static_cast<float>(CornerSigns[Index].X);
		const float SignY = static_cast<float>(CornerSigns[Index].Y);
		const FVector Base(SignX * SignalCornerOffset, SignY * SignalCornerOffset, 0.f);

		Poles.Add(MakePart(FString::Printf(TEXT("Pole_%d"), Index), Cylinder,
			Base + FVector(0.f, 0.f, PoleHeight * 0.5f),
			FVector(0.14f, 0.14f, PoleHeight / 100.f), Housing));
		HeadsNS.Add(MakePart(FString::Printf(TEXT("HeadNS_%d"), Index), Cube,
			Base + FVector(-SignX * 42.f, 0.f, HeadHeight),
			FVector(0.28f, 0.22f, 0.88f), Housing));
		HeadsEW.Add(MakePart(FString::Printf(TEXT("HeadEW_%d"), Index), Cube,
			Base + FVector(0.f, -SignY * 42.f, HeadHeight),
			FVector(0.22f, 0.28f, 0.88f), Housing));

		const FVector NSLens = Base + FVector(-SignX * 66.f, 0.f, 0.f);
		MakeLens(FString::Printf(TEXT("RedLampNS_%d"), Index),
			NSLens + FVector(0.f, 0.f, HeadHeight + 50.f), RedLampsNS);
		MakeLens(FString::Printf(TEXT("AmberLampNS_%d"), Index),
			NSLens + FVector(0.f, 0.f, HeadHeight), AmberLampsNS);
		MakeLens(FString::Printf(TEXT("GreenLampNS_%d"), Index),
			NSLens + FVector(0.f, 0.f, HeadHeight - 50.f), GreenLampsNS);

		const FVector EWLens = Base + FVector(0.f, -SignY * 66.f, 0.f);
		MakeLens(FString::Printf(TEXT("RedLampEW_%d"), Index),
			EWLens + FVector(0.f, 0.f, HeadHeight + 50.f), RedLampsEW);
		MakeLens(FString::Printf(TEXT("AmberLampEW_%d"), Index),
			EWLens + FVector(0.f, 0.f, HeadHeight), AmberLampsEW);
		MakeLens(FString::Printf(TEXT("GreenLampEW_%d"), Index),
			EWLens + FVector(0.f, 0.f, HeadHeight - 50.f), GreenLampsEW);
	}
}

void ASprawlTrafficLight::ResolveIntersectionIndices()
{
	if (IntersectionX >= 0 && IntersectionY >= 0)
	{
		return;
	}

	const FVector Location = GetActorLocation();
	IntersectionX = USprawlCityGridSubsystem::NearestRoadIndex(Location.X);
	IntersectionY = USprawlCityGridSubsystem::NearestRoadIndex(Location.Y);
}

void ASprawlTrafficLight::SnapToIntersection(float GroundZ)
{
	ResolveIntersectionIndices();
	const FVector2D Center = USprawlCityGridSubsystem::IntersectionCenter(
		IntersectionX, IntersectionY);
	SetActorLocation(FVector(Center.X, Center.Y, GroundZ));
}

void ASprawlTrafficLight::BeginPlay()
{
	Super::BeginPlay();
	SnapToIntersection();

	auto CreateMaterials = [](const TArray<TObjectPtr<UStaticMeshComponent>>& Lamps,
		TArray<TObjectPtr<UMaterialInstanceDynamic>>& Materials)
	{
		Materials.Reset();
		Materials.Reserve(Lamps.Num());
		for (UStaticMeshComponent* Lamp : Lamps)
		{
			Materials.Add(Lamp ? Lamp->CreateAndSetMaterialInstanceDynamic(0) : nullptr);
		}
	};
	CreateMaterials(RedLampsNS, RedMatsNS);
	CreateMaterials(AmberLampsNS, AmberMatsNS);
	CreateMaterials(GreenLampsNS, GreenMatsNS);
	CreateMaterials(RedLampsEW, RedMatsEW);
	CreateMaterials(AmberLampsEW, AmberMatsEW);
	CreateMaterials(GreenLampsEW, GreenMatsEW);
}

void ASprawlTrafficLight::ApplyLensColor(UMaterialInstanceDynamic* Mat,
	const FLinearColor& Color, bool bActive) const
{
	if (!Mat)
	{
		return;
	}

	const FLinearColor Output = bActive ? Color : Color * 0.035f;
	Mat->SetVectorParameterValue(TEXT("GlowColor"), Output);
	Mat->SetVectorParameterValue(TEXT("Color"), Output);
}

void ASprawlTrafficLight::ApplySignalState(
	const TArray<TObjectPtr<UMaterialInstanceDynamic>>& RedMats,
	const TArray<TObjectPtr<UMaterialInstanceDynamic>>& AmberMats,
	const TArray<TObjectPtr<UMaterialInstanceDynamic>>& GreenMats,
	ESprawlSignal Signal) const
{
	for (int32 Index = 0; Index < RedMats.Num(); ++Index)
	{
		ApplyLensColor(RedMats[Index], RedLens, Signal == ESprawlSignal::Red);
		if (AmberMats.IsValidIndex(Index))
		{
			ApplyLensColor(AmberMats[Index], AmberLens, Signal == ESprawlSignal::Amber);
		}
		if (GreenMats.IsValidIndex(Index))
		{
			ApplyLensColor(GreenMats[Index], GreenLens, Signal == ESprawlSignal::Green);
		}
	}
}

void ASprawlTrafficLight::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const USprawlCityGridSubsystem* Grid =
		GetWorld() ? GetWorld()->GetSubsystem<USprawlCityGridSubsystem>() : nullptr;
	if (!Grid)
	{
		return;
	}

	const ESprawlSignal NS = Grid->GetSignal(IntersectionX, IntersectionY, true);
	const ESprawlSignal EW = Grid->GetSignal(IntersectionX, IntersectionY, false);
	if (bFirstUpdate || NS != LastNS)
	{
		ApplySignalState(RedMatsNS, AmberMatsNS, GreenMatsNS, NS);
		LastNS = NS;
	}
	if (bFirstUpdate || EW != LastEW)
	{
		ApplySignalState(RedMatsEW, AmberMatsEW, GreenMatsEW, EW);
		LastEW = EW;
	}
	bFirstUpdate = false;
}
