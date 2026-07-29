#include "Game/BaamCardViewLibrary.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Game/BaamCardLog.h"
#include "Game/BaamCardType.h"
#include "Game/BaamDataSubsystem.h"
#include "Game/BaamGameDataTypes.h"
#include "Game/BaamGameplayTags.h"
#include "Game/BaamGameState.h"
#include "Game/BaamPlayerState.h"

namespace
{
	const UBaamDataSubsystem* GetDataSubsystem(const UObject* WorldContextObject)
	{
		const UWorld* World = GEngine
			? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
			: nullptr;
		const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		return GI ? GI->GetSubsystem<UBaamDataSubsystem>() : nullptr;
	}
}

FBangCardView UBaamCardViewLibrary::MakeCardView(const UObject* WorldContextObject, const FBaamCardInstance& Card)
{
	FBangCardView View;
	View.InstanceId = Card.InstanceId;
	View.CardId     = Card.CardId;

	const UBaamDataSubsystem* Data = GetDataSubsystem(WorldContextObject);
	const FBaamCardRow* Row = Data ? Data->GetCardRow(Card.CardId) : nullptr;

	if (!Row)
	{
		// 조용히 빈 카드가 되면 "왜 이름이 안 보이지" 로 시간을 버린다. 화면에 원인을 띄운다.
		View.DisplayName = FText::FromString(FString::Printf(TEXT("%s (Row 없음)"), *Card.CardId.ToString()));
		View.Description = FText::FromString(TEXT("DT_BaamCard 에 이 CardId 의 Row 가 없습니다."));
		View.bPlayable   = false;
		UE_LOG(LogBaamCard, Warning, TEXT("MakeCardView: '%s' Row 를 찾지 못했습니다."), *Card.CardId.ToString());
		return View;
	}

	View.DisplayName  = Row->DisplayName;
	View.Description  = Row->Description;
	View.bNeedsTarget = Row->TraitTags.HasTag(Bang::Card::Trait::NeedsTargetSeat.GetTag());

	// TODO(5.x): 사거리·턴당 BANG 한도·현재 페이즈를 반영한 실제 판정으로 교체.
	//            어디까지나 표시용 힌트이고, 최종 판정은 언제나 서버가 한다.
	View.bPlayable = true;

	return View;
}

TArray<FBangCardView> UBaamCardViewLibrary::MakeHandViews(const ABaamPlayerState* PlayerState)
{
	TArray<FBangCardView> Views;
	if (!PlayerState)
	{
		return Views;
	}

	const TArray<FBaamCardInstance>& Hand = PlayerState->GetHand();
	Views.Reserve(Hand.Num());

	//	자기 차례가 아니면 손패를 회색으로 표시한다(플레이 존이 드롭도 거부한다).
	//	Discard 페이즈에서도 조작이 필요하므로 두 페이즈 모두 허용한다.
	//	어디까지나 표시용 힌트이고 최종 판정은 서버가 다시 한다.
	const UWorld* World = PlayerState->GetWorld();
	const ABaamGameState* GS = World ? World->GetGameState<ABaamGameState>() : nullptr;
	const int32 Seat = PlayerState->GetSeatIndex();
	const bool bCanAct = GS && (GS->CanSeatPlayCards(Seat) || GS->CanSeatDiscard(Seat));

	for (const FBaamCardInstance& Card : Hand)
	{
		FBangCardView& View = Views.Add_GetRef(MakeCardView(PlayerState, Card));
		View.bPlayable = View.bPlayable && bCanAct;
	}

	return Views;
}
