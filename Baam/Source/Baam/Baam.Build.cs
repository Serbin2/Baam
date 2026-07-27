// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Baam : ModuleRules
{
	public Baam(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			// --- GAS (Team4Project 의 GAS 시스템과 동일 구성) ---
			"GameplayAbilities",   // UGameplayAbility / UGameplayEffect / UAttributeSet
			"GameplayTags",        // NativeGameplayTags (UE_DEFINE_GAMEPLAY_TAG)
			"GameplayTasks",        // UAbilityTask (응답 창 WaitForResolution 등)
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		PublicIncludePaths.Add(ModuleDirectory);
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
