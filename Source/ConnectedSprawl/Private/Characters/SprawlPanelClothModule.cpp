// The Connected Sprawl - Runtime bridge for Unreal Panel Cloth assets.

#include "Characters/SprawlPanelClothModule.h"

#include "ChaosClothAsset/ClothAssetBase.h"
#include "ChaosClothAsset/ClothComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "PhysicsEngine/PhysicsAsset.h"

namespace
{
constexpr TCHAR ZarriPanelClothAssetPath[] = TEXT(
	"/Game/Import/Characters/Streetwear/PanelCloth/CA_Zarri_Hoodie_Runtime.CA_Zarri_Hoodie_Runtime");
constexpr TCHAR ZarriPhysicsAssetPath[] = TEXT(
	"/Game/MetaHumans/Zarri/Body/PHYS_MHC_Zarri.PHYS_MHC_Zarri");
constexpr TCHAR ZarriPanelClothMaterialPath[] = TEXT(
	"/Game/Import/Characters/Streetwear/PanelCloth/M_ZarriPanelCloth.M_ZarriPanelCloth");
}

USprawlPanelClothModule::USprawlPanelClothModule()
{
	PrimaryComponentTick.bCanEverTick = false;
	PanelClothAsset = TSoftObjectPtr<UChaosClothAssetBase>(
		FSoftObjectPath(ZarriPanelClothAssetPath));
	CollisionPhysicsAsset = TSoftObjectPtr<UPhysicsAsset>(
		FSoftObjectPath(ZarriPhysicsAssetPath));
}

void USprawlPanelClothModule::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	ClearPanelCloth();
	Super::EndPlay(EndPlayReason);
}

bool USprawlPanelClothModule::ApplyToMetaHuman(
	AActor* VisualActor, USkeletalMeshComponent* BodyMesh)
{
	ClearPanelCloth();
	if (!VisualActor || !BodyMesh || !BodyMesh->GetSkeletalMeshAsset())
	{
		return false;
	}
	if (!FParse::Param(
		FCommandLine::Get(), TEXT("SprawlPanelClothPreview")))
	{
		UE_LOG(LogTemp, Display,
			TEXT("[PanelCloth] Preview disabled while sleeve skin weights are refined; keeping rigid streetwear"));
		return false;
	}

	UChaosClothAssetBase* Asset = PanelClothAsset.LoadSynchronous();
	if (!Asset || Asset->HasDataflow()
		|| !Asset->HasValidClothSimulationModels())
	{
		UE_LOG(LogTemp, Display,
			TEXT("[PanelCloth] Zarri runtime hoodie is missing, invalid, or still contains an editor graph; keeping rigid streetwear fallback asset=%s"),
			*PanelClothAsset.ToSoftObjectPath().ToString());
		return false;
	}

	UChaosClothComponent* NewComponent = NewObject<UChaosClothComponent>(
		VisualActor, TEXT("SprawlPanelCloth_Hoodie"));
	if (!NewComponent)
	{
		return false;
	}

	VisualActor->AddInstanceComponent(NewComponent);
	NewComponent->SetupAttachment(BodyMesh);
	NewComponent->SetRelativeTransform(FTransform::Identity);
	NewComponent->SetLeaderPoseComponent(BodyMesh);
	NewComponent->SetBindToLeaderComponent(true);
	NewComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NewComponent->SetGenerateOverlapEvents(false);
	NewComponent->SetCanEverAffectNavigation(false);
	NewComponent->SetReceivesDecals(false);
	NewComponent->ComponentTags.AddUnique(TEXT("SprawlPanelCloth"));
	NewComponent->SetAsset(Asset);
	static TSoftObjectPtr<UMaterialInterface> PanelClothMaterial{
		FSoftObjectPath(ZarriPanelClothMaterialPath)};
	UMaterialInterface* SharedMaterial = PanelClothMaterial.LoadSynchronous();
	if (!SharedMaterial)
	{
		SharedMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
	}
	if (SharedMaterial)
	{
		for (int32 MaterialIndex = 0;
			MaterialIndex < Asset->GetMaterials().Num(); ++MaterialIndex)
		{
			NewComponent->SetMaterial(MaterialIndex, SharedMaterial);
			if (UMaterialInstanceDynamic* Material =
				NewComponent->CreateDynamicMaterialInstance(MaterialIndex))
			{
				Material->SetVectorParameterValue(
					TEXT("BaseColor"), FLinearColor(0.045f, 0.12f, 0.32f));
			}
		}
	}
	NewComponent->SetCollideWithEnvironment(false);
	NewComponent->SetTeleportDistanceThreshold(250.f);
	NewComponent->SetTeleportRotationThreshold(60.f);

	if (UPhysicsAsset* PhysicsAsset = CollisionPhysicsAsset.LoadSynchronous())
	{
		NewComponent->AddCollisionSource(BodyMesh, PhysicsAsset, true);
	}

	NewComponent->RegisterComponent();
	NewComponent->SetEnableSimulation(true);
	NewComponent->ForceNextUpdateTeleportAndReset();
	ClothComponent = NewComponent;

	UE_LOG(LogTemp, Display,
		TEXT("[PanelClothAudit] PASS actor=%s asset=%s models=%d collision=%s"),
		*VisualActor->GetName(), *Asset->GetName(),
		Asset->GetNumClothSimulationModels(),
		CollisionPhysicsAsset.IsValid() ? TEXT("metahuman_sphyls") : TEXT("asset_only"));
	return true;
}

void USprawlPanelClothModule::ClearPanelCloth()
{
	if (IsValid(ClothComponent))
	{
		ClothComponent->SetEnableSimulation(false);
		ClothComponent->ResetCollisionSources();
		ClothComponent->DestroyComponent();
	}
	ClothComponent = nullptr;
}

bool USprawlPanelClothModule::IsPanelClothActive() const
{
	return IsValid(ClothComponent)
		&& ClothComponent->GetAsset()
		&& ClothComponent->IsSimulationEnabled();
}

FSoftObjectPath USprawlPanelClothModule::GetConfiguredAssetPath() const
{
	return PanelClothAsset.ToSoftObjectPath();
}
