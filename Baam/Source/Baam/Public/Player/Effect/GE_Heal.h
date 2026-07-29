// 회복 GE (Instant) — Health 를 SetByCaller.Heal 값만큼 가산한다.
//   값은 양수(회복량). MaxHealth 초과분은 AttributeSet 이 클램프한다.
//   Beer(자신 +1) 는 GA_Heal 이, Saloon(전원) 은 GA_Saloon 이 이 GE 를 적용한다.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Heal.generated.h"

UCLASS()
class BAAM_API UGE_Heal : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_Heal();
};
