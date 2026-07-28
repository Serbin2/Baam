// BANG! 플레이어 컨트롤러 — 클라 입력을 서버로 중계한다 (md §1.2).
// HUD 는 GAS 인풋 바인딩을 쓰지 않고, 카드 사용 의도를 이 커스텀 RPC 로 보낸다.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BaamPlayerController.generated.h"

class UGameplayAbility;

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

protected:
	// 클라 → 서버. 검증 후 카드 사용을 처리한다.
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRequestPlayCard(int32 InstanceId, int32 TargetSeat);

	// 서버: 검증 통과 후 실제 처리. 카드의 메커니즘 GA 를 부여+발동한다.
	void HandlePlayCard(int32 InstanceId, int32 TargetSeat);

	// BANG! 메커니즘 GA. 1단계에서는 하드코딩 — 이후 카드 데이터(UBangCardDef)에서 조회.
	UPROPERTY(EditDefaultsOnly, Category = "Bang")
	TSubclassOf<UGameplayAbility> BangAbilityClass;
};
