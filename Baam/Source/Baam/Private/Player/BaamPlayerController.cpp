#include "Player/BaamPlayerController.h"
#include "Player/Ability/GA_Bang.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemGlobals.h"

#include "Game/BaamCardLog.h"
#include "Game/BaamCardViewLibrary.h"
#include "Game/BaamPlayerState.h"
#include "Game/BaamSeatViewLibrary.h"
#include "UI/BangHandWidget.h"
#include "UI/BangSeatBoardWidget.h"
#include "Components/InputComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

ABaamPlayerController::ABaamPlayerController()
{
	// 1단계 기본값 — 이후 카드 데이터에서 GA 를 조회하게 바뀐다.
	BangAbilityClass = UGA_Bang::StaticClass();
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
	// 로컬에서는 검증하지 않는다 (클라 표시는 힌트일 뿐, 권위는 서버). 그대로 중계.
	ServerRequestPlayCard(InstanceId, TargetSeat);
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

void ABaamPlayerController::HandlePlayCard(int32 InstanceId, int32 TargetSeat)
{
	// TODO(2단계): GameState 규칙 검증 — 내 턴? Phase.Play? 사거리? BangLimit?
	UE_LOG(LogTemp, Log, TEXT("[Bang] ServerRequestPlayCard 수신 — PC=%s Instance=%d TargetSeat=%d"),
		*GetName(), InstanceId, TargetSeat);

	// 시전자의 ASC 를 얻는다 (현재 ASC 는 캐릭터 소유).
	UAbilitySystemComponent* ASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetPawn());
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bang] HandlePlayCard: 시전자 ASC 없음 (Pawn=%s)."),
			GetPawn() ? *GetPawn()->GetName() : TEXT("null"));
		return;
	}

	if (!BangAbilityClass)
	{
		return;
	}

	// 카드 주도 발동 (md §1.1): 카드가 지정한 메커니즘 GA 를 즉석 부여+발동.
	// TODO(2단계): InstanceId → 카드 데이터에서 GA/파라미터/대상요구를 조회해 분기.
	FGameplayAbilitySpec Spec(BangAbilityClass, /*Level=*/1, /*InputID=*/INDEX_NONE, /*SourceObject=*/this);
	ASC->GiveAbilityAndActivateOnce(Spec);
}
