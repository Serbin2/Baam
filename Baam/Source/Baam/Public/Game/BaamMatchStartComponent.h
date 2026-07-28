// BANG! 판 시작 진행 — 서버 권위. 맵 이동 없이 제자리에서 판을 연다(Prototype-Workflow.md §1.4).
// 세션 계층이 StartMatch 를 부르고, 이 컴포넌트가 게임모드에 역할 배정을 시킨다.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BaamMatchStartComponent.generated.h"

class ABaamGameMode;

UCLASS(ClassGroup = (Baam), Config = Game, meta = (BlueprintSpawnableComponent))
class BAAM_API UBaamMatchStartComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBaamMatchStartComponent();

	// 현재 월드의 게임모드에 붙은 컴포넌트. 서버가 아니면 nullptr.
	static UBaamMatchStartComponent* Find(const UWorld* World);

	// 게임모드의 PostLogin 에서 불린다. 정원이 차면 그대로 판을 연다.
	void NotifyPlayerJoined();

	// 서버 전용. 역할을 배정하고 판을 연다. 이미 시작했거나 인원이 안 맞으면 false.
	UFUNCTION(BlueprintCallable, Category = "Baam|Match")
	bool StartMatch();

	// 지금 새 플레이어를 받아도 되는가. 거절 사유는 OutError 로 돌려준다.
	// 클라의 bAllowJoinInProgress 검사는 검색 스냅샷이라 못 믿는다 — 서버 판정이 최종.
	bool CanAcceptPlayer(FString& OutError) const;

	UFUNCTION(BlueprintPure, Category = "Baam|Match")
	bool IsMatchStarted() const { return bMatchStarted; }

protected:
	// 이 인원이 모이면 자동으로 시작한다. 4~7 로 클램프된다.
	// DefaultGame.ini [/Script/Baam.BaamMatchStartComponent] 로 덮어쓴다.
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Baam|Match")
	int32 AutoStartPlayers = 4;

private:
	ABaamGameMode* GetGameMode() const;

	bool bMatchStarted = false;
};
