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

	//	서버에는 OnRep 이 오지 않는다 — 리슨서버 호스트의 UI 를 위해 여기서 직접 알린다.
	OnHandChanged.Broadcast();
}

void ABaamPlayerState::OnRep_Hand()
{
	UE_LOG(LogBaamCard, Verbose, TEXT("[PlayerState] OnRep_Hand — 좌석 %d, %d 장"), SeatIndex, Hand.Num());
	OnHandChanged.Broadcast();
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
			OnHandChanged.Broadcast();   //	서버(리슨 호스트) 자신의 UI 갱신용
			return true;
		}
	}
	return false;
}
