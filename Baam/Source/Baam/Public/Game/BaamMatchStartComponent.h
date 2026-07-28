// BANG! 판 시작 진행 — 서버 권위. 트래블 인원이 다 도착하면 게임모드에 역할 배정을 시킨다.
// 도착 인원은 UBaamGameInstance 가 출발 시점에 기록해 둔 수와 비교한다.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BaamMatchStartComponent.generated.h"

class ABaamGameMode;

UCLASS(ClassGroup = (Baam), meta = (BlueprintSpawnableComponent))
class BAAM_API UBaamMatchStartComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBaamMatchStartComponent();

	// 게임모드의 입장 훅에서 불린다(신규 로그인 / 심리스 트래블 / 폰 스폰 직후).
	void NotifyPlayerArrived();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	ABaamGameMode* GetGameMode() const;

	// 전원 도착이면 역할 배정을 1회 실행한다.
	void TryStartMatch();

	// 훅이 안 불리는 경로를 위한 보조 감시(성공 또는 시간 초과 시 중단).
	void StartArrivalWatch();

	bool bMatchStarted = false;

	FTimerHandle ArrivalTimer;
	int32 ArrivalAttempts = 0;

	// 0.5초 간격 x 20회 = 10초.
	static constexpr float ArrivalPollInterval = 0.5f;
	static constexpr int32 ArrivalMaxAttempts = 20;
};
