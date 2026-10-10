// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NuriChat : ModuleRules
{
	public NuriChat(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			//initial Depedencies

			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",

			// UI
			"UMG","Slate", "SlateCore",

        });

		PrivateDependencyModuleNames.AddRange(new string[] {});

        PublicIncludePaths.AddRange(new string[] { "NuriChat" });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
