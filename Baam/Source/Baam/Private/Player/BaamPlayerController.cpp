#include "Player/BaamPlayerController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GameplayAbilityTypes.h"   // FGameplayEventData

#include "Game/BaamCardLog.h"
#include "Game/BaamCardType.h"                // FBaamCardInstance
#include "Game/BaamDebug.h"
#include "Game/BaamCardViewLibrary.h"
#include "Game/BaamDataSubsystem.h"           // GetCardRow
#include "Game/BaamGameDataTypes.h"           // FBaamCardRow
#include "Game/BaamGameState.h"               // PushToDiscard
#include "Game/BaamPlayerState.h"
#include "Game/BaamSeatViewLibrary.h"
#include "GameFramework/GameStateBase.h"      // PlayerArray
#include "UI/BangHandWidget.h"
#include "UI/BangSeatBoardWidget.h"
#include "Components/InputComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

ABaamPlayerController::ABaamPlayerController()
{
	// GA 는 AbilityByCardId(BP 디폴트에서 채움)로 조회한다. 하드코딩 폴백은 없앴다
	// (엉뚱한 GA 로 조용히 새는 버그 방지).
}

// ======================================================================================
//  손패 UI 배선
//
//  위젯 생성과 PlayerState 복제 도착은 순서가 보장되지 않는다.
//  클라에서는 보통 위젯이 먼저 만들어지고 PlayerState 가 나중에 오지만, 리슨서버 호스트는
//  반대다. 그래서 양쪽 진입점에서 모두 TryBindHandDelegate() 를 부른다.
// ======================================================================================

void ABaamPlayerController::SetHandWidget(UBangHandWidget* InHandWidget)
{
	//	위젯이 갈아끼워지면 이전 구독을 정리한다. RefreshHandWidget 은 자주 불리므로
	//	바인딩은 여기서 한 번만 한다 — 거기서 하면 갱신할 때마다 중복 구독된다.
	if (HandWidget && HandWidget != InHandWidget)
	{
		HandWidget->OnCardPlayRequested.RemoveDynamic(this, &ABaamPlayerController::HandleCardPlayRequested);
	}

	HandWidget = InHandWidget;

	if (HandWidget)
	{
		HandWidget->OnCardPlayRequested.AddDynamic(this, &ABaamPlayerController::HandleCardPlayRequested);
	}

	TryBindHandDelegate();
	RefreshHandWidget();
}

void ABaamPlayerController::SetHandInteractionLocked(bool bLocked)
{
	if (HandWidget)
	{
		HandWidget->SetInteractionLocked(bLocked);
	}
}

void ABaamPlayerController::HandleCardPlayRequested(const FBangCardView& Card)
{
	if (!Card.IsValid())
	{
		return;
	}

	//	이미 다른 카드가 대상 선택 중이면 무시한다.
	//	손패 잠금이 정상 동작하면 여기까지 오지 않지만, 잠금이 걸리기 전의 입력이
	//	큐에 남아 있을 수 있어 한 번 더 막는다. 안 그러면 ContextId 가 뒤엉킨다.
	if (SeatBoardWidget && SeatBoardWidget->IsSelectingTarget())
	{
		UE_LOG(LogBaamCard, Verbose, TEXT("[PC] 대상 선택 진행 중 — 카드 %d 요청 무시"), Card.InstanceId);
		return;
	}

	//	Discard 페이즈에서는 중앙 드롭이 "사용" 이 아니라 "버리기" 다.
	//	기존 드래그 UI 를 그대로 재사용하므로 버리기 전용 위젯이 필요 없다.
	if (IsMyTurnToDiscard())
	{
		ServerRequestDiscardCard(Card.InstanceId);
		return;
	}

	if (!Card.bNeedsTarget)
	{
		//	대상이 필요 없는 카드(맥주, 역마차 등) — 바로 서버로.
		RequestPlayCard(Card.InstanceId, INDEX_NONE);
		return;
	}

	//	대상이 필요한 카드 — 좌석 선택으로 넘긴다.
	//	잠금을 먼저 걸어야 선택 대기 중 두 번째 카드를 던지지 못한다.
	SetHandInteractionLocked(true);
	BeginTargetSelectionForCard(Card.InstanceId);

	//	보드가 없거나 고를 좌석이 없어 선택이 즉시 끝난 경우 잠금이 남지 않게 한다.
	//	(즉시 취소된 경우엔 취소 핸들러가 이미 풀었으므로 여기서는 no-op)
	if (!SeatBoardWidget || !SeatBoardWidget->IsSelectingTarget())
	{
		SetHandInteractionLocked(false);
	}
}

void ABaamPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent)
	{
		return;
	}

	//	대상 선택 취소. CancelTargetSelection 은 선택 중이 아니면 아무것도 하지 않으므로
	//	평소 우클릭에는 영향이 없다.
	//	⚠️ PIE 에서는 ESC 를 에디터가 먼저 먹어 세션이 종료된다 — 테스트는 우클릭으로 할 것.
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ABaamPlayerController::CancelTargetSelection);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ABaamPlayerController::CancelTargetSelection);
}

void ABaamPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// 클라: PlayerState 가 이제야 도착했다. 여기서 구독하지 않으면 첫 손패를 놓친다.
	TryBindHandDelegate();
	RefreshHandWidget();
}

void ABaamPlayerController::TryBindHandDelegate()
{
	ABaamPlayerState* PS = GetPlayerState<ABaamPlayerState>();
	if (!PS || BoundPlayerState.Get() == PS)
	{
		return;   // 아직 없거나 이미 같은 PlayerState 에 구독 중
	}

	// PlayerState 가 갈아끼워진 경우(재접속 등) 이전 구독을 정리한다.
	if (ABaamPlayerState* Previous = BoundPlayerState.Get())
	{
		Previous->OnHandChanged.RemoveDynamic(this, &ABaamPlayerController::HandleHandChanged);
	}

	PS->OnHandChanged.AddDynamic(this, &ABaamPlayerController::HandleHandChanged);
	BoundPlayerState = PS;

	UE_LOG(LogBaamCard, Verbose, TEXT("[PC] 손패 델리게이트 구독 — 좌석 %d"), PS->GetSeatIndex());
}

void ABaamPlayerController::HandleHandChanged()
{
	RefreshHandWidget();
}

void ABaamPlayerController::RefreshHandWidget()
{
	if (!HandWidget)
	{
		return;   // HUD 가 아직 SetHandWidget 을 부르지 않았다. 부르면 그때 다시 그린다.
	}

	const ABaamPlayerState* PS = GetPlayerState<ABaamPlayerState>();
	if (!PS)
	{
		HandWidget->ClearHand();
		return;
	}

	// 게임 로직 타입 → UI 뷰모델. 이 한 줄이 로직/UI 경계다.
	// OnCardPlayRequested 구독은 SetHandWidget 에서 한 번만 한다(여기서 하면 중복 구독).
	HandWidget->SetHand(UBaamCardViewLibrary::MakeHandViews(PS));
}

// ======================================================================================
//  좌석 보드 UI 배선
//
//  좌석 정보(HP / 손패 장수 / 장비 / 생사)는 바뀌는 경로가 여럿이라 단일 델리게이트가 없다.
//  프로토타입에서는 타이머로 주기 갱신한다. SetSeats 가 좌석 번호로 위젯을 재사용하므로
//  갱신 중에도 마우스 호버나 대상 선택 상태가 끊기지 않는다.
//  TODO: 어트리뷰트 변경 델리게이트 + PlayerArray 변경 훅으로 교체하면 폴링을 없앨 수 있다.
// ======================================================================================

void ABaamPlayerController::SetSeatBoardWidget(UBangSeatBoardWidget* InSeatBoard)
{
	//	위젯이 갈아끼워지면 이전 구독을 정리한다.
	if (SeatBoardWidget && SeatBoardWidget != InSeatBoard)
	{
		SeatBoardWidget->OnSeatSelected.RemoveDynamic(this, &ABaamPlayerController::HandleSeatSelected);
		SeatBoardWidget->OnTargetSelectionCancelled.RemoveDynamic(this, &ABaamPlayerController::HandleTargetSelectionCancelled);
	}

	SeatBoardWidget = InSeatBoard;

	if (SeatBoardWidget)
	{
		SeatBoardWidget->OnSeatSelected.AddDynamic(this, &ABaamPlayerController::HandleSeatSelected);
		SeatBoardWidget->OnTargetSelectionCancelled.AddDynamic(this, &ABaamPlayerController::HandleTargetSelectionCancelled);
	}

	RefreshSeatBoard();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SeatRefreshTimer);
		if (SeatBoardWidget && SeatBoardRefreshInterval > 0.f)
		{
			World->GetTimerManager().SetTimer(SeatRefreshTimer, this,
				&ABaamPlayerController::RefreshSeatBoard, SeatBoardRefreshInterval, /*bLoop=*/true);
		}
	}
}

void ABaamPlayerController::RefreshSeatBoard()
{
	if (!SeatBoardWidget)
	{
		return;
	}
	SeatBoardWidget->SetSeats(UBaamSeatViewLibrary::MakeSeatViews(this));
}

// ======================================================================================
//  대상 좌석 선택
// ======================================================================================

void ABaamPlayerController::BeginTargetSelection(int32 ContextId, const FText& Prompt, const TArray<int32>& SelectableSeats)
{
	if (!SeatBoardWidget)
	{
		UE_LOG(LogBaamCard, Warning, TEXT("[PC] 좌석 보드가 없어 대상 선택을 시작할 수 없습니다."));
		return;
	}

	FBangTargetRequest Request;
	Request.ContextId        = ContextId;
	Request.Prompt           = Prompt;
	Request.SelectableSeats  = SelectableSeats;
	Request.bAllowCancel     = true;

	SeatBoardWidget->BeginTargetSelection(Request);
}

void ABaamPlayerController::BeginTargetSelectionForCard(int32 CardInstanceId)
{
	//	사거리 판정은 아직 없다 — 살아 있는 다른 좌석 전부를 후보로 연다(STEP 7/8 에서 좁힌다).
	BeginTargetSelection(
		CardInstanceId,
		NSLOCTEXT("Bang", "SelectTargetSeat", "대상을 선택하세요"),
		UBaamSeatViewLibrary::GetSelectableSeats(this, /*bExcludeSelf=*/true));
}

void ABaamPlayerController::CancelTargetSelection()
{
	if (SeatBoardWidget)
	{
		SeatBoardWidget->CancelTargetSelection();
	}
}

void ABaamPlayerController::HandleSeatSelected(int32 SeatIndex, int32 ContextId)
{
	UE_LOG(LogBaamCard, Log, TEXT("[PC] 대상 좌석 확정 — Seat=%d Context(카드 InstanceId)=%d"),
		SeatIndex, ContextId);

	SetHandInteractionLocked(false);

	//	기본 동작: ContextId 를 카드 InstanceId 로 보고 서버에 사용 요청을 보낸다.
	//	다른 용도로 쓰려면 이 델리게이트를 구독해 분기하면 된다.
	RequestPlayCard(ContextId, SeatIndex);

	OnTargetSeatSelected.Broadcast(SeatIndex, ContextId);
}

void ABaamPlayerController::HandleTargetSelectionCancelled(int32 ContextId)
{
	UE_LOG(LogBaamCard, Verbose, TEXT("[PC] 대상 선택 취소 — Context=%d"), ContextId);

	//	취소되면 카드는 손패에 그대로 남는다(서버에 보내지 않았다). 다시 집을 수 있게 푼다.
	SetHandInteractionLocked(false);

	OnTargetSelectionCancelled.Broadcast(ContextId);
}

void ABaamPlayerController::RequestPlayCard(int32 InstanceId, int32 TargetSeat)
{
	// 클라(요청 발신) 확인용. 이 줄이 안 뜨면 UI 바인딩이 RequestPlayCard 를 안 부르는 것.
	BaamDebug::Screen(
		FString::Printf(TEXT("[클라] 카드요청 보냄  Instance=%d  TargetSeat=%d  (→ 서버)"),
			InstanceId, TargetSeat),
		FColor(180, 180, 255), /*Time=*/5.f);

	// 로컬에서는 검증하지 않는다 (클라 표시는 힌트일 뿐, 권위는 서버). 그대로 중계.
	ServerRequestPlayCard(InstanceId, TargetSeat);
}

void ABaamPlayerController::RequestPlayCardByTag(FGameplayTag CardId, int32 TargetSeat)
{
	// 손패에서 그 종류의 첫 장을 찾아 InstanceId 로 넘긴다 (테스트 편의 함수).
	const ABaamPlayerState* PS = GetPlayerState<ABaamPlayerState>();
	if (!PS || !CardId.IsValid())
	{
		return;
	}

	for (const FBaamCardInstance& Card : PS->GetHand())
	{
		if (Card.CardId == CardId.GetTagName())
		{
			RequestPlayCard(Card.InstanceId, TargetSeat);
			return;
		}
	}

	BaamDebug::Screen(
		FString::Printf(TEXT("[클라] 손패에 %s 가 없습니다 — 요청 취소."), *CardId.ToString()),
		FColor::Red, /*Time=*/5.f);
}

bool ABaamPlayerController::ServerRequestPlayCard_Validate(int32 InstanceId, int32 TargetSeat)
{
	// 형식 검증만 (악의적 페이로드 차단). 규칙 검증은 _Implementation 에서.
	return InstanceId != INDEX_NONE;
}

void ABaamPlayerController::ServerRequestPlayCard_Implementation(int32 InstanceId, int32 TargetSeat)
{
	HandlePlayCard(InstanceId, TargetSeat);
}

// 턴 검증 우회 — GA 를 단독으로 테스트할 때만 켠다. 0 이면 정상 규칙 적용.
static int32 GBangIgnoreTurnOrder = 0;
static FAutoConsoleVariableRef CVarBangIgnoreTurnOrder(
	TEXT("Bang.IgnoreTurnOrder"),
	GBangIgnoreTurnOrder,
	TEXT("1 이면 턴/페이즈 검증을 건너뛴다(GA 단독 테스트용). 기본 0."),
	ECVF_Default);

void ABaamPlayerController::HandlePlayCard(int32 InstanceId, int32 TargetSeat)
{
	UE_LOG(LogTemp, Log, TEXT("[Bang] ServerRequestPlayCard 수신 — PC=%s Instance=%d TargetSeat=%d"),
		*GetName(), InstanceId, TargetSeat);

	// ── 턴/페이즈 검증 ──
	// 클라의 UI 표시는 힌트일 뿐이므로 서버가 반드시 다시 본다.
	// TODO: 사거리(WeaponRange vs 거리)·턴당 BANG 한도(BangLimit)는 아직 없다.
	{
		const ABaamGameState* GS = GetWorld() ? GetWorld()->GetGameState<ABaamGameState>() : nullptr;
		const int32 MySeat = GetMySeatIndex();

		if (GBangIgnoreTurnOrder == 0 && (!GS || !GS->CanSeatPlayCards(MySeat)))
		{
			BaamDebug::Screen(
				FString::Printf(TEXT("[서버] 지금은 좌석 %d 의 차례가 아닙니다 (현재 %d / %s). 카드 #%d 거부."),
					MySeat,
					GS ? GS->GetCurrentSeat() : INDEX_NONE,
					GS ? *GS->GetPhaseTag().ToString() : TEXT("-"),
					InstanceId),
				FColor::Orange, /*Time=*/5.f);
			return;
		}
	}

	// 시전자의 ASC 를 얻는다 (현재 ASC 는 캐릭터 소유).
	UAbilitySystemComponent* ASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetPawn());
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bang] HandlePlayCard: 시전자 ASC 없음 (Pawn=%s)."),
			GetPawn() ? *GetPawn()->GetName() : TEXT("null"));
		return;
	}

	// ── InstanceId → 손패에서 카드 조회 → 카드 종류 태그 + 메커니즘 태그 ──
	// TODO(2단계): 손패에 없으면 거부는 이미 하고 있고, 실제 사용 시
	//              RemoveCardFromHand 로 그 인스턴스를 빼는 처리가 남았다.
	FGameplayTag CardId;    // 맵 조회 키 (Card.Id.*)
	FGameplayTag EventTag;  // 페이로드/로그용 메커니즘 태그 (Ability.*)
	if (const ABaamPlayerState* PS = GetPlayerState<ABaamPlayerState>())
	{
		const UGameInstance* GI = GetGameInstance();
		const UBaamDataSubsystem* Data = GI ? GI->GetSubsystem<UBaamDataSubsystem>() : nullptr;

		for (const FBaamCardInstance& Card : PS->GetHand())
		{
			if (Card.InstanceId == InstanceId)
			{
				if (const FBaamCardRow* Row = Data ? Data->GetCardRow(Card.CardId) : nullptr)
				{
					CardId   = Row->CardIdTag;
					EventTag = Row->AbilityEventTag;
				}
				break;
			}
		}
	}

	if (!CardId.IsValid())
	{
		BaamDebug::Screen(
			FString::Printf(TEXT("[서버] 카드 Instance=%d 를 손패에서 못 찾음(또는 Row 의 CardIdTag 비어있음)."),
				InstanceId),
			FColor::Red, 8.f);
		return;
	}

	// ── CardId 태그 → GA 클래스 (맵을 "카드 종류"로 직접 조회) ──
	//  ⚠️ 맵(AbilityByCardId)의 키는 카드 태그(Card.Id.*)다 — 유저 설정에 맞춰 카드 기준 조회.
	//     (메커니즘 태그 Ability.* 로 거치지 않는다.)
	const TSubclassOf<UGameplayAbility>* Found = AbilityByCardId.Find(CardId);
	if (!Found || !*Found)
	{
		// 진단: 런타임에 맵에 실제로 뭐가 들었는지 그대로 찍는다.
		//   맵(0) 비어있음   → 빌드/BP 미반영
		//   키는 있는데 값 None → BP 에서 GA 클래스를 안 골랐음
		//   키가 다름          → 이 카드 태그가 맵에 없음
		FString Dump;
		for (const TPair<FGameplayTag, TSubclassOf<UGameplayAbility>>& Pair : AbilityByCardId)
		{
			Dump += FString::Printf(TEXT("%s=%s; "),
				*Pair.Key.ToString(),
				Pair.Value ? *Pair.Value->GetName() : TEXT("None"));
		}
		BaamDebug::Screen(
			FString::Printf(TEXT("[서버] GA 매핑 없음! 카드 %s 를 맵에서 못 찾음.  현재 맵(%d개): %s"),
				*CardId.ToString(), AbilityByCardId.Num(),
				Dump.IsEmpty() ? TEXT("(비어있음)") : *Dump),
			FColor::Red, 10.f);
		return;
	}
	TSubclassOf<UGameplayAbility> AbilityClass = *Found;

	// ── 대상/시전자 페이로드 구성 (뱅처럼 TargetSeat 이 있는 카드는 대상 폰을 넘긴다) ──
	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = GetPawn();
	if (AActor* TargetActor = ResolveTargetActor(TargetSeat))
	{
		Payload.Target = TargetActor;
	}

	BaamDebug::Screen(
		FString::Printf(TEXT("[서버] 카드처리  %s #%d  메커니즘[%s] → %s  대상:%s"),
			*CardId.ToString(),
			InstanceId,
			EventTag.IsValid() ? *EventTag.ToString() : TEXT("-"),
			*AbilityClass->GetName(),
			Payload.Target.Get() ? *Payload.Target->GetName() : TEXT("없음")),
		FColor(180, 180, 255), /*Time=*/5.f);

	// 카드 주도 발동 (md §1.1): 서버 권위로 즉석 부여+1회 발동. 페이로드가 GA 의 TriggerEventData 로 전달된다.
	FGameplayAbilitySpec Spec(AbilityClass, /*Level=*/1, /*InputID=*/INDEX_NONE, /*SourceObject=*/this);
	const FGameplayAbilitySpecHandle ActivatedHandle = ASC->GiveAbilityAndActivateOnce(Spec, &Payload);

	// GiveAbilityAndActivateOnce 는 발동에 실패하면 ClearAbility 후 무효 핸들을 돌려준다
	// (AbilitySystemComponent_Abilities.cpp: InternalTryActivateAbility 실패 분기).
	// 즉 핸들 유효성이 곧 "발동 성공" 이다. 실패한 카드는 소비하지 않는다 —
	// GA 매핑 오류나 ActivationBlockedTags(State.Dead 등)로 카드가 조용히 사라지면 추적이 어렵다.
	if (!ActivatedHandle.IsValid())
	{
		BaamDebug::Screen(
			FString::Printf(TEXT("[서버] %s 발동 실패 — 카드 #%d 는 손패에 그대로 둡니다."),
				*AbilityClass->GetName(), InstanceId),
			FColor::Red, /*Time=*/8.f);
		return;
	}

	// 뱅 규칙: 낸 카드는 결과와 무관하게 버린 패로 간다.
	// (뱅!이 빗나가도 그 뱅! 카드는 돌아오지 않는다)
	ConsumeCard(InstanceId);
}

// ======================================================================================
//  턴
// ======================================================================================

int32 ABaamPlayerController::GetMySeatIndex() const
{
	const ABaamPlayerState* PS = GetPlayerState<ABaamPlayerState>();
	return PS ? PS->GetSeatIndex() : INDEX_NONE;
}

bool ABaamPlayerController::IsMyTurnToPlay() const
{
	const ABaamGameState* GS = GetWorld() ? GetWorld()->GetGameState<ABaamGameState>() : nullptr;
	return GS && GS->CanSeatPlayCards(GetMySeatIndex());
}

bool ABaamPlayerController::IsMyTurnToDiscard() const
{
	const ABaamGameState* GS = GetWorld() ? GetWorld()->GetGameState<ABaamGameState>() : nullptr;
	return GS && GS->CanSeatDiscard(GetMySeatIndex());
}

void ABaamPlayerController::RequestEndTurn()
{
	ServerRequestEndTurn();
}

void ABaamPlayerController::ServerRequestEndTurn_Implementation()
{
	if (ABaamGameState* GS = GetWorld() ? GetWorld()->GetGameState<ABaamGameState>() : nullptr)
	{
		//	좌석은 서버가 가진 PlayerState 에서 읽는다 — 클라가 보낸 값을 믿지 않는다.
		GS->RequestEndTurn(GetMySeatIndex());
	}
}

bool ABaamPlayerController::ServerRequestDiscardCard_Validate(int32 InstanceId)
{
	return InstanceId != INDEX_NONE;
}

void ABaamPlayerController::ServerRequestDiscardCard_Implementation(int32 InstanceId)
{
	ABaamGameState* GS = GetWorld() ? GetWorld()->GetGameState<ABaamGameState>() : nullptr;
	const int32 MySeat = GetMySeatIndex();

	//	Discard 페이즈 + 자기 턴에서만 허용. 아니면 손패를 마음대로 버릴 수 있게 된다.
	if (!GS || !GS->CanSeatDiscard(MySeat))
	{
		BaamDebug::Screen(TEXT("[서버] 지금은 버리기 페이즈가 아닙니다."), FColor::Red, 5.f);
		return;
	}

	ConsumeCard(InstanceId);
	GS->NotifyCardDiscarded(MySeat);
}

void ABaamPlayerController::ConsumeCard(int32 InstanceId)
{
	ABaamPlayerState* PS = GetPlayerState<ABaamPlayerState>();
	if (!PS)
	{
		return;
	}

	// 손패 변경은 반드시 이 함수를 거친다 — HandCount 동기화와 OnHandChanged 통지가 여기 있다.
	FBaamCardInstance Removed;
	if (!PS->RemoveCardFromHand(InstanceId, Removed))
	{
		// 발동 직전에 손패에서 찾아 검증했으므로 여기 걸리면 다른 경로가 손패를 건드린 것이다.
		UE_LOG(LogBaamCard, Warning,
			TEXT("[Bang] ConsumeCard: Instance=%d 를 손패에서 찾지 못했습니다 (이미 제거됨?)."), InstanceId);
		return;
	}

	// 버린 패로. DiscardTop 갱신과 DeckCount/DiscardCount 재계산이 함께 처리된다.
	if (ABaamGameState* GS = GetWorld() ? GetWorld()->GetGameState<ABaamGameState>() : nullptr)
	{
		GS->PushToDiscard(Removed);
	}

	BaamDebug::Screen(
		FString::Printf(TEXT("[서버] 카드 소비  %s #%d → 버린 패  (남은 손패 %d 장)"),
			*Removed.CardId.ToString(), Removed.InstanceId, PS->GetHandCount()),
		FColor(150, 200, 150), /*Time=*/5.f);
}

AActor* ABaamPlayerController::ResolveTargetActor(int32 TargetSeat) const
{
	if (TargetSeat == INDEX_NONE)
	{
		return nullptr;
	}

	const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GS)
	{
		return nullptr;
	}

	for (APlayerState* PS : GS->PlayerArray)
	{
		const ABaamPlayerState* BPS = Cast<ABaamPlayerState>(PS);
		if (BPS && BPS->GetSeatIndex() == TargetSeat)
		{
			return PS->GetPawn();
		}
	}
	return nullptr;
}
