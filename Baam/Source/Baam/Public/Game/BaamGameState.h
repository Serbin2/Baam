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
class UBaamDiceComponent;

UCLASS()
class BAAM_API ABaamGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ABaamGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//	확률 판정(주사위) 전담. 어디서든 UBaamDiceComponent::Get(this) 로도 얻을 수 있다.
	UFUNCTION(BlueprintPure, Category = "Baam|Dice")
	UBaamDiceComponent* GetDice() const { return Dice; }

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

	//	현재 페이즈. 클라도 읽는다 — 로비 UI 가 Phase.Play 를 보고 스스로 물러난다.
	UFUNCTION(BlueprintPure, Category = "Baam|Phase")
	FGameplayTag GetPhaseTag() const { return PhaseTag; }

	//	서버 전용. 페이즈를 바꾼다.
	void SetPhaseTag(FGameplayTag InPhase);

	//	전원에게 복제되는 공개 카운트. 클라 UI/검증은 이 값을 본다.
	UFUNCTION(BlueprintPure, Category = "Baam|Deck")
	int32 GetDeckCount() const { return DeckCount; }

	UFUNCTION(BlueprintPure, Category = "Baam|Deck")
	int32 GetDiscardCount() const { return DiscardCount; }


protected:

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
	
	//	⚠️ 덱 셔플 전용 스트림이다. 주사위 판정은 UBaamDiceComponent 가 별도 스트림을 쓴다
	//	   (한 스트림을 공유하면 판정 횟수가 셔플 결과를 밀어 재현이 깨진다).
	UPROPERTY()
	FRandomStream RandomStream;
	//	시드 보존을 위해 한번 초기화 하면 다시는 초기화 하지 않는다.
	//	게임 초기화시에만 false로 되돌릴 것.
	UPROPERTY()
	bool bInitialized = false;

	//	확률 판정 전담 컴포넌트. 매치 시드로 InitializeDeck 에서 함께 초기화된다.
	UPROPERTY(VisibleAnywhere, Category = "Baam|Dice")
	TObjectPtr<UBaamDiceComponent> Dice;
};
