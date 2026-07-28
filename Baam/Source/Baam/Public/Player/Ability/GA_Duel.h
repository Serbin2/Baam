// 결투(Duel) — 대상 1명과 번갈아 BANG! 을 낸다. 먼저 못 내는 쪽이 피해 1.

#pragma once

#include "CoreMinimal.h"
#include "Player/Ability/BaamGameplayAbility.h"
#include "GA_Duel.generated.h"

UCLASS()
class BAAM_API UGA_Duel : public UBaamGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Duel();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 결투에서 지는 쪽이 받는 피해.
	UPROPERTY(EditDefaultsOnly, Category = "Bang")
	int32 DamageAmount = 1;
};
