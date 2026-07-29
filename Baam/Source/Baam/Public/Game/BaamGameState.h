// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaamCardType.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameStateBase.h"
#include "BaamGameState.generated.h"


/**
 * 
 */
UCLASS()
class BAAM_API ABaamGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//	서버에서 덱을 구성·셔플한다. 이미 초기화됐으면 아무것도 하지 않는다(시드 보존).
	//	Seed == 0 이면 새 시드를 뽑아 쓰고, 사용한 값을 로그에 남긴다 — 재현에 필수.
	void InitializeDeck(int32 Seed = 0);

	bool DrawFromDeck(FBaamCardInstance& OutCard);   // 소진 시 Discard 리셔플
	void PushToDiscard(const FBaamCardInstance& Card);

	/**
	 * 뽑기(Draw!) 판정. 수트를 쓰지 않으므로 확률 굴림으로 대체한다.
	 * 반드시 GameState 의 시드된 스트림을 쓸 것
	 */
	bool PerformDrawCheck(int32 Seat, float SuccessChance);
	
	//	⚠️ 서버 전용 데이터다. 클라에서 부르면 항상 빈 배열이 나온다(덱 내용은 복제되지 않는다).
	void GetDeck(TArray<FBaamCardInstance>& OutDeck) const;

	//	전원에게 복제되는 공개 카운트. 클라 UI/검증은 이 값을 본다.
	UFUNCTION(BlueprintPure, Category = "Baam|Deck")
	int32 GetDeckCount() const { return DeckCount; }

	UFUNCTION(BlueprintPure, Category = "Baam|Deck")
	int32 GetDiscardCount() const { return DiscardCount; }

	// ══════════════════════════════════════════════════════════════════
	//  턴 진행 (서버 권위)
	//
	//  TurnStart → Draw(DrawCount장) → Play → [Discard] → 다음 살아있는 좌석
	//  Draw 는 자동으로 넘어가고, Play 는 플레이어가 EndTurn 을 부를 때까지 머문다.
	//  손패가 한도(=현재 Health)를 넘으면 Discard 로 가서 버릴 때까지 대기한다.
	// ══════════════════════════════════════════════════════════════════

	/** 판 시작 — 초기 손패를 나눠주고 첫 턴을 연다. 역할 배정이 끝난 뒤 호출할 것. */
	void StartMatch();

	/** 현재 턴 좌석. 아직 시작 전이면 INDEX_NONE. */
	UFUNCTION(BlueprintPure, Category = "Baam|Turn")
	int32 GetCurrentSeat() const { return CurrentSeat; }

	UFUNCTION(BlueprintPure, Category = "Baam|Turn")
	FGameplayTag GetPhaseTag() const { return PhaseTag; }

	/** 이 좌석이 지금 카드를 낼 수 있는가 (자기 턴 + Play 페이즈). */
	UFUNCTION(BlueprintPure, Category = "Baam|Turn")
	bool CanSeatPlayCards(int32 Seat) const;

	/** 이 좌석이 지금 손패를 버려야 하는가 (자기 턴 + Discard 페이즈). */
	UFUNCTION(BlueprintPure, Category = "Baam|Turn")
	bool CanSeatDiscard(int32 Seat) const;

	/** 턴 종료 요청. 자기 턴이 아니면 무시된다. 손패 초과면 Discard 페이즈로 간다. */
	void RequestEndTurn(int32 RequestingSeat);

	/** 손패를 한 장 버렸다는 통지. 한도 이하가 되면 턴을 넘긴다. */
	void NotifyCardDiscarded(int32 Seat);

	/** 손패 한도 = 현재 생명력(최대치가 아님). 좌석이 없으면 0. */
	UFUNCTION(BlueprintPure, Category = "Baam|Turn")
	int32 GetHandLimitForSeat(int32 Seat) const;

	// ── 좌석 조회 (거리 계산·턴 진행의 공통 기준) ──

	/** 살아 있는 좌석만 좌석 번호 순으로. 사망자는 원에서 빠지므로 거리도 함께 바뀐다. */
	UFUNCTION(BlueprintPure, Category = "Baam|Seat")
	TArray<int32> GetAliveSeatsInTableOrder() const;

	UFUNCTION(BlueprintPure, Category = "Baam|Seat")
	class ABaamPlayerState* GetPlayerStateBySeat(int32 Seat) const;

	UFUNCTION(BlueprintPure, Category = "Baam|Seat")
	bool IsSeatAlive(int32 Seat) const;

	/** 좌석의 역할 태그(Role.*). 캐릭터가 없으면 무효 태그. */
	UFUNCTION(BlueprintPure, Category = "Baam|Seat")
	FGameplayTag GetSeatRole(int32 Seat) const;

	// ══════════════════════════════════════════════════════════════════
	//  사망 / 승패
	// ══════════════════════════════════════════════════════════════════

	/**
	 * 서버 전용. 좌석이 사망했다. 캐릭터가 사망을 확정한 뒤 호출한다.
	 *   손패·장비 정리 → 보상/벌칙 → 승패 판정 → (미결이면) 턴 진행
	 * KillerSeat 은 모르면 INDEX_NONE.
	 */
	void HandleSeatDeath(int32 DeadSeat, int32 KillerSeat);

	/** 승패가 결정됐으면 Phase.GameOver 로 전환하고 true. */
	bool CheckWinCondition();

	/** 승리 진영(Role.*). 아직 미정이면 무효 태그. */
	UFUNCTION(BlueprintPure, Category = "Baam|Match")
	FGameplayTag GetWinningRole() const { return WinningRoleTag; }

	UFUNCTION(BlueprintPure, Category = "Baam|Match")
	bool IsMatchOver() const { return PhaseTag == GameOverPhaseTag(); }

	/** 좌석의 현재 Health. ASC 를 못 찾으면 0. */
	UFUNCTION(BlueprintPure, Category = "Baam|Seat")
	int32 GetSeatHealth(int32 Seat) const;

protected:
	//	헤더에서 태그 상수를 직접 못 쓰므로(네이티브 태그는 .cpp 정의) 래퍼로 감싼다.
	static FGameplayTag GameOverPhaseTag();

	// ── 턴 진행 내부 ──
	void BeginTurn(int32 Seat);
	void EnterDrawPhase();
	void EnterPlayPhase();
	void AdvanceToNextSeat();
	void SetPhase(const FGameplayTag& NewPhase);

	//	서버 시작 시 덱을 준비한다.
	virtual void BeginPlay() override;

	void ShuffleDeck();
	void ReshuffleDeck();

	//	Deck / Discard / DiscardTop 을 건드린 뒤 공개 카운트를 다시 맞춘다.
	void RefreshPublicCounts();
	
	// 서버 전용
	UPROPERTY()
	TArray<FBaamCardInstance> Deck;
	UPROPERTY()
	TArray<FBaamCardInstance> Discard;

	// 공개 정보 
	UPROPERTY(Replicated) 
	int32 DeckCount = 0;
	UPROPERTY(Replicated) 
	int32 DiscardCount = 0;
	
	//	버린 패 사용하는 효과 제거 예정
	UPROPERTY(Replicated) 
	FBaamCardInstance DiscardTop;   // 버린 패 맨 위 1장은 공개 정보
	
	//	현재 진행상황 태그
	UPROPERTY(Replicated)
	FGameplayTag PhaseTag;          // Phase.*

	//	현재 턴 좌석. 전원 공개 — 좌석 UI 의 강조와 카드 사용 검증이 이 값을 본다.
	UPROPERTY(Replicated)
	int32 CurrentSeat = INDEX_NONE;

	//	승리 진영. 판이 끝나면 채워진다.
	UPROPERTY(Replicated)
	FGameplayTag WinningRoleTag;

	//	무법자를 사살했을 때 가해자가 뽑는 장수 (뱅 원작 3장).
	UPROPERTY(EditDefaultsOnly, Category = "Baam|Rule", meta = (ClampMin = "0"))
	int32 OutlawKillReward = 3;

	//	턴 시작 시 뽑는 장수의 폴백. 정상 경로는 어트리뷰트 DrawCount 를 읽는다.
	UPROPERTY(EditDefaultsOnly, Category = "Baam|Turn", meta = (ClampMin = "1"))
	int32 DefaultDrawCount = 2;

	//	판 시작 시 나눠줄 초기 손패. 0 이하면 좌석의 Health 만큼 준다(뱅 원작 규칙).
	UPROPERTY(EditDefaultsOnly, Category = "Baam|Turn")
	int32 InitialHandSize = 0;

	//	무한 루프 방지 — 살아있는 좌석을 못 찾을 때의 안전장치.
	bool bMatchRunning = false;
	
	UPROPERTY()
	FRandomStream RandomStream;
	//	시드 보존을 위해 한번 초기화 하면 다시는 초기화 하지 않는다.
	//	게임 초기화시에만 false로 되돌릴 것.
	UPROPERTY()
	bool bInitialized = false;
};
