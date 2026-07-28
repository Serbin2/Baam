#include "Player/Ability/GA_GeneralStore.h"
#include "Game/BaamGameplayTags.h"
#include "AbilitySystemComponent.h"

UGA_GeneralStore::UGA_GeneralStore()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(Bang::Ability::GeneralStore.GetTag());
	SetAssetTags(AssetTags);

	ActivationBlockedTags.AddTag(Bang::State::Dead.GetTag());
}

void UGA_GeneralStore::ActivateAbility(
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

	// TODO(§3): 살아있는 인원 수만큼 덱에서 공개 → 현재 플레이어부터 시계방향으로
	//           PickFromRevealed 응답을 순차 요청. 고른 카드는 각자 손으로, 나머지 정리.
	UE_LOG(LogTemp, Log, TEXT("[Bang] GA_GeneralStore — 공개 후 순차 선택"));

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
