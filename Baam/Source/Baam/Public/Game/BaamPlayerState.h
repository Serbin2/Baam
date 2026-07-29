// BANG! 플레이어 상태 — 좌석/손패/장비/생존 등 플레이어별 데이터의 소유자(md §1.2).
// 손패는 COND_OwnerOnly 로 본인에게만, 장수는 전원에게 복제한다.
// 아직 카드 계층이 없어 좌석만 들고 있다 — 손패/장비는 M2 에서 여기에 붙인다.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GameplayTagContainer.h"
#include "BaamPlayerState.generated.h"

struct FBaamCardInstance;

/** 손패 내용이 바뀌었다. UI 는 여기에 구독해 갱신한다. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBaamHandChanged);

UCLASS()
class BAAM_API ABaamPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ABaamPlayerState();

	/**
	 * 손패 변경 알림.
	 *
	 * ⚠️ 리슨서버 호스트는 자기 PlayerState 에 OnRep 이 호출되지 않는다(서버는 복제를 받지 않으므로).
	 *    그래서 AddCardToHand / RemoveCardFromHand 안에서도 같이 브로드캐스트한다.
	 *    이걸 놓치면 "클라는 손패가 보이는데 호스트만 안 보인다" 가 된다.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Baam|Player")
	FOnBaamHandChanged OnHandChanged;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 좌석 번호(0-based). 거리 계산과 대상 지정의 기준. 미배정이면 INDEX_NONE.
	UFUNCTION(BlueprintPure, Category = "Baam|Player")
	int32 GetSeatIndex() const { return SeatIndex; }

	// 서버 전용. 판 시작 시 게임모드가 좌석을 매긴다.
	void SetSeatIndex(int32 InSeat);
	
	//	서버 전용. 손패 변경. HandCount 동기화를 한곳에 모으기 위해 Hand 는 이 둘로만 건드린다.
	void AddCardToHand(const FBaamCardInstance& Card);
	bool RemoveCardFromHand(int32 InstanceId, FBaamCardInstance& OutRemoved);

	//	⚠️ COND_OwnerOnly 라 "본인 클라"와 "서버"에서만 내용이 차 있다.
	//	   다른 플레이어의 PlayerState 에서 부르면 클라에서는 항상 빈 배열이어야 정상이다
	//	   (비어 있지 않다면 은닉 정보가 새고 있는 것).
	const TArray<FBaamCardInstance>& GetHand() const { return Hand; }

	//	전원에게 복제되는 공개 장수. 남의 손패 크기는 이 값으로만 알 수 있다.
	UFUNCTION(BlueprintPure, Category = "Baam|Player")
	int32 GetHandCount() const { return HandCount; }

	//	장착 중인 파란 카드. 공개 정보라 전원에게 복제된다.
	const TArray<FBaamCardInstance>& GetEquipment() const { return Equipment; }

	// ── 역할 공개 ──
	//
	// 진짜 역할은 ABaamCharacter::CharacterTag 에 있고 COND_OwnerOnly 로 본인에게만 간다.
	// 여기 값은 "공개된 역할" 만 담는다 — 보안관(판 시작 시), 사망자, 판 종료 시 전원.
	// 비공개면 무효 태그다. UI 는 반드시 이 값을 봐야 한다.
	UFUNCTION(BlueprintPure, Category = "Baam|Player")
	FGameplayTag GetPublicRole() const { return PublicRoleTag; }

	//	서버 전용. 역할을 공개 상태로 올린다.
	void SetPublicRole(const FGameplayTag& InRole);

	// ── 생사 ──
	//
	// ⚠️ ASC 의 State.Dead 루즈 태그는 복제되지 않으므로 클라가 볼 수 없다.
	//    "죽었다" 는 공개 사실이므로 여기에 복제 변수로 둔다.
	//    (루즈 태그는 서버에서 GA 의 ActivationBlockedTags 를 막는 용도로만 쓴다)
	UFUNCTION(BlueprintPure, Category = "Baam|Player")
	bool IsDead() const { return bIsDead; }

	//	서버 전용. 사망 확정 시 한 번만 호출된다.
	void SetDead(bool bInDead);

	/**
	 * 서버 전용. 손패와 장비를 전부 비우고 그 카드들을 돌려준다(사망 처리용).
	 * 호출자가 버린 패로 보내야 한다.
	 */
	void TakeAllCards(TArray<FBaamCardInstance>& OutCards);

private:
	//	소유 클라에 Hand 가 복제되면 호출된다.
	//	TODO(3.2): 여기서 OnHandChanged 를 브로드캐스트해 손패 UI 를 갱신한다.
	//	⚠️ 리슨서버 호스트는 OnRep 이 호출되지 않으므로, 그때는 AddCardToHand /
	//	   RemoveCardFromHand 안에서도 같은 알림을 쏴야 한다.
	UFUNCTION()
	void OnRep_Hand();

	UPROPERTY(Replicated)
	int32 SeatIndex = INDEX_NONE;

	UPROPERTY(ReplicatedUsing = OnRep_Hand)
	TArray<FBaamCardInstance> Hand;       // COND_OwnerOnly
	UPROPERTY(Replicated)                   
	int32 HandCount = 0;                   // 전원 공개
	UPROPERTY(Replicated)
	TArray<FBaamCardInstance> Equipment;   // 전원 공개 (파란 카드)

	UPROPERTY(Replicated)
	bool bIsDead = false;                  // 전원 공개

	UPROPERTY(Replicated)
	FGameplayTag PublicRoleTag;            // 전원 공개 (공개된 역할만)

};
