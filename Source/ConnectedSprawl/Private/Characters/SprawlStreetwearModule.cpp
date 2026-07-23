// The Connected Sprawl - Authored modular streetwear assembly for Zarri.

#include "Characters/SprawlStreetwearModule.h"

#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
constexpr TCHAR StreetwearRoot[] = TEXT("/Game/Import/Characters/Streetwear/");

FSprawlStreetwearPieceDefinition StreetwearPiece(
	const TCHAR* Name,
	ESprawlStreetwearLayer Layer,
	ESprawlStreetwearAnchor Anchor,
	const FVector& AuthoredCenterCm,
	float UniformScaleMultiplier = 1.f)
{
	FSprawlStreetwearPieceDefinition Result;
	Result.PieceId = FName(Name);
	Result.MeshPath = FSoftObjectPath(FString::Printf(
		TEXT("%s%s.%s"), StreetwearRoot, Name, Name));
	Result.Layer = Layer;
	Result.Anchor = Anchor;
	Result.AuthoredCenterCm = AuthoredCenterCm;
	Result.UniformScaleMultiplier = UniformScaleMultiplier;
	Result.bRequired = true;
	return Result;
}

bool StreetwearFiniteVector(const FVector& Vector)
{
	return FMath::IsFinite(Vector.X)
		&& FMath::IsFinite(Vector.Y)
		&& FMath::IsFinite(Vector.Z);
}

FName StreetwearFindBone(
	const USkeletalMeshComponent* BodyMesh,
	std::initializer_list<const TCHAR*> Candidates)
{
	if (!BodyMesh)
	{
		return NAME_None;
	}
	for (const TCHAR* Candidate : Candidates)
	{
		const FName Bone(Candidate);
		if (BodyMesh->GetBoneIndex(Bone) != INDEX_NONE)
		{
			return Bone;
		}
	}
	return NAME_None;
}

FName StreetwearResolveAnchorBone(
	const USkeletalMeshComponent* BodyMesh,
	ESprawlStreetwearAnchor Anchor)
{
	switch (Anchor)
	{
	case ESprawlStreetwearAnchor::Spine:
		return StreetwearFindBone(
			BodyMesh, {TEXT("spine_03"), TEXT("spine_04"), TEXT("Spine2")});
	case ESprawlStreetwearAnchor::Pelvis:
		return StreetwearFindBone(BodyMesh, {TEXT("pelvis"), TEXT("Hips")});
	case ESprawlStreetwearAnchor::Head:
		return StreetwearFindBone(BodyMesh, {TEXT("head"), TEXT("Head")});
	case ESprawlStreetwearAnchor::UpperArmLeft:
		return StreetwearFindBone(BodyMesh, {TEXT("upperarm_l"), TEXT("LeftArm")});
	case ESprawlStreetwearAnchor::UpperArmRight:
		return StreetwearFindBone(BodyMesh, {TEXT("upperarm_r"), TEXT("RightArm")});
	case ESprawlStreetwearAnchor::LowerArmLeft:
		return StreetwearFindBone(BodyMesh, {TEXT("lowerarm_l"), TEXT("LeftForeArm")});
	case ESprawlStreetwearAnchor::LowerArmRight:
		return StreetwearFindBone(BodyMesh, {TEXT("lowerarm_r"), TEXT("RightForeArm")});
	case ESprawlStreetwearAnchor::ThighLeft:
		return StreetwearFindBone(BodyMesh, {TEXT("thigh_l"), TEXT("LeftUpLeg")});
	case ESprawlStreetwearAnchor::ThighRight:
		return StreetwearFindBone(BodyMesh, {TEXT("thigh_r"), TEXT("RightUpLeg")});
	case ESprawlStreetwearAnchor::CalfLeft:
		return StreetwearFindBone(BodyMesh, {TEXT("calf_l"), TEXT("LeftLeg")});
	case ESprawlStreetwearAnchor::CalfRight:
		return StreetwearFindBone(BodyMesh, {TEXT("calf_r"), TEXT("RightLeg")});
	// MetaHuman foot sockets stop updating once the foot is hidden. Shoes ride
	// the live calf instead and retain their body-facing orientation.
	case ESprawlStreetwearAnchor::FootLeft:
		return StreetwearFindBone(BodyMesh, {TEXT("calf_l"), TEXT("LeftLeg")});
	case ESprawlStreetwearAnchor::FootRight:
		return StreetwearFindBone(BodyMesh, {TEXT("calf_r"), TEXT("RightLeg")});
	default:
		return NAME_None;
	}
}

FLinearColor StreetwearLayerColor(
	ESprawlStreetwearLayer Layer,
	const FSprawlWardrobeOutfit& Outfit)
{
	switch (Layer)
	{
	case ESprawlStreetwearLayer::Hoodie:
		return Outfit.TopColor;
	case ESprawlStreetwearLayer::Bomber:
		return Outfit.OuterwearColor;
	case ESprawlStreetwearLayer::Cargo:
		return Outfit.BottomColor;
	case ESprawlStreetwearLayer::Beanie:
		return FLinearColor::LerpUsingHSV(
			Outfit.OuterwearColor, Outfit.AccentColor, 0.25f);
	case ESprawlStreetwearLayer::Footwear:
		return Outfit.ShoeColor;
	default:
		return FLinearColor::White;
	}
}

void StreetwearApplyPalette(
	UStaticMeshComponent* Component,
	ESprawlStreetwearLayer Layer,
	const FSprawlWardrobeOutfit& Outfit)
{
	if (!Component)
	{
		return;
	}
	// Preserve the imported trainer's multi-material navy/teal/sole treatment.
	if (Layer == ESprawlStreetwearLayer::Footwear)
	{
		return;
	}
	const FLinearColor Color = StreetwearLayerColor(Layer, Outfit);
	static TSoftObjectPtr<UMaterialInterface> WardrobeMaterial(
		FSoftObjectPath(TEXT(
			"/Game/Materials/M_WardrobeAccessory.M_WardrobeAccessory")));
	UMaterialInterface* SharedMaterial = WardrobeMaterial.LoadSynchronous();
	static const FName ColorParameters[] = {
		TEXT("Base Color"), TEXT("BaseColor"), TEXT("Color"),
		TEXT("PrimaryColor"), TEXT("Tint"), TEXT("TintColor"),
		TEXT("diffuse_color"), TEXT("diffuse_color_1"),
		TEXT("B_diffuse_color"), TEXT("C_color")};
	for (int32 MaterialIndex = 0;
		MaterialIndex < Component->GetNumMaterials(); ++MaterialIndex)
	{
		if (SharedMaterial)
		{
			Component->SetMaterial(MaterialIndex, SharedMaterial);
		}
		UMaterialInstanceDynamic* Material =
			Component->CreateDynamicMaterialInstance(MaterialIndex);
		if (!Material)
		{
			continue;
		}
		for (const FName Parameter : ColorParameters)
		{
			Material->SetVectorParameterValue(Parameter, Color);
		}
	}
}

bool StreetwearMaterialIsBaseGarment(const UMaterialInterface* Material)
{
	if (!Material)
	{
		return false;
	}
	const FString Path = Material->GetPathName();
	return Path.Contains(TEXT("DefaultGarment"), ESearchCase::IgnoreCase)
		|| Path.Contains(TEXT("_DG_"), ESearchCase::IgnoreCase)
		|| Path.Contains(TEXT("Shirt"), ESearchCase::IgnoreCase)
		|| Path.Contains(TEXT("Short"), ESearchCase::IgnoreCase);
}

bool StreetwearIsBaseGarmentComponent(const UMeshComponent* Component)
{
	if (!Component || Component->ComponentHasTag(TEXT("SprawlStreetwear"))
		|| Component->ComponentHasTag(TEXT("SprawlWardrobe")))
	{
		return false;
	}

	const FString Name = Component->GetName();
	const bool bNamedGarment =
		Name.Contains(TEXT("Torso"), ESearchCase::IgnoreCase)
		|| Name.Contains(TEXT("Legs"), ESearchCase::IgnoreCase)
		|| Name.Contains(TEXT("Cloth"), ESearchCase::IgnoreCase)
		|| Name.Contains(TEXT("Garment"), ESearchCase::IgnoreCase)
		|| Name.Contains(TEXT("Outfit"), ESearchCase::IgnoreCase);
	if (bNamedGarment)
	{
		return true;
	}

	int32 MaterialCount = 0;
	int32 GarmentMaterialCount = 0;
	for (int32 MaterialIndex = 0;
		MaterialIndex < Component->GetNumMaterials(); ++MaterialIndex)
	{
		if (const UMaterialInterface* Material = Component->GetMaterial(MaterialIndex))
		{
			++MaterialCount;
			GarmentMaterialCount += StreetwearMaterialIsBaseGarment(Material) ? 1 : 0;
		}
	}
	// Never hide a mixed body/skin component just because one slot is clothing.
	return MaterialCount > 0 && GarmentMaterialCount == MaterialCount;
}

bool StreetwearResolveLiveSegment(
	const USkeletalMeshComponent* BodyMesh,
	const FSprawlStreetwearPieceDefinition& Piece,
	FVector& OutStart,
	FVector& OutEnd)
{
	if (!BodyMesh)
	{
		return false;
	}
	const FString Name = Piece.PieceId.ToString();
	const bool bLeft = Name.EndsWith(TEXT("_L"), ESearchCase::IgnoreCase);
	const bool bRight = Name.EndsWith(TEXT("_R"), ESearchCase::IgnoreCase);
	const bool bLowerFollower = Name.Contains(
		TEXT("LowerFollower"), ESearchCase::IgnoreCase);
	if (Name.Contains(TEXT("Sleeve"), ESearchCase::IgnoreCase)
		&& (bLeft || bRight))
	{
		const FName StartBone = bLowerFollower
			? (bLeft
				? StreetwearFindBone(BodyMesh, {TEXT("lowerarm_l"), TEXT("LeftForeArm")})
				: StreetwearFindBone(BodyMesh, {TEXT("lowerarm_r"), TEXT("RightForeArm")}))
			: (bLeft
				? StreetwearFindBone(BodyMesh, {TEXT("upperarm_l"), TEXT("LeftArm")})
				: StreetwearFindBone(BodyMesh, {TEXT("upperarm_r"), TEXT("RightArm")}));
		const FName EndBone = bLowerFollower
			? (bLeft
				? StreetwearFindBone(BodyMesh, {TEXT("hand_l"), TEXT("LeftHand")})
				: StreetwearFindBone(BodyMesh, {TEXT("hand_r"), TEXT("RightHand")}))
			: (bLeft
				? StreetwearFindBone(BodyMesh, {TEXT("lowerarm_l"), TEXT("LeftForeArm")})
				: StreetwearFindBone(BodyMesh, {TEXT("lowerarm_r"), TEXT("RightForeArm")}));
		if (StartBone.IsNone() || EndBone.IsNone())
		{
			return false;
		}
		OutStart = BodyMesh->GetSocketLocation(StartBone);
		OutEnd = BodyMesh->GetSocketLocation(EndBone);
		const FVector OverlapDirection = (OutEnd - OutStart).GetSafeNormal();
		OutStart -= OverlapDirection * 4.f;
		OutEnd += OverlapDirection * 4.f;
		return true;
	}
	if (Name.Contains(TEXT("CargoJoggers_Leg_"), ESearchCase::IgnoreCase)
		&& (bLeft || bRight))
	{
		const FName StartBone = bLowerFollower
			? (bLeft
				? StreetwearFindBone(BodyMesh, {TEXT("calf_l"), TEXT("LeftLeg")})
				: StreetwearFindBone(BodyMesh, {TEXT("calf_r"), TEXT("RightLeg")}))
			: (bLeft
				? StreetwearFindBone(BodyMesh, {TEXT("thigh_l"), TEXT("LeftUpLeg")})
				: StreetwearFindBone(BodyMesh, {TEXT("thigh_r"), TEXT("RightUpLeg")}));
		const FName EndBone = bLowerFollower
			? (bLeft
				? StreetwearFindBone(BodyMesh, {TEXT("foot_l"), TEXT("LeftFoot")})
				: StreetwearFindBone(BodyMesh, {TEXT("foot_r"), TEXT("RightFoot")}))
			: (bLeft
				? StreetwearFindBone(BodyMesh, {TEXT("calf_l"), TEXT("LeftLeg")})
				: StreetwearFindBone(BodyMesh, {TEXT("calf_r"), TEXT("RightLeg")}));
		if (StartBone.IsNone() || EndBone.IsNone())
		{
			return false;
		}
		OutStart = BodyMesh->GetSocketLocation(StartBone);
		OutEnd = BodyMesh->GetSocketLocation(EndBone);
		const FVector OverlapDirection = (OutEnd - OutStart).GetSafeNormal();
		OutStart -= OverlapDirection * 4.f;
		OutEnd += OverlapDirection * 4.f;
		return true;
	}
	return false;
}
}

USprawlStreetwearModule::USprawlStreetwearModule()
{
	// Rigid limb shells need a post-animation refit so the static source
	// art follows bent elbows and knees instead of preserving its spawn pose.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void USprawlStreetwearModule::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearStreetwear();
	Super::EndPlay(EndPlayReason);
}

void USprawlStreetwearModule::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshLiveSegmentFits();
}

FSprawlStreetwearKit USprawlStreetwearModule::CreateZarriNanobananaKit()
{
	FSprawlStreetwearKit Result;
	Result.KitId = TEXT("Zarri_Nanobanana_Streetwear");
	Result.AuthoredHeightCm = 184.f;
	TArray<FSprawlStreetwearPieceDefinition>& Pieces = Result.Pieces;
	Pieces.Reserve(45);

	// Oversized hoodie: its body sits inside the slightly enlarged bomber.
	Pieces.Add(StreetwearPiece(TEXT("SW_OversizedHoodie_Body"),
		ESprawlStreetwearLayer::Hoodie, ESprawlStreetwearAnchor::Spine,
		FVector(0.f, 0.f, 136.f), 0.82f));
	Pieces.Add(StreetwearPiece(TEXT("SW_OversizedHoodie_Sleeve_L"),
		ESprawlStreetwearLayer::Hoodie, ESprawlStreetwearAnchor::UpperArmLeft,
		FVector(1.f, -35.5f, 133.5f), 0.82f));
	Pieces.Add(StreetwearPiece(TEXT("SW_OversizedHoodie_Sleeve_R"),
		ESprawlStreetwearLayer::Hoodie, ESprawlStreetwearAnchor::UpperArmRight,
		FVector(1.f, 35.5f, 133.5f), 0.82f));
	Pieces.Add(StreetwearPiece(TEXT("SW_OversizedHoodie_Cuff_L"),
		ESprawlStreetwearLayer::Hoodie, ESprawlStreetwearAnchor::LowerArmLeft,
		FVector(2.f, -43.f, 110.f), 0.82f));
	Pieces.Add(StreetwearPiece(TEXT("SW_OversizedHoodie_Cuff_R"),
		ESprawlStreetwearLayer::Hoodie, ESprawlStreetwearAnchor::LowerArmRight,
		FVector(2.f, 43.f, 110.f), 0.82f));
	Pieces.Add(StreetwearPiece(TEXT("SW_OversizedHoodie_KangarooPocket"),
		ESprawlStreetwearLayer::Hoodie, ESprawlStreetwearAnchor::Spine,
		FVector(18.1f, 0.f, 123.f), 0.82f));
	Pieces.Add(StreetwearPiece(TEXT("SW_OversizedHoodie_Hem"),
		ESprawlStreetwearLayer::Hoodie, ESprawlStreetwearAnchor::Spine,
		FVector(0.f, 0.f, 109.f), 0.82f));
	Pieces.Add(StreetwearPiece(TEXT("SW_OversizedHoodie_Hood"),
		ESprawlStreetwearLayer::Hoodie, ESprawlStreetwearAnchor::Spine,
		FVector(-4.5f, 0.f, 168.f), 0.82f));
	Pieces.Add(StreetwearPiece(TEXT("SW_OversizedHoodie_Drawcord_L"),
		ESprawlStreetwearLayer::Hoodie, ESprawlStreetwearAnchor::Spine,
		FVector(19.5f, -6.25f, 157.f), 0.82f));
	Pieces.Add(StreetwearPiece(TEXT("SW_OversizedHoodie_Drawcord_R"),
		ESprawlStreetwearLayer::Hoodie, ESprawlStreetwearAnchor::Spine,
		FVector(19.5f, 6.25f, 157.f), 0.82f));

	// Weatherproof bomber: 3.5% separation prevents the two authored shells
	// from occupying the same depth plane after fitting.
	Pieces.Add(StreetwearPiece(TEXT("SW_TechBomber_Body"),
		ESprawlStreetwearLayer::Bomber, ESprawlStreetwearAnchor::Spine,
		FVector(0.f, 0.f, 137.5f), 0.94f));
	Pieces.Add(StreetwearPiece(TEXT("SW_TechBomber_Sleeve_L"),
		ESprawlStreetwearLayer::Bomber, ESprawlStreetwearAnchor::UpperArmLeft,
		FVector(0.5f, -35.5f, 132.f), 0.94f));
	Pieces.Add(StreetwearPiece(TEXT("SW_TechBomber_Sleeve_R"),
		ESprawlStreetwearLayer::Bomber, ESprawlStreetwearAnchor::UpperArmRight,
		FVector(0.5f, 35.5f, 132.f), 0.94f));
	Pieces.Add(StreetwearPiece(TEXT("SW_TechBomber_Cuff_L"),
		ESprawlStreetwearLayer::Bomber, ESprawlStreetwearAnchor::LowerArmLeft,
		FVector(1.f, -44.f, 109.f), 0.94f));
	Pieces.Add(StreetwearPiece(TEXT("SW_TechBomber_Cuff_R"),
		ESprawlStreetwearLayer::Bomber, ESprawlStreetwearAnchor::LowerArmRight,
		FVector(1.f, 44.f, 109.f), 0.94f));
	Pieces.Add(StreetwearPiece(TEXT("SW_TechBomber_Hem"),
		ESprawlStreetwearLayer::Bomber, ESprawlStreetwearAnchor::Spine,
		FVector(0.f, 0.f, 113.f), 0.94f));
	Pieces.Add(StreetwearPiece(TEXT("SW_TechBomber_Zipper"),
		ESprawlStreetwearLayer::Bomber, ESprawlStreetwearAnchor::Spine,
		FVector(20.4f, 0.f, 139.f), 0.94f));
	Pieces.Add(StreetwearPiece(TEXT("SW_TechBomber_Pocket_L"),
		ESprawlStreetwearLayer::Bomber, ESprawlStreetwearAnchor::Spine,
		FVector(19.6f, -16.f, 129.f), 0.94f));
	Pieces.Add(StreetwearPiece(TEXT("SW_TechBomber_Pocket_R"),
		ESprawlStreetwearLayer::Bomber, ESprawlStreetwearAnchor::Spine,
		FVector(19.6f, 16.f, 129.f), 0.94f));

	Pieces.Add(StreetwearPiece(TEXT("SW_CargoJoggers_Waist"),
		ESprawlStreetwearLayer::Cargo, ESprawlStreetwearAnchor::Pelvis,
		FVector(0.f, 0.f, 105.f)));
	Pieces.Add(StreetwearPiece(TEXT("SW_CargoJoggers_Waistband"),
		ESprawlStreetwearLayer::Cargo, ESprawlStreetwearAnchor::Pelvis,
		FVector(0.f, 0.f, 110.f)));
	Pieces.Add(StreetwearPiece(TEXT("SW_CargoJoggers_Leg_L"),
		ESprawlStreetwearLayer::Cargo, ESprawlStreetwearAnchor::ThighLeft,
		FVector(0.f, -11.4f, 58.f), 1.1f));
	Pieces.Add(StreetwearPiece(TEXT("SW_CargoJoggers_Leg_R"),
		ESprawlStreetwearLayer::Cargo, ESprawlStreetwearAnchor::ThighRight,
		FVector(0.f, 11.4f, 58.f), 1.1f));
	Pieces.Add(StreetwearPiece(TEXT("SW_CargoJoggers_Pocket_L"),
		ESprawlStreetwearLayer::Cargo, ESprawlStreetwearAnchor::ThighLeft,
		FVector(11.5f, -12.f, 69.f)));
	Pieces.Add(StreetwearPiece(TEXT("SW_CargoJoggers_Pocket_R"),
		ESprawlStreetwearLayer::Cargo, ESprawlStreetwearAnchor::ThighRight,
		FVector(11.5f, 12.f, 69.f)));
	Pieces.Add(StreetwearPiece(TEXT("SW_CargoJoggers_Cuff_L"),
		ESprawlStreetwearLayer::Cargo, ESprawlStreetwearAnchor::CalfLeft,
		FVector(0.f, -10.8f, 12.f)));
	Pieces.Add(StreetwearPiece(TEXT("SW_CargoJoggers_Cuff_R"),
		ESprawlStreetwearLayer::Cargo, ESprawlStreetwearAnchor::CalfRight,
		FVector(0.f, 10.8f, 12.f)));

	Pieces.Add(StreetwearPiece(TEXT("SW_RibBeanie_Crown"),
		ESprawlStreetwearLayer::Beanie, ESprawlStreetwearAnchor::Head,
		FVector(0.f, 0.f, 193.f), 0.78f));
	Pieces.Add(StreetwearPiece(TEXT("SW_RibBeanie_Cuff"),
		ESprawlStreetwearLayer::Beanie, ESprawlStreetwearAnchor::Head,
		FVector(0.f, 0.f, 184.f), 0.78f));

	// Use the authored Zarri Velocity pair instead of the procedural footwear
	// fallback that produced detached shards in the live game. The hero keeps
	// the major silhouette/material pieces; tiny tread/lace objects stay out of
	// the iPhone-first draw budget.
	for (const bool bLeft : {true, false})
	{
		const TCHAR* Side = bLeft ? TEXT("L") : TEXT("R");
		const float SideY = bLeft ? -6.8f : 6.8f;
		const ESprawlStreetwearAnchor Anchor = bLeft
			? ESprawlStreetwearAnchor::FootLeft
			: ESprawlStreetwearAnchor::FootRight;
		const auto AddShoePiece = [&Pieces, Side, SideY, Anchor](
			const TCHAR* Suffix, const FVector& Center)
		{
			const FString Name = FString::Printf(
				TEXT("SM_ZarriVelocity_%s_%s"), Side, Suffix);
			FVector SideCenter = Center;
			SideCenter.Y = SideY;
			Pieces.Add(StreetwearPiece(*Name,
				ESprawlStreetwearLayer::Footwear, Anchor, SideCenter));
		};
		AddShoePiece(TEXT("Sole"), FVector(0.65f, 0.f, 1.8f));
		AddShoePiece(TEXT("Upper"), FVector(0.65f, 0.f, 6.f));
		AddShoePiece(TEXT("Tongue"), FVector(-2.f, 0.f, 7.9f));
		AddShoePiece(TEXT("LayeredVamp"), FVector(7.8f, 0.f, 5.3f));
		AddShoePiece(TEXT("InstepStrap"), FVector(3.5f, 0.f, 6.4f));
		AddShoePiece(TEXT("ToeBumper"), FVector(14.4f, 0.f, 2.7f));
		AddShoePiece(TEXT("HeelClip"), FVector(-12.f, 0.f, 5.5f));
		AddShoePiece(TEXT("Collar"), FVector(-9.f, 0.f, 9.2f));
	}
	return Result;
}

bool USprawlStreetwearModule::ValidateKit(
	const FSprawlStreetwearKit& Kit, FString& OutError)
{
	if (Kit.KitId.IsNone())
	{
		OutError = TEXT("Streetwear kit requires an ID");
		return false;
	}
	if (!FMath::IsFinite(Kit.AuthoredHeightCm)
		|| Kit.AuthoredHeightCm < 150.f || Kit.AuthoredHeightCm > 220.f)
	{
		OutError = TEXT("Streetwear authored height must remain human-scale");
		return false;
	}
	if (Kit.Pieces.IsEmpty())
	{
		OutError = TEXT("Streetwear kit requires mesh pieces");
		return false;
	}

	TSet<FName> PieceIds;
	TSet<FString> MeshPaths;
	int32 LayerCounts[5] = {0, 0, 0, 0, 0};
	for (const FSprawlStreetwearPieceDefinition& Piece : Kit.Pieces)
	{
		const FString Path = Piece.MeshPath.ToString();
		if (Piece.PieceId.IsNone() || PieceIds.Contains(Piece.PieceId))
		{
			OutError = TEXT("Streetwear piece IDs must be non-empty and unique");
			return false;
		}
		if (!Piece.MeshPath.IsValid()
			|| !Path.StartsWith(StreetwearRoot)
			|| MeshPaths.Contains(Path))
		{
			OutError = TEXT("Streetwear mesh paths must be unique project assets");
			return false;
		}
		if (!StreetwearFiniteVector(Piece.AuthoredCenterCm)
			|| !FMath::IsFinite(Piece.UniformScaleMultiplier)
			|| Piece.UniformScaleMultiplier < 0.7f
			|| Piece.UniformScaleMultiplier > 1.2f)
		{
			OutError = TEXT("Streetwear fit data is invalid");
			return false;
		}
		PieceIds.Add(Piece.PieceId);
		MeshPaths.Add(Path);
		++LayerCounts[static_cast<int32>(Piece.Layer)];
	}
	if (LayerCounts[0] < 3 || LayerCounts[1] < 3
		|| LayerCounts[2] < 3 || LayerCounts[3] < 1
		|| LayerCounts[4] < 4)
	{
		OutError = TEXT("Streetwear kit must include hoodie, bomber, cargo, beanie, and footwear geometry");
		return false;
	}
	OutError.Reset();
	return true;
}

FString USprawlStreetwearModule::DescribeKit(const FSprawlStreetwearKit& Kit)
{
	return FString::Printf(
		TEXT("%s: %d authored pieces fitted from %.0f cm body space"),
		*Kit.KitId.ToString(), Kit.Pieces.Num(), Kit.AuthoredHeightCm);
}

bool USprawlStreetwearModule::SupportsOutfit(const FSprawlWardrobeOutfit& Outfit)
{
	return Outfit.OutfitId.ToString().StartsWith(
		TEXT("NanobananaTech_"), ESearchCase::IgnoreCase);
}

bool USprawlStreetwearModule::ApplyToMetaHuman(
	AActor* VisualActor,
	USkeletalMeshComponent* BodyMesh,
	const FSprawlWardrobeOutfit& Outfit)
{
	ClearStreetwear();
	if (!VisualActor || !BodyMesh || !SupportsOutfit(Outfit))
	{
		return false;
	}

	const FSprawlStreetwearKit Kit = CreateZarriNanobananaKit();
	FString Error;
	if (!ValidateKit(Kit, Error))
	{
		UE_LOG(LogTemp, Error, TEXT("[Streetwear] Invalid kit: %s"), *Error);
		return false;
	}

	const FVector Up = BodyMesh->GetUpVector().GetSafeNormal(
		SMALL_NUMBER, FVector::UpVector);
	const FVector Forward = FVector::VectorPlaneProject(
		GetOwner() ? GetOwner()->GetActorForwardVector()
			: VisualActor->GetActorForwardVector(), Up).GetSafeNormal(
			SMALL_NUMBER, BodyMesh->GetForwardVector());
	const FQuat OutfitRotation = FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat();

	const FName LeftFoot = StreetwearFindBone(
		BodyMesh, {TEXT("foot_l"), TEXT("LeftFoot")});
	const FName RightFoot = StreetwearFindBone(
		BodyMesh, {TEXT("foot_r"), TEXT("RightFoot")});
	const FName Head = StreetwearFindBone(BodyMesh, {TEXT("head"), TEXT("Head")});
	FVector FootCenter = VisualActor->GetActorLocation();
	if (!LeftFoot.IsNone() && !RightFoot.IsNone())
	{
		FootCenter = (BodyMesh->GetSocketLocation(LeftFoot)
			+ BodyMesh->GetSocketLocation(RightFoot)) * 0.5f;
	}

	float BodyScale = 1.f;
	if (!Head.IsNone())
	{
		const float LiveHeight = FVector::DotProduct(
			BodyMesh->GetSocketLocation(Head) - FootCenter, Up) + 16.f;
		BodyScale = FMath::Clamp(LiveHeight / Kit.AuthoredHeightCm, 0.82f, 1.08f);
	}
	const FVector OutfitOrigin = FootCenter - Up * (8.f * BodyScale);
	LiveFitBodyMesh = BodyMesh;
	LiveFitBodyScale = BodyScale;

	for (const FSprawlStreetwearPieceDefinition& Piece : Kit.Pieces)
	{
		UStaticMesh* Mesh = Cast<UStaticMesh>(Piece.MeshPath.TryLoad());
		const FName AnchorBone = StreetwearResolveAnchorBone(BodyMesh, Piece.Anchor);
		UStaticMeshComponent* PrimaryPiece = nullptr;
		if (Mesh && !AnchorBone.IsNone())
		{
			PrimaryPiece = SpawnPiece(
				VisualActor, BodyMesh, Mesh, Piece, AnchorBone,
				OutfitOrigin, OutfitRotation, BodyScale, Outfit);
		}
		if (!PrimaryPiece)
		{
			if (Piece.bRequired)
			{
				++MissingRequiredPieceCount;
			}
			UE_LOG(LogTemp, Warning,
				TEXT("[Streetwear] Missing required=%s piece=%s asset=%s anchor=%d"),
				Piece.bRequired ? TEXT("true") : TEXT("false"),
				*Piece.PieceId.ToString(), *Piece.MeshPath.ToString(),
					static_cast<int32>(Piece.Anchor));
			continue;
		}

		const FString PieceName = Piece.PieceId.ToString();
		const bool bSegmentedSleeve = PieceName.Contains(
			TEXT("Sleeve"), ESearchCase::IgnoreCase);
		const bool bSegmentedLeg = PieceName.Contains(
			TEXT("CargoJoggers_Leg_"), ESearchCase::IgnoreCase);
		if (bSegmentedSleeve || bSegmentedLeg)
		{
			const bool bLeft = PieceName.EndsWith(
				TEXT("_L"), ESearchCase::IgnoreCase);
			FSprawlStreetwearPieceDefinition LowerFollower = Piece;
			const FString BaseName = PieceName.LeftChop(2);
			LowerFollower.PieceId = FName(*FString::Printf(
				TEXT("%s_LowerFollower_%s"), *BaseName,
				bLeft ? TEXT("L") : TEXT("R")));
			LowerFollower.Anchor = bSegmentedLeg
				? (bLeft ? ESprawlStreetwearAnchor::CalfLeft
					: ESprawlStreetwearAnchor::CalfRight)
				: (bLeft ? ESprawlStreetwearAnchor::LowerArmLeft
					: ESprawlStreetwearAnchor::LowerArmRight);
			const FName LowerAnchor = StreetwearResolveAnchorBone(
				BodyMesh, LowerFollower.Anchor);
			if (LowerAnchor.IsNone() || !SpawnPiece(
				VisualActor, BodyMesh, Mesh, LowerFollower, LowerAnchor,
				OutfitOrigin, OutfitRotation, BodyScale, Outfit))
			{
				++MissingRequiredPieceCount;
				UE_LOG(LogTemp, Warning,
					TEXT("[Streetwear] Could not fit lower follower for %s"),
					*PieceName);
			}
		}
	}

	if (MissingRequiredPieceCount > 0)
	{
		ClearStreetwear();
		return false;
	}

	HideBaseGarments(VisualActor);
	HideBareFeet(BodyMesh);
	SetComponentTickEnabled(LiveFitComponents.Num() > 0);
	UE_LOG(LogTemp, Display,
		TEXT("[StreetwearAudit] PASS actor=%s kit=%s pieces=%d hidden_base=%d scale=%.3f"),
		*VisualActor->GetName(), *Kit.KitId.ToString(), SpawnedPieces.Num(),
		HiddenBaseGarments.Num(), BodyScale);
	return MissingRequiredPieceCount == 0;
}

UStaticMeshComponent* USprawlStreetwearModule::SpawnPiece(
	AActor* VisualActor,
	USkeletalMeshComponent* BodyMesh,
	UStaticMesh* Mesh,
	const FSprawlStreetwearPieceDefinition& Piece,
	FName AnchorBone,
	const FVector& OutfitOrigin,
	const FQuat& OutfitRotation,
	float BodyScale,
	const FSprawlWardrobeOutfit& Outfit)
{
	if (!VisualActor || !BodyMesh || !Mesh || AnchorBone.IsNone())
	{
		return nullptr;
	}

	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(
		VisualActor, Piece.PieceId);
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
	Component->ComponentTags.AddUnique(TEXT("SprawlStreetwear"));
	Component->RegisterComponent();

	const FVector Forward = OutfitRotation.GetForwardVector();
	const FVector Right = OutfitRotation.GetRightVector();
	const FVector Up = OutfitRotation.GetUpVector();
	FVector DesiredCenter = OutfitOrigin
		+ Forward * (Piece.AuthoredCenterCm.X * BodyScale)
		+ Right * (Piece.AuthoredCenterCm.Y * BodyScale)
		+ Up * (Piece.AuthoredCenterCm.Z * BodyScale);
	const FBox SourceBounds = Mesh->GetBoundingBox();
	FVector PieceScale(BodyScale * Piece.UniformScaleMultiplier);
	FQuat PieceRotation = OutfitRotation;
	FVector SegmentStart;
	FVector SegmentEnd;
	if (StreetwearResolveLiveSegment(
		BodyMesh, Piece, SegmentStart, SegmentEnd))
	{
		const FVector SegmentDirection = (SegmentEnd - SegmentStart).GetSafeNormal();
		const FVector SourceSize = SourceBounds.GetSize();
		int32 LongestAxis = 0;
		if (SourceSize.Y > SourceSize.X && SourceSize.Y >= SourceSize.Z)
		{
			LongestAxis = 1;
		}
		else if (SourceSize.Z > SourceSize.X && SourceSize.Z > SourceSize.Y)
		{
			LongestAxis = 2;
		}
		const FVector LocalAxis = LongestAxis == 0
			? FVector::ForwardVector
			: LongestAxis == 1 ? FVector::RightVector : FVector::UpVector;
		const FVector CurrentWorldAxis = OutfitRotation.RotateVector(LocalAxis);
		PieceRotation = FQuat::FindBetweenNormals(
			CurrentWorldAxis, SegmentDirection) * OutfitRotation;
		DesiredCenter = (SegmentStart + SegmentEnd) * 0.5f;
		const float SourceLength = SourceSize[LongestAxis];
		if (SourceLength > KINDA_SMALL_NUMBER)
		{
			PieceScale[LongestAxis] =
				FVector::Dist(SegmentStart, SegmentEnd) / SourceLength;
		}
		if (Piece.Layer == ESprawlStreetwearLayer::Cargo)
		{
			// A static tube follows the thigh-to-foot chord rather than bending at
			// the knee. The baggy radial allowance keeps the animated skin inside.
			constexpr float RadialAllowance = 1.15f;
			for (int32 Axis = 0; Axis < 3; ++Axis)
			{
				if (Axis != LongestAxis)
				{
					PieceScale[Axis] *= RadialAllowance;
				}
			}
		}
		LiveFitComponents.Add(Component);
		LiveFitDefinitions.Add(Piece);
	}
	const FVector SourceCenter = SourceBounds.GetCenter();
	const FVector WorldLocation = DesiredCenter
		- PieceRotation.RotateVector(SourceCenter * PieceScale);
	Component->SetWorldTransform(FTransform(
		PieceRotation, WorldLocation, PieceScale));
	Component->SetAbsolute(false, false, true);
	Component->AttachToComponent(
		BodyMesh, FAttachmentTransformRules::KeepWorldTransform, AnchorBone);
	StreetwearApplyPalette(Component, Piece.Layer, Outfit);
	SpawnedPieces.Add(Component);
	SpawnedPieceLayers.Add(Piece.Layer);
	return Component;
}

void USprawlStreetwearModule::RefreshLiveSegmentFits()
{
	USkeletalMeshComponent* BodyMesh = LiveFitBodyMesh.Get();
	const AActor* Owner = GetOwner();
	if (!BodyMesh || !Owner
		|| LiveFitComponents.Num() != LiveFitDefinitions.Num())
	{
		return;
	}
	const FVector Up = BodyMesh->GetUpVector().GetSafeNormal(
		SMALL_NUMBER, FVector::UpVector);
	const FVector Forward = FVector::VectorPlaneProject(
		Owner->GetActorForwardVector(), Up).GetSafeNormal(
		SMALL_NUMBER, BodyMesh->GetForwardVector());
	const FQuat OutfitRotation = FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat();

	for (int32 Index = 0; Index < LiveFitComponents.Num(); ++Index)
	{
		UStaticMeshComponent* Component = LiveFitComponents[Index].Get();
		const FSprawlStreetwearPieceDefinition& Piece = LiveFitDefinitions[Index];
		UStaticMesh* Mesh = Component ? Component->GetStaticMesh() : nullptr;
		FVector SegmentStart;
		FVector SegmentEnd;
		if (!Component || !Mesh || !StreetwearResolveLiveSegment(
			BodyMesh, Piece, SegmentStart, SegmentEnd))
		{
			continue;
		}
		const FVector SegmentDirection =
			(SegmentEnd - SegmentStart).GetSafeNormal();
		if (SegmentDirection.IsNearlyZero())
		{
			continue;
		}
		const FBox SourceBounds = Mesh->GetBoundingBox();
		const FVector SourceSize = SourceBounds.GetSize();
		int32 LongestAxis = 0;
		if (SourceSize.Y > SourceSize.X && SourceSize.Y >= SourceSize.Z)
		{
			LongestAxis = 1;
		}
		else if (SourceSize.Z > SourceSize.X && SourceSize.Z > SourceSize.Y)
		{
			LongestAxis = 2;
		}
		const FVector LocalAxis = LongestAxis == 0
			? FVector::ForwardVector
			: LongestAxis == 1 ? FVector::RightVector : FVector::UpVector;
		const FQuat PieceRotation = FQuat::FindBetweenNormals(
			OutfitRotation.RotateVector(LocalAxis), SegmentDirection)
			* OutfitRotation;
		FVector PieceScale(
			LiveFitBodyScale * Piece.UniformScaleMultiplier);
		const float SourceLength = SourceSize[LongestAxis];
		if (SourceLength > KINDA_SMALL_NUMBER)
		{
			PieceScale[LongestAxis] =
				FVector::Dist(SegmentStart, SegmentEnd) / SourceLength;
		}
		if (Piece.Layer == ESprawlStreetwearLayer::Cargo)
		{
			constexpr float RadialAllowance = 1.15f;
			for (int32 Axis = 0; Axis < 3; ++Axis)
			{
				if (Axis != LongestAxis)
				{
					PieceScale[Axis] *= RadialAllowance;
				}
			}
		}
		const FVector DesiredCenter = (SegmentStart + SegmentEnd) * 0.5f;
		const FVector WorldLocation = DesiredCenter
			- PieceRotation.RotateVector(SourceBounds.GetCenter() * PieceScale);
		Component->SetWorldTransform(FTransform(
			PieceRotation, WorldLocation, PieceScale));
	}
}

void USprawlStreetwearModule::HideBaseGarments(AActor* VisualActor)
{
	if (!VisualActor)
	{
		return;
	}
	TInlineComponentArray<UMeshComponent*> MeshComponents(VisualActor);
	for (UMeshComponent* Component : MeshComponents)
	{
		if (!StreetwearIsBaseGarmentComponent(Component))
		{
			continue;
		}
		HiddenBaseGarments.Add(Component);
		Component->SetVisibility(false, true);
		Component->SetHiddenInGame(true, true);
		if (FParse::Param(FCommandLine::Get(), TEXT("SprawlAuditWardrobe")))
		{
			UE_LOG(LogTemp, Display,
				TEXT("[StreetwearAudit] hid base garment component=%s"),
				*Component->GetName());
		}
	}
}

void USprawlStreetwearModule::HideBareFeet(USkeletalMeshComponent* BodyMesh)
{
	if (!BodyMesh)
	{
		return;
	}
	HiddenFootBodyMesh = BodyMesh;
	for (const FName FootBone : {
		StreetwearFindBone(BodyMesh, {TEXT("foot_l"), TEXT("LeftFoot")}),
		StreetwearFindBone(BodyMesh, {TEXT("foot_r"), TEXT("RightFoot")})})
	{
		if (FootBone.IsNone())
		{
			continue;
		}
		BodyMesh->HideBoneByName(FootBone, PBO_None);
		HiddenFootBones.Add(FootBone);
	}
}

void USprawlStreetwearModule::ClearStreetwear()
{
	for (UStaticMeshComponent* Component : SpawnedPieces)
	{
		if (IsValid(Component))
		{
			Component->DestroyComponent();
		}
	}
	SpawnedPieces.Reset();
	SpawnedPieceLayers.Reset();
	for (const TWeakObjectPtr<UMeshComponent>& WeakComponent : HiddenBaseGarments)
	{
		if (UMeshComponent* Component = WeakComponent.Get())
		{
			Component->SetVisibility(true, true);
			Component->SetHiddenInGame(false, true);
		}
	}
	HiddenBaseGarments.Reset();
	if (USkeletalMeshComponent* BodyMesh = HiddenFootBodyMesh.Get())
	{
		for (const FName FootBone : HiddenFootBones)
		{
			BodyMesh->UnHideBoneByName(FootBone);
		}
	}
	HiddenFootBones.Reset();
	HiddenFootBodyMesh.Reset();
	LiveFitComponents.Reset();
	LiveFitDefinitions.Reset();
	LiveFitBodyMesh.Reset();
	LiveFitBodyScale = 1.f;
	SetComponentTickEnabled(false);
	MissingRequiredPieceCount = 0;
}

void USprawlStreetwearModule::SetLayerVisibility(
	ESprawlStreetwearLayer Layer, bool bVisible)
{
	const int32 PieceCount = FMath::Min(
		SpawnedPieces.Num(), SpawnedPieceLayers.Num());
	for (int32 Index = 0; Index < PieceCount; ++Index)
	{
		if (SpawnedPieceLayers[Index] == Layer
			&& IsValid(SpawnedPieces[Index]))
		{
			SpawnedPieces[Index]->SetVisibility(bVisible, true);
			SpawnedPieces[Index]->SetHiddenInGame(!bVisible, true);
		}
	}
}
