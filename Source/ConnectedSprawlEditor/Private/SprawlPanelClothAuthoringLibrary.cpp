// The Connected Sprawl - Reproducible Panel Cloth authoring commands.

#include "SprawlPanelClothAuthoringLibrary.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "ChaosClothAsset/ClothAsset.h"
#include "ChaosClothAsset/ClothAssetFactory.h"
#include "ChaosClothAsset/CollectionClothFacade.h"
#include "ChaosClothAsset/ImportedValue.h"
#include "ChaosClothAsset/SetPhysicsAssetNode.h"
#include "ChaosClothAsset/SimulationCollisionConfigNode.h"
#include "ChaosClothAsset/SimulationDampingConfigNode.h"
#include "ChaosClothAsset/SimulationMassConfigNode.h"
#include "ChaosClothAsset/SimulationMaxDistanceConfigNode.h"
#include "ChaosClothAsset/SimulationSolverConfigNode.h"
#include "ChaosClothAsset/StaticMeshImportNode.h"
#include "ChaosClothAsset/TransferSkinWeightsNode.h"
#include "ChaosClothAsset/WeightMapNode.h"
#include "Dataflow/DataflowBlueprintLibrary.h"
#include "Dataflow/DataflowGraph.h"
#include "Dataflow/DataflowObject.h"
#include "Dataflow/DataflowObjectInterface.h"
#include "Dataflow/DataflowVertexAttributeEditableNode.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "AssetCompilingManager.h"
#include "Materials/Material.h"
#include "Misc/PackageName.h"
#include "ObjectTools.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

namespace
{
constexpr TCHAR AssetPackagePath[] = TEXT(
	"/Game/Import/Characters/Streetwear/PanelCloth/CA_Zarri_Hoodie");
constexpr TCHAR AssetObjectPath[] = TEXT(
	"/Game/Import/Characters/Streetwear/PanelCloth/CA_Zarri_Hoodie.CA_Zarri_Hoodie");
constexpr TCHAR RuntimeAssetPackagePath[] = TEXT(
	"/Game/Import/Characters/Streetwear/PanelCloth/CA_Zarri_Hoodie_Runtime");
constexpr TCHAR RuntimeAssetObjectPath[] = TEXT(
	"/Game/Import/Characters/Streetwear/PanelCloth/CA_Zarri_Hoodie_Runtime.CA_Zarri_Hoodie_Runtime");
constexpr TCHAR StaticTemplatePath[] = TEXT(
	"/ChaosClothAsset/DF_StaticMeshClothTemplate.DF_StaticMeshClothTemplate");
constexpr TCHAR SimMeshPath[] = TEXT(
	"/Game/Import/Characters/Streetwear/SM_RealCloth_Hoodie.SM_RealCloth_Hoodie");
constexpr TCHAR RenderMeshPath[] = TEXT(
	"/Game/Import/Characters/Streetwear/PanelCloth/SM_ZarriHoodie_Render.SM_ZarriHoodie_Render");
constexpr TCHAR BodyMeshPath[] = TEXT(
	"/Game/MetaHumans/Zarri/Body/SKM_MHC_Zarri_BodyMesh.SKM_MHC_Zarri_BodyMesh");
constexpr TCHAR PhysicsAssetPath[] = TEXT(
	"/Game/MetaHumans/Zarri/Body/PHYS_MHC_Zarri.PHYS_MHC_Zarri");
constexpr TCHAR PanelClothMaterialPackagePath[] = TEXT(
	"/Game/Import/Characters/Streetwear/PanelCloth/M_ZarriPanelCloth");
constexpr TCHAR PanelClothMaterialObjectPath[] = TEXT(
	"/Game/Import/Characters/Streetwear/PanelCloth/M_ZarriPanelCloth.M_ZarriPanelCloth");
constexpr TCHAR WardrobeMaterialPath[] = TEXT(
	"/Game/Materials/M_WardrobeAccessory.M_WardrobeAccessory");

template<typename NodeType>
NodeType* FindNode(
	const TSharedPtr<UE::Dataflow::FGraph>& Graph,
	const FName PreferredName = NAME_None)
{
	if (!Graph)
	{
		return nullptr;
	}
	if (!PreferredName.IsNone())
	{
		if (const TSharedPtr<FDataflowNode> Named =
			Graph->FindBaseNode(PreferredName))
		{
			if (NodeType* Typed = Named->AsType<NodeType>())
			{
				return Typed;
			}
		}
	}
	for (const TSharedPtr<FDataflowNode>& Node : Graph->GetNodes())
	{
		if (Node)
		{
			if (NodeType* Typed = Node->AsType<NodeType>())
			{
				return Typed;
			}
		}
	}
	return nullptr;
}

void Invalidate(FDataflowNode* Node)
{
	if (Node)
	{
		Node->Invalidate();
	}
}

bool SaveAsset(
	UObject* Asset,
	const TCHAR* PackagePath,
	FString& OutMessage)
{
	const FString Filename = FPackageName::LongPackageNameToFilename(
		PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	if (Asset && UPackage::SavePackage(
		Asset->GetOutermost(), Asset, *Filename, SaveArgs))
	{
		return true;
	}
	OutMessage = FString::Printf(
		TEXT("Panel Cloth asset could not be saved: %s"), *Filename);
	return false;
}
}

bool USprawlPanelClothAuthoringLibrary::CreateOrUpdateZarriHoodieAsset(
	FString& OutMessage)
{
	// Runtime cloth is generated output. Finish any pending load/build before
	// replacing it, and do this before loading source assets so collection
	// cannot invalidate an authoring pointer owned by this function.
	if (UChaosClothAsset* ExistingRuntimeAsset = LoadObject<UChaosClothAsset>(
		nullptr, RuntimeAssetObjectPath))
	{
		const TArray<UObject*> RuntimeAssetsToFinish = {
			ExistingRuntimeAsset};
		FAssetCompilingManager::Get().FinishCompilationForObjects(
			RuntimeAssetsToFinish);
		const TArray<FAssetData> RuntimeAssetsToReplace = {
			FAssetData(ExistingRuntimeAsset)};
		if (ObjectTools::DeleteAssets(
			RuntimeAssetsToReplace, false) != 1)
		{
			OutMessage = TEXT(
				"Unreal could not replace the generated hoodie runtime asset");
			return false;
		}
		CollectGarbage(RF_NoFlags);
	}
	UMaterial* PanelClothMaterial = LoadObject<UMaterial>(
		nullptr, PanelClothMaterialObjectPath);
	if (!PanelClothMaterial)
	{
		UMaterial* WardrobeMaterial = LoadObject<UMaterial>(
			nullptr, WardrobeMaterialPath);
		UPackage* MaterialPackage = CreatePackage(
			PanelClothMaterialPackagePath);
		PanelClothMaterial = WardrobeMaterial
			? Cast<UMaterial>(StaticDuplicateObject(
				WardrobeMaterial, MaterialPackage,
				TEXT("M_ZarriPanelCloth")))
			: nullptr;
		if (!PanelClothMaterial)
		{
			OutMessage = TEXT(
				"Unreal could not create Zarri's Panel Cloth material");
			return false;
		}
		PanelClothMaterial->SetFlags(RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(PanelClothMaterial);
	}
	PanelClothMaterial->TwoSided = true;
	PanelClothMaterial->SetUsageByFlag(MATUSAGE_SkeletalMesh, true);
	PanelClothMaterial->SetUsageByFlag(MATUSAGE_Clothing, true);
	PanelClothMaterial->PostEditChange();
	PanelClothMaterial->MarkPackageDirty();
	if (!SaveAsset(
		PanelClothMaterial, PanelClothMaterialPackagePath, OutMessage))
	{
		return false;
	}

	UStaticMesh* SimMesh = LoadObject<UStaticMesh>(nullptr, SimMeshPath);
	UStaticMesh* DetailedRenderMesh = LoadObject<UStaticMesh>(nullptr, RenderMeshPath);
	USkeletalMesh* BodyMesh = LoadObject<USkeletalMesh>(nullptr, BodyMeshPath);
	UPhysicsAsset* PhysicsAsset = LoadObject<UPhysicsAsset>(nullptr, PhysicsAssetPath);
	if (!SimMesh || !BodyMesh || !PhysicsAsset)
	{
		OutMessage = FString::Printf(
			TEXT("Missing Panel Cloth input: sim=%s body=%s physics=%s"),
			SimMesh ? TEXT("ok") : TEXT("missing"),
			BodyMesh ? TEXT("ok") : TEXT("missing"),
			PhysicsAsset ? TEXT("ok") : TEXT("missing"));
		return false;
	}
	// The combined concept FBX is retained as a future render shell, but it
	// does not yet share a fitted deformer space with the simulation surface.
	// Shipping it here produces exploded slabs on the animated MetaHuman. The
	// fitted low-resolution source is therefore both sim and render proxy.
	UStaticMesh* RenderMesh = SimMesh;
	UE_LOG(LogTemp, Display,
		TEXT("[PanelClothAuthoring] Inputs sim=%s source_lods=%d vertices=%d render_proxy=%s detailed_shell=%s"),
		*SimMesh->GetPathName(), SimMesh->GetNumSourceModels(),
		SimMesh->GetNumVertices(0), *RenderMesh->GetPathName(),
		DetailedRenderMesh ? *DetailedRenderMesh->GetPathName() : TEXT("missing"));

	UChaosClothAsset* ClothAsset = LoadObject<UChaosClothAsset>(
		nullptr, AssetObjectPath);
	bool bCreated = false;
	if (!ClothAsset)
	{
		UPackage* Package = CreatePackage(AssetPackagePath);
		const FString TemplatePath(StaticTemplatePath);
		ClothAsset = Cast<UChaosClothAsset>(
			UChaosClothAssetFactory::CreateClothAssetFromTemplate(
				UChaosClothAsset::StaticClass(), Package,
				TEXT("CA_Zarri_Hoodie"), RF_Public | RF_Standalone,
				&TemplatePath, true));
		if (!ClothAsset)
		{
			OutMessage = TEXT("Unreal could not clone the Static Mesh Cloth template");
			return false;
		}
		FAssetRegistryModule::AssetCreated(ClothAsset);
		bCreated = true;
	}

	UDataflow* Dataflow = ClothAsset->GetDataflow();
	const TSharedPtr<UE::Dataflow::FGraph> Graph =
		Dataflow ? Dataflow->GetDataflowGraph() : nullptr;
	if (!Dataflow || !Graph)
	{
		OutMessage = TEXT("The hoodie cloth asset has no embedded Dataflow graph");
		return false;
	}
	FChaosClothAssetStaticMeshImportNode_v2* SimImport = FindNode<
		FChaosClothAssetStaticMeshImportNode_v2>(
			Graph, TEXT("StaticMeshImport_SIM"));
	FChaosClothAssetStaticMeshImportNode_v2* RenderImport = FindNode<
		FChaosClothAssetStaticMeshImportNode_v2>(
			Graph, TEXT("StaticMeshImport_RENDER"));
	FChaosClothAssetTransferSkinWeightsNode* SkinWeights = FindNode<
		FChaosClothAssetTransferSkinWeightsNode>(
			Graph, TEXT("TransferSkinWeights"));
	FChaosClothAssetSetPhysicsAssetNode* SetPhysics = FindNode<
		FChaosClothAssetSetPhysicsAssetNode>(
			Graph, TEXT("SetPhysicsAsset"));
	FChaosClothAssetWeightMapNode* MaxDistanceMap = FindNode<
		FChaosClothAssetWeightMapNode>(
			Graph, TEXT("WeightMap_MaxDistance"));
	if (!SimImport || !RenderImport || SimImport == RenderImport
		|| !SkinWeights || !SetPhysics || !MaxDistanceMap)
	{
		OutMessage = TEXT(
			"The Unreal Static Mesh Cloth template does not contain its expected authoring nodes");
		UE_LOG(LogTemp, Error, TEXT("[PanelClothAuthoring] FAIL %s"),
			*OutMessage);
		return false;
	}
	if (FDataflowInput* SimMeshInput = SimImport->FindInput(
		&SimImport->StaticMesh))
	{
		// The stock template exposes an empty helper input for interactive use.
		// Unattended authoring owns this value directly instead.
		Graph->ClearConnections(SimMeshInput);
	}
	if (FDataflowInput* RenderMeshInput = RenderImport->FindInput(
		&RenderImport->StaticMesh))
	{
		Graph->ClearConnections(RenderMeshInput);
	}

	SimImport->StaticMesh = SimMesh;
	SimImport->bImportSimMesh = true;
	SimImport->bImportRenderMesh = false;
	SimImport->LODIndex = 0;
	SimImport->SimMeshSection = INDEX_NONE;
	SimImport->UVChannel = INDEX_NONE;
	Invalidate(SimImport);

	RenderImport->StaticMesh = RenderMesh;
	RenderImport->bImportSimMesh = false;
	RenderImport->bImportRenderMesh = true;
	RenderImport->LODIndex = 0;
	RenderImport->RenderMeshSection = INDEX_NONE;
	Invalidate(RenderImport);

	SkinWeights->TargetMeshType = EChaosClothAssetTransferTargetMeshType::All;
	SkinWeights->RenderMeshSourceType =
		EChaosClothAssetTransferRenderMeshSource::SkeletalMesh;
	SkinWeights->SkeletalMesh = BodyMesh;
	SkinWeights->LodIndex = 0;
	SkinWeights->TransferMethod =
		EChaosClothAssetTransferSkinWeightsMethod::InpaintWeights;
	SkinWeights->RadiusPercentage = -1.0;
	SkinWeights->NormalThreshold = -1.0;
	SkinWeights->LayeredMeshSupport = true;
	SkinWeights->NumSmoothingIterations = 8;
	SkinWeights->SmoothingStrength = 0.12f;
	SkinWeights->MaxNumInfluences = EChaosClothAssetMaxNumInfluences::Four;
	Invalidate(SkinWeights);

	SetPhysics->PhysicsAsset = PhysicsAsset;
	Invalidate(SetPhysics);

	MaxDistanceMap->OutputName.StringValue = TEXT("MaxDistance");
	MaxDistanceMap->MeshTarget = EChaosClothAssetWeightMapMeshTarget::Simulation;
	MaxDistanceMap->MapOverrideType =
		EChaosClothAssetWeightMapOverrideType::ReplaceAll;
	Invalidate(MaxDistanceMap);

	if (FChaosClothAssetSimulationMassConfigNode* Mass = FindNode<
		FChaosClothAssetSimulationMassConfigNode>(Graph))
	{
		Mass->MassMode = EClothMassMode::Density;
		Mass->DensityWeighted.Low = 0.42f;
		Mass->DensityWeighted.High = 0.42f;
		Mass->DensityWeighted.WeightMap = TEXT("Density");
		Mass->MinPerParticleMass = 0.0001f;
		Invalidate(Mass);
	}
	if (FChaosClothAssetSimulationMaxDistanceConfigNode* MaxDistance = FindNode<
		FChaosClothAssetSimulationMaxDistanceConfigNode>(Graph))
	{
		MaxDistance->MaxDistance.Low = 0.f;
		MaxDistance->MaxDistance.High = 2.f;
		MaxDistance->MaxDistance.WeightMap = TEXT("MaxDistance");
		Invalidate(MaxDistance);
	}
	if (FChaosClothAssetSimulationSolverConfigNode* Solver = FindNode<
		FChaosClothAssetSimulationSolverConfigNode>(Graph))
	{
		Solver->NumIterations = 4;
		Solver->MaxNumIterations = 6;
		Solver->NumSubstepsImported.bUseImportedValue = false;
		Solver->NumSubstepsImported.ImportedValue = 2;
		Solver->bEnableDynamicSubstepping = false;
		Invalidate(Solver);
	}
	if (FChaosClothAssetSimulationCollisionConfigNode* Collision = FindNode<
		FChaosClothAssetSimulationCollisionConfigNode>(Graph))
	{
		Collision->bEnableSimpleColliders = true;
		Collision->bUsePlanarConstraintForSimpleColliders = false;
		Collision->bEnableComplexColliders = false;
		Collision->bEnableSkinnedTriangleMeshCollisions = false;
		Collision->bUseCCD = false;
		Invalidate(Collision);
	}
	if (FChaosClothAssetSimulationDampingConfigNode* Damping = FindNode<
		FChaosClothAssetSimulationDampingConfigNode>(Graph))
	{
		Damping->DampingCoefficientWeighted.Low = 0.015f;
		Damping->DampingCoefficientWeighted.High = 0.015f;
		Damping->LocalDampingLinearCoefficient = 0.08f;
		Damping->LocalDampingAngularCoefficient = 0.04f;
		Invalidate(Damping);
	}

	// Build once so the paint node can query the imported sim topology.
	UDataflowBlueprintLibrary::EvaluateTerminalNodeByName(
		Dataflow, ClothAsset->GetDataflowInstance().GetDataflowTerminal(),
		ClothAsset);

	UE::Dataflow::FEngineContext Context(ClothAsset);
	FDataflowVertexAttributeEditableNode* EditableMap = MaxDistanceMap;
	TArray<float> Weights;
	EditableMap->GetVertexAttributeValues(Context, Weights);
	if (Weights.IsEmpty())
	{
		OutMessage = TEXT("The hoodie simulation mesh produced no paintable vertices");
		UE_LOG(LogTemp, Error, TEXT("[PanelClothAuthoring] FAIL %s"),
			*OutMessage);
		return false;
	}
	const TArray<TSharedRef<const FManagedArrayCollection>>& ClothCollections =
		static_cast<const UChaosClothAsset*>(ClothAsset)->GetClothCollections();
	if (ClothCollections.IsEmpty())
	{
		OutMessage = TEXT("The hoodie build produced no cloth collection");
		UE_LOG(LogTemp, Error, TEXT("[PanelClothAuthoring] FAIL %s"),
			*OutMessage);
		return false;
	}
	const UE::Chaos::ClothAsset::FCollectionClothConstFacade ClothFacade(
		ClothCollections[0]);
	const TConstArrayView<FVector3f> Positions = ClothFacade.GetSimPosition3D();
	if (Positions.Num() != Weights.Num())
	{
		OutMessage = TEXT("The hoodie paint map and simulation topology disagree");
		UE_LOG(LogTemp, Error, TEXT("[PanelClothAuthoring] FAIL %s"),
			*OutMessage);
		return false;
	}
	float MinHeight = TNumericLimits<float>::Max();
	float MaxHeight = TNumericLimits<float>::Lowest();
	for (const FVector3f& Position : Positions)
	{
		MinHeight = FMath::Min(MinHeight, Position.Z);
		MaxHeight = FMath::Max(MaxHeight, Position.Z);
	}
	const float HeightRange = FMath::Max(MaxHeight - MinHeight, 1.f);
	for (int32 Index = 0; Index < Weights.Num(); ++Index)
	{
		// Pin the shoulder band, feather the upper torso, and allow the lower
		// fleece body to travel the full two centimetres configured above.
		const float DownFromShoulder =
			(MaxHeight - Positions[Index].Z) / HeightRange;
		Weights[Index] = FMath::SmoothStep(
			0.12f, 0.48f, DownFromShoulder);
	}
	EditableMap->SetVertexAttributeValues(
		Context, Weights, TArray<int32>());
	MaxDistanceMap->Invalidate();

	UDataflowBlueprintLibrary::EvaluateTerminalNodeByName(
		Dataflow, ClothAsset->GetDataflowInstance().GetDataflowTerminal(),
		ClothAsset);
	ClothAsset->MarkPackageDirty();
	Dataflow->MarkPackageDirty();
	if (!SaveAsset(ClothAsset, AssetPackagePath, OutMessage))
	{
		return false;
	}

	// Keep the editable Dataflow graph in the source asset and bake a second,
	// graph-free asset for game/runtime loading. Chaos cloth authoring nodes are
	// editor-only; embedding them in the runtime asset makes standalone builds
	// report missing node types and attempt to prune the graph on load.
	UPackage* RuntimePackage = CreatePackage(RuntimeAssetPackagePath);
	FStructProperty* AssetGuidProperty = FindFProperty<FStructProperty>(
		UChaosClothAsset::StaticClass(), TEXT("AssetGuid"));
	FGuid* SourceAssetGuid = AssetGuidProperty
		? AssetGuidProperty->ContainerPtrToValuePtr<FGuid>(ClothAsset)
		: nullptr;
	if (!SourceAssetGuid)
	{
		OutMessage = TEXT("Unreal could not assign the baked hoodie a unique identity");
		return false;
	}
	UChaosClothAsset* RuntimeAsset = nullptr;
	{
		// Temporarily change the source identity so the fresh duplicate builds a
		// separate deterministic DDC entry, then restore the editable source.
		TGuardValue<FGuid> SourceIdentityGuard(
			*SourceAssetGuid, FGuid::NewGuid());
		RuntimeAsset = Cast<UChaosClothAsset>(
			StaticDuplicateObject(
				ClothAsset, RuntimePackage,
				TEXT("CA_Zarri_Hoodie_Runtime")));
	}
	if (!RuntimeAsset)
	{
		OutMessage = TEXT("Unreal could not duplicate the baked hoodie runtime asset");
		return false;
	}
	RuntimeAsset->SetFlags(RF_Public | RF_Standalone);
	FAssetRegistryModule::AssetCreated(RuntimeAsset);
	RuntimeAsset->SetDataflow(nullptr);
	const TArray<UObject*> RuntimeAssetsToFinish = {RuntimeAsset};
	FAssetCompilingManager::Get().FinishCompilationForObjects(
		RuntimeAssetsToFinish);
	if (!RuntimeAsset->HasValidClothSimulationModels())
	{
		OutMessage = TEXT(
			"Baked hoodie runtime duplicate contains no valid simulation model");
		UE_LOG(LogTemp, Error, TEXT("[PanelClothAuthoring] FAIL %s"),
			*OutMessage);
		return false;
	}
	RuntimeAsset->MarkPackageDirty();
	if (!SaveAsset(RuntimeAsset, RuntimeAssetPackagePath, OutMessage))
	{
		return false;
	}

	OutMessage = FString::Printf(
		TEXT("%s editable %s and created baked %s with %d painted sim vertices; render=%s"),
		bCreated ? TEXT("Created") : TEXT("Updated"), AssetObjectPath,
		RuntimeAssetObjectPath, Weights.Num(), *RenderMesh->GetPathName());
	UE_LOG(LogTemp, Display, TEXT("[PanelClothAuthoring] PASS %s"),
		*OutMessage);
	return ClothAsset->HasValidClothSimulationModels()
		&& !RuntimeAsset->HasDataflow()
		&& RuntimeAsset->HasValidClothSimulationModels();
}
