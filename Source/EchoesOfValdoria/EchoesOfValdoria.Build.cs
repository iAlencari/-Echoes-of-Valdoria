// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class EchoesOfValdoria : ModuleRules
{
	public EchoesOfValdoria(ReadOnlyTargetRules Target) : base(Target)
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
			"UMG",
			"Slate",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"NetCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"EchoesOfValdoria",
			"EchoesOfValdoria/Valdoria",
			"EchoesOfValdoria/Variant_Platforming",
			"EchoesOfValdoria/Variant_Platforming/Animation",
			"EchoesOfValdoria/Variant_Combat",
			"EchoesOfValdoria/Variant_Combat/AI",
			"EchoesOfValdoria/Variant_Combat/Animation",
			"EchoesOfValdoria/Variant_Combat/Gameplay",
			"EchoesOfValdoria/Variant_Combat/Interfaces",
			"EchoesOfValdoria/Variant_Combat/UI",
			"EchoesOfValdoria/Variant_SideScrolling",
			"EchoesOfValdoria/Variant_SideScrolling/AI",
			"EchoesOfValdoria/Variant_SideScrolling/Gameplay",
			"EchoesOfValdoria/Variant_SideScrolling/Interfaces",
			"EchoesOfValdoria/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
