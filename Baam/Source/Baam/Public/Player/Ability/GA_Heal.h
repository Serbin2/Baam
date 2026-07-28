// 회복 — Beer(자신 +1). 시전자 자신의 Health 를 HealAmount 만큼 올린다.
// 파라미터로 회복량을 지정 (Beer=1). Saloon(전원 회복)은 GA_Saloon 이 담당.

#pragma once

#include "CoreMinimal.h"
#include "Player/Ability/BaamGameplayAbility.h"
#include "GA_Heal.generated.h"

UCLASS()
class BAAM_API UGA_Heal : public UBaamGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Heal();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 회복량 (Beer = 1). 카드 데이터에서 지정.
	UPROPERTY(EditDefaultsOnly, Category = "Bang")
	int32 HealAmount = 1;
};
