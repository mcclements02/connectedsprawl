// The Connected Sprawl - lightweight enterable city interiors and portals.

#include "World/SprawlEnterableInteriors.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "World/SprawlCityGridSubsystem.h"
#include "World/SprawlInteriorPropLibrary.h"

using Grid = USprawlCityGridSubsystem;

namespace
{
constexpr float InteriorFloorZ = 12000.f;
constexpr float InteriorHeight = 340.f;
constexpr float WallThickness = 30.f;

const FLinearColor StructureTint(0.25f, 0.26f, 0.28f, 1.f);
const FLinearColor FurnitureTint(0.24f, 0.16f, 0.10f, 1.f);
const FLinearColor TextileTint(0.65f, 0.68f, 0.72f, 1.f);
const FLinearColor DarkFixtureTint(0.025f, 0.035f, 0.05f, 1.f);
const FLinearColor StoreTint(0.12f, 0.66f, 0.36f, 1.f);
const FLinearColor OfficeTint(0.12f, 0.42f, 0.86f, 1.f);
const FLinearColor CondoTint(0.92f, 0.55f, 0.18f, 1.f);

void AddBox(TArray<FTransform>& Target, const FVector& Center,
	const FVector& Size, const FRotator& Rotation = FRotator::ZeroRotator)
{
	Target.Emplace(Rotation, Center, Size / 100.f);
}

void AddRoomShell(FSprawlEnterableBuildingSpec& Spec)
{
	const FVector C = Spec.InteriorCenter;
	const float HX = Spec.InteriorHalfExtent.X;
	const float HY = Spec.InteriorHalfExtent.Y;
	AddBox(Spec.Structure, C - FVector(0.f, 0.f, 12.f),
		FVector(HX * 2.f, HY * 2.f, 24.f));
	AddBox(Spec.Structure, C + FVector(0.f, 0.f, InteriorHeight),
		FVector(HX * 2.f, HY * 2.f, 24.f));
	AddBox(Spec.Structure, C + FVector(0.f, HY, InteriorHeight * 0.5f),
		FVector(HX * 2.f, WallThickness, InteriorHeight));
	AddBox(Spec.Structure, C + FVector(0.f, -HY, InteriorHeight * 0.5f),
		FVector(HX * 2.f, WallThickness, InteriorHeight));
	AddBox(Spec.Structure, C + FVector(HX, 0.f, InteriorHeight * 0.5f),
		FVector(WallThickness, HY * 2.f, InteriorHeight));
	AddBox(Spec.Structure, C + FVector(-HX, 0.f, InteriorHeight * 0.5f),
		FVector(WallThickness, HY * 2.f, InteriorHeight));

	// Interior exit door and a warm ceiling strip provide a consistent visual
	// language even when a destination has unique furniture.
	AddBox(Spec.AccentPieces, C + FVector(0.f, -HY + 18.f, 130.f),
		FVector(210.f, 18.f, 260.f));
	AddBox(Spec.AccentPieces, C + FVector(0.f, 0.f, 318.f),
		FVector(760.f, 24.f, 12.f));

	// A shallow, non-colliding facade treatment marks the real sidewalk door
	// without deleting or replacing the authored marketplace building behind it.
	const FVector Outward = Spec.ExteriorFacing.Vector().GetSafeNormal2D();
	FVector Door = Spec.ExteriorDoorLocation;
	Door.Z = 130.f;
	const bool bFacesX = FMath::Abs(Outward.X) > 0.5f;
	const FVector DoorSize = bFacesX
		? FVector(28.f, 220.f, 260.f)
		: FVector(220.f, 28.f, 260.f);
	const FVector CanopySize = bFacesX
		? FVector(150.f, 300.f, 20.f)
		: FVector(300.f, 150.f, 20.f);
	AddBox(Spec.ExteriorPieces, Door, DoorSize);
	AddBox(Spec.ExteriorPieces,
		Door + Outward * 45.f + FVector(0.f, 0.f, 170.f), CanopySize);
	AddBox(Spec.ExteriorPieces,
		Door + FVector(0.f, 0.f, 315.f),
		bFacesX ? FVector(34.f, 330.f, 78.f) : FVector(330.f, 34.f, 78.f));
}

void AddStoreFurniture(FSprawlEnterableBuildingSpec& Spec)
{
	const FVector C = Spec.InteriorCenter;
	// Cabinet checkout counter: segmented fronts, a thin worktop, kickplate,
	// and register read as furniture instead of one stretched collision box.
	for (const float X : { -300.f, 0.f, 300.f })
	{
		AddBox(Spec.Furniture, C + FVector(X, 500.f, 51.f),
			FVector(282.f, 86.f, 102.f));
	}
	AddBox(Spec.Furniture, C + FVector(0.f, 500.f, 106.f),
		FVector(920.f, 112.f, 12.f));
	AddBox(Spec.DarkFixtures, C + FVector(0.f, 452.f, 11.f),
		FVector(900.f, 8.f, 22.f));
	AddBox(Spec.DarkFixtures, C + FVector(260.f, 476.f, 126.f),
		FVector(62.f, 48.f, 28.f), FRotator(0.f, 0.f, -8.f));

	// Two open gondola shelves have thin boards and uprights. Textured product
	// meshes from the prop library populate them at runtime.
	for (const float X : { -860.f, 860.f })
	{
		for (const float Y : { -420.f, 0.f, 420.f })
		{
			AddBox(Spec.Furniture, C + FVector(X, Y, 116.f),
				FVector(18.f, 18.f, 232.f));
		}
		for (const float Z : { 25.f, 86.f, 148.f, 212.f })
		{
			AddBox(Spec.Furniture, C + FVector(X, 0.f, Z),
				FVector(105.f, 900.f, 12.f));
		}
		AddBox(Spec.AccentPieces, C + FVector(X * 0.94f, 0.f, 112.f),
			FVector(8.f, 850.f, 16.f));
	}
}

void AddOfficeFurniture(FSprawlEnterableBuildingSpec& Spec)
{
	const FVector C = Spec.InteriorCenter;
	// Reception and storage stay modular; desks and chairs use authored meshes.
	for (const float X : { -230.f, 0.f, 230.f })
	{
		AddBox(Spec.Furniture, C + FVector(X, 680.f, 50.f),
			FVector(214.f, 75.f, 100.f));
	}
	AddBox(Spec.Furniture, C + FVector(0.f, 680.f, 106.f),
		FVector(680.f, 100.f, 12.f));
	for (const float X : { -920.f, 920.f })
	{
		for (const float Z : { 42.f, 126.f })
		{
			AddBox(Spec.Furniture, C + FVector(X, 220.f, Z),
				FVector(130.f, 260.f, 78.f));
		}
	}
	for (const float X : { -540.f, 540.f })
	{
		for (const float Y : { -30.f, 250.f })
		{
			AddBox(Spec.DarkFixtures, C + FVector(X, Y + 18.f, 132.f),
				FVector(128.f, 10.f, 72.f));
			AddBox(Spec.DarkFixtures, C + FVector(X, Y + 18.f, 91.f),
				FVector(12.f, 38.f, 25.f));
		}
	}
	AddBox(Spec.AccentPieces, C + FVector(0.f, 190.f, 3.f),
		FVector(720.f, 500.f, 6.f));
}

void AddCondoFurniture(FSprawlEnterableBuildingSpec& Spec)
{
	const FVector C = Spec.InteriorCenter;
	// Bed frame, mattress, duvet, pillows, and headboard.
	AddBox(Spec.Furniture, C + FVector(610.f, 330.f, 14.f),
		FVector(680.f, 610.f, 28.f));
	AddBox(Spec.Furniture, C + FVector(610.f, 625.f, 78.f),
		FVector(680.f, 24.f, 156.f));
	AddBox(Spec.TextilePieces, C + FVector(610.f, 320.f, 47.f),
		FVector(625.f, 540.f, 38.f));
	AddBox(Spec.TextilePieces, C + FVector(610.f, 220.f, 71.f),
		FVector(610.f, 325.f, 20.f));
	for (const float X : { 470.f, 750.f })
	{
		AddBox(Spec.TextilePieces, C + FVector(X, 505.f, 73.f),
			FVector(220.f, 95.f, 24.f));
	}

	// Upholstered sofa with separate seat/back/arms and a timber plinth.
	AddBox(Spec.Furniture, C + FVector(-650.f, 250.f, 17.f),
		FVector(175.f, 650.f, 34.f));
	AddBox(Spec.TextilePieces, C + FVector(-630.f, 250.f, 52.f),
		FVector(150.f, 565.f, 38.f));
	AddBox(Spec.TextilePieces, C + FVector(-720.f, 250.f, 105.f),
		FVector(38.f, 565.f, 125.f));
	for (const float Y : { -25.f, 525.f })
	{
		AddBox(Spec.TextilePieces, C + FVector(-640.f, Y, 83.f),
			FVector(170.f, 48.f, 100.f));
	}

	// Three lower cabinets, worktop, backsplash, sink, and cooktop form a real
	// kitchen silhouette instead of one floor-to-ceiling monolith.
	for (const float Y : { -430.f, -170.f, 90.f })
	{
		AddBox(Spec.Furniture, C + FVector(-900.f, Y, 91.f),
			FVector(142.f, 242.f, 182.f));
		AddBox(Spec.DarkFixtures, C + FVector(-826.f, Y, 88.f),
			FVector(6.f, 220.f, 150.f));
	}
	AddBox(Spec.Furniture, C + FVector(-900.f, -170.f, 188.f),
		FVector(175.f, 790.f, 14.f));
	AddBox(Spec.DarkFixtures, C + FVector(-900.f, -300.f, 198.f),
		FVector(105.f, 150.f, 8.f));
	AddBox(Spec.DarkFixtures, C + FVector(-900.f, 25.f, 198.f),
		FVector(110.f, 145.f, 8.f));
}

UMaterialInstanceDynamic* MakeTint(UMaterialInterface* Base, UObject* Owner,
	const FLinearColor& Color, float Roughness)
{
	if (!Base || !Owner)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* Material =
		UMaterialInstanceDynamic::Create(Base, Owner);
	Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
	Material->SetVectorParameterValue(TEXT("Color"), Color);
	Material->SetScalarParameterValue(TEXT("Roughness"), Roughness);
	return Material;
}
}

FBox FSprawlEnterableBuildingSpec::GetInteriorBounds() const
{
	return FBox(
		InteriorCenter + FVector(-InteriorHalfExtent.X, -InteriorHalfExtent.Y, -50.f),
		InteriorCenter + FVector(InteriorHalfExtent.X, InteriorHalfExtent.Y,
			InteriorHeight + 50.f));
}

bool FSprawlEnterableBuildingSpec::IsValid() const
{
	if (Id.IsNone() || DisplayName.IsEmpty()
		|| InteriorHalfExtent.X < 800.f || InteriorHalfExtent.Y < 600.f
		|| Structure.Num() < 6 || Furniture.Num() < 4
		|| AccentPieces.IsEmpty() || ExteriorPieces.Num() < 3)
	{
		return false;
	}
	if (!Grid::IsInsideCityBounds(
			ExteriorReturnLocation.X, ExteriorReturnLocation.Y)
		|| Grid::IsOnRoadSurface(
			ExteriorReturnLocation.X, ExteriorReturnLocation.Y)
		|| Grid::IsOverLake(
			ExteriorReturnLocation.X, ExteriorReturnLocation.Y, 50.f)
		|| !Grid::IsInsideCityBounds(InteriorCenter.X, InteriorCenter.Y)
		|| InteriorCenter.Z < 3000.f)
	{
		return false;
	}
	const FBox Interior = GetInteriorBounds();
	return Interior.IsInsideOrOn(InteriorEntryLocation)
		&& Interior.IsInsideOrOn(InteriorExitLocation)
		&& FVector::DistSquared(InteriorEntryLocation, InteriorExitLocation)
			> FMath::Square(FSprawlEnterableInteriorsLayout::InteractionRadius);
}

FSprawlEnterableInteriorsLayout FSprawlEnterableInteriorsLayout::Build()
{
	FSprawlEnterableInteriorsLayout Layout;

	FSprawlEnterableBuildingSpec Store;
	Store.Id = TEXT("JunctionMarket");
	Store.DisplayName = NSLOCTEXT("Sprawl", "JunctionMarket", "Junction Market");
	Store.Kind = ESprawlBuildingKind::Store;
	Store.ExteriorFacing = FRotator(0.f, 90.f, 0.f);
	Store.ExteriorDoorLocation = FVector(
		Grid::BlockCenter(2), Grid::BlockCenter(3) + 850.f, 120.f);
	Store.ExteriorReturnLocation = FVector(
		Grid::BlockCenter(2), Grid::BlockCenter(3) + 930.f, 120.f);
	Store.InteriorCenter = FVector(-6000.f, -3500.f, InteriorFloorZ);
	Store.InteriorEntryLocation = Store.InteriorCenter + FVector(0.f, -200.f, 110.f);
	Store.InteriorExitLocation = Store.InteriorCenter + FVector(0.f, -650.f, 110.f);
	Store.AccentColor = StoreTint;
	AddRoomShell(Store);
	AddStoreFurniture(Store);
	Layout.Buildings.Add(MoveTemp(Store));

	FSprawlEnterableBuildingSpec Office;
	Office.Id = TEXT("FounderHouse");
	Office.DisplayName = NSLOCTEXT("Sprawl", "FounderHouse", "Founder House Offices");
	Office.Kind = ESprawlBuildingKind::Office;
	Office.ExteriorFacing = FRotator(0.f, 180.f, 0.f);
	Office.ExteriorDoorLocation = FVector(
		Grid::BlockCenter(4) - 850.f, Grid::BlockCenter(4), 120.f);
	Office.ExteriorReturnLocation = FVector(
		Grid::BlockCenter(4) - 930.f, Grid::BlockCenter(4), 120.f);
	Office.InteriorCenter = FVector(0.f, -3500.f, InteriorFloorZ);
	Office.InteriorEntryLocation = Office.InteriorCenter + FVector(0.f, -200.f, 110.f);
	Office.InteriorExitLocation = Office.InteriorCenter + FVector(0.f, -650.f, 110.f);
	Office.AccentColor = OfficeTint;
	AddRoomShell(Office);
	AddOfficeFurniture(Office);
	Layout.Buildings.Add(MoveTemp(Office));

	FSprawlEnterableBuildingSpec Condo;
	Condo.Id = TEXT("CanalViewCondos");
	Condo.DisplayName = NSLOCTEXT("Sprawl", "CanalViewCondos", "Canal View Condos");
	Condo.Kind = ESprawlBuildingKind::Condo;
	Condo.ExteriorFacing = FRotator(0.f, 0.f, 0.f);
	Condo.ExteriorDoorLocation = FVector(
		Grid::BlockCenter(1) + 850.f, Grid::BlockCenter(5), 120.f);
	Condo.ExteriorReturnLocation = FVector(
		Grid::BlockCenter(1) + 930.f, Grid::BlockCenter(5), 120.f);
	Condo.InteriorCenter = FVector(6000.f, -3500.f, InteriorFloorZ);
	Condo.InteriorEntryLocation = Condo.InteriorCenter + FVector(0.f, -200.f, 110.f);
	Condo.InteriorExitLocation = Condo.InteriorCenter + FVector(0.f, -650.f, 110.f);
	Condo.AccentColor = CondoTint;
	AddRoomShell(Condo);
	AddCondoFurniture(Condo);
	Layout.Buildings.Add(MoveTemp(Condo));

	return Layout;
}

bool FSprawlEnterableInteriorsLayout::IsValid() const
{
	if (Buildings.Num() != ExpectedBuildingCount)
	{
		return false;
	}
	TSet<FName> Ids;
	TSet<ESprawlBuildingKind> Kinds;
	for (int32 Index = 0; Index < Buildings.Num(); ++Index)
	{
		const FSprawlEnterableBuildingSpec& Building = Buildings[Index];
		if (!Building.IsValid() || Ids.Contains(Building.Id)
			|| Kinds.Contains(Building.Kind))
		{
			return false;
		}
		Ids.Add(Building.Id);
		Kinds.Add(Building.Kind);
		for (int32 Other = Index + 1; Other < Buildings.Num(); ++Other)
		{
			if (Building.GetInteriorBounds().Intersect(
				Buildings[Other].GetInteriorBounds()))
			{
				return false;
			}
		}
	}
	return Kinds.Num() == ExpectedBuildingCount;
}

const FSprawlEnterableBuildingSpec* FSprawlEnterableInteriorsLayout::FindInterior(
	const FVector& Location) const
{
	for (const FSprawlEnterableBuildingSpec& Building : Buildings)
	{
		if (Building.GetInteriorBounds().IsInsideOrOn(Location))
		{
			return &Building;
		}
	}
	return nullptr;
}

bool FSprawlEnterableInteriorsLayout::FindTransition(const FVector& Location,
	FSprawlInteriorTransition& OutTransition) const
{
	OutTransition = FSprawlInteriorTransition();
	if (const FSprawlEnterableBuildingSpec* Inside = FindInterior(Location))
	{
		if (FVector::DistSquared(Location, Inside->InteriorExitLocation)
			> FMath::Square(InteractionRadius))
		{
			return false;
		}
		OutTransition.BuildingId = Inside->Id;
		OutTransition.BuildingName = Inside->DisplayName;
		OutTransition.Target = FTransform(
			Inside->ExteriorFacing, Inside->ExteriorReturnLocation);
		OutTransition.bEntering = false;
		return true;
	}

	if (Location.Z > 1000.f)
	{
		return false;
	}
	const FSprawlEnterableBuildingSpec* Best = nullptr;
	float BestDistanceSq = FMath::Square(InteractionRadius);
	for (const FSprawlEnterableBuildingSpec& Building : Buildings)
	{
		const float DistanceSq = FVector::DistSquared(
			Location, Building.ExteriorReturnLocation);
		if (DistanceSq <= BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			Best = &Building;
		}
	}
	if (!Best)
	{
		return false;
	}
	OutTransition.BuildingId = Best->Id;
	OutTransition.BuildingName = Best->DisplayName;
	OutTransition.Target = FTransform(
		FRotator(0.f, 90.f, 0.f), Best->InteriorEntryLocation);
	OutTransition.bEntering = true;
	return true;
}

FVector FSprawlEnterableInteriorsLayout::ResolveMapLocation(
	const FVector& Location) const
{
	if (const FSprawlEnterableBuildingSpec* Inside = FindInterior(Location))
	{
		return Inside->ExteriorReturnLocation;
	}
	return Location;
}

ASprawlEnterableInteriors::ASprawlEnterableInteriors()
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

	auto MakeInstances = [this](const TCHAR* Name, bool bCollision,
		bool bCastShadow)
	{
		UHierarchicalInstancedStaticMeshComponent* Component =
			CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(Name);
		Component->SetupAttachment(RootComponent);
		Component->SetCollisionEnabled(bCollision
			? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		if (bCollision)
		{
			Component->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		}
		Component->SetGenerateOverlapEvents(false);
		Component->SetCanEverAffectNavigation(false);
		Component->SetCastShadow(bCastShadow);
		Component->SetReceivesDecals(false);
		if (CubeMesh)
		{
			Component->SetStaticMesh(CubeMesh);
		}
		return Component;
	};

	Structure = MakeInstances(TEXT("InteriorStructure"), true, false);
	Furniture = MakeInstances(TEXT("InteriorFurniture"), true, true);
	Textiles = MakeInstances(TEXT("InteriorTextiles"), true, true);
	DarkFixtures = MakeInstances(TEXT("InteriorDarkFixtures"), false, true);
	StoreAccents = MakeInstances(TEXT("StoreAccents"), false, false);
	OfficeAccents = MakeInstances(TEXT("OfficeAccents"), false, false);
	CondoAccents = MakeInstances(TEXT("CondoAccents"), false, false);
}

void ASprawlEnterableInteriors::BeginPlay()
{
	Super::BeginPlay();
	RuntimeLayout = FSprawlEnterableInteriorsLayout::Build();
	BuildInteriors();
	BuildRealProps();
	BuildLabels();
	UE_LOG(LogTemp, Display,
		TEXT("[EnterableInteriors] valid=%d buildings=%d instances=%d "
			"real_props=%d real_types=%d labels=%d"),
		RuntimeLayout.IsValid(), RuntimeLayout.Buildings.Num(),
		GetBuiltInstanceCount(), BuiltRealPropCount,
		RuntimePropComponents.Num(), RuntimeLabels.Num());
}

void ASprawlEnterableInteriors::BuildInteriors()
{
	if (!RuntimeLayout.IsValid())
	{
		return;
	}
	UMaterialInstanceDynamic* StructureFallback = MakeTint(
		OpaqueBase, this, StructureTint, 0.92f);
	UMaterialInstanceDynamic* FurnitureMaterial = MakeTint(
		OpaqueBase, this, FurnitureTint, 0.78f);
	UMaterialInstanceDynamic* TextileMaterial = MakeTint(
		OpaqueBase, this, TextileTint, 0.96f);
	UMaterialInstanceDynamic* DarkFixtureMaterial = MakeTint(
		OpaqueBase, this, DarkFixtureTint, 0.38f);
	UMaterialInstanceDynamic* StoreMaterial = MakeTint(
		OpaqueBase, this, StoreTint, 0.58f);
	UMaterialInstanceDynamic* OfficeMaterial = MakeTint(
		OpaqueBase, this, OfficeTint, 0.52f);
	UMaterialInstanceDynamic* CondoMaterial = MakeTint(
		OpaqueBase, this, CondoTint, 0.62f);
	for (UMaterialInstanceDynamic* Material : {
		StructureFallback, FurnitureMaterial, TextileMaterial,
		DarkFixtureMaterial, StoreMaterial,
		OfficeMaterial, CondoMaterial })
	{
		if (Material)
		{
			RuntimeMaterials.Add(Material);
		}
	}

	if (Structure)
	{
		Structure->SetMaterial(0, ConcreteMaterial
			? ConcreteMaterial.Get() : StructureFallback);
	}
	if (Furniture && FurnitureMaterial)
	{
		Furniture->SetMaterial(0, FurnitureMaterial);
	}
	if (Textiles && TextileMaterial)
	{
		Textiles->SetMaterial(0, TextileMaterial);
	}
	if (DarkFixtures && DarkFixtureMaterial)
	{
		DarkFixtures->SetMaterial(0, DarkFixtureMaterial);
	}
	if (StoreAccents && StoreMaterial) StoreAccents->SetMaterial(0, StoreMaterial);
	if (OfficeAccents && OfficeMaterial) OfficeAccents->SetMaterial(0, OfficeMaterial);
	if (CondoAccents && CondoMaterial) CondoAccents->SetMaterial(0, CondoMaterial);

	for (const FSprawlEnterableBuildingSpec& Building : RuntimeLayout.Buildings)
	{
		for (const FTransform& Transform : Building.Structure)
		{
			Structure->AddInstance(Transform);
		}
		for (const FTransform& Transform : Building.Furniture)
		{
			Furniture->AddInstance(Transform);
		}
		for (const FTransform& Transform : Building.TextilePieces)
		{
			Textiles->AddInstance(Transform);
		}
		for (const FTransform& Transform : Building.DarkFixtures)
		{
			DarkFixtures->AddInstance(Transform);
		}
		UHierarchicalInstancedStaticMeshComponent* AccentComponent = nullptr;
		switch (Building.Kind)
		{
		case ESprawlBuildingKind::Store: AccentComponent = StoreAccents; break;
		case ESprawlBuildingKind::Office: AccentComponent = OfficeAccents; break;
		default: AccentComponent = CondoAccents; break;
		}
		if (AccentComponent)
		{
			for (const FTransform& Transform : Building.AccentPieces)
			{
				AccentComponent->AddInstance(Transform);
			}
			for (const FTransform& Transform : Building.ExteriorPieces)
			{
				AccentComponent->AddInstance(Transform);
			}
		}
	}
}

void ASprawlEnterableInteriors::BuildRealProps()
{
	BuiltRealPropCount = 0;
	if (!RuntimeLayout.IsValid())
	{
		return;
	}
	for (const FSprawlInteriorPropDefinition& Definition :
		FSprawlInteriorPropLibrary::GetDefinitions())
	{
		TArray<FSprawlInteriorPropPlacement> Placements;
		for (const FSprawlEnterableBuildingSpec& Building : RuntimeLayout.Buildings)
		{
			for (const FSprawlInteriorPropPlacement& Placement :
				FSprawlInteriorPropLibrary::BuildForBuilding(
					Building.Kind, Building.InteriorCenter))
			{
				if (Placement.Type == Definition.Type)
				{
					Placements.Add(Placement);
				}
			}
		}
		if (Placements.IsEmpty())
		{
			continue;
		}

		FString ResolvedPath;
		UStaticMesh* Mesh = FSprawlInteriorPropLibrary::ResolveMesh(
			Definition.Type, &ResolvedPath);
		if (!Mesh)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[InteriorProps] missing type=%s; modular fallback retained"),
				*Definition.Id.ToString());
			continue;
		}

		UHierarchicalInstancedStaticMeshComponent* Component =
			NewObject<UHierarchicalInstancedStaticMeshComponent>(
				this, FName(*FString::Printf(TEXT("InteriorProp_%s"),
					*Definition.Id.ToString())));
		AddInstanceComponent(Component);
		Component->SetupAttachment(RootComponent);
		Component->SetStaticMesh(Mesh);
		Component->SetCollisionEnabled(Definition.bCollision
			? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		if (Definition.bCollision)
		{
			Component->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		}
		Component->SetGenerateOverlapEvents(false);
		Component->SetCanEverAffectNavigation(false);
		Component->SetCastShadow(Definition.bCastShadow);
		Component->SetReceivesDecals(true);
		Component->SetCullDistances(0, 7000);
		Component->RegisterComponent();
		for (const FSprawlInteriorPropPlacement& Placement : Placements)
		{
			Component->AddInstance(
				FSprawlInteriorPropLibrary::FitMeshToTarget(
					Placement.Target, Mesh));
			++BuiltRealPropCount;
		}
		RuntimePropMeshes.Add(Mesh);
		RuntimePropComponents.Add(Component);
		UE_LOG(LogTemp, Display,
			TEXT("[InteriorProps] resolved type=%s instances=%d asset=%s"),
			*Definition.Id.ToString(), Placements.Num(), *ResolvedPath);
	}
}

void ASprawlEnterableInteriors::BuildLabels()
{
	if (!RuntimeLayout.IsValid())
	{
		return;
	}
	for (const FSprawlEnterableBuildingSpec& Building : RuntimeLayout.Buildings)
	{
		UTextRenderComponent* ExteriorLabel = NewObject<UTextRenderComponent>(this);
		AddInstanceComponent(ExteriorLabel);
		ExteriorLabel->SetupAttachment(RootComponent);
		ExteriorLabel->SetText(Building.DisplayName);
		ExteriorLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
		ExteriorLabel->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
		ExteriorLabel->SetTextRenderColor(Building.AccentColor.ToFColor(true));
		ExteriorLabel->SetWorldSize(42.f);
		ExteriorLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ExteriorLabel->SetCastShadow(false);
		ExteriorLabel->RegisterComponent();
		ExteriorLabel->SetWorldLocation(
			Building.ExteriorDoorLocation + FVector(0.f, 0.f, 315.f));
		ExteriorLabel->SetWorldRotation(Building.ExteriorFacing);
		RuntimeLabels.Add(ExteriorLabel);

		UTextRenderComponent* ExitLabel = NewObject<UTextRenderComponent>(this);
		AddInstanceComponent(ExitLabel);
		ExitLabel->SetupAttachment(RootComponent);
		ExitLabel->SetText(NSLOCTEXT("Sprawl", "InteriorExit", "E  EXIT"));
		ExitLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
		ExitLabel->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
		ExitLabel->SetTextRenderColor(Building.AccentColor.ToFColor(true));
		ExitLabel->SetWorldSize(36.f);
		ExitLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ExitLabel->SetCastShadow(false);
		ExitLabel->RegisterComponent();
		ExitLabel->SetWorldLocation(
			Building.InteriorExitLocation + FVector(0.f, 14.f, 155.f));
		ExitLabel->SetWorldRotation(FRotator(0.f, 90.f, 0.f));
		RuntimeLabels.Add(ExitLabel);
	}
}

bool ASprawlEnterableInteriors::TryInteract(ACharacter* Character)
{
	if (!Character || !RuntimeLayout.IsValid())
	{
		return false;
	}
	FSprawlInteriorTransition Transition;
	if (!RuntimeLayout.FindTransition(Character->GetActorLocation(), Transition))
	{
		return false;
	}
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (const double* LastTime = LastTransitionTimes.Find(Character);
		LastTime && Now - *LastTime < 0.35)
	{
		return true;
	}
	LastTransitionTimes.Add(Character, Now);
	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_Walking);
	}
	Character->SetActorLocationAndRotation(
		Transition.Target.GetLocation(), Transition.Target.Rotator(), false,
		nullptr, ETeleportType::TeleportPhysics);
	if (AController* Controller = Character->GetController())
	{
		Controller->SetControlRotation(Transition.Target.Rotator());
	}
	const FString Message = FString::Printf(TEXT("%s %s"),
		Transition.bEntering ? TEXT("Entered") : TEXT("Exited"),
		*Transition.BuildingName.ToString());
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			static_cast<uint64>(0x1A7E210), 2.f, FColor::White, Message);
	}
	UE_LOG(LogTemp, Display, TEXT("[EnterableInteriors] %s building=%s pawn=%s"),
		Transition.bEntering ? TEXT("enter") : TEXT("exit"),
		*Transition.BuildingId.ToString(), *Character->GetName());
	return true;
}

bool ASprawlEnterableInteriors::GetInteractionPrompt(
	const AActor* Actor, FText& OutPrompt) const
{
	OutPrompt = FText::GetEmpty();
	if (!Actor)
	{
		return false;
	}
	FSprawlInteriorTransition Transition;
	if (!RuntimeLayout.FindTransition(Actor->GetActorLocation(), Transition))
	{
		return false;
	}
	OutPrompt = FText::Format(
		Transition.bEntering
			? NSLOCTEXT("Sprawl", "EnterBuildingPrompt", "Enter {0}")
			: NSLOCTEXT("Sprawl", "ExitBuildingPrompt", "Exit to {0}"),
		Transition.BuildingName);
	return true;
}

int32 ASprawlEnterableInteriors::GetBuiltInstanceCount() const
{
	int32 Count = 0;
	for (const UHierarchicalInstancedStaticMeshComponent* Component : {
		Structure.Get(), Furniture.Get(), Textiles.Get(), DarkFixtures.Get(),
		StoreAccents.Get(),
		OfficeAccents.Get(), CondoAccents.Get() })
	{
		Count += Component ? Component->GetInstanceCount() : 0;
	}
	for (const UHierarchicalInstancedStaticMeshComponent* Component :
		RuntimePropComponents)
	{
		Count += Component ? Component->GetInstanceCount() : 0;
	}
	return Count;
}

ASprawlEnterableInteriors* ASprawlEnterableInteriors::FindForWorld(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	TActorIterator<ASprawlEnterableInteriors> Existing(World);
	return Existing ? *Existing : nullptr;
}

ASprawlEnterableInteriors* ASprawlEnterableInteriors::EnsureForWorld(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	if (ASprawlEnterableInteriors* Existing = FindForWorld(World))
	{
		return Existing;
	}
	FActorSpawnParameters Params;
	Params.Name = TEXT("SprawlEnterableInteriors");
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<ASprawlEnterableInteriors>(
		FVector::ZeroVector, FRotator::ZeroRotator, Params);
}
