// BANG! 플레이어 상태 — 로비 Ready 플래그를 복제한다. 규약은 IBaamLobbyMember.
// Ready 토글을 여기 두는 이유: 클라가 자기 PlayerState 를 소유해 Server RPC 가 성립한다.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Network/Session/BaamLobbyMemberInterface.h"
#include "BaamPlayerState.generated.h"

UCLASS()
class BAAM_API ABaamPlayerState : public APlayerState, public IBaamLobbyMember
{
	GENERATED_BODY()

public:
	ABaamPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ IBaamLobbyMember
	virtual bool IsReady() const override { return bIsReady; }
	virtual bool IsHost() const override { return bIsHost; }
	virtual void RequestSetReady(bool bReady) override { Server_SetReady(bReady); }
	virtual void SetIsHost(bool bInIsHost) override;
	//~ End

private:
	UFUNCTION(Server, Reliable)
	void Server_SetReady(bool bInReady);

	UPROPERTY(Replicated)
	bool bIsReady = false;

	UPROPERTY(Replicated)
	bool bIsHost = false;
};
