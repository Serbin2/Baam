// 빗나감!(Missed!) — 응답 전용. BANG!/개틀링에 대한 반응으로만 발동된다 (능동 사용 X).
// 응답 창(Resolution)에서 서버가 검증 후 활성화 → 해당 BANG 을 무효화한다.

#pragma once

#include "CoreMinimal.h"
#include "Player/Ability/BaamGameplayAbility.h"
#include "GA_Missed.generated.h"

UCLASS()
class BAAM_API UGA_Missed : public UBaamGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Missed();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
