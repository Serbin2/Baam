#include "Game/BaamMatchStartComponent.h"

#include "Game/BaamGameMode.h"
#include "Network/BaamNetLog.h"
#include "Game/BaamGameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

UBaamMatchStartComponent::UBaamMatchStartComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

ABaamGameMode* UBaamMatchStartComponent::GetGameMode() const
{
	return Cast<ABaamGameMode>(GetOwner());
}

void UBaamMatchStartComponent::BeginPlay()
{
	Super::BeginPlay();
	StartArrivalWatch();
}

void UBaamMatchStartComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ArrivalTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void UBaamMatchStartComponent::NotifyPlayerArrived()
{
	TryStartMatch();
}

void UBaamMatchStartComponent::StartArrivalWatch()
{
	const ABaamGameMode* GM = GetGameMode();
	if (bMatchStarted || !GM || !GM->HasAuthority())
	{
		return;
	}

	const UBaamGameInstance* GI = GM->GetGameInstance<UBaamGameInstance>();
	if (!GI || GI->GetPendingTravelPlayerCount() <= 0)
	{
		// 트래블로 들어온 레벨이 아니다(로비 등) — 판 시작 대상이 아니다.
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ArrivalAttempts = 0;
	World->GetTimerManager().SetTimer(ArrivalTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		++ArrivalAttempts;
		TryStartMatch();

		if (bMatchStarted || ArrivalAttempts >= ArrivalMaxAttempts)
		{
			if (!bMatchStarted)
			{
				UE_LOG(LogBaamNet, Warning,
					TEXT("[MatchStart] 트래블 도착 대기 시간 초과 — 판을 시작하지 못했다"));
			}
			if (const UWorld* W = GetWorld())
			{
				W->GetTimerManager().ClearTimer(ArrivalTimer);
			}
		}
	}), ArrivalPollInterval, /*bLoop=*/true, /*FirstDelay=*/ArrivalPollInterval);
}

void UBaamMatchStartComponent::TryStartMatch()
{
	ABaamGameMode* GM = GetGameMode();
	if (bMatchStarted || !GM || !GM->HasAuthority())
	{
		return;
	}

	UBaamGameInstance* GI = GM->GetGameInstance<UBaamGameInstance>();
	const int32 Expected = GI ? GI->GetPendingTravelPlayerCount() : 0;
	if (Expected <= 0)
	{
		return;
	}

	TArray<APlayerController*> Players;
	GM->CollectPlayers(Players);
	if (Players.Num() < Expected)
	{
		UE_LOG(LogBaamNet, Verbose, TEXT("[MatchStart] 도착 대기 %d/%d"), Players.Num(), Expected);
		return;
	}

	bMatchStarted = true;
	GI->ClearPendingTravelPlayerCount();

	UE_LOG(LogBaamNet, Log, TEXT("[MatchStart] 전원 도착(%d명) — 역할 배정 요청"), Players.Num());
	GM->AssignRoles();
}
