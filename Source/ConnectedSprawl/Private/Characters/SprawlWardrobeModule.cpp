// The Connected Sprawl - Deterministic, reusable MetaHuman wardrobe layers.

#include "Characters/SprawlWardrobeModule.h"

#include "Characters/SprawlFootwearModule.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "ProceduralMeshComponent.h"

#include <initializer_list>

namespace
{
struct FSprawlWardrobePalette
{
	FLinearColor Top;
	FLinearColor Bottom;
	FLinearColor Outerwear;
	FLinearColor Accent;
	FLinearColor Shoes;
	FLinearColor Socks;
};

struct FSprawlAccessoryMesh
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
};

void AddQuad(
	FSprawlAccessoryMesh& Mesh,
	FVector A, FVector B, FVector C, FVector D,
	const FVector& DesiredNormal)
{
	FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
	if (FVector::DotProduct(Normal, DesiredNormal) < 0.f)
	{
		Swap(B, D);
		Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
	}
	const int32 Base = Mesh.Vertices.Num();
	Mesh.Vertices.Append({A, B, C, D});
	Mesh.Triangles.Append({Base, Base + 1, Base + 2, Base, Base + 2, Base + 3});
	Mesh.Normals.Append({Normal, Normal, Normal, Normal});
	Mesh.UV0.Append({
		FVector2D(0.f, 1.f), FVector2D(1.f, 1.f),
		FVector2D(1.f, 0.f), FVector2D(0.f, 0.f)});
}

FSprawlAccessoryMesh BuildHatCrown(bool bBeanie)
{
	struct FRing { float Z; float Radius; };
	const FRing CapRings[] = {
		{-3.0f, 8.7f}, {-0.5f, 8.9f}, {3.0f, 8.2f}, {6.5f, 6.2f}, {8.5f, 1.0f}};
	const FRing BeanieRings[] = {
		{-3.5f, 8.7f}, {-0.5f, 9.0f}, {4.0f, 8.2f}, {8.0f, 5.4f}, {10.5f, 0.9f}};
	const FRing* Rings = bBeanie ? BeanieRings : CapRings;
	const int32 RingCount = bBeanie
		? UE_ARRAY_COUNT(BeanieRings) : UE_ARRAY_COUNT(CapRings);
	constexpr int32 Sides = 14;
	FSprawlAccessoryMesh Result;
	for (int32 RingIndex = 0; RingIndex < RingCount - 1; ++RingIndex)
	{
		for (int32 Side = 0; Side < Sides; ++Side)
		{
			const float A0 = 2.f * PI * static_cast<float>(Side) / Sides;
			const float A1 = 2.f * PI * static_cast<float>(Side + 1) / Sides;
			const FVector Bottom0(
				FMath::Cos(A0) * Rings[RingIndex].Radius,
				FMath::Sin(A0) * Rings[RingIndex].Radius,
				Rings[RingIndex].Z);
			const FVector Bottom1(
				FMath::Cos(A1) * Rings[RingIndex].Radius,
				FMath::Sin(A1) * Rings[RingIndex].Radius,
				Rings[RingIndex].Z);
			const FVector Top1(
				FMath::Cos(A1) * Rings[RingIndex + 1].Radius,
				FMath::Sin(A1) * Rings[RingIndex + 1].Radius,
				Rings[RingIndex + 1].Z);
			const FVector Top0(
				FMath::Cos(A0) * Rings[RingIndex + 1].Radius,
				FMath::Sin(A0) * Rings[RingIndex + 1].Radius,
				Rings[RingIndex + 1].Z);
			const float Mid = (A0 + A1) * 0.5f;
			AddQuad(Result, Bottom0, Bottom1, Top1, Top0,
				FVector(FMath::Cos(Mid), FMath::Sin(Mid), 0.35f));
		}
	}
	return Result;
}

FSprawlAccessoryMesh BuildCapBrim()
{
	FSprawlAccessoryMesh Result;
	const FVector Min(-2.f, -7.2f, -0.8f);
	const FVector Max(13.f, 7.2f, 0.8f);
	const FVector A(Min.X, Min.Y, Min.Z);
	const FVector B(Max.X, Min.Y, Min.Z);
	const FVector C(Max.X, Max.Y, Min.Z);
	const FVector D(Min.X, Max.Y, Min.Z);
	const FVector E(Min.X, Min.Y, Max.Z);
	const FVector F(Max.X, Min.Y, Max.Z);
	const FVector G(Max.X, Max.Y, Max.Z);
	const FVector H(Min.X, Max.Y, Max.Z);
	AddQuad(Result, A, D, C, B, -FVector::UpVector);
	AddQuad(Result, E, F, G, H, FVector::UpVector);
	AddQuad(Result, A, B, F, E, -FVector::RightVector);
	AddQuad(Result, D, H, G, C, FVector::RightVector);
	AddQuad(Result, A, E, H, D, -FVector::ForwardVector);
	AddQuad(Result, B, C, G, F, FVector::ForwardVector);
	return Result;
}

/** One cross-section of a garment loft, in the garment's local frame. */
struct FSprawlGarmentRing
{
	float Height = 0.f;     // along local +Z, up the body or out the limb
	float HalfWidth = 0.f;  // along local +Y, across the body
	float HalfDepth = 0.f;  // along local +X, front to back
};

void CalculateGarmentNormals(FSprawlAccessoryMesh& Mesh)
{
	Mesh.Normals.Init(FVector::ZeroVector, Mesh.Vertices.Num());
	for (int32 TriangleIndex = 0;
		TriangleIndex + 2 < Mesh.Triangles.Num(); TriangleIndex += 3)
	{
		const int32 A = Mesh.Triangles[TriangleIndex];
		const int32 B = Mesh.Triangles[TriangleIndex + 1];
		const int32 C = Mesh.Triangles[TriangleIndex + 2];
		if (!Mesh.Vertices.IsValidIndex(A) || !Mesh.Vertices.IsValidIndex(B)
			|| !Mesh.Vertices.IsValidIndex(C))
		{
			continue;
		}
		const FVector FaceNormal = FVector::CrossProduct(
			Mesh.Vertices[B] - Mesh.Vertices[A],
			Mesh.Vertices[C] - Mesh.Vertices[A]).GetSafeNormal();
		Mesh.Normals[A] += FaceNormal;
		Mesh.Normals[B] += FaceNormal;
		Mesh.Normals[C] += FaceNormal;
	}
	for (FVector& Normal : Mesh.Normals)
	{
		Normal = Normal.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
	}
}

/**
 * Loft an open-ended elliptical tube through the supplied rings. Garments are
 * open at both ends because the body passes through them; the single-sided
 * material hides the interior from outside.
 */
FSprawlAccessoryMesh BuildLoftedShell(
	const TArray<FSprawlGarmentRing>& Rings, int32 Sides)
{
	FSprawlAccessoryMesh Result;
	if (Rings.Num() < 2 || Sides < 3)
	{
		return Result;
	}
	const float LastRing = static_cast<float>(Rings.Num() - 1);
	Result.Vertices.Reserve(Rings.Num() * Sides);
	Result.UV0.Reserve(Rings.Num() * Sides);
	for (int32 RingIndex = 0; RingIndex < Rings.Num(); ++RingIndex)
	{
		for (int32 Side = 0; Side < Sides; ++Side)
		{
			const float Alpha = static_cast<float>(Side) / Sides;
			const float Angle = 2.f * PI * Alpha;
			Result.Vertices.Add(FVector(
				FMath::Cos(Angle) * Rings[RingIndex].HalfDepth,
				FMath::Sin(Angle) * Rings[RingIndex].HalfWidth,
				Rings[RingIndex].Height));
			Result.UV0.Add(FVector2D(
				Alpha, static_cast<float>(RingIndex) / LastRing));
		}
	}
	for (int32 RingIndex = 0; RingIndex + 1 < Rings.Num(); ++RingIndex)
	{
		for (int32 Side = 0; Side < Sides; ++Side)
		{
			const int32 Next = (Side + 1) % Sides;
			const int32 A = RingIndex * Sides + Side;
			const int32 B = (RingIndex + 1) * Sides + Side;
			const int32 C = (RingIndex + 1) * Sides + Next;
			const int32 D = RingIndex * Sides + Next;
			// Rings wind counter-clockwise, so this order faces outward.
			Result.Triangles.Append({A, D, C, A, C, B});
		}
	}
	CalculateGarmentNormals(Result);
	return Result;
}

/** A sleeve is a round loft: equal width and depth at every ring. */
FSprawlAccessoryMesh BuildSleeve(
	float LengthCm, float StartRadiusCm, float EndRadiusCm, float OverlapCm)
{
	const TArray<FSprawlGarmentRing> Rings = {
		{-OverlapCm, StartRadiusCm * 1.04f, StartRadiusCm * 1.04f},
		{LengthCm * 0.35f,
			FMath::Lerp(StartRadiusCm, EndRadiusCm, 0.35f),
			FMath::Lerp(StartRadiusCm, EndRadiusCm, 0.35f)},
		{LengthCm * 0.78f,
			FMath::Lerp(StartRadiusCm, EndRadiusCm, 0.80f),
			FMath::Lerp(StartRadiusCm, EndRadiusCm, 0.80f)},
		{LengthCm, EndRadiusCm, EndRadiusCm},
	};
	return BuildLoftedShell(Rings, 10);
}

void ApplyAccessoryMaterial(UMeshComponent* Component, const FLinearColor& Color)
{
	if (!Component)
	{
		return;
	}
	static TSoftObjectPtr<UMaterialInterface> WardrobeMaterial(
		FSoftObjectPath(TEXT(
			"/Game/Materials/M_WardrobeAccessory.M_WardrobeAccessory")));
	static TSoftObjectPtr<UMaterialInterface> FallbackMaterial(
		FSoftObjectPath(TEXT(
			"/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")));
	UMaterialInterface* Material = WardrobeMaterial.LoadSynchronous();
	if (!Material)
	{
		Material = FallbackMaterial.LoadSynchronous();
	}
	if (Material)
	{
		Component->SetMaterial(0, Material);
		if (UMaterialInstanceDynamic* Dynamic =
			Component->CreateAndSetMaterialInstanceDynamic(0))
		{
			Dynamic->SetVectorParameterValue(TEXT("BaseColor"), Color);
			Dynamic->SetVectorParameterValue(TEXT("Color"), Color);
			Dynamic->SetVectorParameterValue(TEXT("diffuse_color"), Color);
			Dynamic->SetScalarParameterValue(TEXT("Roughness"), 0.72f);
		}
	}
}

const TArray<FSprawlWardrobePalette>& WardrobePalettes()
{
	static const TArray<FSprawlWardrobePalette> Palettes = {
		{ {0.74f, 0.76f, 0.80f}, {0.025f, 0.03f, 0.045f}, {0.035f, 0.055f, 0.11f}, {0.10f, 0.56f, 0.48f}, {0.035f, 0.04f, 0.05f}, {0.78f, 0.80f, 0.82f} },
		{ {0.20f, 0.055f, 0.045f}, {0.04f, 0.035f, 0.03f}, {0.10f, 0.025f, 0.025f}, {0.90f, 0.40f, 0.08f}, {0.055f, 0.045f, 0.04f}, {0.68f, 0.62f, 0.55f} },
		{ {0.035f, 0.14f, 0.085f}, {0.025f, 0.035f, 0.03f}, {0.025f, 0.075f, 0.055f}, {0.72f, 0.55f, 0.10f}, {0.035f, 0.045f, 0.04f}, {0.76f, 0.76f, 0.70f} },
		{ {0.17f, 0.10f, 0.035f}, {0.045f, 0.03f, 0.02f}, {0.09f, 0.055f, 0.025f}, {0.12f, 0.42f, 0.65f}, {0.08f, 0.05f, 0.025f}, {0.56f, 0.48f, 0.36f} },
		{ {0.13f, 0.04f, 0.16f}, {0.03f, 0.025f, 0.04f}, {0.075f, 0.025f, 0.09f}, {0.70f, 0.20f, 0.48f}, {0.045f, 0.035f, 0.055f}, {0.72f, 0.68f, 0.76f} },
		{ {0.13f, 0.14f, 0.16f}, {0.025f, 0.025f, 0.03f}, {0.055f, 0.06f, 0.075f}, {0.18f, 0.50f, 0.70f}, {0.025f, 0.028f, 0.035f}, {0.82f, 0.82f, 0.84f} },
		{ {0.055f, 0.085f, 0.18f}, {0.035f, 0.045f, 0.075f}, {0.025f, 0.04f, 0.09f}, {0.85f, 0.30f, 0.12f}, {0.72f, 0.72f, 0.68f}, {0.82f, 0.80f, 0.72f} },
		{ {0.46f, 0.30f, 0.18f}, {0.055f, 0.05f, 0.045f}, {0.16f, 0.10f, 0.055f}, {0.22f, 0.58f, 0.43f}, {0.055f, 0.04f, 0.03f}, {0.66f, 0.58f, 0.48f} },
	};
	return Palettes;
}

bool WardrobeIsFiniteColor(const FLinearColor& Color)
{
	return FMath::IsFinite(Color.R) && FMath::IsFinite(Color.G)
		&& FMath::IsFinite(Color.B) && FMath::IsFinite(Color.A);
}

bool IsGarmentComponent(const UMeshComponent* Mesh)
{
	if (!Mesh)
	{
		return false;
	}
	const FString Name = Mesh->GetName();
	return Name.Contains(TEXT("Torso"), ESearchCase::IgnoreCase)
		|| Name.Contains(TEXT("Legs"), ESearchCase::IgnoreCase)
		|| Name.Contains(TEXT("Cloth"), ESearchCase::IgnoreCase)
		|| Name.Contains(TEXT("Garment"), ESearchCase::IgnoreCase)
		|| Name.Contains(TEXT("Outfit"), ESearchCase::IgnoreCase);
}

bool IsBottomComponent(const UMeshComponent* Mesh)
{
	const FString Name = Mesh ? Mesh->GetName() : FString();
	return Name.Contains(TEXT("Legs"), ESearchCase::IgnoreCase)
		|| Name.Contains(TEXT("Bottom"), ESearchCase::IgnoreCase)
		|| Name.Contains(TEXT("Short"), ESearchCase::IgnoreCase)
		|| Name.Contains(TEXT("Trouser"), ESearchCase::IgnoreCase)
		|| Name.Contains(TEXT("Pant"), ESearchCase::IgnoreCase);
}

bool IsGarmentMaterial(const UMaterialInterface* Material)
{
	if (!Material)
	{
		return false;
	}
	const FString Path = Material->GetPathName();
	return Path.Contains(TEXT("DefaultGarment"), ESearchCase::IgnoreCase)
		|| Path.Contains(TEXT("_DG_"), ESearchCase::IgnoreCase)
		|| Path.Contains(TEXT("Shirt"), ESearchCase::IgnoreCase)
		|| Path.Contains(TEXT("Short"), ESearchCase::IgnoreCase)
		|| Path.Contains(TEXT("Trouser"), ESearchCase::IgnoreCase)
		|| Path.Contains(TEXT("Jacket"), ESearchCase::IgnoreCase);
}

bool IsBottomMaterial(const UMaterialInterface* Material)
{
	const FString Path = Material ? Material->GetPathName() : FString();
	return Path.Contains(TEXT("Short"), ESearchCase::IgnoreCase)
		|| Path.Contains(TEXT("Bottom"), ESearchCase::IgnoreCase)
		|| Path.Contains(TEXT("Trouser"), ESearchCase::IgnoreCase)
		|| Path.Contains(TEXT("Pant"), ESearchCase::IgnoreCase);
}

FName WardrobeFindBone(
	const USkeletalMeshComponent* BodyMesh,
	std::initializer_list<const TCHAR*> Candidates)
{
	if (!BodyMesh)
	{
		return NAME_None;
	}
	for (const TCHAR* Candidate : Candidates)
	{
		const FName BoneName(Candidate);
		if (BodyMesh->GetBoneIndex(BoneName) != INDEX_NONE)
		{
			return BoneName;
		}
	}
	return NAME_None;
}

FVector WardrobeSafeDirection(
	const FVector& From, const FVector& To, const FVector& Fallback)
{
	const FVector SafeFallback = Fallback.GetSafeNormal(
		SMALL_NUMBER, FVector::UpVector);
	return (To - From).GetSafeNormal(SMALL_NUMBER, SafeFallback);
}

const TCHAR* OuterwearName(ESprawlWardrobeOuterwear Value)
{
	switch (Value)
	{
	case ESprawlWardrobeOuterwear::UtilityJacket: return TEXT("utility jacket");
	case ESprawlWardrobeOuterwear::BomberJacket: return TEXT("bomber jacket");
	case ESprawlWardrobeOuterwear::Blazer: return TEXT("blazer");
	case ESprawlWardrobeOuterwear::TrackJacket: return TEXT("track jacket");
	default: return TEXT("no jacket");
	}
}

const TCHAR* HeadwearName(ESprawlWardrobeHeadwear Value)
{
	switch (Value)
	{
	case ESprawlWardrobeHeadwear::BaseballCap: return TEXT("baseball cap");
	case ESprawlWardrobeHeadwear::Beanie: return TEXT("beanie");
	default: return TEXT("no hat");
	}
}

const TCHAR* FootwearName(ESprawlWardrobeFootwear Value)
{
	switch (Value)
	{
	case ESprawlWardrobeFootwear::HighTopSneakers: return TEXT("high-top sneakers");
	case ESprawlWardrobeFootwear::DressShoes: return TEXT("dress shoes");
	case ESprawlWardrobeFootwear::WorkBoots: return TEXT("work boots");
	case ESprawlWardrobeFootwear::RunningShoes: return TEXT("running shoes");
	case ESprawlWardrobeFootwear::AthleticTrainers: return TEXT("athletic trainers");
	default: return TEXT("low-top sneakers");
	}
}

const TCHAR* SocksName(ESprawlWardrobeSocks Value)
{
	switch (Value)
	{
	case ESprawlWardrobeSocks::Ankle: return TEXT("ankle socks");
	case ESprawlWardrobeSocks::Dress: return TEXT("dress socks");
	default: return TEXT("crew socks");
	}
}
}

bool FSprawlWardrobeOutfit::operator==(
	const FSprawlWardrobeOutfit& Other) const
{
	return OutfitId == Other.OutfitId
		&& Top == Other.Top
		&& Bottom == Other.Bottom
		&& Outerwear == Other.Outerwear
		&& Headwear == Other.Headwear
		&& Footwear == Other.Footwear
		&& Socks == Other.Socks
		&& TopColor.Equals(Other.TopColor)
		&& BottomColor.Equals(Other.BottomColor)
		&& OuterwearColor.Equals(Other.OuterwearColor)
		&& AccentColor.Equals(Other.AccentColor)
		&& ShoeColor.Equals(Other.ShoeColor)
		&& SockColor.Equals(Other.SockColor);
}

USprawlWardrobeModule::USprawlWardrobeModule()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USprawlWardrobeModule::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearWardrobe();
	Super::EndPlay(EndPlayReason);
}

int32 USprawlWardrobeModule::GetAccessoryCount() const
{
	return SpawnedAccessories.Num()
		+ (FootwearModule ? FootwearModule->GetPresentationPieceCount() : 0);
}

bool USprawlWardrobeModule::SwapAthleticShoes(
	ESprawlAthleticShoePreset Preset)
{
	return USprawlAthleticShoeModule::SwapToPreset(FootwearModule, Preset);
}

FSprawlWardrobeOutfit USprawlWardrobeModule::DevelopOutfit(
	ESprawlWardrobeStyle Style, int32 Seed)
{
	FRandomStream Stream(Seed ^ 0x57415244); // "WARD"
	const TArray<FSprawlWardrobePalette>& Palettes = WardrobePalettes();
	const FSprawlWardrobePalette& Palette = Palettes[
		Stream.RandRange(0, Palettes.Num() - 1)];

	FSprawlWardrobeOutfit Result;
	Result.TopColor = Palette.Top;
	Result.BottomColor = Palette.Bottom;
	Result.OuterwearColor = Palette.Outerwear;
	Result.AccentColor = Palette.Accent;
	Result.ShoeColor = Palette.Shoes;
	Result.SockColor = Palette.Socks;

	float JacketChance = 0.5f;
	float HatChance = 0.3f;
	const TCHAR* StyleName = TEXT("Street");
	switch (Style)
	{
	case ESprawlWardrobeStyle::SmartCasual:
		StyleName = TEXT("Smart");
		Result.Top = Stream.FRand() < 0.5f
			? ESprawlWardrobeTop::Knit : ESprawlWardrobeTop::ButtonDown;
		Result.Bottom = Stream.FRand() < 0.55f
			? ESprawlWardrobeBottom::Trousers : ESprawlWardrobeBottom::Jeans;
		Result.Footwear = Stream.FRand() < 0.45f
			? ESprawlWardrobeFootwear::DressShoes
			: ESprawlWardrobeFootwear::LowTopSneakers;
		Result.Socks = ESprawlWardrobeSocks::Dress;
		JacketChance = 0.58f;
		HatChance = 0.12f;
		break;
	case ESprawlWardrobeStyle::Corporate:
		StyleName = TEXT("Office");
		Result.Top = ESprawlWardrobeTop::ButtonDown;
		Result.Bottom = ESprawlWardrobeBottom::Trousers;
		Result.Footwear = ESprawlWardrobeFootwear::DressShoes;
		Result.Socks = ESprawlWardrobeSocks::Dress;
		JacketChance = 0.82f;
		HatChance = 0.04f;
		break;
	case ESprawlWardrobeStyle::Workwear:
		StyleName = TEXT("Work");
		Result.Top = Stream.FRand() < 0.55f
			? ESprawlWardrobeTop::Tee : ESprawlWardrobeTop::Knit;
		Result.Bottom = ESprawlWardrobeBottom::Cargo;
		Result.Footwear = ESprawlWardrobeFootwear::WorkBoots;
		Result.Socks = ESprawlWardrobeSocks::Crew;
		JacketChance = 0.74f;
		HatChance = 0.46f;
		break;
	case ESprawlWardrobeStyle::Athletic:
		StyleName = TEXT("Active");
		Result.Top = Stream.FRand() < 0.5f
			? ESprawlWardrobeTop::Tee : ESprawlWardrobeTop::Hoodie;
		Result.Bottom = ESprawlWardrobeBottom::Joggers;
		Result.Footwear = ESprawlWardrobeFootwear::AthleticTrainers;
		Result.Socks = Stream.FRand() < 0.5f
			? ESprawlWardrobeSocks::Ankle : ESprawlWardrobeSocks::Crew;
		JacketChance = 0.44f;
		HatChance = 0.36f;
		break;
	default:
		Result.Top = Stream.FRand() < 0.45f
			? ESprawlWardrobeTop::Tee : ESprawlWardrobeTop::Hoodie;
		Result.Bottom = Stream.FRand() < 0.55f
			? ESprawlWardrobeBottom::Jeans : ESprawlWardrobeBottom::Cargo;
		Result.Footwear = Stream.FRand() < 0.35f
			? ESprawlWardrobeFootwear::HighTopSneakers
			: ESprawlWardrobeFootwear::LowTopSneakers;
		Result.Socks = ESprawlWardrobeSocks::Crew;
		break;
	}

	if (Stream.FRand() < JacketChance)
	{
		switch (Style)
		{
		case ESprawlWardrobeStyle::Corporate:
		case ESprawlWardrobeStyle::SmartCasual:
			Result.Outerwear = ESprawlWardrobeOuterwear::Blazer;
			break;
		case ESprawlWardrobeStyle::Workwear:
			Result.Outerwear = ESprawlWardrobeOuterwear::UtilityJacket;
			break;
		case ESprawlWardrobeStyle::Athletic:
			Result.Outerwear = ESprawlWardrobeOuterwear::TrackJacket;
			break;
		default:
			Result.Outerwear = ESprawlWardrobeOuterwear::BomberJacket;
			break;
		}
	}
	if (Stream.FRand() < HatChance)
	{
		Result.Headwear = Stream.FRand() < 0.58f
			? ESprawlWardrobeHeadwear::BaseballCap
			: ESprawlWardrobeHeadwear::Beanie;
	}

	Result.OutfitId = FName(*FString::Printf(
		TEXT("%s_%08X"), StyleName, static_cast<uint32>(Seed)));
	return Result;
}

FSprawlWardrobeOutfit USprawlWardrobeModule::CreateZarriOutfit(int32 Seed)
{
	FSprawlWardrobeOutfit Result = DevelopOutfit(
		ESprawlWardrobeStyle::Streetwear, Seed);
	Result.OutfitId = FName(*FString::Printf(
		TEXT("ZarriFounder_%08X"), static_cast<uint32>(Seed)));
	Result.Top = ESprawlWardrobeTop::Tee;
	Result.Bottom = ESprawlWardrobeBottom::Cargo;
	Result.Outerwear = ESprawlWardrobeOuterwear::BomberJacket;
	Result.Headwear = ESprawlWardrobeHeadwear::None;
	const FSprawlAthleticShoeDefinition AthleticShoes =
		USprawlAthleticShoeModule::DevelopPreset(
			ESprawlAthleticShoePreset::ZarriVelocity);
	Result.Footwear = AthleticShoes.Footwear;
	Result.Socks = AthleticShoes.Socks;
	Result.TopColor = FLinearColor(0.72f, 0.74f, 0.78f, 1.f);
	Result.BottomColor = FLinearColor(0.025f, 0.03f, 0.045f, 1.f);
	Result.OuterwearColor = FLinearColor(0.06f, 0.13f, 0.32f, 1.f);
	Result.AccentColor = AthleticShoes.AccentColor;
	Result.ShoeColor = AthleticShoes.UpperColor;
	Result.SockColor = AthleticShoes.SockColor;
	return Result;
}

FSprawlWardrobeOutfit USprawlWardrobeModule::CreateNanobananaOutfit(int32 Seed)
{
	FSprawlWardrobeOutfit Result = DevelopOutfit(
		ESprawlWardrobeStyle::Streetwear, Seed);
	Result.OutfitId = FName(*FString::Printf(
		TEXT("NanobananaTech_%08X"), static_cast<uint32>(Seed)));
	Result.Top = ESprawlWardrobeTop::Hoodie;
	Result.Bottom = ESprawlWardrobeBottom::Cargo;
	Result.Outerwear = ESprawlWardrobeOuterwear::BomberJacket;
	Result.Headwear = ESprawlWardrobeHeadwear::Beanie;
	Result.Footwear = ESprawlWardrobeFootwear::HighTopSneakers;
	Result.Socks = ESprawlWardrobeSocks::Crew;
	Result.TopColor = FLinearColor(0.04f, 0.07f, 0.16f, 1.f);      // Deep Cyberpunk Navy
	Result.BottomColor = FLinearColor(0.02f, 0.025f, 0.035f, 1.f);  // Dark Graphite Cargo
	Result.OuterwearColor = FLinearColor(0.05f, 0.09f, 0.22f, 1.f); // Weatherproof Tech Navy
	Result.AccentColor = FLinearColor(0.15f, 0.85f, 0.95f, 1.f);   // Neon Cyan CyberDrip
	Result.ShoeColor = FLinearColor(0.08f, 0.10f, 0.14f, 1.f);     // High-Top Cyber Black
	Result.SockColor = FLinearColor(0.85f, 0.88f, 0.92f, 1.f);
	return Result;
}

bool USprawlWardrobeModule::ValidateOutfit(
	const FSprawlWardrobeOutfit& Outfit, FString& OutError)
{
	if (Outfit.OutfitId.IsNone())
	{
		OutError = TEXT("OutfitId is required");
		return false;
	}
	if (!WardrobeIsFiniteColor(Outfit.TopColor)
		|| !WardrobeIsFiniteColor(Outfit.BottomColor)
		|| !WardrobeIsFiniteColor(Outfit.OuterwearColor)
		|| !WardrobeIsFiniteColor(Outfit.AccentColor)
		|| !WardrobeIsFiniteColor(Outfit.ShoeColor)
		|| !WardrobeIsFiniteColor(Outfit.SockColor))
	{
		OutError = TEXT("Outfit colors must be finite");
		return false;
	}
	OutError.Reset();
	return true;
}

FString USprawlWardrobeModule::DescribeOutfit(
	const FSprawlWardrobeOutfit& Outfit)
{
	return FString::Printf(
		TEXT("%s: %s, %s, shoes=%s, socks=%s"),
		*Outfit.OutfitId.ToString(), OuterwearName(Outfit.Outerwear),
		HeadwearName(Outfit.Headwear), FootwearName(Outfit.Footwear),
		SocksName(Outfit.Socks));
}

FSprawlJacketProfile USprawlWardrobeModule::DevelopJacketProfile(
	ESprawlWardrobeOuterwear Outerwear,
	float ShoulderHalfWidthCm,
	float TorsoHeightCm)
{
	const float Shoulder = FMath::Clamp(
		FMath::IsFinite(ShoulderHalfWidthCm)
			? FMath::Abs(ShoulderHalfWidthCm) : 18.f,
		13.f, 26.f);
	const float Torso = FMath::Clamp(
		FMath::IsFinite(TorsoHeightCm) ? FMath::Abs(TorsoHeightCm) : 50.f,
		38.f, 68.f);

	// Every factor is relative to the shoulder-joint half-span. Anthropometry
	// puts bideltoid breadth at roughly 1.4x biacromial breadth, so a shell cut
	// at 1.0x would sit inside the torso instead of over it.
	FSprawlJacketProfile Profile;
	Profile.ChestHalfWidthCm = Shoulder * 1.34f;
	Profile.ShoulderHalfWidthCm = Shoulder * 1.52f;
	Profile.ChestHalfDepthCm = Shoulder * 0.90f;
	Profile.HemHalfWidthCm = Shoulder * 1.22f;
	Profile.ShellHeightCm = Torso * 0.92f;
	Profile.HemDropCm = Torso * 0.42f;
	Profile.CollarHeightCm = 5.f;
	Profile.SleeveUpperRadiusCm = Shoulder * 0.44f;
	Profile.SleeveCuffRadiusCm = Shoulder * 0.29f;
	Profile.SleeveReach = 1.f;

	switch (Outerwear)
	{
	case ESprawlWardrobeOuterwear::BomberJacket:
		// Cropped and blousy, gathered into a tight ribbed hem.
		Profile.HemHalfWidthCm = Shoulder * 1.14f;
		Profile.ChestHalfDepthCm = Shoulder * 0.96f;
		Profile.ShellHeightCm = Torso * 0.80f;
		Profile.HemDropCm = Torso * 0.30f;
		Profile.CollarHeightCm = 4.2f;
		break;
	case ESprawlWardrobeOuterwear::Blazer:
		// Longer and straighter, with a structured shoulder and taller lapel.
		Profile.ShellHeightCm = Torso * 1.02f;
		Profile.HemDropCm = Torso * 0.52f;
		Profile.HemHalfWidthCm = Shoulder * 1.28f;
		Profile.ShoulderHalfWidthCm = Shoulder * 1.60f;
		Profile.ChestHalfDepthCm = Shoulder * 0.88f;
		Profile.CollarHeightCm = 6.2f;
		break;
	case ESprawlWardrobeOuterwear::UtilityJacket:
		Profile.ChestHalfWidthCm = Shoulder * 1.38f;
		Profile.ShoulderHalfWidthCm = Shoulder * 1.62f;
		Profile.HemHalfWidthCm = Shoulder * 1.34f;
		Profile.ShellHeightCm = Torso * 0.96f;
		Profile.HemDropCm = Torso * 0.46f;
		Profile.CollarHeightCm = 5.6f;
		break;
	case ESprawlWardrobeOuterwear::TrackJacket:
		Profile.ChestHalfWidthCm = Shoulder * 1.24f;
		Profile.ShoulderHalfWidthCm = Shoulder * 1.50f;
		Profile.ChestHalfDepthCm = Shoulder * 0.86f;
		Profile.HemHalfWidthCm = Shoulder * 1.16f;
		Profile.ShellHeightCm = Torso * 0.86f;
		Profile.HemDropCm = Torso * 0.34f;
		Profile.SleeveUpperRadiusCm = Shoulder * 0.41f;
		Profile.SleeveCuffRadiusCm = Shoulder * 0.27f;
		Profile.CollarHeightCm = 4.6f;
		break;
	default:
		break;
	}
	return Profile;
}

bool USprawlWardrobeModule::ValidateJacketProfile(
	const FSprawlJacketProfile& Profile, FString& OutError)
{
	const bool bFinite = FMath::IsFinite(Profile.ChestHalfWidthCm)
		&& FMath::IsFinite(Profile.ShoulderHalfWidthCm)
		&& FMath::IsFinite(Profile.ChestHalfDepthCm)
		&& FMath::IsFinite(Profile.HemHalfWidthCm)
		&& FMath::IsFinite(Profile.ShellHeightCm)
		&& FMath::IsFinite(Profile.HemDropCm)
		&& FMath::IsFinite(Profile.CollarHeightCm)
		&& FMath::IsFinite(Profile.SleeveUpperRadiusCm)
		&& FMath::IsFinite(Profile.SleeveCuffRadiusCm)
		&& FMath::IsFinite(Profile.SleeveReach);
	if (!bFinite)
	{
		OutError = TEXT("Jacket profile must be finite");
		return false;
	}
	if (Profile.ChestHalfWidthCm < 12.f || Profile.ChestHalfWidthCm > 40.f
		|| Profile.ShoulderHalfWidthCm < 15.f || Profile.ShoulderHalfWidthCm > 46.f
		|| Profile.ChestHalfDepthCm < 7.f || Profile.ChestHalfDepthCm > 28.f
		|| Profile.HemHalfWidthCm < 10.f || Profile.HemHalfWidthCm > 40.f
		|| Profile.ShellHeightCm < 28.f || Profile.ShellHeightCm > 75.f
		|| Profile.HemDropCm < 10.f || Profile.HemDropCm > 42.f
		|| Profile.CollarHeightCm < 2.f || Profile.CollarHeightCm > 10.f
		|| Profile.SleeveUpperRadiusCm < 3.5f || Profile.SleeveUpperRadiusCm > 17.f
		|| Profile.SleeveCuffRadiusCm < 2.5f || Profile.SleeveCuffRadiusCm > 10.f
		|| Profile.SleeveReach < 0.3f || Profile.SleeveReach > 1.05f)
	{
		OutError = TEXT("Jacket profile falls outside human-scale bounds");
		return false;
	}
	// A jacket that is deeper than it is wide is wearing the body, not worn.
	if (Profile.ChestHalfDepthCm > Profile.ChestHalfWidthCm)
	{
		OutError = TEXT("Jacket depth must not exceed its width");
		return false;
	}
	// The shell must flare out to the deltoids, or the sleeves float free.
	if (Profile.ShoulderHalfWidthCm <= Profile.ChestHalfWidthCm)
	{
		OutError = TEXT("Jacket shoulders must be wider than its chest");
		return false;
	}
	OutError.Reset();
	return true;
}

void USprawlWardrobeModule::ApplyGarmentPalette(
	AActor* VisualActor, const FSprawlWardrobeOutfit& Outfit)
{
	if (!VisualActor)
	{
		return;
	}
	const FLinearColor EffectiveTop =
		Outfit.Outerwear == ESprawlWardrobeOuterwear::None
		? Outfit.TopColor : Outfit.OuterwearColor;
	TInlineComponentArray<UMeshComponent*> Meshes(VisualActor);
	for (UMeshComponent* Mesh : Meshes)
	{
		for (int32 MaterialIndex = 0;
			MaterialIndex < Mesh->GetNumMaterials(); ++MaterialIndex)
		{
			UMaterialInterface* SourceMaterial = Mesh->GetMaterial(MaterialIndex);
			if (!IsGarmentComponent(Mesh) && !IsGarmentMaterial(SourceMaterial))
			{
				continue;
			}
			const FLinearColor LayerColor =
				IsBottomComponent(Mesh) || IsBottomMaterial(SourceMaterial)
				? Outfit.BottomColor : EffectiveTop;
			UMaterialInstanceDynamic* Material =
				Mesh->CreateDynamicMaterialInstance(MaterialIndex);
			if (!Material)
			{
				continue;
			}
			// MetaHuman DefaultGarment parameter names vary by plugin release.
			// Setting an absent parameter is harmless and keeps this future-proof.
			Material->SetVectorParameterValue(TEXT("PrimaryColorShirt"), EffectiveTop);
			Material->SetVectorParameterValue(TEXT("SecondaryColorShirt"), Outfit.TopColor);
			Material->SetVectorParameterValue(TEXT("PrimaryColorShort"), Outfit.BottomColor);
			Material->SetVectorParameterValue(TEXT("SecondaryColorShort"), Outfit.BottomColor * 0.62f);
			Material->SetVectorParameterValue(TEXT("diffuse_color_1"), LayerColor);
			Material->SetVectorParameterValue(TEXT("diffuse_color_2"), LayerColor * 0.72f);
			Material->SetVectorParameterValue(TEXT("B_diffuse_color"), LayerColor);
			Material->SetVectorParameterValue(TEXT("C_color"), LayerColor);
			Material->SetVectorParameterValue(TEXT("diffuse_color"), LayerColor);
			if (FParse::Param(FCommandLine::Get(), TEXT("SprawlAuditWardrobe")))
			{
				UE_LOG(LogTemp, Display,
					TEXT("[WardrobeAudit] garment component=%s material=%s layer=%s"),
					*Mesh->GetName(), SourceMaterial
						? *SourceMaterial->GetPathName() : TEXT("none"),
					IsBottomComponent(Mesh) || IsBottomMaterial(SourceMaterial)
						? TEXT("bottom") : TEXT("top"));
			}
		}
	}
}

UStaticMeshComponent* USprawlWardrobeModule::SpawnAccessory(
	AActor* VisualActor,
	USkeletalMeshComponent* BodyMesh,
	UStaticMesh* Mesh,
	FName BoneName,
	const FTransform& WorldTransform,
	const FLinearColor& Color,
	FName ComponentName)
{
	if (!VisualActor || !BodyMesh || !Mesh || BoneName.IsNone())
	{
		return nullptr;
	}

	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(
		VisualActor, ComponentName);
	if (!Component)
	{
		return nullptr;
	}
	VisualActor->AddInstanceComponent(Component);
	Component->SetMobility(EComponentMobility::Movable);
	Component->SetStaticMesh(Mesh);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetGenerateOverlapEvents(false);
	Component->SetCanEverAffectNavigation(false);
	Component->SetCastShadow(false);
	Component->SetReceivesDecals(false);
	Component->SetComponentTickEnabled(false);
	Component->ComponentTags.AddUnique(TEXT("SprawlWardrobe"));
	Component->RegisterComponent();
	Component->SetWorldTransform(WorldTransform);
	Component->SetAbsolute(false, false, true);
	Component->AttachToComponent(
		BodyMesh, FAttachmentTransformRules::KeepWorldTransform, BoneName);

	ApplyAccessoryMaterial(Component, Color);
	SpawnedAccessories.Add(Component);
	return Component;
}

UProceduralMeshComponent* USprawlWardrobeModule::SpawnProceduralAccessory(
	AActor* VisualActor,
	USkeletalMeshComponent* BodyMesh,
	FName BoneName,
	const FTransform& WorldTransform,
	const TArray<FVector>& Vertices,
	const TArray<int32>& Triangles,
	const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0,
	const FLinearColor& Color,
	FName ComponentName)
{
	if (!VisualActor || !BodyMesh || BoneName.IsNone()
		|| Vertices.IsEmpty() || Triangles.IsEmpty())
	{
		return nullptr;
	}

	UProceduralMeshComponent* Component =
		NewObject<UProceduralMeshComponent>(VisualActor, ComponentName);
	if (!Component)
	{
		return nullptr;
	}
	VisualActor->AddInstanceComponent(Component);
	Component->SetMobility(EComponentMobility::Movable);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetGenerateOverlapEvents(false);
	Component->SetCanEverAffectNavigation(false);
	Component->SetCastShadow(false);
	Component->SetReceivesDecals(false);
	Component->SetComponentTickEnabled(false);
	Component->ComponentTags.AddUnique(TEXT("SprawlWardrobe"));
	TArray<FLinearColor> VertexColors;
	VertexColors.Init(Color, Vertices.Num());
	Component->CreateMeshSection_LinearColor(
		0, Vertices, Triangles, Normals, UV0, VertexColors, {}, false);
	Component->RegisterComponent();
	// Each accessory is one solid colour, so a dynamic instance of the shading
	// material beats the vertex-colour debug material this used to reach for.
	ApplyAccessoryMaterial(Component, Color);
	Component->SetWorldTransform(WorldTransform);
	Component->SetAbsolute(false, false, true);
	Component->AttachToComponent(
		BodyMesh, FAttachmentTransformRules::KeepWorldTransform, BoneName);
	SpawnedAccessories.Add(Component);
	return Component;
}

bool USprawlWardrobeModule::IsAccessoryVisible(const UMeshComponent* Component)
{
	if (!Component || !Component->IsRegistered() || !Component->IsVisible())
	{
		return false;
	}
	const UMaterialInterface* Material = Component->GetMaterial(0);
	if (!Material)
	{
		return false;
	}
	// A debug view-mode material draws nothing in the normal shading path, so a
	// garment carrying one is geometry the player will never see.
	const UMaterial* Base = Material->GetMaterial();
	return Base && Base->MaterialDomain == MD_Surface
		&& !Base->GetPathName().Contains(TEXT("EngineDebugMaterials"))
		&& !Base->GetPathName().Contains(TEXT("ViewMode"));
}

bool USprawlWardrobeModule::ApplyToMetaHuman(
	AActor* VisualActor,
	USkeletalMeshComponent* BodyMesh,
	const FSprawlWardrobeOutfit& Outfit)
{
	ClearWardrobe();
	FString Error;
	if (!VisualActor || !BodyMesh || !ValidateOutfit(Outfit, Error))
	{
		return false;
	}

	ApplyGarmentPalette(VisualActor, Outfit);
	if (!FootwearModule)
	{
		AActor* OwnerActor = GetOwner();
		if (OwnerActor)
		{
			FootwearModule = OwnerActor->FindComponentByClass<USprawlFootwearModule>();
			if (!FootwearModule)
			{
				FootwearModule = NewObject<USprawlFootwearModule>(
					OwnerActor, TEXT("SprawlFootwearModule"));
				if (FootwearModule)
				{
					OwnerActor->AddInstanceComponent(FootwearModule);
					FootwearModule->RegisterComponent();
				}
			}
		}
	}
	const bool bFootwearComplete = FootwearModule
		&& FootwearModule->ApplyToMetaHuman(
			VisualActor, BodyMesh, Outfit.Footwear, Outfit.Socks,
			Outfit.ShoeColor, Outfit.SockColor, Outfit.AccentColor);

	const FName HeadBone = WardrobeFindBone(BodyMesh, {TEXT("head"), TEXT("Head")});
	const FName NeckBone = WardrobeFindBone(BodyMesh, {TEXT("neck_02"), TEXT("neck_01"), TEXT("Neck")});
	if (Outfit.Headwear != ESprawlWardrobeHeadwear::None && !HeadBone.IsNone())
	{
		const FVector Head = BodyMesh->GetSocketLocation(HeadBone);
		const FVector Neck = NeckBone.IsNone()
			? Head - BodyMesh->GetUpVector() * 15.f
			: BodyMesh->GetSocketLocation(NeckBone);
		const FVector Up = WardrobeSafeDirection(Neck, Head, BodyMesh->GetUpVector());
		const FVector Forward = FVector::VectorPlaneProject(
			VisualActor->GetActorForwardVector(), Up).GetSafeNormal(
				SMALL_NUMBER, FVector::ForwardVector);
		const FQuat HatRotation = FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat();
		const FSprawlAccessoryMesh CrownMesh = BuildHatCrown(
			Outfit.Headwear == ESprawlWardrobeHeadwear::Beanie);
		const FTransform CrownTransform(
			HatRotation, Head + Up * 8.5f, FVector::OneVector);
		SpawnProceduralAccessory(
			VisualActor, BodyMesh, HeadBone, CrownTransform,
			CrownMesh.Vertices, CrownMesh.Triangles, CrownMesh.Normals, CrownMesh.UV0,
			Outfit.OuterwearColor, TEXT("WardrobeHatCrown"));
		if (Outfit.Headwear == ESprawlWardrobeHeadwear::BaseballCap)
		{
			const FSprawlAccessoryMesh BrimMesh = BuildCapBrim();
			const FTransform BrimTransform(
				HatRotation, Head + Up * 8.5f, FVector::OneVector);
			SpawnProceduralAccessory(
				VisualActor, BodyMesh, HeadBone, BrimTransform,
				BrimMesh.Vertices, BrimMesh.Triangles, BrimMesh.Normals, BrimMesh.UV0,
				Outfit.AccentColor, TEXT("WardrobeHatBrim"));
		}
	}

	if (Outfit.Outerwear != ESprawlWardrobeOuterwear::None)
	{
		const FName ChestBone = WardrobeFindBone(
			BodyMesh, {TEXT("spine_04"), TEXT("spine_03"), TEXT("Spine2")});
		const FName UpperChestBone = WardrobeFindBone(
			BodyMesh, {TEXT("spine_05"), TEXT("spine_04"), TEXT("Spine3")});
		const FName PelvisBone = WardrobeFindBone(
			BodyMesh, {TEXT("pelvis"), TEXT("Hips")});
		const FName TorsoNeckBone = WardrobeFindBone(
			BodyMesh, {TEXT("neck_01"), TEXT("Neck")});
		const FName UpperArmBones[2] = {
			WardrobeFindBone(BodyMesh, {TEXT("upperarm_l"), TEXT("LeftArm")}),
			WardrobeFindBone(BodyMesh, {TEXT("upperarm_r"), TEXT("RightArm")})};
		const FName LowerArmBones[2] = {
			WardrobeFindBone(BodyMesh, {TEXT("lowerarm_l"), TEXT("LeftForeArm")}),
			WardrobeFindBone(BodyMesh, {TEXT("lowerarm_r"), TEXT("RightForeArm")})};
		const FName HandBones[2] = {
			WardrobeFindBone(BodyMesh, {TEXT("hand_l"), TEXT("LeftHand")}),
			WardrobeFindBone(BodyMesh, {TEXT("hand_r"), TEXT("RightHand")})};

		if (!ChestBone.IsNone())
		{
			const FVector Chest = BodyMesh->GetSocketLocation(ChestBone);
			const FVector UpperChest = UpperChestBone.IsNone()
				? Chest + BodyMesh->GetUpVector() * 18.f
				: BodyMesh->GetSocketLocation(UpperChestBone);
			const FVector Up = WardrobeSafeDirection(Chest, UpperChest, BodyMesh->GetUpVector());
			const FVector Forward = FVector::VectorPlaneProject(
				VisualActor->GetActorForwardVector(), Up).GetSafeNormal(
					SMALL_NUMBER, FVector::ForwardVector);
			const FQuat DetailRotation = FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat();

			// Cut the jacket from the body it is going on, not from constants.
			float ShoulderHalfWidth = 18.f;
			if (!UpperArmBones[0].IsNone() && !UpperArmBones[1].IsNone())
			{
				ShoulderHalfWidth = FVector::Dist(
					BodyMesh->GetSocketLocation(UpperArmBones[0]),
					BodyMesh->GetSocketLocation(UpperArmBones[1])) * 0.5f;
			}
			float TorsoHeight = 50.f;
			if (!PelvisBone.IsNone() && !TorsoNeckBone.IsNone())
			{
				TorsoHeight = FMath::Abs(FVector::DotProduct(
					BodyMesh->GetSocketLocation(TorsoNeckBone)
						- BodyMesh->GetSocketLocation(PelvisBone), Up));
			}

			const FSprawlJacketProfile Profile = DevelopJacketProfile(
				Outfit.Outerwear, ShoulderHalfWidth, TorsoHeight);
			FString ProfileError;
			if (!ValidateJacketProfile(Profile, ProfileError))
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[Wardrobe] %s kept its base layer: %s"),
					*VisualActor->GetName(), *ProfileError);
			}
			else
			{
				// The shoulder line is measured, not derived: the shell has to
				// stop where this body's shoulders actually are.
				float ShoulderRise = 13.f;
				if (!UpperArmBones[0].IsNone() && !UpperArmBones[1].IsNone())
				{
					const FVector ShoulderMid = (
						BodyMesh->GetSocketLocation(UpperArmBones[0])
						+ BodyMesh->GetSocketLocation(UpperArmBones[1])) * 0.5f;
					ShoulderRise = FVector::DotProduct(ShoulderMid - Chest, Up);
				}
				const float ShellTop = FMath::Clamp(ShoulderRise, 6.f, 26.f) + 2.f;
				struct FShellStation { float Alpha; float HalfWidth; float HalfDepth; };
				const FShellStation Stations[] = {
					{0.00f, Profile.HemHalfWidthCm * 0.97f,
						Profile.ChestHalfDepthCm * 0.88f},
					{0.10f, Profile.HemHalfWidthCm,
						Profile.ChestHalfDepthCm * 0.92f},
					{0.46f, FMath::Lerp(Profile.HemHalfWidthCm,
						Profile.ChestHalfWidthCm, 0.65f),
						Profile.ChestHalfDepthCm * 0.97f},
					{0.74f, Profile.ChestHalfWidthCm, Profile.ChestHalfDepthCm},
					{0.92f, Profile.ShoulderHalfWidthCm,
						Profile.ChestHalfDepthCm * 0.98f},
					{1.00f, Profile.ShoulderHalfWidthCm * 0.58f,
						Profile.ChestHalfDepthCm * 0.66f}};
				TArray<FSprawlGarmentRing> Rings;
				Rings.Reserve(UE_ARRAY_COUNT(Stations) + 2);
				for (const FShellStation& Station : Stations)
				{
					Rings.Add({
						FMath::Lerp(-Profile.HemDropCm, ShellTop, Station.Alpha),
						Station.HalfWidth, Station.HalfDepth});
				}
				// The collar pinches in above the shoulders, then flares open.
				const float NeckHalfWidth = Profile.ShoulderHalfWidthCm * 0.44f;
				Rings.Add({ShellTop + Profile.CollarHeightCm * 0.55f,
					NeckHalfWidth, NeckHalfWidth * 0.92f});
				Rings.Add({ShellTop + Profile.CollarHeightCm,
					NeckHalfWidth * 1.14f, NeckHalfWidth * 1.06f});

				const FSprawlAccessoryMesh Shell = BuildLoftedShell(Rings, 14);
				UProceduralMeshComponent* ShellComponent = SpawnProceduralAccessory(
					VisualActor, BodyMesh, ChestBone,
					FTransform(DetailRotation, Chest, FVector::OneVector),
					Shell.Vertices, Shell.Triangles, Shell.Normals, Shell.UV0,
					Outfit.OuterwearColor, TEXT("WardrobeJacketShell"));
				if (FParse::Param(FCommandLine::Get(), TEXT("SprawlAuditWardrobe")))
				{
					UE_LOG(LogTemp, Display,
						TEXT("[JacketAudit] actor=%s joint_half=%.2f torso=%.2f chest_half=%.2f shoulder_half=%.2f hem_drop=%.2f shoulder_rise=%.2f shell_top=%.2f sleeve_r=%.2f verts=%d component=%s"),
						*VisualActor->GetName(), ShoulderHalfWidth, TorsoHeight,
						Profile.ChestHalfWidthCm, Profile.ShoulderHalfWidthCm,
						Profile.HemDropCm, ShoulderRise, ShellTop,
						Profile.SleeveUpperRadiusCm, Shell.Vertices.Num(),
						IsAccessoryVisible(ShellComponent)
							? TEXT("drawable") : TEXT("not-drawable"));
				}

				// Sleeves ride their own arm bones so elbows bend inside them.
				for (int32 Side = 0; Side < 2; ++Side)
				{
					if (UpperArmBones[Side].IsNone() || LowerArmBones[Side].IsNone())
					{
						continue;
					}
					const FVector ShoulderJoint =
						BodyMesh->GetSocketLocation(UpperArmBones[Side]);
					const FVector Elbow =
						BodyMesh->GetSocketLocation(LowerArmBones[Side]);
					const FVector Wrist = HandBones[Side].IsNone()
						? Elbow + (Elbow - ShoulderJoint)
						: BodyMesh->GetSocketLocation(HandBones[Side]);
					const float MidRadius = FMath::Lerp(
						Profile.SleeveUpperRadiusCm, Profile.SleeveCuffRadiusCm, 0.55f);

					const float UpperLength = FVector::Dist(ShoulderJoint, Elbow);
					if (UpperLength > KINDA_SMALL_NUMBER)
					{
						const FSprawlAccessoryMesh Sleeve = BuildSleeve(
							UpperLength, Profile.SleeveUpperRadiusCm, MidRadius,
							Profile.SleeveUpperRadiusCm * 0.5f);
						SpawnProceduralAccessory(
							VisualActor, BodyMesh, UpperArmBones[Side],
							FTransform(FRotationMatrix::MakeFromZ(
								WardrobeSafeDirection(ShoulderJoint, Elbow, Up)).ToQuat(),
								ShoulderJoint, FVector::OneVector),
							Sleeve.Vertices, Sleeve.Triangles,
							Sleeve.Normals, Sleeve.UV0, Outfit.OuterwearColor,
							Side == 0 ? TEXT("WardrobeSleeveUpperL")
								: TEXT("WardrobeSleeveUpperR"));
					}

					const float LowerLength = FVector::Dist(Elbow, Wrist);
					if (LowerLength > KINDA_SMALL_NUMBER)
					{
						const FSprawlAccessoryMesh Cuff = BuildSleeve(
							LowerLength * Profile.SleeveReach, MidRadius,
							Profile.SleeveCuffRadiusCm,
							Profile.SleeveCuffRadiusCm * 0.6f);
						SpawnProceduralAccessory(
							VisualActor, BodyMesh, LowerArmBones[Side],
							FTransform(FRotationMatrix::MakeFromZ(
								WardrobeSafeDirection(Elbow, Wrist, Up)).ToQuat(),
								Elbow, FVector::OneVector),
							Cuff.Vertices, Cuff.Triangles,
							Cuff.Normals, Cuff.UV0, Outfit.OuterwearColor,
							Side == 0 ? TEXT("WardrobeSleeveLowerL")
								: TEXT("WardrobeSleeveLowerR"));
					}
				}

				// The zip rides proud of the shell front instead of inside it.
				static TSoftObjectPtr<UStaticMesh> Cube(
					FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")));
				if (UStaticMesh* CubeMesh = Cube.LoadSynchronous())
				{
					SpawnAccessory(
						VisualActor, BodyMesh, CubeMesh, ChestBone,
						FTransform(DetailRotation,
							Chest + Up * ((ShellTop - Profile.HemDropCm) * 0.5f)
								+ Forward * (Profile.ChestHalfDepthCm + 0.45f),
							FVector(0.012f, 0.008f, Profile.ShellHeightCm * 0.0055f)),
						Outfit.AccentColor, TEXT("WardrobeJacketZip"));
				}
			}
		}
	}

	if (!bFootwearComplete)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Wardrobe] %s did not receive a complete fitted footwear pair"),
			*VisualActor->GetName());
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("SprawlAuditWardrobe")))
	{
		UE_LOG(LogTemp, Display,
			TEXT("[WardrobeAudit] actor=%s outfit=%s accessories=%d"),
			*VisualActor->GetName(), *DescribeOutfit(Outfit),
			GetAccessoryCount());
	}
	return bFootwearComplete;
}

void USprawlWardrobeModule::ClearWardrobe()
{
	if (FootwearModule)
	{
		FootwearModule->ClearFootwear();
	}
	for (UMeshComponent* Component : SpawnedAccessories)
	{
		if (IsValid(Component))
		{
			Component->DestroyComponent();
		}
	}
	SpawnedAccessories.Reset();
}
