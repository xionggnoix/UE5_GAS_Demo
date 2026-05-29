// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DZTest : ModuleRules
{
	public DZTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"DZTest",
			"DZTest/Variant_Horror",
			"DZTest/Variant_Horror/UI",
			"DZTest/Variant_Shooter",
			"DZTest/Variant_Shooter/AI",
			"DZTest/Variant_Shooter/Abilities",
			"DZTest/Variant_Shooter/UI",
			"DZTest/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
