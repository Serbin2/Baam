// BANG! 카드/덱 콘솔 테스트 트리거. Network/Session/BaamSessionExec.cpp 와 같은 관례:
//   선언은 헤더에 항상 남기고(UFUNCTION(Exec)), 본문만 #if !UE_BUILD_SHIPPING 으로 감싼다.
//
// 호스트 클래스 선택 메모 (Docs/Card-Implementation-Workflow.md §exec 표):
//   덱은 GameState 의 서버 전용 데이터다. GameInstance 에 두면 PIE 의 클라 창에서도 명령이
//   호출되어 "덱이 비어 있습니다" 만 찍히고 원인을 알 수 없다.
//   GameMode 에 두면 World->GetAuthGameMode() 가 클라에서 null 이라 명령이 해석조차 되지 않고
//   "Command not recognized" 가 떠서, 서버 창에서 쳐야 한다는 사실이 즉시 드러난다.

#include "Game/BaamGameMode.h"
#include "Player/BaamPlayerController.h"

#include "Game/BaamCardLog.h"
#include "Game/BaamCardType.h"
#include "Game/BaamDataSubsystem.h"
#include "Game/BaamGameDataTypes.h"
#include "Game/BaamGameState.h"
#include "Game/BaamPlayerState.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

// BaamGameDataTypes.h 는 UGameplayEffect 를 전방 선언만 한다.
// TSubclassOf<UGameplayEffect> 를 bool 로 평가하면 UGameplayEffect::StaticClass() 가 필요하므로
// 완전한 타입이 있어야 한다.
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Game/BaamGameplayTags.h"
#include "Player/Effect/GE_Damage.h"

// 덱 총 장수 검증 기준. 0 = 검사하지 않음. 목표 구성이 정해지면 켜서 쓴다.
static int32 GBangExpectedDeckSize = 0;
static FAutoConsoleVariableRef CVarBangExpectedDeckSize(
	TEXT("Bang.ExpectedDeckSize"),
	GBangExpectedDeckSize,
	TEXT("Baam_DumpDeck 이 검사할 덱 총 장수. 0 이면 검사하지 않는다."),
	ECVF_Default);

void ABaamGameMode::Baam_DumpDeck()
{
#if !UE_BUILD_SHIPPING
	// 기대 총 장수. 0 이면 검사하지 않는다.
	//   기본판 전사 시절에는 80 이 고정 검증값이었지만, BAAM 은 카드 구성이 다르므로(GDD §6)
	//   목표 장수를 정했을 때만 켜서 쓴다. `Bang.ExpectedDeckSize 64` 처럼 설정.
	const int32 ExpectedDeckSize = GBangExpectedDeckSize;

	const UGameInstance* GI = GetGameInstance();
	const UBaamDataSubsystem* Data = GI ? GI->GetSubsystem<UBaamDataSubsystem>() : nullptr;
	if (!Data)
	{
		UE_LOG(LogBaamCard, Warning, TEXT("[Exec] BaamDataSubsystem 없음"));
		return;
	}

	ABaamGameState* State = GetGameState<ABaamGameState>();
	if (!State)
	{
		UE_LOG(LogBaamCard, Warning, TEXT("[Exec] ABaamGameState 없음"));
		return;
	}

	// Deck 은 Initialize() 에서 한 번 만들어 캐시된다. 여기서 다시 만들지 않으므로
	// 두 번 연속 실행하면 항상 같은 결과가 나와야 한다.
	TArray<FBaamCardInstance> Deck;
	State->GetDeck(Deck);
	if (Deck.IsEmpty())
	{
		UE_LOG(LogBaamCard, Warning,
			TEXT("[Exec] 덱이 비어 있습니다. DT_BaamCard 의 QuantityOfCard 가 전부 0 이거나 "
			     "프로젝트 설정(Baam Data ▸ CardTable)이 비어 있습니다."));
		return;
	}

	// 종류별 집계 + InstanceId 중복 검사를 한 번에.
	TMap<FName, int32> Counts;
	TSet<int32> SeenIds;
	int32 NumDuplicateIds = 0;

	Counts.Reserve(Deck.Num());
	SeenIds.Reserve(Deck.Num());

	for (const FBaamCardInstance& Card : Deck)
	{
		++Counts.FindOrAdd(Card.CardId);

		bool bAlreadySeen = false;
		SeenIds.Add(Card.InstanceId, &bAlreadySeen);
		if (bAlreadySeen)
		{
			++NumDuplicateIds;
		}
	}

	Counts.KeySort([](const FName& A, const FName& B) { return A.Compare(B) < 0; });

	UE_LOG(LogBaamCard, Log, TEXT("===== Baam_DumpDeck ====="));

	int32 NumUnimplemented = 0;
	for (const TPair<FName, int32>& Pair : Counts)
	{
		// GA(AbilityEventTag) 도 GE(EquipEffect) 도 없는 카드가 덱에 들어가 있으면
		// 손패에는 나오지만 사용해도 아무 일이 일어나지 않는다. 구현 진행도 추적용.
		const FBaamCardRow* Row = Data->GetCardRow(Pair.Key);
		const bool bUnimplemented = (Row != nullptr) && !Row->AbilityEventTag.IsValid() && !Row->EquipEffect;
		if (bUnimplemented)
		{
			++NumUnimplemented;
		}

		UE_LOG(LogBaamCard, Log, TEXT("  %-32s %3d 장%s"),
			*Pair.Key.ToString(),
			Pair.Value,
			(Row == nullptr)  ? TEXT("   [!! DT_BaamCard 에 Row 없음]") :
			bUnimplemented    ? TEXT("   [미구현: GA/GE 미지정]") :
			                    TEXT(""));
	}

	if (ExpectedDeckSize > 0)
	{
		UE_LOG(LogBaamCard, Log, TEXT("총 %d 종류 / %d 장   (기대 %d : %s)"),
			Counts.Num(), Deck.Num(), ExpectedDeckSize,
			(Deck.Num() == ExpectedDeckSize) ? TEXT("일치") : TEXT("불일치 ← QuantityOfCard 확인"));
	}
	else
	{
		UE_LOG(LogBaamCard, Log, TEXT("총 %d 종류 / %d 장   (기대 장수 미설정 — Bang.ExpectedDeckSize 로 켤 수 있음)"),
			Counts.Num(), Deck.Num());
	}

	if (NumUnimplemented > 0)
	{
		UE_LOG(LogBaamCard, Warning,
			TEXT("GA/GE 미지정 카드 %d 종이 덱에 포함되어 있습니다 (사용해도 효과 없음). "
			     "구현 전까지 QuantityOfCard 를 0 으로 두면 구현된 카드만으로 테스트할 수 있습니다."),
			NumUnimplemented);
	}

	if (NumDuplicateIds > 0)
	{
		// 중복되면 손패에서 카드를 제거할 때 엉뚱한 카드가 지워진다. 반드시 잡고 넘어갈 것.
		UE_LOG(LogBaamCard, Error,
			TEXT("InstanceId 중복 %d 건 — BuildDeck 의 NextInstanceId 카운터를 확인하세요."),
			NumDuplicateIds);
	}
#endif // !UE_BUILD_SHIPPING
}

void ABaamGameMode::Baam_PrintDeck()
{
#if !UE_BUILD_SHIPPING

	ABaamGameState* State = GetGameState<ABaamGameState>();
	if (!State)
	{
		UE_LOG(LogBaamCard, Warning, TEXT("[Exec] ABaamGameState 없음"));
		return;
	}

	// 셔플된 실제 순서. DrawFromDeck 이 0 번부터 뽑으므로 이 출력의 위에서부터 나온다.
	TArray<FBaamCardInstance> Deck;
	State->GetDeck(Deck);
	if (Deck.IsEmpty())
	{
		UE_LOG(LogBaamCard, Warning,
			TEXT("[Exec] 덱이 비어 있습니다. DT_BaamCard 의 QuantityOfCard 가 전부 0 이거나 "
				 "프로젝트 설정(Baam Data ▸ CardTable)이 비어 있습니다."));
		return;
	}
	
	for (const FBaamCardInstance& Card : Deck)
	{
		FName name = Card.CardId;
		FString names = name.ToString();
		int32 num = Card.InstanceId;
		UE_LOG(LogBaamCard, Warning, TEXT("[Exec] DeckList Number : %d, Name : %s"), num, *names);
	}
#endif // !UE_BUILD_SHIPPING
}

// ======================================================================================
//  2.7 — 손패 배분 / 조회
//
//  두 명령의 호스트 클래스가 다른 것이 요점이다:
//    Baam_DealHand : 덱을 소비하는 서버 권위 조작 → GameMode (클라 창에서는 해석되지 않음)
//    Baam_DumpHand : 창마다 자기 손패를 확인 → PlayerController (각 클라에서 따로 실행)
// ======================================================================================

void ABaamGameMode::Baam_DealHand(int32 Count)
{
#if !UE_BUILD_SHIPPING
	//	exec 는 생략된 뒤쪽 인자를 0 으로 넘긴다. C++ 기본값에 기대지 말 것.
	if (Count <= 0)
	{
		Count = 5;
	}

	ABaamGameState* State = GetGameState<ABaamGameState>();
	if (!State)
	{
		UE_LOG(LogBaamCard, Warning, TEXT("[Exec] ABaamGameState 없음"));
		return;
	}

	TArray<APlayerController*> Players;
	CollectPlayers(Players);
	if (Players.IsEmpty())
	{
		UE_LOG(LogBaamCard, Warning, TEXT("[Exec] 접속한 플레이어가 없습니다."));
		return;
	}

	UE_LOG(LogBaamCard, Log, TEXT("===== Baam_DealHand (%d 명 × %d 장) ====="), Players.Num(), Count);

	for (APlayerController* PC : Players)
	{
		ABaamPlayerState* PS = PC ? PC->GetPlayerState<ABaamPlayerState>() : nullptr;
		if (!PS)
		{
			UE_LOG(LogBaamCard, Warning, TEXT("  %s : ABaamPlayerState 없음 — 스킵"),
				PC ? *PC->GetName() : TEXT("null"));
			continue;
		}

		TArray<FString> Dealt;
		Dealt.Reserve(Count);

		for (int32 i = 0; i < Count; ++i)
		{
			FBaamCardInstance Card;
			if (!State->DrawFromDeck(Card))
			{
				UE_LOG(LogBaamCard, Warning, TEXT("  덱 소진 — %d 장에서 중단"), Dealt.Num());
				break;
			}

			//	손패 변경은 반드시 이 함수를 거친다 (HandCount 동기화가 여기에 있다).
			PS->AddCardToHand(Card);
			Dealt.Add(FString::Printf(TEXT("%s#%d"), *Card.CardId.ToString(), Card.InstanceId));
		}

		UE_LOG(LogBaamCard, Log, TEXT("  좌석 %d (%s) → %d 장 : %s"),
			PS->GetSeatIndex(), *PC->GetName(), Dealt.Num(), *FString::Join(Dealt, TEXT(", ")));
	}

	UE_LOG(LogBaamCard, Log, TEXT("배분 후 — 덱 %d 장 / 버린 패 %d 장"),
		State->GetDeckCount(), State->GetDiscardCount());
#endif // !UE_BUILD_SHIPPING
}

void ABaamPlayerController::Baam_DumpHand()
{
#if !UE_BUILD_SHIPPING
	const ABaamPlayerState* PS = GetPlayerState<ABaamPlayerState>();
	if (!PS)
	{
		UE_LOG(LogBaamCard, Warning, TEXT("[Exec] ABaamPlayerState 없음"));
		return;
	}

	const bool bIsServer = HasAuthority();
	const TArray<FBaamCardInstance>& Hand = PS->GetHand();

	UE_LOG(LogBaamCard, Log, TEXT("===== Baam_DumpHand — 좌석 %d (%s) ====="),
		PS->GetSeatIndex(), bIsServer ? TEXT("서버") : TEXT("클라"));

	for (const FBaamCardInstance& Card : Hand)
	{
		UE_LOG(LogBaamCard, Log, TEXT("  #%-3d %s"), Card.InstanceId, *Card.CardId.ToString());
	}

	//	Hand.Num() 과 HandCount 가 어긋나면 손패를 Add/RemoveCardFromHand 를 거치지 않고 건드린 것이다.
	UE_LOG(LogBaamCard, Log, TEXT("내 손패 %d 장 (HandCount=%d%s)"),
		Hand.Num(), PS->GetHandCount(),
		(Hand.Num() == PS->GetHandCount()) ? TEXT("") : TEXT(" ← 불일치!"));

	const ABaamGameState* GS = GetWorld() ? GetWorld()->GetGameState<ABaamGameState>() : nullptr;
	if (!GS)
	{
		return;
	}

	//	이 값이 클라에서 0 이면 GetLifetimeReplicatedProps 등록이 빠진 것이다.
	UE_LOG(LogBaamCard, Log, TEXT("덱 %d 장 / 버린 패 %d 장"),
		GS->GetDeckCount(), GS->GetDiscardCount());

	//	── 은닉 정보 검증 ──
	//	클라에서는 남의 Hand 배열이 반드시 비어 있어야 한다(COND_OwnerOnly).
	//	서버(리슨 호스트)는 모든 PlayerState 를 그대로 들고 있으므로 차 있는 것이 정상이다.
	for (APlayerState* Other : GS->PlayerArray)
	{
		const ABaamPlayerState* OtherPS = Cast<ABaamPlayerState>(Other);
		if (!OtherPS || OtherPS == PS)
		{
			continue;
		}

		const int32 VisibleCards = OtherPS->GetHand().Num();
		const bool bLeak = !bIsServer && VisibleCards > 0;

		UE_LOG(LogBaamCard, Log, TEXT("  타 좌석 %d : 공개 장수 %d, 내용 %d 개 보임%s"),
			OtherPS->GetSeatIndex(), OtherPS->GetHandCount(), VisibleCards,
			bLeak ? TEXT("  ← 은닉 정보 누출! COND_OwnerOnly 확인") :
			bIsServer ? TEXT("  (서버라 정상)") : TEXT(""));
	}
#endif // !UE_BUILD_SHIPPING
}

// ======================================================================================
//  턴 — 조회는 창마다 자기 기준이므로 PlayerController 에 둔다.
// ======================================================================================

void ABaamPlayerController::Baam_EndTurn()
{
#if !UE_BUILD_SHIPPING
	RequestEndTurn();
#endif
}

void ABaamPlayerController::Baam_DumpTurn()
{
#if !UE_BUILD_SHIPPING
	const ABaamGameState* GS = GetWorld() ? GetWorld()->GetGameState<ABaamGameState>() : nullptr;
	if (!GS)
	{
		UE_LOG(LogBaamCard, Warning, TEXT("[Exec] ABaamGameState 없음"));
		return;
	}

	const int32 MySeat = GetMySeatIndex();

	UE_LOG(LogBaamCard, Log, TEXT("===== Baam_DumpTurn ====="));
	UE_LOG(LogBaamCard, Log, TEXT("  현재 턴 좌석 : %d   페이즈 : %s"),
		GS->GetCurrentSeat(), *GS->GetPhaseTag().ToString());
	UE_LOG(LogBaamCard, Log, TEXT("  내 좌석 : %d   낼 수 있나 : %s   버려야 하나 : %s"),
		MySeat,
		IsMyTurnToPlay()    ? TEXT("예") : TEXT("아니오"),
		IsMyTurnToDiscard() ? TEXT("예") : TEXT("아니오"));

	if (const ABaamPlayerState* PS = GetPlayerState<ABaamPlayerState>())
	{
		UE_LOG(LogBaamCard, Log, TEXT("  손패 %d 장 / 한도 %d (한도 = 현재 Health)"),
			PS->GetHandCount(), GS->GetHandLimitForSeat(MySeat));
		UE_LOG(LogBaamCard, Log, TEXT("  카드 사용 %d / %d  (남음 : %s)"),
			PS->GetCardsUsedThisTurn(), GS->GetCardUseLimitForSeat(MySeat),
			GS->HasCardUsesLeft(MySeat) ? TEXT("예") : TEXT("아니오"));
	}

	const TArray<int32> Alive = GS->GetAliveSeatsInTableOrder();
	FString AliveStr;
	for (const int32 Seat : Alive)
	{
		AliveStr += FString::Printf(TEXT("%d(HP%d) "), Seat, GS->GetSeatHealth(Seat));
	}
	UE_LOG(LogBaamCard, Log, TEXT("  살아있는 좌석 : %s"), AliveStr.IsEmpty() ? TEXT("(없음)") : *AliveStr);
#endif
}

// ======================================================================================
//  사망 / 승패 테스트 — 서버 권위이므로 GameMode 에 둔다.
// ======================================================================================

void ABaamGameMode::Baam_Damage(int32 Seat, int32 Amount)
{
#if !UE_BUILD_SHIPPING
	ABaamGameState* GS = GetGameState<ABaamGameState>();
	if (!GS)
	{
		UE_LOG(LogBaamCard, Warning, TEXT("[Exec] ABaamGameState 없음"));
		return;
	}

	const ABaamPlayerState* PS = GS->GetPlayerStateBySeat(Seat);
	UAbilitySystemComponent* ASC =
		PS ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PS->GetPawn()) : nullptr;
	if (!ASC)
	{
		UE_LOG(LogBaamCard, Warning, TEXT("[Exec] 좌석 %d 의 ASC 를 찾지 못했습니다."), Seat);
		return;
	}

	// exec 는 생략된 인자를 0 으로 넘긴다. 0 이면 즉사시킨다(승패 판정 테스트 편의).
	const int32 Damage = (Amount > 0) ? Amount : FMath::Max(1, GS->GetSeatHealth(Seat));

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UGE_Damage::StaticClass(), 1.f, Context);
	if (Spec.IsValid())
	{
		// 피해는 음수로 넣는다(Health 가산 = 감소).
		Spec.Data->SetSetByCallerMagnitude(Bang::SetByCaller::Damage.GetTag(), -static_cast<float>(Damage));
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}

	UE_LOG(LogBaamCard, Log, TEXT("[Exec] 좌석 %d 에 피해 %d — 남은 HP %d"),
		Seat, Damage, GS->GetSeatHealth(Seat));
#endif
}
