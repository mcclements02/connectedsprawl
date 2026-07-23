// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ConnectedSprawl : ModuleRules
{
	public ConnectedSprawl(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"ChaosVehicles",
			"PhysicsCore",
			"UMG",
			"GameplayTags",
			"GameplayTasks",
			"AIModule",
			"NavigationSystem",
			"Niagara",
			"ProceduralMeshComponent",
			"ChaosClothAssetEngine"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"AssetRegistry",
			"Slate",
			"SlateCore"
		});

		PublicIncludePaths.AddRange(new string[] {
			"ConnectedSprawl/Public"
		});
	}
}
