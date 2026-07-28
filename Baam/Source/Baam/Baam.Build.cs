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
			"UMG",
			// --- 세션(방코드 온라인, M6) ---
			"OnlineSubsystem",      // IOnlineSessionPtr / FOnlineSessionSettings 를 헤더에서 사용
			"DeveloperSettings"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate", "SlateCore",
			"OnlineSubsystemUtils"  // Online::GetSubsystem 헬퍼 (.cpp 전용)
		});

		PublicIncludePaths.Add(ModuleDirectory);
	}
}
