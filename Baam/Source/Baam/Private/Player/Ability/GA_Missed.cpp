#include "Player/Ability/GA_Missed.h"
#include "Game/BaamGameplayTags.h"
#include "AbilitySystemComponent.h"

UGA_Missed::UGA_Missed()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(Bang::Ability::Missed.GetTag());
	// Reaction 성질 — 응답 창에서만 사용 (능동 사용 검증에서 걸러진다).
	AssetTags.AddTag(Bang::Card::Trait::Reaction.GetTag());
	SetAssetTags(AssetTags);

	ActivationBlockedTags.AddTag(Bang::State::Dead.GetTag());
}

void UGA_Missed::ActivateAbility(
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

	// TODO(§3): 진행 중인 응답 요청(FBangResolutionRequest)에 Missed! 응답을 제출 →
	//           GA_Bang 의 WaitForResolution 이 무효로 이어진다.
	//           MissedRequired(Slab the Killer=2)만큼 필요하면 요청이 계속 유지된다.
	const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	UE_LOG(LogTemp, Log, TEXT("[Bang] GA_Missed — %s 빗나감!"),
		Avatar ? *Avatar->GetName() : TEXT("null"));

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
