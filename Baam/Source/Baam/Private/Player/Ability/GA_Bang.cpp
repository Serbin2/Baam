#include "Player/Ability/GA_Bang.h"
#include "Game/BaamGameplayTags.h"
#include "AbilitySystemComponent.h"

UGA_Bang::UGA_Bang()
{
	// Asset 태그 = Ability.Bang (메커니즘 식별).
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(Bang::Ability::Bang.GetTag());
	SetAssetTags(AssetTags);
}

void UGA_Bang::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/true);
		return;
	}

	// 1단계: 발동이 서버에 도달했는지 확인용 로그.
	// TODO(2단계): 대상 좌석으로 GE_Damage 적용 + FBangActionCue 방송(연출).
	const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	UE_LOG(LogTemp, Log, TEXT("[Bang] GA_Bang 발동 — Avatar=%s"),
		Avatar ? *Avatar->GetName() : TEXT("null"));

	EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}
