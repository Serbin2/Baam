// 견제 — 당황(Panic!, 훔치기) / 캣 발루(Cat Balou, 버리기).
// 대상의 손패(뒷면) 또는 장비 1장을 선택 → bSteal 이면 내 손으로, 아니면 버린 패로.

#pragma once

#include "CoreMinimal.h"
#include "Player/Ability/BaamGameplayAbility.h"
#include "GA_StealOrDiscard.generated.h"

UCLASS()
class BAAM_API UGA_StealOrDiscard : public UBaamGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_StealOrDiscard();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// true = 훔치기(Panic! → 내 손으로) / false = 버리기(Cat Balou → 버린 패).
	UPROPERTY(EditDefaultsOnly, Category = "Bang")
	bool bSteal = true;
};
