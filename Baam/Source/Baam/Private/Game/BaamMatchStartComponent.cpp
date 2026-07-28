#include "Game/BaamMatchStartComponent.h"

#include "Game/BaamGameMode.h"
#include "Game/BaamGameInstance.h"
#include "Network/BaamNetLog.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Math/UnrealMathUtility.h"

UBaamMatchStartComponent::UBaamMatchStartComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBaamMatchStartComponent::NotifyPlayerJoined()
{
	const ABaamGameMode* GM = GetGameMode();
	if (bMatchStarted || !GM || !GM->HasAuthority())
	{
		return;
	}

	const int32 Threshold =
		FMath::Clamp(AutoStartPlayers, ABaamGameMode::MinPlayers, ABaamGameMode::MaxPlayers);

	TArray<APlayerController*> Players;
	GM->CollectPlayers(Players);
	if (Players.Num() < Threshold)
	{
		UE_LOG(LogBaamNet, Log, TEXT("[MatchStart] 로비 대기 %d/%d"), Players.Num(), Threshold);
		return;
	}

	StartMatch();
}

UBaamMatchStartComponent* UBaamMatchStartComponent::Find(const UWorld* World)
{
	// GetAuthGameMode 는 서버에서만 유효하다 — 클라에서는 자연히 nullptr.
	ABaamGameMode* GM = World ? World->GetAuthGameMode<ABaamGameMode>() : nullptr;
	return GM ? GM->FindComponentByClass<UBaamMatchStartComponent>() : nullptr;
}

ABaamGameMode* UBaamMatchStartComponent::GetGameMode() const
{
	return Cast<ABaamGameMode>(GetOwner());
}

bool UBaamMatchStartComponent::CanAcceptPlayer(FString& OutError) const
{
	if (bMatchStarted)
	{
		OutError = TEXT("판이 이미 시작되었습니다");
		return false;
	}
	return true;
}

bool UBaamMatchStartComponent::StartMatch()
{
	ABaamGameMode* GM = GetGameMode();
	if (!GM || !GM->HasAuthority())
	{
		return false;
	}

	if (bMatchStarted)
	{
		UE_LOG(LogBaamNet, Warning, TEXT("[MatchStart] 이미 시작된 판이다"));
		return false;
	}

	TArray<APlayerController*> Players;
	GM->CollectPlayers(Players);

	const int32 Num = Players.Num();
	if (Num < ABaamGameMode::MinPlayers || Num > ABaamGameMode::MaxPlayers)
	{
		UE_LOG(LogBaamNet, Warning, TEXT("[MatchStart] 인원 %d 명은 지원 범위(%d~%d) 밖 — 시작 취소"),
			Num, ABaamGameMode::MinPlayers, ABaamGameMode::MaxPlayers);
		return false;
	}

	bMatchStarted = true;

	// 난입 차단은 여기서 한다 — 자동/수동 어느 경로로 시작하든 동일하게 걸려야 한다.
	// 빠지면 시작 후 들어온 플레이어가 역할 없이 판에 남는다.
	if (UBaamGameInstance* GI = GM->GetGameInstance<UBaamGameInstance>())
	{
		GI->SetAllowJoinInProgress(false);
	}

	UE_LOG(LogBaamNet, Log, TEXT("[MatchStart] 판 시작(%d명) — 역할 배정"), Num);
	GM->AssignRoles();
	return true;
}
