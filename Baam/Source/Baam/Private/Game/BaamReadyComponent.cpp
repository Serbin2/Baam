#include "Game/BaamReadyComponent.h"

#include "Game/BaamMatchStartComponent.h"
#include "Network/BaamNetLog.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

UBaamReadyComponent::UBaamReadyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UBaamReadyComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UBaamReadyComponent, bReady);
}

void UBaamReadyComponent::BeginPlay()
{
	Super::BeginPlay();

	// 호스트는 준비 버튼 없이 바로 시작할 수 있어야 한다 — 서버에서 미리 준비로 둔다.
	const APlayerState* PS = Cast<APlayerState>(GetOwner());
	const APlayerController* PC = PS ? PS->GetPlayerController() : nullptr;
	if (PS && PS->HasAuthority() && PC && PC->IsLocalController())
	{
		SetReadyAuthoritative(true);
	}
}

UBaamReadyComponent* UBaamReadyComponent::Find(const APlayerState* PlayerState)
{
	return PlayerState ? PlayerState->FindComponentByClass<UBaamReadyComponent>() : nullptr;
}

void UBaamReadyComponent::CountLobby(const UWorld* World, int32& OutTotal, int32& OutReady)
{
	OutTotal = 0;
	OutReady = 0;

	const AGameStateBase* GS = World ? World->GetGameState() : nullptr;
	if (!GS)
	{
		return;
	}

	for (const APlayerState* PS : GS->PlayerArray)
	{
		if (!PS || PS->IsOnlyASpectator())
		{
			continue;
		}
		++OutTotal;

		const UBaamReadyComponent* Ready = Find(PS);
		if (Ready && Ready->IsReady())
		{
			++OutReady;
		}
	}
}

void UBaamReadyComponent::RequestSetReady(bool bInReady)
{
	const AActor* Owner = GetOwner();
	if (Owner && Owner->HasAuthority())
	{
		SetReadyAuthoritative(bInReady);
		return;
	}
	ServerSetReady(bInReady);
}

void UBaamReadyComponent::ServerSetReady_Implementation(bool bInReady)
{
	SetReadyAuthoritative(bInReady);
}

void UBaamReadyComponent::SetReadyAuthoritative(bool bInReady)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || bReady == bInReady)
	{
		return;
	}

	// 시작한 판에서는 준비 상태를 더 건드리지 않는다.
	UBaamMatchStartComponent* Match = UBaamMatchStartComponent::Find(GetWorld());
	if (Match && Match->IsMatchStarted())
	{
		return;
	}

	bReady = bInReady;
	UE_LOG(LogBaamNet, Log, TEXT("[Ready] %s → %s"),
		*Owner->GetName(), bReady ? TEXT("준비") : TEXT("해제"));

	if (Match)
	{
		Match->NotifyPlayerJoined();
	}
}
