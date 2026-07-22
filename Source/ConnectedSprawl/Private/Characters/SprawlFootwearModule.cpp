// The Connected Sprawl - Anatomically fitted MetaHuman footwear presentation.

#include "Characters/SprawlFootwearModule.h"

#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
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
struct FSprawlFootwearMesh
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
};

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
		const FName BoneName(Candidate);
		if (BodyMesh->GetBoneIndex(BoneName) != INDEX_NONE)
		{
			return BoneName;
		}
	}
	return NAME_None;
}

FVector SafeDirection(
	const FVector& From, const FVector& To, const FVector& Fallback)
{
	return (To - From).GetSafeNormal(
		SMALL_NUMBER,
		Fallback.GetSafeNormal(SMALL_NUMBER, FVector::UpVector));
}

void CalculateSmoothNormals(FSprawlFootwearMesh& Mesh)
{
	Mesh.Normals.Init(FVector::ZeroVector, Mesh.Vertices.Num());
	for (int32 TriangleIndex = 0;
		TriangleIndex + 2 < Mesh.Triangles.Num(); TriangleIndex += 3)
	{
		const int32 A = Mesh.Triangles[TriangleIndex];
		const int32 B = Mesh.Triangles[TriangleIndex + 1];
		const int32 C = Mesh.Triangles[TriangleIndex + 2];
		if (!Mesh.Vertices.IsValidIndex(A)
			|| !Mesh.Vertices.IsValidIndex(B)
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

void AddLoftCaps(
	FSprawlFootwearMesh& Mesh,
	int32 RingStride,
	int32 RingCount,
	bool bClosedRing)
{
	if (RingStride < 3 || RingCount < 2)
	{
		return;
	}
	for (int32 End = 0; End < 2; ++End)
	{
		const int32 RingBase = End == 0 ? 0 : (RingCount - 1) * RingStride;
		FVector Center = FVector::ZeroVector;
		for (int32 Point = 0; Point < RingStride; ++Point)
		{
			Center += Mesh.Vertices[RingBase + Point];
		}
		Center /= static_cast<float>(RingStride);
		const int32 CapRingBase = Mesh.Vertices.Num();
		for (int32 Point = 0; Point < RingStride; ++Point)
		{
			const FVector CapVertex = Mesh.Vertices[RingBase + Point];
			Mesh.Vertices.Add(CapVertex);
			Mesh.UV0.Add(FVector2D(
				0.5f + (CapVertex.Y - Center.Y) * 0.04f,
				0.5f + (CapVertex.Z - Center.Z) * 0.04f));
		}
		const int32 CenterIndex = Mesh.Vertices.Add(Center);
		Mesh.UV0.Add(FVector2D(0.5f, 0.5f));
		const int32 SegmentCount = bClosedRing ? RingStride : RingStride - 1;
		for (int32 Point = 0; Point < SegmentCount; ++Point)
		{
			const int32 Next = (Point + 1) % RingStride;
			if (End == 0)
			{
				Mesh.Triangles.Append({
					CenterIndex, CapRingBase + Next, CapRingBase + Point});
			}
			else
			{
				Mesh.Triangles.Append({
					CenterIndex, CapRingBase + Point, CapRingBase + Next});
			}
		}
	}
}

FSprawlFootwearMesh BuildRoundedUpper(
	const FSprawlFootwearDimensions& Dimensions,
	ESprawlWardrobeFootwear Footwear)
{
	constexpr int32 StationCount = 8;
	constexpr int32 CrossSegments = 12;
	constexpr int32 RingStride = CrossSegments + 1;
	const float StationFractions[StationCount] = {
		0.f, 0.10f, 0.26f, 0.46f, 0.65f, 0.81f, 0.93f, 1.f};
	const float WidthFractions[StationCount] = {
		0.72f, 0.88f, 0.96f, 1.f, 0.99f, 0.94f, 0.82f, 0.68f};
	const float HeightFractions[StationCount] = {
		0.82f, 1.f, 0.95f, 0.82f, 0.67f, 0.53f, 0.40f, 0.31f};
	const bool bTall = Footwear == ESprawlWardrobeFootwear::HighTopSneakers
		|| Footwear == ESprawlWardrobeFootwear::WorkBoots;
	const bool bAthletic =
		Footwear == ESprawlWardrobeFootwear::AthleticTrainers;
	const float HalfWidth = Dimensions.ShoeWidthCm * 0.5f;
	const float BaseZ = Dimensions.SoleThicknessCm * 0.72f;

	FSprawlFootwearMesh Result;
	Result.Vertices.Reserve(StationCount * RingStride + 2);
	Result.UV0.Reserve(StationCount * RingStride + 2);
	for (int32 Station = 0; Station < StationCount; ++Station)
	{
		const float X = StationFractions[Station] * Dimensions.ShoeLengthCm;
		const float Width = HalfWidth * WidthFractions[Station];
		float Height = Dimensions.UpperHeightCm * HeightFractions[Station];
		if (bAthletic)
		{
			// Keep the heel locked while tapering toward a flexible toe box.
			Height *= Station <= 2 ? 1.06f : 0.96f;
		}
		if (bTall && Station <= 2)
		{
			Height = Dimensions.UpperHeightCm * (Station == 1 ? 1.f : 0.92f);
		}
		for (int32 Cross = 0; Cross <= CrossSegments; ++Cross)
		{
			const float Alpha = static_cast<float>(Cross) / CrossSegments;
			const float Angle = PI - PI * Alpha;
			const float Y = FMath::Cos(Angle) * Width;
			const float Z = BaseZ + FMath::Sin(Angle) * (Height - BaseZ);
			Result.Vertices.Add(FVector(X, Y, Z));
			Result.UV0.Add(FVector2D(
				StationFractions[Station], Alpha));
		}
	}
	for (int32 Station = 0; Station < StationCount - 1; ++Station)
	{
		for (int32 Cross = 0; Cross < CrossSegments; ++Cross)
		{
			const int32 A = Station * RingStride + Cross;
			const int32 B = (Station + 1) * RingStride + Cross;
			const int32 C = B + 1;
			const int32 D = A + 1;
			Result.Triangles.Append({A, B, C, A, C, D});
		}
	}
	AddLoftCaps(Result, RingStride, StationCount, false);
	CalculateSmoothNormals(Result);
	return Result;
}

FSprawlFootwearMesh BuildRoundedSole(
	const FSprawlFootwearDimensions& Dimensions,
	ESprawlWardrobeFootwear Footwear)
{
	constexpr int32 StationCount = 9;
	constexpr int32 CrossSegments = 12;
	const float StationFractions[StationCount] = {
		0.f, 0.06f, 0.18f, 0.36f, 0.56f, 0.74f, 0.88f, 0.96f, 1.f};
	const float WidthFractions[StationCount] = {
		0.68f, 0.82f, 0.94f, 1.f, 0.99f, 0.96f, 0.88f, 0.74f, 0.62f};
	const float DressNarrowing =
		Footwear == ESprawlWardrobeFootwear::DressShoes ? 0.92f : 1.f;
	const float HalfWidth = Dimensions.ShoeWidthCm * 0.53f * DressNarrowing;
	const float HalfHeight = Dimensions.SoleThicknessCm * 0.5f;
	const int32 RingStride = CrossSegments;

	FSprawlFootwearMesh Result;
	Result.Vertices.Reserve(StationCount * RingStride + 2);
	Result.UV0.Reserve(StationCount * RingStride + 2);
	for (int32 Station = 0; Station < StationCount; ++Station)
	{
		const float X = StationFractions[Station] * Dimensions.ShoeLengthCm;
		const float Width = HalfWidth * WidthFractions[Station];
		const float ToeLift = FMath::Max(0.f,
			StationFractions[Station] - 0.82f) * Dimensions.SoleThicknessCm * 1.8f;
		for (int32 Cross = 0; Cross < CrossSegments; ++Cross)
		{
			const float Alpha = static_cast<float>(Cross) / CrossSegments;
			const float Angle = -2.f * PI * Alpha;
			Result.Vertices.Add(FVector(
				X,
				FMath::Cos(Angle) * Width,
				HalfHeight + FMath::Sin(Angle) * HalfHeight + ToeLift));
			Result.UV0.Add(FVector2D(StationFractions[Station], Alpha));
		}
	}
	for (int32 Station = 0; Station < StationCount - 1; ++Station)
	{
		for (int32 Cross = 0; Cross < CrossSegments; ++Cross)
		{
			const int32 Next = (Cross + 1) % CrossSegments;
			const int32 A = Station * RingStride + Cross;
			const int32 B = (Station + 1) * RingStride + Cross;
			const int32 C = (Station + 1) * RingStride + Next;
			const int32 D = Station * RingStride + Next;
			Result.Triangles.Append({A, B, C, A, C, D});
		}
	}
	AddLoftCaps(Result, RingStride, StationCount, true);
	CalculateSmoothNormals(Result);
	return Result;
}

void AddBox(
	FSprawlFootwearMesh& Mesh,
	const FVector& Min,
	const FVector& Max)
{
	const int32 Base = Mesh.Vertices.Num();
	const FVector Corners[8] = {
		{Min.X, Min.Y, Min.Z}, {Max.X, Min.Y, Min.Z},
		{Max.X, Max.Y, Min.Z}, {Min.X, Max.Y, Min.Z},
		{Min.X, Min.Y, Max.Z}, {Max.X, Min.Y, Max.Z},
		{Max.X, Max.Y, Max.Z}, {Min.X, Max.Y, Max.Z}};
	Mesh.Vertices.Append(Corners, UE_ARRAY_COUNT(Corners));
	for (int32 Corner = 0; Corner < UE_ARRAY_COUNT(Corners); ++Corner)
	{
		Mesh.UV0.Add(FVector2D(
			(Corner & 1) ? 1.f : 0.f,
			(Corner & 2) ? 1.f : 0.f));
	}
	const int32 Indices[] = {
		0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7,
		0, 1, 5, 0, 5, 4, 3, 7, 6, 3, 6, 2,
		0, 4, 7, 0, 7, 3, 1, 2, 6, 1, 6, 5};
	for (const int32 Index : Indices)
	{
		Mesh.Triangles.Add(Base + Index);
	}
}

FSprawlFootwearMesh BuildShoeDetails(
	const FSprawlFootwearDimensions& Dimensions,
	ESprawlWardrobeFootwear Footwear)
{
	FSprawlFootwearMesh Result;
	const bool bHasLaces = Footwear != ESprawlWardrobeFootwear::DressShoes
		&& Footwear != ESprawlWardrobeFootwear::WorkBoots;
	if (bHasLaces)
	{
		const float HalfWidth = Dimensions.ShoeWidthCm * 0.31f;
		for (int32 Lace = 0; Lace < 4; ++Lace)
		{
			const float X = Dimensions.ShoeLengthCm * (0.39f + Lace * 0.075f);
			const float Z = Dimensions.SoleThicknessCm
				+ Dimensions.UpperHeightCm * (0.70f - Lace * 0.055f);
			AddBox(Result,
				FVector(X - 0.35f, -HalfWidth, Z),
				FVector(X + 0.35f, HalfWidth, Z + 0.35f));
		}
	}

	if (Footwear == ESprawlWardrobeFootwear::AthleticTrainers)
	{
		// Twin lateral support stripes, heel clip, and a raised toe bumper give
		// this pair its own silhouette while sharing the details draw section.
		const float HalfWidth = Dimensions.ShoeWidthCm * 0.5f;
		for (const float Side : {-1.f, 1.f})
		{
			const float CenterY = Side * HalfWidth * 0.91f;
			AddBox(Result,
				FVector(Dimensions.ShoeLengthCm * 0.24f,
					CenterY - 0.24f, Dimensions.SoleThicknessCm + 1.15f),
				FVector(Dimensions.ShoeLengthCm * 0.66f,
					CenterY + 0.24f, Dimensions.SoleThicknessCm + 2.05f));
			AddBox(Result,
				FVector(Dimensions.ShoeLengthCm * 0.34f,
					CenterY - 0.26f, Dimensions.SoleThicknessCm + 2.05f),
				FVector(Dimensions.ShoeLengthCm * 0.57f,
					CenterY + 0.26f, Dimensions.SoleThicknessCm + 3.35f));
		}
		AddBox(Result,
			FVector(0.20f, -HalfWidth * 0.72f,
				Dimensions.SoleThicknessCm + 1.0f),
			FVector(Dimensions.ShoeLengthCm * 0.085f, HalfWidth * 0.72f,
				Dimensions.SoleThicknessCm + 4.2f));
		AddBox(Result,
			FVector(Dimensions.ShoeLengthCm * 0.91f, -HalfWidth * 0.63f,
				Dimensions.SoleThicknessCm * 0.70f),
			FVector(Dimensions.ShoeLengthCm * 0.985f, HalfWidth * 0.63f,
				Dimensions.SoleThicknessCm + 1.55f));
	}

	// A recessed collar makes the upper read as a shoe around the ankle rather
	// than as a continuation of the leg. It shares the lace section/draw call.
	constexpr int32 CollarSides = 18;
	const float CenterX = Dimensions.ShoeLengthCm * 0.20f;
	const float OuterX = Dimensions.ShoeLengthCm * 0.13f;
	const float OuterY = Dimensions.ShoeWidthCm * 0.41f;
	const float InnerX = OuterX * 0.67f;
	const float InnerY = OuterY * 0.70f;
	const float CollarZ = Dimensions.UpperHeightCm * 0.84f;
	for (int32 Side = 0; Side < CollarSides; ++Side)
	{
		const float A0 = 2.f * PI * static_cast<float>(Side) / CollarSides;
		const float A1 = 2.f * PI * static_cast<float>(Side + 1) / CollarSides;
		const int32 Base = Result.Vertices.Num();
		Result.Vertices.Append({
			FVector(CenterX + FMath::Cos(A0) * OuterX,
				FMath::Sin(A0) * OuterY, CollarZ),
			FVector(CenterX + FMath::Cos(A1) * OuterX,
				FMath::Sin(A1) * OuterY, CollarZ),
			FVector(CenterX + FMath::Cos(A1) * InnerX,
				FMath::Sin(A1) * InnerY, CollarZ + 0.12f),
			FVector(CenterX + FMath::Cos(A0) * InnerX,
				FMath::Sin(A0) * InnerY, CollarZ + 0.12f)});
		Result.Triangles.Append({Base, Base + 1, Base + 2, Base, Base + 2, Base + 3});
		Result.UV0.Append({
			FVector2D(0.f, 0.f), FVector2D(1.f, 0.f),
			FVector2D(1.f, 1.f), FVector2D(0.f, 1.f)});
	}
	CalculateSmoothNormals(Result);
	return Result;
}

FSprawlFootwearMesh BuildSock(const FSprawlFootwearDimensions& Dimensions)
{
	constexpr int32 Sides = 16;
	FSprawlFootwearMesh Result;
	for (int32 Ring = 0; Ring < 2; ++Ring)
	{
		const float Z = Ring == 0 ? 0.f : Dimensions.SockLengthCm;
		const float RadiusX = Ring == 0 ? 5.35f : 5.75f;
		const float RadiusY = Ring == 0 ? 5.65f : 6.05f;
		for (int32 Side = 0; Side < Sides; ++Side)
		{
			const float Alpha = static_cast<float>(Side) / Sides;
			const float Angle = 2.f * PI * Alpha;
			Result.Vertices.Add(FVector(
				FMath::Cos(Angle) * RadiusX,
				FMath::Sin(Angle) * RadiusY,
				Z));
			Result.UV0.Add(FVector2D(Alpha, static_cast<float>(Ring)));
		}
	}
	for (int32 Side = 0; Side < Sides; ++Side)
	{
		const int32 Next = (Side + 1) % Sides;
		Result.Triangles.Append({Side, Next, Sides + Next, Side, Sides + Next, Sides + Side});
	}
	CalculateSmoothNormals(Result);
	return Result;
}

void CreateSection(
	UProceduralMeshComponent* Component,
	int32 SectionIndex,
	const FSprawlFootwearMesh& Mesh,
	const FLinearColor& Color,
	float Roughness,
	float Specular)
{
	if (!Component || Mesh.Vertices.IsEmpty() || Mesh.Triangles.IsEmpty())
	{
		return;
	}
	TArray<FLinearColor> VertexColors;
	VertexColors.Init(Color, Mesh.Vertices.Num());
	Component->CreateMeshSection_LinearColor(
		SectionIndex, Mesh.Vertices, Mesh.Triangles, Mesh.Normals,
		Mesh.UV0, VertexColors, {}, false);

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
	if (!Material)
	{
		return;
	}
	Component->SetMaterial(SectionIndex, Material);
	if (UMaterialInstanceDynamic* Dynamic =
		Component->CreateDynamicMaterialInstance(SectionIndex))
	{
		Dynamic->SetVectorParameterValue(TEXT("BaseColor"), Color);
		Dynamic->SetVectorParameterValue(TEXT("Color"), Color);
		Dynamic->SetVectorParameterValue(TEXT("diffuse_color"), Color);
		Dynamic->SetScalarParameterValue(TEXT("Roughness"), Roughness);
		Dynamic->SetScalarParameterValue(TEXT("Specular"), Specular);
	}
}

FLinearColor SoleColor(
	ESprawlWardrobeFootwear Footwear,
	const FLinearColor& ShoeColor)
{
	if (Footwear == ESprawlWardrobeFootwear::DressShoes
		|| Footwear == ESprawlWardrobeFootwear::WorkBoots)
	{
		return ShoeColor * 0.52f;
	}
	return FLinearColor::LerpUsingHSV(
		ShoeColor, FLinearColor(0.11f, 0.12f, 0.14f, 1.f), 0.28f);
}
}

USprawlFootwearModule::USprawlFootwearModule()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void USprawlFootwearModule::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateShoeFollowers();
}

void USprawlFootwearModule::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	ClearFootwear();
	Super::EndPlay(EndPlayReason);
}

FSprawlFootwearDimensions USprawlFootwearModule::DevelopDimensions(
	ESprawlWardrobeFootwear Footwear,
	ESprawlWardrobeSocks Socks,
	float HeelToBallDistanceCm)
{
	const float Reference = FMath::Clamp(
		FMath::IsFinite(HeelToBallDistanceCm)
			? FMath::Abs(HeelToBallDistanceCm) : 14.f,
		10.5f, 16.5f);
	const float Scale = Reference / 14.f;
	FSprawlFootwearDimensions Result;
	Result.ShoeLengthCm = FMath::Clamp(Reference * 1.84f, 23.5f, 30.5f);
	Result.ShoeWidthCm = FMath::Clamp(Result.ShoeLengthCm * 0.37f, 8.6f, 10.8f);
	Result.UpperHeightCm = 8.5f * Scale;
	Result.SoleThicknessCm = 2.2f * Scale;

	switch (Footwear)
	{
	case ESprawlWardrobeFootwear::HighTopSneakers:
		Result.UpperHeightCm = 13.2f * Scale;
		Result.SoleThicknessCm = 2.45f * Scale;
		break;
	case ESprawlWardrobeFootwear::DressShoes:
		Result.ShoeLengthCm = FMath::Min(31.f, Result.ShoeLengthCm * 1.03f);
		Result.ShoeWidthCm *= 0.90f;
		Result.UpperHeightCm = 7.2f * Scale;
		Result.SoleThicknessCm = 1.65f * Scale;
		break;
	case ESprawlWardrobeFootwear::WorkBoots:
		Result.ShoeWidthCm = FMath::Min(11.2f, Result.ShoeWidthCm * 1.05f);
		Result.UpperHeightCm = 15.f * Scale;
		Result.SoleThicknessCm = 2.8f * Scale;
		break;
	case ESprawlWardrobeFootwear::RunningShoes:
		Result.ShoeWidthCm = FMath::Min(11.f, Result.ShoeWidthCm * 1.03f);
		Result.UpperHeightCm = 8.2f * Scale;
		Result.SoleThicknessCm = 2.65f * Scale;
		break;
	case ESprawlWardrobeFootwear::AthleticTrainers:
		Result.ShoeWidthCm = FMath::Min(11.2f, Result.ShoeWidthCm * 1.055f);
		Result.UpperHeightCm = 8.8f * Scale;
		Result.SoleThicknessCm = 2.95f * Scale;
		break;
	default:
		break;
	}

	Result.SockLengthCm = Socks == ESprawlWardrobeSocks::Ankle
		? 6.5f * Scale
		: Socks == ESprawlWardrobeSocks::Dress ? 18.f * Scale : 13.5f * Scale;
	return Result;
}

bool USprawlFootwearModule::ValidateDimensions(
	const FSprawlFootwearDimensions& Dimensions,
	FString& OutError)
{
	const bool bFinite = FMath::IsFinite(Dimensions.ShoeLengthCm)
		&& FMath::IsFinite(Dimensions.ShoeWidthCm)
		&& FMath::IsFinite(Dimensions.UpperHeightCm)
		&& FMath::IsFinite(Dimensions.SoleThicknessCm)
		&& FMath::IsFinite(Dimensions.SockLengthCm);
	if (!bFinite)
	{
		OutError = TEXT("Footwear dimensions must be finite");
		return false;
	}
	if (Dimensions.ShoeLengthCm < 22.f || Dimensions.ShoeLengthCm > 32.f
		|| Dimensions.ShoeWidthCm < 7.5f || Dimensions.ShoeWidthCm > 12.f
		|| Dimensions.UpperHeightCm < 5.f || Dimensions.UpperHeightCm > 19.f
		|| Dimensions.SoleThicknessCm < 1.f || Dimensions.SoleThicknessCm > 4.f
		|| Dimensions.SockLengthCm < 4.f || Dimensions.SockLengthCm > 24.f)
	{
		OutError = TEXT("Footwear dimensions fall outside human-scale bounds");
		return false;
	}
	OutError.Reset();
	return true;
}

FTransform USprawlFootwearModule::ResolveBoneFollowTransform(
	const FTransform& BoneWorldTransform,
	const FTransform& PresentationRelativeTransform)
{
	FTransform ScaleSafeBone = BoneWorldTransform;
	ScaleSafeBone.SetScale3D(FVector::OneVector);
	ScaleSafeBone.NormalizeRotation();
	FTransform Result = PresentationRelativeTransform * ScaleSafeBone;
	Result.SetScale3D(FVector::OneVector);
	Result.NormalizeRotation();
	return Result;
}

FTransform USprawlFootwearModule::ResolveAnimatedShoeTransform(
	const FTransform& AnchorWorldTransform,
	const FTransform& ShoeAnchorRelativeTransform,
	const FTransform& BodyWorldTransform,
	const FTransform& ShoeBodyRelativeFacing)
{
	FTransform Result = ResolveBoneFollowTransform(
		AnchorWorldTransform, ShoeAnchorRelativeTransform);
	FTransform ScaleSafeBody = BodyWorldTransform;
	ScaleSafeBody.SetScale3D(FVector::OneVector);
	ScaleSafeBody.NormalizeRotation();
	FTransform FacingOffset = ShoeBodyRelativeFacing;
	FacingOffset.SetLocation(FVector::ZeroVector);
	FacingOffset.SetScale3D(FVector::OneVector);
	FacingOffset.NormalizeRotation();
	const FTransform FacingWorld = FacingOffset * ScaleSafeBody;
	Result.SetRotation(FacingWorld.GetRotation());
	Result.NormalizeRotation();
	return Result;
}

UProceduralMeshComponent* USprawlFootwearModule::CreatePresentationComponent(
	AActor* VisualActor,
	USkeletalMeshComponent* BodyMesh,
	FName BoneName,
	const FTransform& WorldTransform,
	FName ComponentName)
{
	if (!VisualActor || !BodyMesh || BoneName.IsNone())
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
	Component->ComponentTags.AddUnique(TEXT("SprawlFootwear"));
	Component->RegisterComponent();
	Component->SetWorldTransform(WorldTransform);
	Component->SetAbsolute(false, false, true);
	Component->AttachToComponent(
		BodyMesh, FAttachmentTransformRules::KeepWorldTransform, BoneName);
	SpawnedPieces.Add(Component);
	return Component;
}

bool USprawlFootwearModule::ApplyToMetaHuman(
	AActor* VisualActor,
	USkeletalMeshComponent* BodyMesh,
	ESprawlWardrobeFootwear Footwear,
	ESprawlWardrobeSocks Socks,
	const FLinearColor& ShoeColor,
	const FLinearColor& SockColor,
	const FLinearColor& AccentColor)
{
	ClearFootwear();
	if (!VisualActor || !BodyMesh)
	{
		return false;
	}
	FittedShoes.SetNum(2);
	ShoeAnchorBones.SetNum(2);
	ShoeAnchorOffsets.SetNum(2);
	ShoeBodyFacingOffsets.SetNum(2);

	const FName FootBones[] = {
		FindBone(BodyMesh, {TEXT("foot_l"), TEXT("LeftFoot")}),
		FindBone(BodyMesh, {TEXT("foot_r"), TEXT("RightFoot")})};
	const FName BallBones[] = {
		FindBone(BodyMesh, {TEXT("ball_l"), TEXT("LeftToeBase")}),
		FindBone(BodyMesh, {TEXT("ball_r"), TEXT("RightToeBase")})};
	const FName CalfBones[] = {
		FindBone(BodyMesh, {TEXT("calf_l"), TEXT("LeftLeg")}),
		FindBone(BodyMesh, {TEXT("calf_r"), TEXT("RightLeg")})};
	const FName IkFootBones[] = {
		FindBone(BodyMesh, {TEXT("ik_foot_l"), TEXT("ik_foot_left")}),
		FindBone(BodyMesh, {TEXT("ik_foot_r"), TEXT("ik_foot_right")})};

	FVector FeetCenter = FVector::ZeroVector;
	int32 ValidFootLocations = 0;
	for (const FName FootBone : FootBones)
	{
		if (!FootBone.IsNone())
		{
			FeetCenter += BodyMesh->GetSocketLocation(FootBone);
			++ValidFootLocations;
		}
	}
	if (ValidFootLocations > 0)
	{
		FeetCenter /= static_cast<float>(ValidFootLocations);
	}

	TArray<UMeshComponent*> FeetToHide;
	TInlineComponentArray<UMeshComponent*> VisualMeshes(VisualActor);
	for (UMeshComponent* Mesh : VisualMeshes)
	{
		if (!Mesh || Mesh == BodyMesh || !Mesh->IsVisible())
		{
			continue;
		}
		FString Evidence = Mesh->GetName();
		if (const USkeletalMeshComponent* Skeletal =
			Cast<USkeletalMeshComponent>(Mesh))
		{
			if (const USkeletalMesh* Asset = Skeletal->GetSkeletalMeshAsset())
			{
				Evidence += TEXT(" ");
				Evidence += Asset->GetPathName();
			}
		}
		for (int32 MaterialIndex = 0;
			MaterialIndex < Mesh->GetNumMaterials(); ++MaterialIndex)
		{
			if (const UMaterialInterface* Material = Mesh->GetMaterial(MaterialIndex))
			{
				Evidence += TEXT(" ");
				Evidence += Material->GetPathName();
			}
		}
		const bool bNamedFeet = Evidence.Contains(TEXT("Feet"), ESearchCase::IgnoreCase)
			|| Evidence.Contains(TEXT("_foot"), ESearchCase::IgnoreCase)
			|| Evidence.Contains(TEXT("/foot"), ESearchCase::IgnoreCase);
		const FBoxSphereBounds Bounds = Mesh->Bounds;
		const bool bFootSizedAtAnkles = ValidFootLocations == 2
			&& Bounds.SphereRadius > 2.f
			&& FMath::Abs(Bounds.Origin.Z - FeetCenter.Z) < 24.f
			&& Bounds.BoxExtent.Z < 24.f
			&& Bounds.BoxExtent.X < 42.f
			&& Bounds.BoxExtent.Y < 42.f;
		if (FParse::Param(FCommandLine::Get(), TEXT("SprawlAuditWardrobe"))
			&& (bNamedFeet || bFootSizedAtAnkles))
		{
			UE_LOG(LogTemp, Display,
				TEXT("[FootwearAudit] render component=%s evidence=%s center_z=%.2f extent=%s feet_candidate=%s"),
				*Mesh->GetName(), *Evidence, Bounds.Origin.Z,
				*Bounds.BoxExtent.ToCompactString(),
				bNamedFeet || bFootSizedAtAnkles ? TEXT("true") : TEXT("false"));
		}
		if (bNamedFeet || bFootSizedAtAnkles)
		{
			FeetToHide.Add(Mesh);
		}
	}

	for (int32 Side = 0; Side < 2; ++Side)
	{
		if (FootBones[Side].IsNone())
		{
			continue;
		}
		const FVector Foot = BodyMesh->GetSocketLocation(FootBones[Side]);
		const FVector Calf = CalfBones[Side].IsNone()
			? Foot + BodyMesh->GetUpVector() * 35.f
			: BodyMesh->GetSocketLocation(CalfBones[Side]);
		const FVector Up = SafeDirection(Foot, Calf, BodyMesh->GetUpVector());
		const FVector FallbackForward = FVector::VectorPlaneProject(
			VisualActor->GetActorForwardVector(), Up).GetSafeNormal(
				SMALL_NUMBER, FVector::ForwardVector);
		const FVector Ball = BallBones[Side].IsNone()
			? Foot + FallbackForward * 14.f - Up * 5.f
			: BodyMesh->GetSocketLocation(BallBones[Side]);
		FVector Forward = FVector::VectorPlaneProject(Ball - Foot, Up).GetSafeNormal();
		if (Forward.IsNearlyZero())
		{
			Forward = FallbackForward;
		}
		const float HeelToBall = FMath::Abs(FVector::DotProduct(Ball - Foot, Forward));
		const FSprawlFootwearDimensions Dimensions =
			DevelopDimensions(Footwear, Socks, HeelToBall);
		FString DimensionError;
		if (!ValidateDimensions(Dimensions, DimensionError))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Footwear] %s"), *DimensionError);
			continue;
		}

		const float HeelBack = FMath::Clamp(HeelToBall * 0.38f, 4.2f, 6.4f);
		const float BallCenterZ = Dimensions.SoleThicknessCm + 2.15f;
		const FVector ShoeOrigin = Ball
			- Forward * (HeelBack + HeelToBall)
			- Up * BallCenterZ;
		const FQuat ShoeRotation = FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat();
		const bool bUsableIkAnchor = !IkFootBones[Side].IsNone()
			&& FVector::DistSquared(
				BodyMesh->GetSocketLocation(IkFootBones[Side]), Foot)
				< FMath::Square(8.f);
		const FName ShoeAnchor = bUsableIkAnchor
			? IkFootBones[Side] : CalfBones[Side];
		if (ShoeAnchor.IsNone())
		{
			continue;
		}
		UProceduralMeshComponent* Shoe = CreatePresentationComponent(
			VisualActor, BodyMesh, ShoeAnchor,
			FTransform(ShoeRotation, ShoeOrigin, FVector::OneVector),
			Side == 0 ? TEXT("FittedShoeL") : TEXT("FittedShoeR"));
		if (Shoe)
		{
			const FSprawlFootwearMesh Upper = BuildRoundedUpper(Dimensions, Footwear);
			const FSprawlFootwearMesh Sole = BuildRoundedSole(Dimensions, Footwear);
			const FSprawlFootwearMesh Details = BuildShoeDetails(Dimensions, Footwear);
			const float UpperRoughness = Footwear == ESprawlWardrobeFootwear::DressShoes
				? 0.42f : Footwear == ESprawlWardrobeFootwear::WorkBoots
					? 0.78f : Footwear == ESprawlWardrobeFootwear::AthleticTrainers
						? 0.54f : 0.62f;
			CreateSection(Shoe, 0, Upper, ShoeColor, UpperRoughness, 0.30f);
			CreateSection(Shoe, 1, Sole, SoleColor(Footwear, ShoeColor), 0.82f, 0.18f);
			CreateSection(Shoe, 2, Details, AccentColor, 0.70f, 0.22f);
			FTransform AnchorWorld = BodyMesh->GetSocketTransform(
				ShoeAnchor, RTS_World);
			AnchorWorld.SetScale3D(FVector::OneVector);
			AnchorWorld.NormalizeRotation();
			FTransform BodyWorld = BodyMesh->GetComponentTransform();
			BodyWorld.SetScale3D(FVector::OneVector);
			BodyWorld.NormalizeRotation();
			FTransform ShoeWorld = Shoe->GetComponentTransform();
			ShoeWorld.SetScale3D(FVector::OneVector);
			ShoeWorld.NormalizeRotation();
			FittedShoes[Side] = Shoe;
			ShoeAnchorBones[Side] = ShoeAnchor;
			ShoeAnchorOffsets[Side] = ShoeWorld.GetRelativeTransform(AnchorWorld);
			ShoeBodyFacingOffsets[Side] = ShoeWorld.GetRelativeTransform(BodyWorld);
			ShoeBodyFacingOffsets[Side].SetLocation(FVector::ZeroVector);
			ShoeBodyFacingOffsets[Side].SetScale3D(FVector::OneVector);
			++FittedShoeCount;
		}

		const FName SockBone = CalfBones[Side].IsNone()
			? FootBones[Side] : CalfBones[Side];
		const FQuat SockRotation = FRotationMatrix::MakeFromZ(Up).ToQuat();
		UProceduralMeshComponent* Sock = CreatePresentationComponent(
			VisualActor, BodyMesh, SockBone,
			FTransform(SockRotation, Foot - Up * 0.7f, FVector::OneVector),
			Side == 0 ? TEXT("FittedSockL") : TEXT("FittedSockR"));
		if (Sock)
		{
			CreateSection(Sock, 0, BuildSock(Dimensions), SockColor, 0.88f, 0.08f);
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("SprawlAuditWardrobe")))
		{
			UE_LOG(LogTemp, Display,
				TEXT("[FootwearAudit] actor=%s side=%s foot=%s ball=%s calf=%s anchor=%s heel_to_ball=%.2f length=%.2f width=%.2f origin=%s"),
				*VisualActor->GetName(), Side == 0 ? TEXT("L") : TEXT("R"),
				*FootBones[Side].ToString(), *BallBones[Side].ToString(),
				*CalfBones[Side].ToString(), *ShoeAnchor.ToString(), HeelToBall,
				Dimensions.ShoeLengthCm, Dimensions.ShoeWidthCm,
				*ShoeOrigin.ToCompactString());
		}
	}

	if (FittedShoeCount != 2 || SpawnedPieces.Num() != 4)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Footwear] %s could not create a complete fitted pair (%d shoes, %d total pieces)"),
			*VisualActor->GetName(), FittedShoeCount, SpawnedPieces.Num());
		ClearFootwear();
		return false;
	}
	for (UMeshComponent* FeetMesh : FeetToHide)
	{
		FeetMesh->SetVisibility(false, false);
		HiddenFeetMeshes.Add(FeetMesh);
	}
	FootwearBodyMesh = BodyMesh;
	FootwearVisualActor = VisualActor;
	CurrentFootwear = Footwear;
	for (const FName FootBone : FootBones)
	{
		if (!FootBone.IsNone())
		{
			BodyMesh->HideBoneByName(FootBone, PBO_None);
			HiddenFootBones.Add(FootBone);
		}
	}
	AddTickPrerequisiteComponent(BodyMesh);
	SetComponentTickEnabled(true);
	UpdateShoeFollowers();
	return true;
}

bool USprawlFootwearModule::SwapFootwear(
	ESprawlWardrobeFootwear Footwear,
	ESprawlWardrobeSocks Socks,
	const FLinearColor& ShoeColor,
	const FLinearColor& SockColor,
	const FLinearColor& AccentColor)
{
	AActor* VisualActor = FootwearVisualActor;
	USkeletalMeshComponent* BodyMesh = FootwearBodyMesh;
	if (!IsValid(VisualActor) || !IsValid(BodyMesh))
	{
		return false;
	}
	return ApplyToMetaHuman(
		VisualActor, BodyMesh, Footwear, Socks,
		ShoeColor, SockColor, AccentColor);
}

bool USprawlFootwearModule::IsFollowingAnimatedFeet() const
{
	if (!IsValid(FootwearBodyMesh)
		|| FittedShoes.Num() != 2
		|| ShoeAnchorBones.Num() != 2
		|| ShoeAnchorOffsets.Num() != 2
		|| ShoeBodyFacingOffsets.Num() != 2
		|| !IsComponentTickEnabled())
	{
		return false;
	}
	for (int32 Side = 0; Side < 2; ++Side)
	{
		if (!IsValid(FittedShoes[Side])
			|| ShoeAnchorBones[Side].IsNone()
			|| FittedShoes[Side]->GetAttachParent() != FootwearBodyMesh
			|| FittedShoes[Side]->GetAttachSocketName() != ShoeAnchorBones[Side])
		{
			return false;
		}
	}
	return true;
}

void USprawlFootwearModule::UpdateShoeFollowers()
{
	if (!IsValid(FootwearBodyMesh)
		|| FittedShoes.Num() != 2
		|| ShoeAnchorBones.Num() != 2
		|| ShoeAnchorOffsets.Num() != 2
		|| ShoeBodyFacingOffsets.Num() != 2)
	{
		return;
	}
	const FTransform BodyWorld = FootwearBodyMesh->GetComponentTransform();
	for (int32 Side = 0; Side < 2; ++Side)
	{
		if (!IsValid(FittedShoes[Side]) || ShoeAnchorBones[Side].IsNone())
		{
			continue;
		}
		const FTransform AnchorWorld = FootwearBodyMesh->GetSocketTransform(
			ShoeAnchorBones[Side], RTS_World);
		const FTransform FollowWorld = ResolveAnimatedShoeTransform(
			AnchorWorld, ShoeAnchorOffsets[Side],
			BodyWorld, ShoeBodyFacingOffsets[Side]);
		FittedShoes[Side]->SetWorldTransform(
			FollowWorld, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void USprawlFootwearModule::ClearFootwear()
{
	SetComponentTickEnabled(false);
	if (IsValid(FootwearBodyMesh))
	{
		RemoveTickPrerequisiteComponent(FootwearBodyMesh);
	}
	for (UProceduralMeshComponent* Piece : SpawnedPieces)
	{
		if (IsValid(Piece))
		{
			Piece->DestroyComponent();
		}
	}
	SpawnedPieces.Reset();
	FittedShoes.Reset();
	ShoeAnchorBones.Reset();
	ShoeAnchorOffsets.Reset();
	ShoeBodyFacingOffsets.Reset();
	FittedShoeCount = 0;
	if (IsValid(FootwearBodyMesh))
	{
		for (const FName FootBone : HiddenFootBones)
		{
			FootwearBodyMesh->UnHideBoneByName(FootBone);
		}
	}
	HiddenFootBones.Reset();
	FootwearBodyMesh = nullptr;
	FootwearVisualActor = nullptr;
	for (UMeshComponent* FeetMesh : HiddenFeetMeshes)
	{
		if (IsValid(FeetMesh))
		{
			FeetMesh->SetVisibility(true, false);
		}
	}
	HiddenFeetMeshes.Reset();
}
