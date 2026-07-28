#include "Game/BaamPlayerState.h"

#include "Game/BaamCardLog.h"
#include "Game/BaamCardType.h"
#include "Net/UnrealNetwork.h"

ABaamPlayerState::ABaamPlayerState()
{
	// 로비 표시가 즉시 따라오도록 기본값(2Hz)보다 자주 보낸다.
	SetNetUpdateFrequency(10.f);
}

void ABaamPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABaamPlayerState, Hand, COND_OwnerOnly);
	DOREPLIFETIME(ABaamPlayerState, SeatIndex);
	DOREPLIFETIME(ABaamPlayerState, HandCount);
	DOREPLIFETIME(ABaamPlayerState, Equipment);
}

void ABaamPlayerState::SetSeatIndex(int32 InSeat)
{
	if (HasAuthority())
	{
		SeatIndex = InSeat;
	}
}

void ABaamPlayerState::AddCardToHand(const FBaamCardInstance& Card)
{
	if (!HasAuthority())
	{
		return;
	}
	Hand.Add(Card);
	//	Hand 는 COND_OwnerOnly 라 남에게 복제되지 않는다. Hand 를 건드릴 때마다 반드시 함께 갱신한다.
	HandCount = Hand.Num();
}

void ABaamPlayerState::OnRep_Hand()
{
	//	TODO: 손패 UI 갱신 알림. 지금은 복제 도착만 확인한다.
	UE_LOG(LogBaamCard, Verbose, TEXT("[PlayerState] OnRep_Hand — 좌석 %d, %d 장"), SeatIndex, Hand.Num());
}

bool ABaamPlayerState::RemoveCardFromHand(int32 InstanceId, FBaamCardInstance& OutRemoved)
{
	if (!HasAuthority())
	{
		return false;
	}
	for (int32 i = 0; i < Hand.Num(); i++)
	{
		if (Hand[i].InstanceId == InstanceId)
		{
			OutRemoved = Hand[i];
			Hand.RemoveAt(i);
			HandCount = Hand.Num();
			return true;
		}
	}
	return false;
}
