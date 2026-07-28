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

	DOREPLIFETIME(ABaamPlayerState, SeatIndex);
}

void ABaamPlayerState::SetSeatIndex(int32 InSeat)
{
	if (HasAuthority())
	{
		SeatIndex = InSeat;
	}
}
