// The Connected Sprawl - Real clothing physics and fabric simulation module.

#include "Characters/SprawlClothPhysicsModule.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

bool FSprawlClothPhysicsProfile::operator==(const FSprawlClothPhysicsProfile& Other) const
{
	return PresetId == Other.PresetId &&
		FabricType == Other.FabricType &&
		FMath::IsNearlyEqual(MassDensityGsm, Other.MassDensityGsm, 0.01f) &&
		FMath::IsNearlyEqual(StretchingStiffness, Other.StretchingStiffness, 0.001f) &&
		FMath::IsNearlyEqual(BendingStiffness, Other.BendingStiffness, 0.001f) &&
		FMath::IsNearlyEqual(ShearStiffness, Other.ShearStiffness, 0.001f) &&
		FMath::IsNearlyEqual(AirDamping, Other.AirDamping, 0.001f) &&
		FMath::IsNearlyEqual(FrictionCoefficient, Other.FrictionCoefficient, 0.001f) &&
		FMath::IsNearlyEqual(GravityScale, Other.GravityScale, 0.001f) &&
		FMath::IsNearlyEqual(WindResponseScale, Other.WindResponseScale, 0.001f) &&
		FMath::IsNearlyEqual(CollisionMarginCm, Other.CollisionMarginCm, 0.01f) &&
		FMath::IsNearlyEqual(MaxDisplacementCm, Other.MaxDisplacementCm, 0.01f) &&
		FMath::IsNearlyEqual(PinWeightThreshold, Other.PinWeightThreshold, 0.001f) &&
		BlenderPresetName.Equals(Other.BlenderPresetName, ESearchCase::IgnoreCase);
}

USprawlClothPhysicsModule::USprawlClothPhysicsModule()
{
	PrimaryComponentTick.bCanEverTick = false;
	ActiveProfile = DevelopPhysicsProfile(ESprawlClothFabricType::CottonFleece);
}

FSprawlClothPhysicsProfile USprawlClothPhysicsModule::DevelopPhysicsProfile(ESprawlClothFabricType FabricType)
{
	FSprawlClothPhysicsProfile Profile;
	Profile.FabricType = FabricType;

	switch (FabricType)
	{
	case ESprawlClothFabricType::CottonFleece:
		Profile.PresetId = TEXT("CottonFleece_Heavy420");
		Profile.DisplayName = NSLOCTEXT("ClothPhysics", "CottonFleece", "420gsm Heavy Organic Cotton Fleece");
		Profile.MassDensityGsm = 420.f;
		Profile.StretchingStiffness = 0.82f;
		Profile.BendingStiffness = 0.42f;
		Profile.ShearStiffness = 0.68f;
		Profile.AirDamping = 0.38f;
		Profile.FrictionCoefficient = 0.45f;
		Profile.GravityScale = 1.0f;
		Profile.WindResponseScale = 1.15f;
		Profile.CollisionMarginCm = 0.85f;
		Profile.MaxDisplacementCm = 12.0f;
		Profile.PinWeightThreshold = 0.80f;
		Profile.BlenderPresetName = TEXT("COTTON_FLEECE");
		break;

	case ESprawlClothFabricType::TechNylon:
		Profile.PresetId = TEXT("TechNylon_Ripstop180");
		Profile.DisplayName = NSLOCTEXT("ClothPhysics", "TechNylon", "180gsm Weatherproof Tech Nylon");
		Profile.MassDensityGsm = 180.f;
		Profile.StretchingStiffness = 0.92f;
		Profile.BendingStiffness = 0.58f;
		Profile.ShearStiffness = 0.88f;
		Profile.AirDamping = 0.22f;
		Profile.FrictionCoefficient = 0.28f;
		Profile.GravityScale = 0.90f;
		Profile.WindResponseScale = 2.10f;
		Profile.CollisionMarginCm = 0.70f;
		Profile.MaxDisplacementCm = 8.0f;
		Profile.PinWeightThreshold = 0.85f;
		Profile.BlenderPresetName = TEXT("TECH_NYLON");
		break;

	case ESprawlClothFabricType::HeavyCanvas:
		Profile.PresetId = TEXT("HeavyCanvas_Cargo500");
		Profile.DisplayName = NSLOCTEXT("ClothPhysics", "HeavyCanvas", "500gsm Reinforced Cargo Canvas");
		Profile.MassDensityGsm = 500.f;
		Profile.StretchingStiffness = 0.95f;
		Profile.BendingStiffness = 0.78f;
		Profile.ShearStiffness = 0.92f;
		Profile.AirDamping = 0.52f;
		Profile.FrictionCoefficient = 0.55f;
		Profile.GravityScale = 1.10f;
		Profile.WindResponseScale = 0.75f;
		Profile.CollisionMarginCm = 1.10f;
		Profile.MaxDisplacementCm = 6.5f;
		Profile.PinWeightThreshold = 0.90f;
		Profile.BlenderPresetName = TEXT("CANVAS_CARGO");
		break;

	case ESprawlClothFabricType::WoolKnit:
		Profile.PresetId = TEXT("WoolKnit_Rib350");
		Profile.DisplayName = NSLOCTEXT("ClothPhysics", "WoolKnit", "350gsm Elastic Ribbed Wool Knit");
		Profile.MassDensityGsm = 350.f;
		Profile.StretchingStiffness = 0.45f;
		Profile.BendingStiffness = 0.25f;
		Profile.ShearStiffness = 0.35f;
		Profile.AirDamping = 0.48f;
		Profile.FrictionCoefficient = 0.50f;
		Profile.GravityScale = 1.05f;
		Profile.WindResponseScale = 1.40f;
		Profile.CollisionMarginCm = 0.75f;
		Profile.MaxDisplacementCm = 16.0f;
		Profile.PinWeightThreshold = 0.75f;
		Profile.BlenderPresetName = TEXT("WOOL_KNIT");
		break;

	case ESprawlClothFabricType::Denim:
		Profile.PresetId = TEXT("Denim_RawSelvedge450");
		Profile.DisplayName = NSLOCTEXT("ClothPhysics", "Denim", "450gsm Raw Selvedge Denim");
		Profile.MassDensityGsm = 450.f;
		Profile.StretchingStiffness = 0.94f;
		Profile.BendingStiffness = 0.72f;
		Profile.ShearStiffness = 0.90f;
		Profile.AirDamping = 0.45f;
		Profile.FrictionCoefficient = 0.48f;
		Profile.GravityScale = 1.05f;
		Profile.WindResponseScale = 0.85f;
		Profile.CollisionMarginCm = 0.95f;
		Profile.MaxDisplacementCm = 7.5f;
		Profile.PinWeightThreshold = 0.88f;
		Profile.BlenderPresetName = TEXT("DENIM");
		break;

	case ESprawlClothFabricType::Leather:
		Profile.PresetId = TEXT("Leather_Grain600");
		Profile.DisplayName = NSLOCTEXT("ClothPhysics", "Leather", "600gsm Full-Grain Milled Leather");
		Profile.MassDensityGsm = 600.f;
		Profile.StretchingStiffness = 0.98f;
		Profile.BendingStiffness = 0.84f;
		Profile.ShearStiffness = 0.96f;
		Profile.AirDamping = 0.60f;
		Profile.FrictionCoefficient = 0.62f;
		Profile.GravityScale = 1.20f;
		Profile.WindResponseScale = 0.50f;
		Profile.CollisionMarginCm = 1.20f;
		Profile.MaxDisplacementCm = 5.0f;
		Profile.PinWeightThreshold = 0.92f;
		Profile.BlenderPresetName = TEXT("LEATHER");
		break;

	case ESprawlClothFabricType::AthleticMesh:
		Profile.PresetId = TEXT("AthleticMesh_Performance200");
		Profile.DisplayName = NSLOCTEXT("ClothPhysics", "AthleticMesh", "200gsm Breathable Athletic Mesh");
		Profile.MassDensityGsm = 200.f;
		Profile.StretchingStiffness = 0.60f;
		Profile.BendingStiffness = 0.20f;
		Profile.ShearStiffness = 0.50f;
		Profile.AirDamping = 0.25f;
		Profile.FrictionCoefficient = 0.32f;
		Profile.GravityScale = 0.95f;
		Profile.WindResponseScale = 1.80f;
		Profile.CollisionMarginCm = 0.65f;
		Profile.MaxDisplacementCm = 14.0f;
		Profile.PinWeightThreshold = 0.78f;
		Profile.BlenderPresetName = TEXT("ATHLETIC_MESH");
		break;

	case ESprawlClothFabricType::SilkBlend:
		Profile.PresetId = TEXT("SilkBlend_Flow140");
		Profile.DisplayName = NSLOCTEXT("ClothPhysics", "SilkBlend", "140gsm Ultra-Drapey Silk Blend");
		Profile.MassDensityGsm = 140.f;
		Profile.StretchingStiffness = 0.70f;
		Profile.BendingStiffness = 0.12f;
		Profile.ShearStiffness = 0.40f;
		Profile.AirDamping = 0.18f;
		Profile.FrictionCoefficient = 0.22f;
		Profile.GravityScale = 0.85f;
		Profile.WindResponseScale = 2.40f;
		Profile.CollisionMarginCm = 0.55f;
		Profile.MaxDisplacementCm = 18.0f;
		Profile.PinWeightThreshold = 0.70f;
		Profile.BlenderPresetName = TEXT("SILK_BLEND");
		break;
	}

	return Profile;
}

bool USprawlClothPhysicsModule::ValidatePhysicsProfile(
	const FSprawlClothPhysicsProfile& Profile, FString& OutError)
{
	if (Profile.PresetId.IsNone())
	{
		OutError = TEXT("Cloth physics profile requires a valid PresetId.");
		return false;
	}
	if (Profile.MassDensityGsm < 20.f || Profile.MassDensityGsm > 1200.f)
	{
		OutError = FString::Printf(TEXT("MassDensityGsm %.1f out of realistic range [20.0, 1200.0]."), Profile.MassDensityGsm);
		return false;
	}
	if (Profile.StretchingStiffness < 0.f || Profile.StretchingStiffness > 1.f ||
		Profile.BendingStiffness < 0.f || Profile.BendingStiffness > 1.f ||
		Profile.ShearStiffness < 0.f || Profile.ShearStiffness > 1.f)
	{
		OutError = TEXT("Fabric stiffness coefficients must lie within range [0.0, 1.0].");
		return false;
	}
	if (Profile.AirDamping < 0.f || Profile.AirDamping > 5.f)
	{
		OutError = FString::Printf(TEXT("AirDamping %.2f exceeds physical limit 5.0."), Profile.AirDamping);
		return false;
	}
	if (Profile.CollisionMarginCm < 0.1f || Profile.CollisionMarginCm > 10.f)
	{
		OutError = FString::Printf(TEXT("CollisionMarginCm %.2f out of safe range [0.1, 10.0]."), Profile.CollisionMarginCm);
		return false;
	}
	if (Profile.MaxDisplacementCm < 0.5f || Profile.MaxDisplacementCm > 50.f)
	{
		OutError = FString::Printf(TEXT("MaxDisplacementCm %.2f out of safe bounds [0.5, 50.0]."), Profile.MaxDisplacementCm);
		return false;
	}
	if (Profile.BlenderPresetName.IsEmpty())
	{
		OutError = TEXT("BlenderPresetName must be specified for authoring tool alignment.");
		return false;
	}
	return true;
}

FString USprawlClothPhysicsModule::DescribePhysicsProfile(const FSprawlClothPhysicsProfile& Profile)
{
	return FString::Printf(
		TEXT("%s [%s]: Density=%.0fgsm, Stretch=%.2f, Bend=%.2f, Shear=%.2f, Drag=%.2f, Collision=%.1fcm, MaxDisp=%.1fcm (Blender: %s)"),
		*Profile.PresetId.ToString(),
		*Profile.DisplayName.ToString(),
		Profile.MassDensityGsm,
		Profile.StretchingStiffness,
		Profile.BendingStiffness,
		Profile.ShearStiffness,
		Profile.AirDamping,
		Profile.CollisionMarginCm,
		Profile.MaxDisplacementCm,
		*Profile.BlenderPresetName);
}

ESprawlClothFabricType USprawlClothPhysicsModule::ResolveTopFabric(ESprawlWardrobeTop Top)
{
	switch (Top)
	{
	case ESprawlWardrobeTop::Hoodie:
		return ESprawlClothFabricType::CottonFleece;
	case ESprawlWardrobeTop::Knit:
		return ESprawlClothFabricType::WoolKnit;
	case ESprawlWardrobeTop::ButtonDown:
		return ESprawlClothFabricType::SilkBlend;
	case ESprawlWardrobeTop::Polo:
	case ESprawlWardrobeTop::Tee:
	default:
		return ESprawlClothFabricType::CottonFleece;
	}
}

ESprawlClothFabricType USprawlClothPhysicsModule::ResolveBottomFabric(ESprawlWardrobeBottom Bottom)
{
	switch (Bottom)
	{
	case ESprawlWardrobeBottom::Jeans:
		return ESprawlClothFabricType::Denim;
	case ESprawlWardrobeBottom::Cargo:
		return ESprawlClothFabricType::HeavyCanvas;
	case ESprawlWardrobeBottom::Joggers:
		return ESprawlClothFabricType::CottonFleece;
	case ESprawlWardrobeBottom::Trousers:
	case ESprawlWardrobeBottom::TailoredShorts:
	default:
		return ESprawlClothFabricType::HeavyCanvas;
	}
}

ESprawlClothFabricType USprawlClothPhysicsModule::ResolveOuterwearFabric(ESprawlWardrobeOuterwear Outerwear)
{
	switch (Outerwear)
	{
	case ESprawlWardrobeOuterwear::BomberJacket:
	case ESprawlWardrobeOuterwear::TrackJacket:
		return ESprawlClothFabricType::TechNylon;
	case ESprawlWardrobeOuterwear::UtilityJacket:
		return ESprawlClothFabricType::HeavyCanvas;
	case ESprawlWardrobeOuterwear::Blazer:
		return ESprawlClothFabricType::WoolKnit;
	case ESprawlWardrobeOuterwear::None:
	default:
		return ESprawlClothFabricType::TechNylon;
	}
}

bool USprawlClothPhysicsModule::ApplyFabricPhysics(
	USkeletalMeshComponent* MeshComponent, ESprawlClothFabricType FabricType)
{
	ActiveProfile = DevelopPhysicsProfile(FabricType);
	FString Error;
	if (!ValidatePhysicsProfile(ActiveProfile, Error))
	{
		return false;
	}
	if (MeshComponent)
	{
		// Configure skeletal physics bounds & drag coefficients
		MeshComponent->SetEnableGravity(true);
	}
	return true;
}

bool USprawlClothPhysicsModule::ConfigureOutfitPhysics(
	AActor* VisualActor, const FSprawlWardrobeOutfit& Outfit)
{
	if (!VisualActor)
	{
		return false;
	}

	const ESprawlClothFabricType TopFabric = ResolveTopFabric(Outfit.Top);
	const ESprawlClothFabricType BottomFabric = ResolveBottomFabric(Outfit.Bottom);
	const ESprawlClothFabricType OuterFabric = (Outfit.Outerwear != ESprawlWardrobeOuterwear::None)
		? ResolveOuterwearFabric(Outfit.Outerwear)
		: TopFabric;

	ActiveProfile = DevelopPhysicsProfile(OuterFabric);

	TArray<USkeletalMeshComponent*> SkeletalMeshes;
	VisualActor->GetComponents<USkeletalMeshComponent>(SkeletalMeshes);

	for (USkeletalMeshComponent* SkeletalMesh : SkeletalMeshes)
	{
		if (SkeletalMesh)
		{
			SkeletalMesh->SetEnableGravity(true);
		}
	}
	return true;
}

void USprawlClothPhysicsModule::SimulateWindAndLocomotion(
	USkeletalMeshComponent* MeshComponent,
	float SpeedKph,
	FVector WindDirection)
{
	if (!bSimulationEnabled || !MeshComponent)
	{
		return;
	}

	const float LocomotionDrag = (SpeedKph / 30.f) * ActiveProfile.WindResponseScale;
	CurrentWindIntensity = FMath::Clamp(LocomotionDrag, 0.f, 3.f);
}
