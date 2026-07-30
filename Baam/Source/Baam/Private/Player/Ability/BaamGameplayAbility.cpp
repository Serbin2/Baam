#include "Player/Ability/BaamGameplayAbility.h"
#include "Game/BaamPlayerState.h"
#include "GameFramework/Pawn.h"

UBaamGameplayAbility::UBaamGameplayAbility()
{
	// 서버 권위 실행 — 클라 예측 없음 (md §1.2).
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy   = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

ABaamPlayerState* UBaamGameplayAbility::GetBaamPlayerState(AActor* Actor)
{
	const APawn* Pawn = Cast<APawn>(Actor);
	return Pawn ? Pawn->GetPlayerState<ABaamPlayerState>() : nullptr;
}
