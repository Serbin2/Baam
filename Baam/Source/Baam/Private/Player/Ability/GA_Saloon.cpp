#include "Player/Ability/GA_Saloon.h"
#include "Game/BaamGameplayTags.h"
#include "AbilitySystemComponent.h"

UGA_Saloon::UGA_Saloon()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(Bang::Ability::Saloon.GetTag());
	SetAssetTags(AssetTags);

	ActivationBlockedTags.AddTag(Bang::State::Dead.GetTag());
}

void UGA_Saloon::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// TODO(§M4): GameState 의 살아있는 전원 ASC 에 GE_Heal(HealAmount) 적용.
	//            각자 MaxHealth 로 클램프.
	UE_LOG(LogTemp, Log, TEXT("[Bang] GA_Saloon — 전원 회복 %d"), HealAmount);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
