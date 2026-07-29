// BANG! 플레이어 컨트롤러 — 클라 입력을 서버로 중계한다 (md §1.2).
// HUD 는 GAS 인풋 바인딩을 쓰지 않고, 카드 사용 의도를 이 커스텀 RPC 로 보낸다.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "BaamPlayerController.generated.h"

class UGameplayAbility;
class UBangHandWidget;
class ABaamPlayerState;

UCLASS()
class BAAM_API ABaamPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABaamPlayerController();

	// 클라(소유): HUD 의 OnCardPlayRequested 에 바인딩한다. 서버로 중계만 한다.
	//   CardId     : 사용할 카드 종류 태그 (Card.Id.Bang / Card.Id.Beer ...). 숫자 InstanceId 대신
	//                읽기 쉬운 태그로 지정한다. 같은 카드가 여러 장이면 손패의 첫 장을 쓴다.
	//   TargetSeat : 대상 좌석. 대상이 필요 없으면 INDEX_NONE.
	UFUNCTION(BlueprintCallable, Category = "Bang")
	void RequestPlayCard(FGameplayTag CardId, int32 TargetSeat);

	// ── 손패 UI 배선  ────────────────────────────────────
	//
	// HUD(WBP_BangHUD)의 Event Construct 에서 자기 안의 손패 위젯을 한 번 넘겨주면,
	// 이후 갱신은 전부 여기서 처리한다. BP 쪽 작업은 이 노드 하나가 전부다.
	//
	// 위젯 생성과 PlayerState 복제 도착은 순서가 보장되지 않으므로, 양쪽 경로
	// (SetHandWidget / OnRep_PlayerState) 에서 모두 바인딩을 시도한다.
	UFUNCTION(BlueprintCallable, Category = "Bang|UI")
	void SetHandWidget(UBangHandWidget* InHandWidget);

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
	// 클라 → 서버. 검증 후 카드 사용을 처리한다.
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRequestPlayCard(FGameplayTag CardId, int32 TargetSeat);

	// 서버: 검증 통과 후 실제 처리. 카드가 구동할 GA 를 부여+발동한다.
	void HandlePlayCard(FGameplayTag CardId, int32 TargetSeat);

	// 카드 종류(Card.Id.*) → 그 카드가 구동할 GA 매핑.
	//   ⚠️ 반드시 에디터(BP 컨트롤러 디폴트)에서 채운다. 비어 있으면 카드가 발동되지 않는다:
	//     Card.Id.Bang→GA_Bang, Card.Id.Beer→GA_Heal, Card.Id.Missed→GA_Missed, ...
	//   키는 RequestPlayCard 로 넘어온 CardId 태그와 그대로 매칭된다.
	UPROPERTY(EditDefaultsOnly, Category = "Bang")
	TMap<FGameplayTag, TSubclassOf<UGameplayAbility>> AbilityByCardId;

	// TargetSeat → 그 좌석 플레이어의 폰. 대상이 필요 없는 카드면 INDEX_NONE → nullptr.
	AActor* ResolveTargetActor(int32 TargetSeat) const;

private:
	// HUD 가 넘겨준 손패 위젯. 소유는 HUD 에 있고 여기서는 참조만 한다.
	UPROPERTY()
	TObjectPtr<UBangHandWidget> HandWidget;

	// 중복 구독을 막기 위해 현재 구독 중인 PlayerState 를 기억한다.
	UPROPERTY()
	TWeakObjectPtr<ABaamPlayerState> BoundPlayerState;
};
