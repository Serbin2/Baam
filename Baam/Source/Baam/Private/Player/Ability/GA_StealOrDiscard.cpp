#include "Player/Ability/GA_StealOrDiscard.h"
#include "Game/BaamGameplayTags.h"
#include "AbilitySystemComponent.h"

UGA_StealOrDiscard::UGA_StealOrDiscard()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(Bang::Ability::StealOrDiscard.GetTag());
	SetAssetTags(AssetTags);

	ActivationBlockedTags.AddTag(Bang::State::Dead.GetTag());
}

void UGA_StealOrDiscard::ActivateAbility(
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

	// TODO(§3): 대상 좌석에게 PickFromPlayer 응답 요청(손패 뒷면 index or 장비 선택).
	//           선택된 카드를 bSteal 이면 시전자 손으로, 아니면 버린 패로 이동.
	UE_LOG(LogTemp, Log, TEXT("[Bang] GA_StealOrDiscard — %s"),
		bSteal ? TEXT("Panic!(훔치기)") : TEXT("CatBalou(버리기)"));

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
