#include "Player/Ability/GA_AreaBang.h"
#include "Game/BaamGameplayTags.h"
#include "AbilitySystemComponent.h"

UGA_AreaBang::UGA_AreaBang()
{
	// 기본값 = 개틀링(Missed! 로 방어). 인디언은 카드 데이터에서 Response.Allow.Bang 으로.
	ResponseAllow = Bang::Response::Allow::Missed.GetTag();

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(Bang::Ability::AreaBang.GetTag());
	SetAssetTags(AssetTags);

	ActivationBlockedTags.AddTag(Bang::State::Dead.GetTag());
}

void UGA_AreaBang::ActivateAbility(
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

	// TODO(§3): 시전자 기준 시계방향 살아있는 전원에게 ResponseAllow 요청을 순차 push.
	//           막지 못한 좌석마다 GE_Damage(DamageAmount) 적용.
	UE_LOG(LogTemp, Log, TEXT("[Bang] GA_AreaBang — 응답=%s 피해=%d"),
		*ResponseAllow.ToString(), DamageAmount);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
