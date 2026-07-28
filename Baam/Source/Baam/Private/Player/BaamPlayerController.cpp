#include "Player/BaamPlayerController.h"
#include "Player/Ability/GA_Bang.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemGlobals.h"

#include "Game/BaamCardLog.h"
#include "Game/BaamCardViewLibrary.h"
#include "Game/BaamPlayerState.h"
#include "UI/BangHandWidget.h"

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
	HandWidget = InHandWidget;
	TryBindHandDelegate();
	RefreshHandWidget();
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
	HandWidget->SetHand(UBaamCardViewLibrary::MakeHandViews(PS));

	// TODO(5.7): HandWidget->OnCardPlayRequested 를 RequestPlayCard 로 연결.
	//            bNeedsTarget 분기(대상 좌석 지정)가 필요해 STEP 5 에서 함께 처리한다.
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
