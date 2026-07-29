// BANG! — 대상 1명에게 피해. 서버 주사위 판정(EBaamDiceOutcome)으로 피해 등급이 갈린다.
//   행운 → 판정 굴림 가산(좋은 눈이 잘 뜸), 힘 → 피해 배율, 대상의 지능 → 받는 피해 경감.
//   등급별 기본 피해: 대성공 2 / 성공 1 / 실패 1(경감) / 대실패 0.
//
// 대상은 TriggerEventData->Target 으로 전달된다(응답 창/타깃팅은 §3에서 확장).
// 대상이 없으면(단독 테스트) 계산된 피해를 로그로만 남기고 적용은 건너뛴다.

#pragma once

#include "CoreMinimal.h"
#include "Player/Ability/BaamGameplayAbility.h"
#include "Game/BaamGameDataTypes.h"   // EBaamDiceOutcome
#include "GA_Bang.generated.h"

UCLASS()
class BAAM_API UGA_Bang : public UBaamGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Bang();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 판정 등급별 기본 피해(스탯 보정 전). 낮은 눈=대실패 … 높은 눈=대성공.
	UPROPERTY(EditDefaultsOnly, Category = "Bang|Damage")
	int32 CriticalSuccessDamage = 2;   // 크리티컬

	UPROPERTY(EditDefaultsOnly, Category = "Bang|Damage")
	int32 SuccessDamage = 1;           // 기본

	UPROPERTY(EditDefaultsOnly, Category = "Bang|Damage")
	int32 FailureDamage = 1;           // 경감(맞긴 함)

	UPROPERTY(EditDefaultsOnly, Category = "Bang|Damage")
	int32 CriticalFailureDamage = 0;   // 빗나감

	// 힘 1당 피해 배율 증가분. 최종 배율 = 1 + Strength * StrengthDamageMult.
	UPROPERTY(EditDefaultsOnly, Category = "Bang|Damage", meta = (ClampMin = "0.0"))
	float StrengthDamageMult = 0.25f;

	// 대상 지능 1당 경감되는 피해량(정수 반올림 후 차감).
	UPROPERTY(EditDefaultsOnly, Category = "Bang|Damage", meta = (ClampMin = "0.0"))
	float IntelligenceMitigation = 0.5f;

	// 판정/피해가 확정됐을 때(연출·UI용) 방송. 서버에서 호출.
	UFUNCTION(BlueprintImplementableEvent, Category = "Bang")
	void OnBangResolved(EBaamDiceOutcome Outcome, int32 FinalDamage, AActor* Target);

	// 등급 → 기본 피해(스탯 보정 전) 매핑.
	int32 TierBaseDamage(EBaamDiceOutcome Outcome) const;
};
