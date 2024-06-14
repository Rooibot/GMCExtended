using UnrealBuildTool;
using System.IO;

public class GMCExtendedAnimation : ModuleRules
{
    public GMCExtendedAnimation(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "GMCCore",
                "GMCExtended",
                "StructUtils",
                "AnimGraphRuntime"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "NetCore",
                "Slate",
                "SlateCore"
            }
        );

        PublicIncludePaths.AddRange(
            new string[] {
                // ... add public include paths required here ...
                Path.Combine(ModuleDirectory, "Public/Actors"),
                Path.Combine(ModuleDirectory, "Public/Animation"),
                Path.Combine(ModuleDirectory, "Public/Components"),
                Path.Combine(ModuleDirectory, "Public/Data"),
                Path.Combine(ModuleDirectory, "Public/Interfaces"),
                Path.Combine(ModuleDirectory, "Public/Modifiers"),
                Path.Combine(ModuleDirectory, "Public/Support")
            }
        );
				
		
        PrivateIncludePaths.AddRange(
            new string[] {
                // ... add other private include paths required here ...
                Path.Combine(ModuleDirectory, "Private/Actors"),
                Path.Combine(ModuleDirectory, "Private/Animation"),
                Path.Combine(ModuleDirectory, "Private/Components"),
                Path.Combine(ModuleDirectory, "Private/Data"),
                Path.Combine(ModuleDirectory, "Private/Interfaces"),
                Path.Combine(ModuleDirectory, "Private/Modifiers"),
                Path.Combine(ModuleDirectory, "Private/Support")
            }
        );
        
    }
}