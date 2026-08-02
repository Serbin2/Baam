// ⚠️ [비활성] AbilityByCardId 에 매핑하지 말 것 (GDD §13.1).
//   회복은 UGA_BaamCardEffects 의 HealSelf 가 담당한다(맥주가 쓴다).
//   전원 회복이 필요해지면 HealSelf 옆에 HealAll Op 를 추가하는 편이 낫다.
//   본문이 TODO 스텁이라 매핑하면 카드만 소비되고 아무도 회복하지 않는다.

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
