#include "Player/Ability/GA_Heal.h"
#include "Game/BaamDiceComponent.h"
#include "Player/Effect/GE_Heal.h"
#include "Player/Component/BaamAttributeSet.h"
#include "Game/BaamGameplayTags.h"
#include "Game/BaamDebug.h"
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

	// TODO(§M4): 생존 2명이면 Beer 무효 — GameState 의 생존 수 확인 후 스킵.
	const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;

	// 자신 ASC 에 GE_Heal(SetByCaller.Heal = 회복량) 적용. MaxHealth 클램프는 AttributeSet 담당.
	UAbilitySystemComponent* SourceASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

	// ── 판정 결과 수신 (GDD §9.1) ──
	//	서버가 카드 데이터로 판정을 끝냈다. 등급별 회복량이 EventMagnitude 로 온다.
	//	Resolution.* 태그가 없으면 이 GA 의 HealAmount 로 폴백한다(구형 경로).
	static const FGameplayTag ResolutionRoot = FGameplayTag::RequestGameplayTag(TEXT("Resolution"));
	const bool bHasServerOutcome =
		TriggerEventData && TriggerEventData->InstigatorTags.HasTag(ResolutionRoot);

	const EBaamDiceOutcome Outcome = bHasServerOutcome
		? UBaamDiceComponent::TagsToOutcome(TriggerEventData->InstigatorTags, EBaamDiceOutcome::Failure)
		: EBaamDiceOutcome::Success;

	const int32 FinalHeal = bHasServerOutcome
		? FMath::RoundToInt(TriggerEventData->EventMagnitude)
		: HealAmount;

	if (SourceASC && FinalHeal > 0)
	{
		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddSourceObject(this);

		const FGameplayEffectSpecHandle Spec =
			SourceASC->MakeOutgoingSpec(UGE_Heal::StaticClass(), 1.f, Context);
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(Bang::SetByCaller::Heal.GetTag(), static_cast<float>(FinalHeal));
			SourceASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	// 적용 후 현재 체력을 함께 표시(회복이 실제로 붙었는지 눈으로 확인).
	const UBaamAttributeSet* AS = SourceASC ? SourceASC->GetSet<UBaamAttributeSet>() : nullptr;
	const float HP    = AS ? AS->GetHealth()    : 0.f;
	const float MaxHP = AS ? AS->GetMaxHealth() : 0.f;

	BaamDebug::Screen(
		FString::Printf(TEXT("맥주  %s → [%s] 회복 +%d  (HP %.0f/%.0f)"),
			Avatar ? *Avatar->GetName() : TEXT("?"),
			*UBaamDiceComponent::GetOutcomeText(Outcome).ToString(), FinalHeal, HP, MaxHP),
		(FinalHeal > 0) ? FColor::Green : FColor(160, 160, 160), /*Time=*/5.f);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
