// BANG! 플레이어 상태 — 좌석/손패/장비/생존 등 플레이어별 데이터의 소유자(md §1.2).
// 손패는 COND_OwnerOnly 로 본인에게만, 장수는 전원에게 복제한다.
// 아직 카드 계층이 없어 좌석만 들고 있다 — 손패/장비는 M2 에서 여기에 붙인다.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BaamPlayerState.generated.h"

UCLASS()
class BAAM_API ABaamPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ABaamPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 좌석 번호(0-based). 거리 계산과 대상 지정의 기준. 미배정이면 INDEX_NONE.
	UFUNCTION(BlueprintPure, Category = "Baam|Player")
	int32 GetSeatIndex() const { return SeatIndex; }

	// 서버 전용. 판 시작 시 게임모드가 좌석을 매긴다.
	void SetSeatIndex(int32 InSeat);

private:
	UPROPERTY(Replicated)
	int32 SeatIndex = INDEX_NONE;
};
