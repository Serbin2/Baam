#include "Game/BaamPlayerState.h"

#include "Net/UnrealNetwork.h"

ABaamPlayerState::ABaamPlayerState()
{
	// 로비 표시가 즉시 따라오도록 기본값(2Hz)보다 자주 보낸다.
	SetNetUpdateFrequency(10.f);
}

void ABaamPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaamPlayerState, bIsReady);
	DOREPLIFETIME(ABaamPlayerState, bIsHost);
}

void ABaamPlayerState::Server_SetReady_Implementation(bool bInReady)
{
	// 호스트는 Ready 대상이 아니다.
	if (!bIsHost)
	{
		bIsReady = bInReady;
	}
}

void ABaamPlayerState::SetIsHost(bool bInIsHost)
{
	if (!HasAuthority())
	{
		return;
	}

	bIsHost = bInIsHost;
	if (bIsHost)
	{
		bIsReady = false;
	}
}
