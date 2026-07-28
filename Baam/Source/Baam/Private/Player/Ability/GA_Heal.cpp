#include "Player/Ability/GA_Heal.h"
#include "Game/BaamGameplayTags.h"
#include "AbilitySystemComponent.h"

UGA_Heal::UGA_Heal()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(Bang::Ability::Heal.GetTag());
	SetAssetTags(AssetTags);

	// 사망한 플레이어는 카드를 쓸 수 없다.
	ActivationBlockedTags.AddTag(Bang::State::Dead.GetTag());
}

void UGA_Heal::ActivateAbility(
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

	// TODO: 자신 ASC 에 GE_Heal(SetByCaller.Heal = HealAmount) 적용.
	//       Health 는 MaxHealth 로 클램프(AttributeSet 에서 처리).
	// TODO(§M4): 생존 2명이면 Beer 무효 — GameState 의 생존 수 확인 후 스킵.
	const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	UE_LOG(LogTemp, Log, TEXT("[Bang] GA_Heal — %s 회복 %d"),
		Avatar ? *Avatar->GetName() : TEXT("null"), HealAmount);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
