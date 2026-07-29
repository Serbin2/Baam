// BANG! 플레이어 컨트롤러 — 클라 입력을 서버로 중계한다 (md §1.2).
// HUD 는 GAS 인풋 바인딩을 쓰지 않고, 카드 사용 의도를 이 커스텀 RPC 로 보낸다.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UI/BangCardView.h"       // FBangCardView (OnCardPlayRequested 콜백 시그니처)
#include "UI/BangSeatView.h"       // FOnBangSeatSelected 등 델리게이트 타입
#include "BaamPlayerController.generated.h"   // 항상 마지막

class UGameplayAbility;
class UBangHandWidget;
class UBangSeatBoardWidget;
class ABaamPlayerState;

UCLASS()
class BAAM_API ABaamPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABaamPlayerController();

	// 클라(소유): HUD 의 OnCardPlayRequested 에 바인딩한다. 서버로 중계만 한다.
	//   InstanceId : 사용할 카드 인스턴스 (FBangCardView.InstanceId)
	//   TargetSeat : 대상 좌석. 대상이 필요 없으면 INDEX_NONE.
	UFUNCTION(BlueprintCallable, Category = "Bang")
	void RequestPlayCard(int32 InstanceId, int32 TargetSeat);

	// ── 손패 UI 배선  ────────────────────────────────────
	//
	// HUD(WBP_BangHUD)의 Event Construct 에서 자기 안의 손패 위젯을 한 번 넘겨주면,
	// 이후 갱신은 전부 여기서 처리한다. BP 쪽 작업은 이 노드 하나가 전부다.
	//
	// 위젯 생성과 PlayerState 복제 도착은 순서가 보장되지 않으므로, 양쪽 경로
	// (SetHandWidget / OnRep_PlayerState) 에서 모두 바인딩을 시도한다.
	UFUNCTION(BlueprintCallable, Category = "Bang|UI")
	void SetHandWidget(UBangHandWidget* InHandWidget);

	// ── 좌석 보드 UI 배선 ────────────────────────────────────────
	//
	// HUD 가 자기 안의 좌석 보드를 한 번 넘겨준다. 이후 갱신은 여기서 처리한다.
	// 좌석 정보(HP/손패 장수/장비)는 바뀌는 경로가 여럿이라 단일 델리게이트가 없다.
	// 프로토타입에서는 타이머로 주기 갱신한다 — 좌석 위젯을 좌석 번호로 재사용하므로
	// 갱신 중에도 마우스 호버나 선택 상태가 끊기지 않는다.
	UFUNCTION(BlueprintCallable, Category = "Bang|UI")
	void SetSeatBoardWidget(UBangSeatBoardWidget* InSeatBoard);

	UFUNCTION(BlueprintCallable, Category = "Bang|UI")
	void RefreshSeatBoard();

	// ── 대상 좌석 선택 ───────────────────────────────────────────

	/**
	 * 대상 선택을 시작한다. 고를 수 있는 좌석은 호출자가 정한다.
	 * ContextId 는 응답에 그대로 실려 돌아온다(보통 카드 InstanceId).
	 */
	UFUNCTION(BlueprintCallable, Category = "Bang|Target")
	void BeginTargetSelection(int32 ContextId, const FText& Prompt, const TArray<int32>& SelectableSeats);

	/**
	 * 카드 사용용 편의 함수 — 살아 있는 다른 좌석 전부를 대상 후보로 연다.
	 * ⚠️ 사거리 판정이 아직 없어 실제 규칙보다 넓다. 최종 판정은 서버가 한다(STEP 7/8).
	 */
	UFUNCTION(BlueprintCallable, Category = "Bang|Target")
	void BeginTargetSelectionForCard(int32 CardInstanceId);

	UFUNCTION(BlueprintCallable, Category = "Bang|Target")
	void CancelTargetSelection();

	/** 좌석이 확정됐다. 기본 동작으로 ServerRequestPlayCard 를 보낸다. */
	UPROPERTY(BlueprintAssignable, Category = "Bang|Target")
	FOnBangSeatSelected OnTargetSeatSelected;

	UPROPERTY(BlueprintAssignable, Category = "Bang|Target")
	FOnBangTargetSelectionCancelled OnTargetSelectionCancelled;

	// ── 콘솔 테스트 트리거 (구현: BaamCardExec.cpp) ──────────────
	//
	// 손패는 창(=플레이어)마다 다르므로 PlayerController 에 둔다. GameMode 에 두면
	// 서버 창에서만 호출되어 "각 클라가 자기 손패만 보는지" 를 검증할 수 없다.
	UFUNCTION(Exec)
	void Baam_DumpHand();

	// 손패 위젯을 현재 PlayerState 의 손패로 다시 그린다. 수동 갱신이 필요할 때 호출.
	UFUNCTION(BlueprintCallable, Category = "Bang|UI")
	void RefreshHandWidget();
	
	virtual void OnRep_PlayerState() override;

protected:
	// PlayerState 의 OnHandChanged 에 구독한다. 위젯/PlayerState 중 늦게 오는 쪽에 맞춰 재시도된다.
	void TryBindHandDelegate();

	// 델리게이트 콜백. Dynamic 델리게이트라 UFUNCTION 이어야 한다.
	UFUNCTION()
	void HandleHandChanged();

	/** 손패에서 카드를 중앙에 드롭했다. 대상이 필요한 카드면 좌석 선택으로 넘어간다. */
	UFUNCTION()
	void HandleCardPlayRequested(const FBangCardView& Card);

	/** 좌석 보드가 좌석 확정을 알려온다. */
	UFUNCTION()
	void HandleSeatSelected(int32 SeatIndex, int32 ContextId);

	UFUNCTION()
	void HandleTargetSelectionCancelled(int32 ContextId);

	/** 우클릭 / ESC 로 대상 선택을 취소한다. */
	virtual void SetupInputComponent() override;

	/** 손패 드래그 잠금. HandWidget 이 없으면 아무것도 하지 않는다. */
	void SetHandInteractionLocked(bool bLocked);

	/** 좌석 정보 주기 갱신 간격(초). 0 이하면 갱신하지 않는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Bang|UI")
	float SeatBoardRefreshInterval = 0.25f;
	// 클라 → 서버. 검증 후 카드 사용을 처리한다.
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRequestPlayCard(int32 InstanceId, int32 TargetSeat);

	// 서버: 검증 통과 후 실제 처리. 카드의 메커니즘 GA 를 부여+발동한다.
	void HandlePlayCard(int32 InstanceId, int32 TargetSeat);

	// BANG! 메커니즘 GA. 1단계에서는 하드코딩 — 이후 카드 데이터(UBangCardDef)에서 조회.
	UPROPERTY(EditDefaultsOnly, Category = "Bang")
	TSubclassOf<UGameplayAbility> BangAbilityClass;

private:
	// HUD 가 넘겨준 손패 위젯. 소유는 HUD 에 있고 여기서는 참조만 한다.
	UPROPERTY()
	TObjectPtr<UBangHandWidget> HandWidget;

	// 중복 구독을 막기 위해 현재 구독 중인 PlayerState 를 기억한다.
	UPROPERTY()
	TWeakObjectPtr<ABaamPlayerState> BoundPlayerState;

	// HUD 가 넘겨준 좌석 보드. 소유는 HUD 에 있고 여기서는 참조만 한다.
	UPROPERTY()
	TObjectPtr<UBangSeatBoardWidget> SeatBoardWidget;

	FTimerHandle SeatRefreshTimer;
};
