// BANG! 로비 참가자 규약 — 세션/게임모드는 PlayerState 구현을 모르고 이 인터페이스에만 의존한다.
// 획득: Cast<IBaamLobbyMember>(PlayerController->PlayerState)

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BaamLobbyMemberInterface.generated.h"

UINTERFACE(MinimalAPI)
class UBaamLobbyMember : public UInterface
{
	GENERATED_BODY()
};

class BAAM_API IBaamLobbyMember
{
	GENERATED_BODY()

public:
	// 로비 준비 상태. 호스트는 집계에서 빠진다.
	virtual bool IsReady() const = 0;

	// 리슨 서버를 띄운 당사자인가.
	virtual bool IsHost() const = 0;

	// 클라 → 서버 요청. 호스트가 부르면 무시된다.
	virtual void RequestSetReady(bool bReady) = 0;

	// 서버 전용. 로그인/트래블 시 게임모드가 표시한다.
	virtual void SetIsHost(bool bInIsHost) = 0;
};
