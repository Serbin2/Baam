#include "Game/BaamGameInstance.h"
#include "Network/Session/BaamSessionFlow.h"
#include "Network/BaamNetLog.h"
#include "UI/BaamSessionMenuController.h"
#include "GameFramework/PlayerController.h"

#if !UE_BUILD_SHIPPING
namespace
{
	IBaamSessionInterface* GetSession(UGameInstance* GI)
	{
		UBaamSessionFlow* Flow = GI ? GI->GetSubsystem<UBaamSessionFlow>() : nullptr;
		if (!Flow)
		{
			UE_LOG(LogBaamNet, Warning, TEXT("[Exec] BaamSessionFlow 서브시스템 없음"));
		}
		return Flow;
	}
}
#endif

void UBaamGameInstance::Baam_Host()
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogBaamNet, Log, TEXT("[Exec] Baam_Host"));
	if (IBaamSessionInterface* Session = GetSession(this))
	{
		Session->HostCreateRoom(FString());
	}
#endif
}

void UBaamGameInstance::Baam_Find()
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogBaamNet, Log, TEXT("[Exec] Baam_Find"));
	if (IBaamSessionInterface* Session = GetSession(this))
	{
		Session->RefreshSessionList(FBaamSessionSearchFilter());
	}
#endif
}

void UBaamGameInstance::Baam_Join()
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogBaamNet, Log, TEXT("[Exec] Baam_Join (index 0)"));
	if (IBaamSessionInterface* Session = GetSession(this))
	{
		Session->JoinSessionByIndex(0);
	}
#endif
}

void UBaamGameInstance::Baam_JoinCode(const FString& Code)
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogBaamNet, Log, TEXT("[Exec] Baam_JoinCode '%s'"), *Code);
	if (IBaamSessionInterface* Session = GetSession(this))
	{
		Session->JoinRoomByCode(Code);
	}
#endif
}

void UBaamGameInstance::Baam_Leave()
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogBaamNet, Log, TEXT("[Exec] Baam_Leave"));
	if (IBaamSessionInterface* Session = GetSession(this))
	{
		Session->LeaveSession();
	}
#endif
}

void UBaamGameInstance::Baam_Ready(int32 bReady)
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogBaamNet, Log, TEXT("[Exec] Baam_Ready %d"), bReady);
	if (IBaamSessionInterface* Session = GetSession(this))
	{
		Session->SetLocalReady(bReady != 0);
	}
#endif
}

void UBaamGameInstance::Baam_Start()
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogBaamNet, Log, TEXT("[Exec] Baam_Start"));
	if (IBaamSessionInterface* Session = GetSession(this))
	{
		Session->StartGame();
	}
#endif
}

void UBaamGameInstance::Baam_RoomCode()
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogBaamNet, Log, TEXT("[Exec] Baam_RoomCode = '%s'"), *GetHostRoomCode());
#endif
}

void UBaamGameInstance::Baam_UI()
{
#if !UE_BUILD_SHIPPING
	if (UBaamSessionMenuController* Menu = GetSubsystem<UBaamSessionMenuController>())
	{
		Menu->Toggle();
	}
#endif
}
