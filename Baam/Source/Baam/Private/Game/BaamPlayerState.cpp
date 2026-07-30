#include "Game/BaamPlayerState.h"

#include "Game/BaamCardLog.h"
#include "Game/BaamCardType.h"
#include "Game/BaamReadyComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

ABaamPlayerState::ABaamPlayerState()
{
	// 로비 표시가 즉시 따라오도록 기본값(2Hz)보다 자주 보낸다.
	SetNetUpdateFrequency(10.f);

	Ready = CreateDefaultSubobject<UBaamReadyComponent>(TEXT("Ready"));
}

void ABaamPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABaamPlayerState, Hand, COND_OwnerOnly);
	DOREPLIFETIME(ABaamPlayerState, SeatIndex);
	DOREPLIFETIME(ABaamPlayerState, HandCount);
	DOREPLIFETIME(ABaamPlayerState, Equipment);
	DOREPLIFETIME(ABaamPlayerState, bIsDead);
	DOREPLIFETIME(ABaamPlayerState, PublicRoleTag);
	DOREPLIFETIME(ABaamPlayerState, CardsUsedThisTurn);
	DOREPLIFETIME(ABaamPlayerState, PendingStatuses);
}

// ══════════════════════════════════════════════════════════════════════════════════════
//  "다음 1회" 대기 상태
// ══════════════════════════════════════════════════════════════════════════════════════

bool ABaamPlayerState::HasPendingStatus(FGameplayTag StatusTag) const
{
	if (!StatusTag.IsValid())
	{
		return false;
	}
	return PendingStatuses.ContainsByPredicate(
		[StatusTag](const FBaamPendingStatus& S) { return S.Tag == StatusTag; });
}

int32 ABaamPlayerState::GetPendingStatusAmount(FGameplayTag StatusTag) const
{
	for (const FBaamPendingStatus& S : PendingStatuses)
	{
		if (S.Tag == StatusTag)
		{
			return S.Amount;
		}
	}
	return 0;
}

bool ABaamPlayerState::AddPendingStatus(FGameplayTag StatusTag, int32 Amount)
{
	if (!HasAuthority() || !StatusTag.IsValid())
	{
		return false;
	}

	//	중첩 금지. 같은 상태를 두 번 걸 수 없다 — 사용 가능 여부는 HandlePlayCard 가
	//	카드를 소비하기 "전에" 미리 검사하므로, 여기까지 오면 정상적으로는 중복이 없다.
	if (HasPendingStatus(StatusTag))
	{
		UE_LOG(LogBaamCard, Warning,
			TEXT("[Status] 좌석 %d 은 이미 %s 를 갖고 있습니다 — 중첩하지 않습니다."),
			SeatIndex, *StatusTag.ToString());
		return false;
	}

	FBaamPendingStatus& Added = PendingStatuses.AddDefaulted_GetRef();
	Added.Tag    = StatusTag;
	Added.Amount = Amount;

	UE_LOG(LogBaamCard, Log, TEXT("[Status] 좌석 %d 에 %s (수치 %d) 부여"),
		SeatIndex, *StatusTag.ToString(), Amount);
	return true;
}

bool ABaamPlayerState::ConsumePendingStatus(FGameplayTag StatusTag, int32& OutAmount)
{
	OutAmount = 0;
	if (!HasAuthority() || !StatusTag.IsValid())
	{
		return false;
	}

	const int32 Index = PendingStatuses.IndexOfByPredicate(
		[StatusTag](const FBaamPendingStatus& S) { return S.Tag == StatusTag; });
	if (Index == INDEX_NONE)
	{
		return false;
	}

	OutAmount = PendingStatuses[Index].Amount;
	PendingStatuses.RemoveAt(Index);

	UE_LOG(LogBaamCard, Log, TEXT("[Status] 좌석 %d 의 %s 소모 (수치 %d)"),
		SeatIndex, *StatusTag.ToString(), OutAmount);
	return true;
}

void ABaamPlayerState::ClearPendingStatuses()
{
	if (HasAuthority())
	{
		PendingStatuses.Reset();
	}
}

void ABaamPlayerState::ResetCardsUsedThisTurn()
{
	if (HasAuthority())
	{
		CardsUsedThisTurn = 0;
	}
}

void ABaamPlayerState::IncrementCardsUsedThisTurn()
{
	if (HasAuthority())
	{
		++CardsUsedThisTurn;
	}
}

int32 ABaamPlayerState::DecrementCardsUsedThisTurn(int32 Count)
{
	if (!HasAuthority() || Count <= 0)
	{
		return 0;
	}

	//	0 미만으로 내려가면 한도를 넘는 추가 사용이 되어 버린다.
	//	"회복" 은 이번 턴에 쓴 만큼까지만으로 해석한다(GDD §14 미결정 — 확정 시 재검토).
	const int32 Restored = FMath::Min(Count, CardsUsedThisTurn);
	CardsUsedThisTurn -= Restored;
	return Restored;
}

void ABaamPlayerState::SetPublicRole(const FGameplayTag& InRole)
{
	if (HasAuthority())
	{
		PublicRoleTag = InRole;
	}
}

void ABaamPlayerState::SetDead(bool bInDead)
{
	if (HasAuthority())
	{
		bIsDead = bInDead;
	}
}

void ABaamPlayerState::TakeAllCards(TArray<FBaamCardInstance>& OutCards)
{
	OutCards.Reset();
	if (!HasAuthority())
	{
		return;
	}

	OutCards.Append(Hand);
	OutCards.Append(Equipment);

	//	장비를 배열에서만 지우면 GE 가 남아 "장비는 없는데 효과는 계속" 이 된다.
	if (UAbilitySystemComponent* ASC = GetOwnedAbilitySystemComponent())
	{
		for (const TPair<FName, FActiveGameplayEffectHandle>& Pair : EquipEffectHandles)
		{
			if (Pair.Value.IsValid())
			{
				ASC->RemoveActiveGameplayEffect(Pair.Value);
			}
		}
	}
	EquipEffectHandles.Reset();

	Hand.Reset();
	Equipment.Reset();
	HandCount = 0;

	OnHandChanged.Broadcast();   //	서버(리슨 호스트) 자신의 UI 갱신용
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

// ══════════════════════════════════════════════════════════════════════════════════════
//  장비(파란 카드)
// ══════════════════════════════════════════════════════════════════════════════════════

UAbilitySystemComponent* ABaamPlayerState::GetOwnedAbilitySystemComponent() const
{
	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetPawn());
}

bool ABaamPlayerState::HasEquippedCard(FName CardId) const
{
	return Equipment.ContainsByPredicate(
		[CardId](const FBaamCardInstance& C) { return C.CardId == CardId; });
}

bool ABaamPlayerState::EquipCard(const FBaamCardInstance& Card, TSubclassOf<UGameplayEffect> EquipEffect)
{
	if (!HasAuthority() || !Card.IsValid())
	{
		return false;
	}

	//	중복 장착 금지 — 같은 장비를 두 번 붙이면 효과가 두 배가 된다.
	if (HasEquippedCard(Card.CardId))
	{
		UE_LOG(LogBaamCard, Warning, TEXT("[Equip] 좌석 %d 은 이미 %s 를 장착 중입니다."),
			SeatIndex, *Card.CardId.ToString());
		return false;
	}

	UAbilitySystemComponent* ASC = GetOwnedAbilitySystemComponent();

	//	EquipEffect 가 비어 있어도 장착은 허용한다 — 아직 GE 를 만들지 않은 장비를
	//	데이터로 먼저 넣어보는 경우가 있다. 다만 효과가 없다는 것은 남긴다.
	if (EquipEffect && ASC)
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(this);

		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EquipEffect, 1.f, Context);
		if (Spec.IsValid())
		{
			const FActiveGameplayEffectHandle Applied = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			if (Applied.IsValid())
			{
				EquipEffectHandles.Add(Card.CardId, Applied);
			}
			else
			{
				//	Instant GE 는 유효 핸들을 주지 않는다 — 장비는 Infinite 여야 한다.
				UE_LOG(LogBaamCard, Warning,
					TEXT("[Equip] %s 의 EquipEffect 가 지속 효과가 아닙니다(Infinite 인지 확인). 해제 시 되돌릴 수 없습니다."),
					*Card.CardId.ToString());
			}
		}
	}
	else if (!EquipEffect)
	{
		UE_LOG(LogBaamCard, Warning, TEXT("[Equip] %s 에 EquipEffect 가 없습니다 — 효과 없이 장착만 됩니다."),
			*Card.CardId.ToString());
	}

	Equipment.Add(Card);
	UE_LOG(LogBaamCard, Log, TEXT("[Equip] 좌석 %d 이 %s#%d 장착 (장비 %d개)"),
		SeatIndex, *Card.CardId.ToString(), Card.InstanceId, Equipment.Num());
	return true;
}

bool ABaamPlayerState::UnequipCard(FName CardId, FBaamCardInstance& OutRemoved)
{
	if (!HasAuthority())
	{
		return false;
	}

	const int32 Index = Equipment.IndexOfByPredicate(
		[CardId](const FBaamCardInstance& C) { return C.CardId == CardId; });
	if (Index == INDEX_NONE)
	{
		return false;
	}

	OutRemoved = Equipment[Index];
	Equipment.RemoveAt(Index);

	//	GE 를 함께 걷어낸다 — 배열만 지우면 효과가 영구히 남는다.
	if (FActiveGameplayEffectHandle* Handle = EquipEffectHandles.Find(CardId))
	{
		if (Handle->IsValid())
		{
			if (UAbilitySystemComponent* ASC = GetOwnedAbilitySystemComponent())
			{
				ASC->RemoveActiveGameplayEffect(*Handle);
			}
		}
		EquipEffectHandles.Remove(CardId);
	}

	UE_LOG(LogBaamCard, Log, TEXT("[Equip] 좌석 %d 이 %s 해제"), SeatIndex, *CardId.ToString());
	return true;
}
