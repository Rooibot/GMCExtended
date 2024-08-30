// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;
using System.IO;

public class GMCExtended : ModuleRules
{
	public GMCExtended(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
				Path.Combine(ModuleDirectory, "Public/Components"),
				Path.Combine(ModuleDirectory, "Public/Interfaces"),
				Path.Combine(ModuleDirectory, "Public/Solvers"),
				Path.Combine(ModuleDirectory, "Public/Support")
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
				Path.Combine(ModuleDirectory, "Private/Components"),
				Path.Combine(ModuleDirectory, "Private/Interfaces"),
				Path.Combine(ModuleDirectory, "Private/Solvers"),
				Path.Combine(ModuleDirectory, "Private/Support")
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"GMCCore",
				"GameplayTags", "PoseSearch",
				"StructUtils"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
