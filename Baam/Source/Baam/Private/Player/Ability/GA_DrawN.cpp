// ⚠️ [비활성] AbilityByCardId 에 매핑하지 말 것 (GDD §13.1).
//   카드 뽑기는 UGA_BaamCardEffects 의 DrawSelf 가 담당한다(휴식·속공이 쓴다).
//   본문이 TODO 스텁이라 매핑하면 카드만 소비되고 한 장도 뽑히지 않는다.
//   정식 경로는 UGA_BaamCardEffects + DT_BaamCard 의 OutcomeEffects 하나뿐이다.

#include "Player/Ability/GA_DrawN.h"
#include "Game/BaamGameplayTags.h"
#include "AbilitySystemComponent.h"

UGA_DrawN::UGA_DrawN()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(Bang::Ability::DrawN.GetTag());
	SetAssetTags(AssetTags);

	ActivationBlockedTags.AddTag(Bang::State::Dead.GetTag());
}

void UGA_DrawN::ActivateAbility(
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

	// TODO(§M1/M2): GameState 덱에서 DrawCount 장을 뽑아 시전자 PlayerState.Hand 로.
	//               덱 소진 시 버린 패 리셔플.
	const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	UE_LOG(LogTemp, Log, TEXT("[Bang] GA_DrawN — %s 뽑기 %d장"),
		Avatar ? *Avatar->GetName() : TEXT("null"), DrawCount);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
