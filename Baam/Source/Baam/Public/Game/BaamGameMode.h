// BANG! 게임모드 — 서버 권위. 접속 인원수(4~7)에 맞춰 역할 태그를 셔플 배정한다.
// 배정은 캐릭터의 SetCharacterTag 로 전달되고, 캐릭터가 DT(FBaamCharacterRow)를 읽어
// 스탯/능력을 스스로 부여한다. (Team4Project 의 GODGameMode::AssignRoles 대응)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameplayTagContainer.h"
#include "BaamGameMode.generated.h"

UCLASS()
class BAAM_API ABaamGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	// 지원 인원 범위 (BANG! 기본판).
	static constexpr int32 MinPlayers = 4;
	static constexpr int32 MaxPlayers = 7;

	// 서버: 접속 인원수에 맞춰 역할 태그를 셔플해 한 명씩 배정한다.
	// 인원이 4~7 밖이면 경고 로그만 남기고 아무것도 하지 않는다.
	// (게임 시작 시점 — 로비 카운트다운/시작 버튼 흐름에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Bang")
	void AssignRoles();

	// 인원수에 맞는 역할 태그 풀을 구성 (셔플 전). 4:S/O/O/R, 5:+D, 6:+O, 7:+D.
	static TArray<FGameplayTag> BuildRolePool(int32 NumPlayers);

protected:
	// 접속한 PlayerController 를 모은다.
	void CollectPlayers(TArray<APlayerController*>& OutPlayers) const;
};
