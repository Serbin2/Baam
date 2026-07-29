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
#include "Game/BaamGameState.h"
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

	FString RoleDisplayName(const FGameplayTag& Role)
	{
		if (Role == Bang::Role::Sheriff.GetTag())  { return TEXT("보안관"); }
		if (Role == Bang::Role::Deputy.GetTag())   { return TEXT("부관"); }
		if (Role == Bang::Role::Outlaw.GetTag())   { return TEXT("무법자"); }
		if (Role == Bang::Role::Renegade.GetTag()) { return TEXT("배신자"); }
		return Role.IsValid() ? Role.ToString() : FString();
	}

	ABaamGameState* GetBaamGameState(const APlayerController* Viewer)
	{
		const UWorld* World = Viewer ? Viewer->GetWorld() : nullptr;
		return World ? World->GetGameState<ABaamGameState>() : nullptr;
	}

	/**
	 * 살아 있는 좌석만 좌석 번호 순으로. 거리 계산의 기준 원이다.
	 *
	 * ⚠️ "살아 있다" 의 판정은 ABaamGameState 한곳에만 둔다. 여기서 따로 계산하면
	 *    UI 는 살아 있다고 보는데 턴 진행은 건너뛰는 식으로 조용히 어긋난다.
	 */
	void CollectAliveSeats(const APlayerController* Viewer, TArray<int32>& OutSeats)
	{
		OutSeats.Reset();

		if (const ABaamGameState* GS = GetBaamGameState(Viewer))
		{
			OutSeats = GS->GetAliveSeatsInTableOrder();
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

	const ABaamGameState* GS = GetBaamGameState(Viewer);
	const int32 CurrentSeat = GS ? GS->GetCurrentSeat() : INDEX_NONE;

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
		}

		//	생사 판정도 GameState 한곳에서만 한다(턴 진행과 어긋나면 안 된다).
		View.bIsAlive = GS ? GS->IsSeatAlive(View.SeatIndex) : true;

		//	파란 카드(장비)는 공개 정보다.
		for (const FBaamCardInstance& Equip : PS->GetEquipment())
		{
			const FBaamCardRow* Row = Data ? Data->GetCardRow(Equip.CardId) : nullptr;
			View.EquipmentNames.Add(Row ? Row->DisplayName : FText::FromName(Equip.CardId));
		}

		//	⚠️ 반드시 PublicRoleTag 를 본다. GetSeatRole() 은 서버 권위 진짜 역할이라
		//	   리슨서버 호스트 화면에서는 전원의 역할이 그대로 노출된다.
		//	   공개 여부 판정은 서버가 이미 끝냈고(보안관 / 사망자 / 판 종료) UI 는 결과만 쓴다.
		FGameplayTag VisibleRole = PS->GetPublicRole();

		//	본인은 자기 역할을 안다 — CharacterTag 가 COND_OwnerOnly 로 자기에게만 온다.
		if (!VisibleRole.IsValid() && View.bIsLocalPlayer && GS)
		{
			VisibleRole = GS->GetSeatRole(View.SeatIndex);
		}

		View.RoleName = VisibleRole.IsValid()
			? FText::FromString(RoleDisplayName(VisibleRole))
			: FText::GetEmpty();

		View.Distance = GetRingDistance(Viewer, ViewerSeat, View.SeatIndex);

		View.bIsCurrentTurn = (CurrentSeat != INDEX_NONE && View.SeatIndex == CurrentSeat);
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
