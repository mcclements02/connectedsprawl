// The Connected Sprawl - Reference-fitted shirt and pants assembly.

#include "Characters/SprawlReferenceClothingModule.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
constexpr TCHAR ReferenceClothingRoot[] = TEXT(
	"/Game/Import/Characters/ReferenceClothing/");

FSprawlReferenceClothingPiece ReferencePiece(
	const TCHAR* Name,
	ESprawlReferenceClothingLayer Layer,
	ESprawlReferenceClothingAnchor Anchor,
	const FVector& AuthoredCenterCm,
	bool bFollowBoneSegment = false,
	const FVector& AuthoredDirection = FVector::UpVector,
	float AuthoredLengthCm = 0.f,
	float SegmentStartFraction = 0.f,
	float SegmentEndFraction = 1.f,
	const FVector2D& FollowerOffsetCm = FVector2D::ZeroVector)
{
	FSprawlReferenceClothingPiece Result;
	Result.PieceId = FName(Name);
	Result.MeshPath = FSoftObjectPath(FString::Printf(
		TEXT("%s%s.%s"), ReferenceClothingRoot, Name, Name));
	Result.Layer = Layer;
	Result.Anchor = Anchor;
	Result.AuthoredCenterCm = AuthoredCenterCm;
	Result.bFollowBoneSegment = bFollowBoneSegment;
	Result.AuthoredDirection = AuthoredDirection.GetSafeNormal();
	Result.AuthoredLengthCm = AuthoredLengthCm;
	Result.SegmentStartFraction = SegmentStartFraction;
	Result.SegmentEndFraction = SegmentEndFraction;
	Result.FollowerOffsetCm = FollowerOffsetCm;
	Result.bRequired = true;
	return Result;
}

bool IsFiniteVector(const FVector& Vector)
{
	return FMath::IsFinite(Vector.X)
		&& FMath::IsFinite(Vector.Y)
		&& FMath::IsFinite(Vector.Z);
}

FName FindBone(
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

FName ResolveAnchorBone(
	const USkeletalMeshComponent* BodyMesh,
	ESprawlReferenceClothingAnchor Anchor)
{
	switch (Anchor)
	{
	case ESprawlReferenceClothingAnchor::Spine:
		return FindBone(
			BodyMesh, {TEXT("spine_03"), TEXT("spine_04"), TEXT("Spine2")});
	case ESprawlReferenceClothingAnchor::Pelvis:
		return FindBone(BodyMesh, {TEXT("pelvis"), TEXT("Hips")});
	case ESprawlReferenceClothingAnchor::UpperArmLeft:
		return FindBone(BodyMesh, {TEXT("upperarm_l"), TEXT("LeftArm")});
	case ESprawlReferenceClothingAnchor::UpperArmRight:
		return FindBone(BodyMesh, {TEXT("upperarm_r"), TEXT("RightArm")});
	case ESprawlReferenceClothingAnchor::LowerArmLeft:
		return FindBone(BodyMesh, {TEXT("lowerarm_l"), TEXT("LeftForeArm")});
	case ESprawlReferenceClothingAnchor::LowerArmRight:
		return FindBone(BodyMesh, {TEXT("lowerarm_r"), TEXT("RightForeArm")});
	case ESprawlReferenceClothingAnchor::HandLeft:
		return FindBone(BodyMesh, {TEXT("hand_l"), TEXT("LeftHand")});
	case ESprawlReferenceClothingAnchor::HandRight:
		return FindBone(BodyMesh, {TEXT("hand_r"), TEXT("RightHand")});
	case ESprawlReferenceClothingAnchor::ThighLeft:
		return FindBone(BodyMesh, {TEXT("thigh_l"), TEXT("LeftUpLeg")});
	case ESprawlReferenceClothingAnchor::ThighRight:
		return FindBone(BodyMesh, {TEXT("thigh_r"), TEXT("RightUpLeg")});
	case ESprawlReferenceClothingAnchor::CalfLeft:
		return FindBone(BodyMesh, {TEXT("calf_l"), TEXT("LeftLeg")});
	case ESprawlReferenceClothingAnchor::CalfRight:
		return FindBone(BodyMesh, {TEXT("calf_r"), TEXT("RightLeg")});
	default:
		return NAME_None;
	}
}

bool ResolveSegment(
	const USkeletalMeshComponent* BodyMesh,
	const FSprawlReferenceClothingPiece& Piece,
	FVector& OutStart,
	FVector& OutEnd)
{
	if (!BodyMesh || !Piece.bFollowBoneSegment)
	{
		return false;
	}
	FName StartBone = NAME_None;
	FName EndBone = NAME_None;
	switch (Piece.Anchor)
	{
	case ESprawlReferenceClothingAnchor::UpperArmLeft:
		StartBone = FindBone(BodyMesh, {TEXT("upperarm_l"), TEXT("LeftArm")});
		EndBone = FindBone(BodyMesh, {TEXT("lowerarm_l"), TEXT("LeftForeArm")});
		break;
	case ESprawlReferenceClothingAnchor::UpperArmRight:
		StartBone = FindBone(BodyMesh, {TEXT("upperarm_r"), TEXT("RightArm")});
		EndBone = FindBone(BodyMesh, {TEXT("lowerarm_r"), TEXT("RightForeArm")});
		break;
	case ESprawlReferenceClothingAnchor::LowerArmLeft:
		StartBone = FindBone(BodyMesh, {TEXT("lowerarm_l"), TEXT("LeftForeArm")});
		EndBone = FindBone(BodyMesh, {TEXT("hand_l"), TEXT("LeftHand")});
		break;
	case ESprawlReferenceClothingAnchor::LowerArmRight:
		StartBone = FindBone(BodyMesh, {TEXT("lowerarm_r"), TEXT("RightForeArm")});
		EndBone = FindBone(BodyMesh, {TEXT("hand_r"), TEXT("RightHand")});
		break;
	case ESprawlReferenceClothingAnchor::ThighLeft:
		StartBone = FindBone(BodyMesh, {TEXT("thigh_l"), TEXT("LeftUpLeg")});
		EndBone = FindBone(BodyMesh, {TEXT("calf_l"), TEXT("LeftLeg")});
		break;
	case ESprawlReferenceClothingAnchor::ThighRight:
		StartBone = FindBone(BodyMesh, {TEXT("thigh_r"), TEXT("RightUpLeg")});
		EndBone = FindBone(BodyMesh, {TEXT("calf_r"), TEXT("RightLeg")});
		break;
	case ESprawlReferenceClothingAnchor::CalfLeft:
		StartBone = FindBone(BodyMesh, {TEXT("calf_l"), TEXT("LeftLeg")});
		EndBone = FindBone(BodyMesh, {TEXT("foot_l"), TEXT("LeftFoot")});
		break;
	case ESprawlReferenceClothingAnchor::CalfRight:
		StartBone = FindBone(BodyMesh, {TEXT("calf_r"), TEXT("RightLeg")});
		EndBone = FindBone(BodyMesh, {TEXT("foot_r"), TEXT("RightFoot")});
		break;
	default:
		return false;
	}
	if (StartBone.IsNone() || EndBone.IsNone())
	{
		return false;
	}
	const FVector BoneStart = BodyMesh->GetSocketLocation(StartBone);
	const FVector BoneEnd = BodyMesh->GetSocketLocation(EndBone);
	OutStart = FMath::Lerp(
		BoneStart, BoneEnd, Piece.SegmentStartFraction);
	OutEnd = FMath::Lerp(
		BoneStart, BoneEnd, Piece.SegmentEndFraction);
	return !OutStart.Equals(OutEnd, KINDA_SMALL_NUMBER);
}

FLinearColor LayerColor(ESprawlReferenceClothingLayer Layer)
{
	return Layer == ESprawlReferenceClothingLayer::Shirt
		? FLinearColor(0.22f, 0.24f, 0.27f)
		: FLinearColor(0.025f, 0.035f, 0.052f);
}

void ApplyPalette(
	UStaticMeshComponent* Component,
	ESprawlReferenceClothingLayer Layer)
{
	if (!Component)
	{
		return;
	}
	static TSoftObjectPtr<UMaterialInterface> WardrobeMaterial(
		FSoftObjectPath(TEXT(
			"/Game/Materials/M_WardrobeAccessory.M_WardrobeAccessory")));
	UMaterialInterface* SharedMaterial = WardrobeMaterial.LoadSynchronous();
	const FLinearColor Color = LayerColor(Layer);
	static const FName ColorParameters[] = {
		TEXT("Base Color"), TEXT("BaseColor"), TEXT("Color"),
		TEXT("PrimaryColor"), TEXT("Tint"), TEXT("TintColor")};
	for (int32 Index = 0; Index < Component->GetNumMaterials(); ++Index)
	{
		if (SharedMaterial)
		{
			Component->SetMaterial(Index, SharedMaterial);
		}
		if (UMaterialInstanceDynamic* Material =
			Component->CreateDynamicMaterialInstance(Index))
		{
			for (const FName Parameter : ColorParameters)
			{
				Material->SetVectorParameterValue(Parameter, Color);
			}
		}
	}
}
}

USprawlReferenceClothingModule::USprawlReferenceClothingModule()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void USprawlReferenceClothingModule::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	ClearClothing();
	Super::EndPlay(EndPlayReason);
}

void USprawlReferenceClothingModule::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshLiveSegmentFits();
}

FSprawlReferenceClothingKit
USprawlReferenceClothingModule::CreateZarriReferenceKit()
{
	FSprawlReferenceClothingKit Result;
	Result.KitId = TEXT("Zarri_Reference_GrayShirt_DarkPants");
	Result.AuthoredHeightCm = 184.f;
	TArray<FSprawlReferenceClothingPiece>& Pieces = Result.Pieces;
	Pieces.Reserve(17);

	Pieces.Add(ReferencePiece(TEXT("SM_ReferenceShirt_Torso"),
		ESprawlReferenceClothingLayer::Shirt,
		ESprawlReferenceClothingAnchor::Spine, FVector(0.f, 0.f, 134.f)));
	Pieces.Add(ReferencePiece(TEXT("SM_ReferenceShirt_Collar"),
		ESprawlReferenceClothingLayer::Shirt,
		ESprawlReferenceClothingAnchor::Spine, FVector(0.f, 0.f, 163.5f)));
	Pieces.Add(ReferencePiece(TEXT("SM_ReferenceShirt_Hem"),
		ESprawlReferenceClothingLayer::Shirt,
		ESprawlReferenceClothingAnchor::Spine, FVector(0.f, 0.f, 104.5f)));
	Pieces.Add(ReferencePiece(TEXT("SM_ReferenceShirt_UpperSleeve_L"),
		ESprawlReferenceClothingLayer::Shirt,
		ESprawlReferenceClothingAnchor::UpperArmLeft,
		FVector(0.f, -38.5f, 143.f), true,
		FVector(0.f, -20.f, -23.f), 30.48f, -0.22f, 1.18f));
	Pieces.Add(ReferencePiece(TEXT("SM_ReferenceShirt_LowerSleeve_L"),
		ESprawlReferenceClothingLayer::Shirt,
		ESprawlReferenceClothingAnchor::LowerArmLeft,
		FVector(0.f, -54.f, 121.5f), true,
		FVector(0.f, -15.f, -24.f), 28.30f, -0.18f, 1.08f));
	Pieces.Add(ReferencePiece(TEXT("SM_ReferenceShirt_Cuff_L"),
		ESprawlReferenceClothingLayer::Shirt,
		ESprawlReferenceClothingAnchor::LowerArmLeft,
		FVector(0.f, -47.5f, 132.5f), true,
		FVector(0.f, -15.f, -24.f), 20.f, -0.24f, 0.24f));
	Pieces.Add(ReferencePiece(TEXT("SM_ReferenceShirt_UpperSleeve_R"),
		ESprawlReferenceClothingLayer::Shirt,
		ESprawlReferenceClothingAnchor::UpperArmRight,
		FVector(0.f, 38.5f, 143.f), true,
		FVector(0.f, 20.f, -23.f), 30.48f, -0.22f, 1.18f));
	Pieces.Add(ReferencePiece(TEXT("SM_ReferenceShirt_LowerSleeve_R"),
		ESprawlReferenceClothingLayer::Shirt,
		ESprawlReferenceClothingAnchor::LowerArmRight,
		FVector(0.f, 54.f, 121.5f), true,
		FVector(0.f, 15.f, -24.f), 28.30f, -0.18f, 1.08f));
	Pieces.Add(ReferencePiece(TEXT("SM_ReferenceShirt_Cuff_R"),
		ESprawlReferenceClothingLayer::Shirt,
		ESprawlReferenceClothingAnchor::LowerArmRight,
		FVector(0.f, 47.5f, 132.5f), true,
		FVector(0.f, 15.f, -24.f), 20.f, -0.24f, 0.24f));

	Pieces.Add(ReferencePiece(TEXT("SM_ReferencePants_Waist"),
		ESprawlReferenceClothingLayer::Pants,
		ESprawlReferenceClothingAnchor::Pelvis, FVector(0.f, 0.f, 95.75f)));
	Pieces.Add(ReferencePiece(TEXT("SM_ReferencePants_Waistband"),
		ESprawlReferenceClothingLayer::Pants,
		ESprawlReferenceClothingAnchor::Pelvis, FVector(0.f, 0.f, 107.5f)));
	Pieces.Add(ReferencePiece(TEXT("SM_ReferencePants_UpperLeg_L"),
		ESprawlReferenceClothingLayer::Pants,
		ESprawlReferenceClothingAnchor::ThighLeft,
		FVector(0.f, -11.15f, 67.5f), true,
		FVector(0.f, 0.7f, -37.f), 37.01f, -0.12f, 1.18f));
	Pieces.Add(ReferencePiece(TEXT("SM_ReferencePants_LowerLeg_L"),
		ESprawlReferenceClothingLayer::Pants,
		ESprawlReferenceClothingAnchor::CalfLeft,
		FVector(0.f, -10.5f, 33.75f), true,
		FVector(0.f, 0.6f, -46.5f), 46.50f, -0.18f, 1.04f,
		FVector2D(-10.5f, 0.f)));
	Pieces.Add(ReferencePiece(TEXT("SM_ReferencePants_Cuff_L"),
		ESprawlReferenceClothingLayer::Pants,
		ESprawlReferenceClothingAnchor::CalfLeft,
		FVector(0.f, -10.8f, 53.f), true,
		FVector(0.f, 0.6f, -46.5f), 25.f, -0.24f, 0.24f));
	Pieces.Add(ReferencePiece(TEXT("SM_ReferencePants_UpperLeg_R"),
		ESprawlReferenceClothingLayer::Pants,
		ESprawlReferenceClothingAnchor::ThighRight,
		FVector(0.f, 11.15f, 67.5f), true,
		FVector(0.f, -0.7f, -37.f), 37.01f, -0.12f, 1.18f));
	Pieces.Add(ReferencePiece(TEXT("SM_ReferencePants_LowerLeg_R"),
		ESprawlReferenceClothingLayer::Pants,
		ESprawlReferenceClothingAnchor::CalfRight,
		FVector(0.f, 10.5f, 33.75f), true,
		FVector(0.f, -0.6f, -46.5f), 46.50f, -0.18f, 1.04f,
		FVector2D(-10.5f, 0.f)));
	Pieces.Add(ReferencePiece(TEXT("SM_ReferencePants_Cuff_R"),
		ESprawlReferenceClothingLayer::Pants,
		ESprawlReferenceClothingAnchor::CalfRight,
		FVector(0.f, 10.8f, 53.f), true,
		FVector(0.f, -0.6f, -46.5f), 25.f, -0.24f, 0.24f));
	return Result;
}

bool USprawlReferenceClothingModule::ValidateKit(
	const FSprawlReferenceClothingKit& Kit, FString& OutError)
{
	if (Kit.KitId.IsNone())
	{
		OutError = TEXT("Reference clothing kit requires an ID");
		return false;
	}
	if (!FMath::IsFinite(Kit.AuthoredHeightCm)
		|| Kit.AuthoredHeightCm < 150.f || Kit.AuthoredHeightCm > 220.f)
	{
		OutError = TEXT("Reference clothing authored height must be human-scale");
		return false;
	}
	TSet<FName> PieceIds;
	TSet<FString> Paths;
	int32 ShirtCount = 0;
	int32 PantsCount = 0;
	for (const FSprawlReferenceClothingPiece& Piece : Kit.Pieces)
	{
		const FString Path = Piece.MeshPath.ToString();
		if (Piece.PieceId.IsNone() || PieceIds.Contains(Piece.PieceId)
			|| !Piece.MeshPath.IsValid()
			|| !Path.StartsWith(ReferenceClothingRoot)
			|| Paths.Contains(Path)
			|| !IsFiniteVector(Piece.AuthoredCenterCm)
			|| !IsFiniteVector(Piece.AuthoredDirection)
			|| !FMath::IsFinite(Piece.FollowerOffsetCm.X)
			|| !FMath::IsFinite(Piece.FollowerOffsetCm.Y)
			|| (Piece.bFollowBoneSegment
				&& (Piece.AuthoredDirection.IsNearlyZero()
					|| !FMath::IsFinite(Piece.AuthoredLengthCm)
					|| Piece.AuthoredLengthCm < 1.f
					|| !FMath::IsFinite(Piece.SegmentStartFraction)
					|| !FMath::IsFinite(Piece.SegmentEndFraction)
					|| Piece.SegmentStartFraction < -0.25f
					|| Piece.SegmentEndFraction > 1.25f
					|| Piece.SegmentStartFraction
						>= Piece.SegmentEndFraction)))
		{
			OutError = TEXT("Reference clothing pieces require unique valid project assets and fit data");
			return false;
		}
		PieceIds.Add(Piece.PieceId);
		Paths.Add(Path);
		if (Piece.Layer == ESprawlReferenceClothingLayer::Shirt)
		{
			++ShirtCount;
		}
		else
		{
			++PantsCount;
		}
	}
	if (ShirtCount != 9 || PantsCount != 8)
	{
		OutError = TEXT("Reference clothing kit requires nine shirt and eight pants pieces");
		return false;
	}
	OutError.Reset();
	return true;
}

FString USprawlReferenceClothingModule::DescribeKit(
	const FSprawlReferenceClothingKit& Kit)
{
	return FString::Printf(
		TEXT("%s: %d smooth pieces mapped from %.0f cm Blender body space"),
		*Kit.KitId.ToString(), Kit.Pieces.Num(), Kit.AuthoredHeightCm);
}

bool USprawlReferenceClothingModule::ApplyToMetaHuman(
	AActor* VisualActor, USkeletalMeshComponent* BodyMesh)
{
	ClearClothing();
	if (!VisualActor || !BodyMesh || !BodyMesh->GetSkeletalMeshAsset())
	{
		return false;
	}
	const FSprawlReferenceClothingKit Kit = CreateZarriReferenceKit();
	FString Error;
	if (!ValidateKit(Kit, Error))
	{
		UE_LOG(LogTemp, Error, TEXT("[ReferenceClothing] Invalid kit: %s"), *Error);
		return false;
	}

	const FVector Up = BodyMesh->GetUpVector().GetSafeNormal(
		SMALL_NUMBER, FVector::UpVector);
	const FVector Forward = FVector::VectorPlaneProject(
		GetOwner() ? GetOwner()->GetActorForwardVector()
			: VisualActor->GetActorForwardVector(), Up).GetSafeNormal(
		SMALL_NUMBER, BodyMesh->GetForwardVector());
	const FQuat OutfitRotation = FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat();
	const FName LeftFoot = FindBone(BodyMesh, {TEXT("foot_l"), TEXT("LeftFoot")});
	const FName RightFoot = FindBone(BodyMesh, {TEXT("foot_r"), TEXT("RightFoot")});
	const FName Head = FindBone(BodyMesh, {TEXT("head"), TEXT("Head")});
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
		BodyScale = FMath::Clamp(
			LiveHeight / Kit.AuthoredHeightCm, 0.82f, 1.08f);
	}
	const FVector OutfitOrigin = FootCenter - Up * (8.f * BodyScale);
	LiveBodyMesh = BodyMesh;
	LiveBodyScale = BodyScale;

	for (const FSprawlReferenceClothingPiece& Piece : Kit.Pieces)
	{
		UStaticMesh* Mesh = Cast<UStaticMesh>(Piece.MeshPath.TryLoad());
		const FName Anchor = ResolveAnchorBone(BodyMesh, Piece.Anchor);
		if (!Mesh || Anchor.IsNone() || !SpawnPiece(
			VisualActor, BodyMesh, Mesh, Piece, Anchor,
			OutfitOrigin, OutfitRotation, BodyScale))
		{
			MissingRequiredPieceCount += Piece.bRequired ? 1 : 0;
			UE_LOG(LogTemp, Warning,
				TEXT("[ReferenceClothing] Missing piece=%s asset=%s anchor=%d"),
				*Piece.PieceId.ToString(), *Piece.MeshPath.ToString(),
				static_cast<int32>(Piece.Anchor));
		}
	}
	if (MissingRequiredPieceCount > 0)
	{
		ClearClothing();
		return false;
	}
	SetComponentTickEnabled(!LiveFitComponents.IsEmpty());
	UE_LOG(LogTemp, Display,
		TEXT("[ReferenceClothingAudit] PASS actor=%s kit=%s pieces=%d live_fits=%d scale=%.3f"),
		*VisualActor->GetName(), *Kit.KitId.ToString(), SpawnedPieces.Num(),
		LiveFitComponents.Num(), BodyScale);
	return true;
}

UStaticMeshComponent* USprawlReferenceClothingModule::SpawnPiece(
	AActor* VisualActor,
	USkeletalMeshComponent* BodyMesh,
	UStaticMesh* Mesh,
	const FSprawlReferenceClothingPiece& Piece,
	FName AnchorBone,
	const FVector& OutfitOrigin,
	const FQuat& OutfitRotation,
	float BodyScale)
{
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
	Component->ComponentTags.AddUnique(TEXT("SprawlReferenceClothing"));
	Component->RegisterComponent();

	const FVector Forward = OutfitRotation.GetForwardVector();
	const FVector Right = OutfitRotation.GetRightVector();
	const FVector Up = OutfitRotation.GetUpVector();
	FVector DesiredCenter = OutfitOrigin
		+ Forward * (Piece.AuthoredCenterCm.X * BodyScale)
		+ Right * (Piece.AuthoredCenterCm.Y * BodyScale)
		+ Up * (Piece.AuthoredCenterCm.Z * BodyScale);
	const FBox Bounds = Mesh->GetBoundingBox();
	FVector PieceScale(BodyScale);
	FQuat PieceRotation = OutfitRotation;
	FVector SegmentStart;
	FVector SegmentEnd;
	if (ResolveSegment(BodyMesh, Piece, SegmentStart, SegmentEnd))
	{
		const FVector Direction = (SegmentEnd - SegmentStart).GetSafeNormal();
		PieceRotation = FQuat::FindBetweenNormals(
			OutfitRotation.RotateVector(FVector::UpVector), Direction)
			* OutfitRotation;
		DesiredCenter = (SegmentStart + SegmentEnd) * 0.5f
			+ Forward * Piece.FollowerOffsetCm.X
			+ Right * Piece.FollowerOffsetCm.Y;
		PieceScale = FVector(
			BodyScale, BodyScale,
			FVector::Dist(SegmentStart, SegmentEnd)
				/ Piece.AuthoredLengthCm);
		LiveFitComponents.Add(Component);
		LiveFitDefinitions.Add(Piece);
	}
	const FVector WorldLocation = DesiredCenter
		- PieceRotation.RotateVector(Bounds.GetCenter() * PieceScale);
	Component->SetWorldTransform(FTransform(
		PieceRotation, WorldLocation, PieceScale));
	Component->SetAbsolute(false, false, true);
	Component->AttachToComponent(
		BodyMesh, FAttachmentTransformRules::KeepWorldTransform, AnchorBone);
	ApplyPalette(Component, Piece.Layer);
	SpawnedPieces.Add(Component);
	SpawnedLayers.Add(Piece.Layer);
	return Component;
}

void USprawlReferenceClothingModule::RefreshLiveSegmentFits()
{
	USkeletalMeshComponent* BodyMesh = LiveBodyMesh.Get();
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
		const FSprawlReferenceClothingPiece& Piece = LiveFitDefinitions[Index];
		UStaticMesh* Mesh = Component ? Component->GetStaticMesh() : nullptr;
		FVector SegmentStart;
		FVector SegmentEnd;
		if (!Mesh || !ResolveSegment(
			BodyMesh, Piece, SegmentStart, SegmentEnd))
		{
			continue;
		}
		const FVector Direction = (SegmentEnd - SegmentStart).GetSafeNormal();
		const FBox Bounds = Mesh->GetBoundingBox();
		const FQuat PieceRotation = FQuat::FindBetweenNormals(
			OutfitRotation.RotateVector(FVector::UpVector), Direction)
			* OutfitRotation;
		const FVector PieceScale(
			LiveBodyScale, LiveBodyScale,
			FVector::Dist(SegmentStart, SegmentEnd)
				/ Piece.AuthoredLengthCm);
		const FVector Center = (SegmentStart + SegmentEnd) * 0.5f
			+ Forward * Piece.FollowerOffsetCm.X
			+ OutfitRotation.GetRightVector() * Piece.FollowerOffsetCm.Y;
		const FVector Location = Center
			- PieceRotation.RotateVector(Bounds.GetCenter() * PieceScale);
		Component->SetWorldTransform(FTransform(
			PieceRotation, Location, PieceScale));
	}
}

void USprawlReferenceClothingModule::ClearClothing()
{
	for (UStaticMeshComponent* Component : SpawnedPieces)
	{
		if (IsValid(Component))
		{
			Component->DestroyComponent();
		}
	}
	SpawnedPieces.Reset();
	SpawnedLayers.Reset();
	LiveFitComponents.Reset();
	LiveFitDefinitions.Reset();
	LiveBodyMesh.Reset();
	LiveBodyScale = 1.f;
	MissingRequiredPieceCount = 0;
	SetComponentTickEnabled(false);
}

void USprawlReferenceClothingModule::SetLayerVisibility(
	ESprawlReferenceClothingLayer Layer, bool bVisible)
{
	const int32 Count = FMath::Min(SpawnedPieces.Num(), SpawnedLayers.Num());
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (SpawnedLayers[Index] == Layer && IsValid(SpawnedPieces[Index]))
		{
			SpawnedPieces[Index]->SetVisibility(bVisible, true);
			SpawnedPieces[Index]->SetHiddenInGame(!bVisible, true);
		}
	}
}
