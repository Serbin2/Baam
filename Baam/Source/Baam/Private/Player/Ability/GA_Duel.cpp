#include "Player/Ability/GA_Duel.h"
#include "Game/BaamGameplayTags.h"
#include "AbilitySystemComponent.h"

UGA_Duel::UGA_Duel()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(Bang::Ability::Duel.GetTag());
	SetAssetTags(AssetTags);

	ActivationBlockedTags.AddTag(Bang::State::Dead.GetTag());
}

void UGA_Duel::ActivateAbility(
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

	// TODO(§3): 대상부터 시작해 두 좌석을 번갈아 Response.Allow.Bang 요청.
	//           먼저 Pass(BANG! 못 냄)한 쪽에 GE_Damage(DamageAmount).
	UE_LOG(LogTemp, Log, TEXT("[Bang] GA_Duel — 피해 %d"), DamageAmount);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
