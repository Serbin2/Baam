// BANG! — 대상 1명에게 피해 1 (Missed!로 막힐 수 있음).
// 1단계에서는 발동 로그만. 실제 GE_Damage 적용과 응답 창은 이후 단계에서 붙인다.

#pragma once

#include "CoreMinimal.h"
#include "Player/Ability/BaamGameplayAbility.h"
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
};
