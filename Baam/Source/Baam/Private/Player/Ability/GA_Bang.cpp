#include "Player/Ability/GA_Bang.h"
#include "Player/Effect/GE_Damage.h"
#include "Player/Component/BaamAttributeSet.h"
#include "Game/BaamGameplayTags.h"
#include "Game/BaamGameMode.h"
#include "Game/BaamDebug.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

UGA_Bang::UGA_Bang()
{
	// Asset 태그 = Ability.Bang (메커니즘 식별).
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(Bang::Ability::Bang.GetTag());
	SetAssetTags(AssetTags);
}

int32 UGA_Bang::TierBaseDamage(EBaamDiceOutcome Outcome) const
{
	switch (Outcome)
	{
	case EBaamDiceOutcome::CriticalSuccess: return CriticalSuccessDamage;
	case EBaamDiceOutcome::Success:         return SuccessDamage;
	case EBaamDiceOutcome::Failure:         return FailureDamage;
	case EBaamDiceOutcome::CriticalFailure: return CriticalFailureDamage;
	default:                                return 0;
	}
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

	UAbilitySystemComponent* SourceASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const UBaamAttributeSet* SourceAS = SourceASC ? SourceASC->GetSet<UBaamAttributeSet>() : nullptr;
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;

	ABaamGameMode* GM = Avatar ? Avatar->GetWorld()->GetAuthGameMode<ABaamGameMode>() : nullptr;
	if (!SourceASC || !SourceAS || !GM)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bang] GA_Bang — 서버 컨텍스트 없음(ASC/AS/GameMode). 중단."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ── 판정: 행운(Luck)을 굴림 보정으로 넣어 주사위를 굴린다 ──
	const int32 LuckBonus = FMath::RoundToInt(SourceAS->GetLuck());
	int32 Roll = 0;
	const EBaamDiceOutcome Outcome = GM->RollForOutcome(Roll, /*Faces=*/0, /*RollBonus=*/LuckBonus);

	// ── 피해 계산: 등급 기본 피해 × 힘 배율 − 대상 지능 경감 ──
	const int32 TierBase = TierBaseDamage(Outcome);

	int32 FinalDamage = 0;
	if (TierBase > 0)
	{
		const float StrengthMult = 1.f + SourceAS->GetStrength() * StrengthDamageMult;
		FinalDamage = FMath::RoundToInt(TierBase * StrengthMult);
	}

	// 대상 확보(응답 창/타깃팅은 §3에서 확장). 지금은 이벤트로 넘어온 대상만 사용.
	AActor* TargetActor = TriggerEventData ? const_cast<AActor*>(TriggerEventData->Target.Get()) : nullptr;
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);

	// 대상 지능만큼 피해 경감.
	if (FinalDamage > 0 && TargetASC)
	{
		if (const UBaamAttributeSet* TargetAS = TargetASC->GetSet<UBaamAttributeSet>())
		{
			const int32 Mitigation = FMath::RoundToInt(TargetAS->GetIntelligence() * IntelligenceMitigation);
			FinalDamage = FMath::Max(0, FinalDamage - Mitigation);
		}
	}

	// 판정 등급별 색: 대성공=주황, 성공=노랑, 실패=회색, 대실패=빨강.
	FColor OutcomeColor = FColor::Yellow;
	switch (Outcome)
	{
	case EBaamDiceOutcome::CriticalSuccess: OutcomeColor = FColor(255, 140, 0); break; // 주황
	case EBaamDiceOutcome::Success:         OutcomeColor = FColor::Yellow;       break;
	case EBaamDiceOutcome::Failure:         OutcomeColor = FColor(160, 160, 160); break; // 회색
	case EBaamDiceOutcome::CriticalFailure: OutcomeColor = FColor::Red;          break;
	}

	BaamDebug::Screen(
		FString::Printf(TEXT("뱅  %s → [%s] 눈%d(행운%+d)  기본%d ×힘 → 피해 %d  대상:%s"),
			Avatar ? *Avatar->GetName() : TEXT("?"),
			*ABaamGameMode::GetOutcomeText(Outcome).ToString(), Roll, LuckBonus, TierBase, FinalDamage,
			TargetActor ? *TargetActor->GetName() : TEXT("없음")),
		OutcomeColor, /*Time=*/5.f);

	// ── 적용: 대상이 있고 피해가 남으면 GE_Damage(음수)로 Health 차감 ──
	if (FinalDamage > 0 && TargetASC)
	{
		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddSourceObject(this);
		Context.AddInstigator(Avatar, Avatar);

		const FGameplayEffectSpecHandle Spec =
			SourceASC->MakeOutgoingSpec(UGE_Damage::StaticClass(), 1.f, Context);
		if (Spec.IsValid())
		{
			// 피해는 음수로 넣는다(Health 가산 = 감소).
			Spec.Data->SetSetByCallerMagnitude(Bang::SetByCaller::Damage.GetTag(), -static_cast<float>(FinalDamage));
			SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
		}
	}

	OnBangResolved(Outcome, FinalDamage, TargetActor);

	EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/true, /*bWasCancelled=*/false);
}
