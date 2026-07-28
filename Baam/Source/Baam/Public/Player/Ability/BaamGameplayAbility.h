// BANG! 어빌리티 베이스 — 턴제라 예측을 쓰지 않는다 (md §1.2).
//   NetExecutionPolicy = ServerOnly, InstancingPolicy = InstancedPerActor.
// 카드 메커니즘 GA(GA_Bang/GA_Heal/...)는 전부 이걸 상속한다.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "BaamGameplayAbility.generated.h"

UCLASS(Abstract)
class BAAM_API UBaamGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UBaamGameplayAbility();
};
