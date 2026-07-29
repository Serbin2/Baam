#include "Game/BaamSeatViewLibrary.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Game/BaamCardLog.h"
#include "Game/BaamCardType.h"
#include "Game/BaamDataSubsystem.h"
#include "Game/BaamGameDataTypes.h"
#include "Game/BaamGameplayTags.h"
#include "Game/BaamPlayerState.h"
#include "Player/Component/BaamAttributeSet.h"

namespace
{
	/** 좌석이 배정된 PlayerState 만 좌석 번호 순으로 모은다. */
	void CollectSeatedPlayerStates(const APlayerController* Viewer, TArray<ABaamPlayerState*>& OutStates)
	{
		OutStates.Reset();

		const UWorld* World = Viewer ? Viewer->GetWorld() : nullptr;
		const AGameStateBase* GS = World ? World->GetGameState() : nullptr;
		if (!GS)
		{
			return;
		}

		for (APlayerState* Base : GS->PlayerArray)
		{
			ABaamPlayerState* PS = Cast<ABaamPlayerState>(Base);
			if (PS && PS->GetSeatIndex() != INDEX_NONE)
			{
				OutStates.Add(PS);
			}
		}

		OutStates.Sort([](const ABaamPlayerState& A, const ABaamPlayerState& B)
		{
			return A.GetSeatIndex() < B.GetSeatIndex();
		});
	}

	/** PlayerState 소유 폰의 ASC. ASC 는 ABaamCharacter 가 들고 있다. */
	const UAbilitySystemComponent* GetSeatASC(const ABaamPlayerState* PS)
	{
		return PS ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PS->GetPawn()) : nullptr;
	}

	/** 살아 있는 좌석만 좌석 번호 순으로. 거리 계산의 기준 원이다. */
	void CollectAliveSeats(const APlayerController* Viewer, TArray<int32>& OutSeats)
	{
		OutSeats.Reset();

		TArray<ABaamPlayerState*> States;
		CollectSeatedPlayerStates(Viewer, States);

		for (const ABaamPlayerState* PS : States)
		{
			const UAbilitySystemComponent* ASC = GetSeatASC(PS);
			//	ASC 를 아직 못 찾았으면(폰 스폰 전) 살아 있는 것으로 본다.
			const bool bAlive = !ASC || ASC->GetNumericAttribute(UBaamAttributeSet::GetHealthAttribute()) > 0.f;
			if (bAlive)
			{
				OutSeats.Add(PS->GetSeatIndex());
			}
		}
	}
}

int32 UBaamSeatViewLibrary::GetRingDistance(const APlayerController* Viewer, int32 FromSeat, int32 ToSeat)
{
	if (FromSeat == INDEX_NONE || ToSeat == INDEX_NONE || FromSeat == ToSeat)
	{
		return INDEX_NONE;
	}

	//	⚠️ 거리는 "살아 있는 좌석" 의 원에서 센다. 사망자는 원에서 빠지므로
	//	   누군가 죽으면 모든 거리가 변한다. 월드 좌표로 계산하면 반드시 틀린다.
	TArray<int32> Alive;
	CollectAliveSeats(Viewer, Alive);

	const int32 i = Alive.IndexOfByKey(FromSeat);
	const int32 j = Alive.IndexOfByKey(ToSeat);
	if (i == INDEX_NONE || j == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	const int32 N = Alive.Num();
	const int32 Step = FMath::Abs(i - j);

	//	TODO(STEP 7): Scope(보는 쪽 -1) / Mustang(보이는 쪽 +1) 어트리뷰트 보정 후
	//	              FMath::Max(1, ...) 로 하한을 건다.
	return FMath::Min(Step, N - Step);
}

ABaamPlayerState* UBaamSeatViewLibrary::FindPlayerStateBySeat(const APlayerController* Viewer, int32 SeatIndex)
{
	if (SeatIndex == INDEX_NONE)
	{
		return nullptr;
	}

	TArray<ABaamPlayerState*> States;
	CollectSeatedPlayerStates(Viewer, States);

	for (ABaamPlayerState* PS : States)
	{
		if (PS->GetSeatIndex() == SeatIndex)
		{
			return PS;
		}
	}
	return nullptr;
}

TArray<FBangSeatView> UBaamSeatViewLibrary::MakeSeatViews(const APlayerController* Viewer)
{
	TArray<FBangSeatView> Views;

	TArray<ABaamPlayerState*> States;
	CollectSeatedPlayerStates(Viewer, States);
	if (States.IsEmpty())
	{
		return Views;
	}

	const ABaamPlayerState* ViewerState = Viewer ? Viewer->GetPlayerState<ABaamPlayerState>() : nullptr;
	const int32 ViewerSeat = ViewerState ? ViewerState->GetSeatIndex() : INDEX_NONE;

	const UWorld* World = Viewer ? Viewer->GetWorld() : nullptr;
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	const UBaamDataSubsystem* Data = GI ? GI->GetSubsystem<UBaamDataSubsystem>() : nullptr;

	Views.Reserve(States.Num());

	for (const ABaamPlayerState* PS : States)
	{
		FBangSeatView& View = Views.AddDefaulted_GetRef();
		View.SeatIndex      = PS->GetSeatIndex();
		View.PlayerName     = FText::FromString(PS->GetPlayerName());
		View.HandCount      = PS->GetHandCount();   //	장수만. 내용은 복제되지 않는다.
		View.bIsLocalPlayer = (PS == ViewerState);

		if (const UAbilitySystemComponent* ASC = GetSeatASC(PS))
		{
			View.Health    = FMath::RoundToInt(ASC->GetNumericAttribute(UBaamAttributeSet::GetHealthAttribute()));
			View.MaxHealth = FMath::RoundToInt(ASC->GetNumericAttribute(UBaamAttributeSet::GetMaxHealthAttribute()));
			View.bIsAlive  = View.Health > 0 && !ASC->HasMatchingGameplayTag(Bang::State::Dead.GetTag());
		}
		else
		{
			//	폰이 아직 스폰되지 않았다. 살아 있는 것으로 두고 HP 는 0/0 으로 표시한다.
			View.bIsAlive = true;
		}

		//	파란 카드(장비)는 공개 정보다.
		for (const FBaamCardInstance& Equip : PS->GetEquipment())
		{
			const FBaamCardRow* Row = Data ? Data->GetCardRow(Equip.CardId) : nullptr;
			View.EquipmentNames.Add(Row ? Row->DisplayName : FText::FromName(Equip.CardId));
		}

		//	TODO(STEP 6): 역할 공개 규칙. 보안관은 항상 공개, 나머지는 사망 시 공개.
		//	현재 ABaamCharacter::CharacterTag 가 전원에게 복제되고 있어 데이터상으로는 이미
		//	모두에게 보인다 — UI 에서만 가리는 것이라 진짜 은닉이 아니다.
		View.RoleName = FText::GetEmpty();

		View.Distance = GetRingDistance(Viewer, ViewerSeat, View.SeatIndex);

		//	TODO(STEP 8): GameState 의 현재 턴 좌석과 비교해 채운다.
		View.bIsCurrentTurn = false;
	}

	return Views;
}

TArray<int32> UBaamSeatViewLibrary::GetSelectableSeats(const APlayerController* Viewer, bool bExcludeSelf)
{
	TArray<int32> Seats;

	const ABaamPlayerState* ViewerState = Viewer ? Viewer->GetPlayerState<ABaamPlayerState>() : nullptr;
	const int32 ViewerSeat = ViewerState ? ViewerState->GetSeatIndex() : INDEX_NONE;

	CollectAliveSeats(Viewer, Seats);

	if (bExcludeSelf && ViewerSeat != INDEX_NONE)
	{
		Seats.Remove(ViewerSeat);
	}

	return Seats;
}
