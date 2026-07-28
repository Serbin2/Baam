#include "Player/Ability/BaamGameplayAbility.h"

UBaamGameplayAbility::UBaamGameplayAbility()
{
	// 서버 권위 실행 — 클라 예측 없음 (md §1.2).
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}
