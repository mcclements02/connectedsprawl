// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ConnectedSprawlEditor : ModuleRules
{
	public ConnectedSprawlEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"ConnectedSprawl",
			"AssetRegistry",
			"AssetTools",
			"UnrealEd",
			"DataflowCore",
			"DataflowEngine",
			"DataflowNodes",
			"ChaosClothAsset",
			"ChaosClothAssetEngine",
			"ChaosClothAssetTools",
			"ChaosClothAssetDataflowNodes"
		});

		PublicIncludePaths.AddRange(new string[] {
			"ConnectedSprawlEditor/Public"
		});
	}
}
