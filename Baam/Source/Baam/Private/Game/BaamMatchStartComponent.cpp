#include "Game/BaamMatchStartComponent.h"

#include "Game/BaamGameMode.h"
#include "Game/BaamGameInstance.h"
#include "Game/BaamReadyComponent.h"
#include "Game/BaamGameState.h"
#include "Game/BaamGameplayTags.h"
#include "Game/BaamGameState.h"
#include "Network/BaamNetLog.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Math/UnrealMathUtility.h"

UBaamMatchStartComponent::UBaamMatchStartComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

int32 UBaamMatchStartComponent::GetMinStartPlayers() const
{
	return FMath::Clamp(MinStartPlayers, ABaamGameMode::MinPlayers, ABaamGameMode::MaxPlayers);
}

void UBaamMatchStartComponent::NotifyPlayerJoined()
{
	const ABaamGameMode* GM = GetGameMode();
	if (bMatchStarted || !GM || !GM->HasAuthority())
	{
		return;
	}

	int32 Total = 0;
	int32 Ready = 0;
	UBaamReadyComponent::CountLobby(GetWorld(), Total, Ready);

	UE_LOG(LogBaamNet, Log, TEXT("[MatchStart] 로비 %d/%d명 · 준비 %d명"),
		Total, GetMinStartPlayers(), Ready);
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

bool UBaamMatchStartComponent::CanStartMatch(FString& OutError) const
{
	if (bMatchStarted)
	{
		OutError = TEXT("이미 시작된 판입니다");
		return false;
	}

	int32 Total = 0;
	int32 Ready = 0;
	UBaamReadyComponent::CountLobby(GetWorld(), Total, Ready);

	const int32 Min = GetMinStartPlayers();
	if (Total < Min)
	{
		OutError = FString::Printf(TEXT("인원 부족 (%d/%d명)"), Total, Min);
		return false;
	}
	if (Total > ABaamGameMode::MaxPlayers)
	{
		OutError = FString::Printf(TEXT("정원 초과 (%d명)"), Total);
		return false;
	}
	if (Ready < Total)
	{
		OutError = FString::Printf(TEXT("준비 대기 (%d/%d명)"), Ready, Total);
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

	FString Error;
	if (!CanStartMatch(Error))
	{
		UE_LOG(LogBaamNet, Warning, TEXT("[MatchStart] 시작 불가 — %s"), *Error);
		return false;
	}

	bMatchStarted = true;

	// 난입 차단은 여기서 한다 — 시작 경로가 늘어나도 동일하게 걸려야 한다.
	// 빠지면 시작 후 들어온 플레이어가 역할 없이 판에 남는다.
	if (UBaamGameInstance* GI = GM->GetGameInstance<UBaamGameInstance>())
	{
		GI->SetAllowJoinInProgress(false);
	}

	TArray<APlayerController*> Players;
	GM->CollectPlayers(Players);

	UE_LOG(LogBaamNet, Log, TEXT("[MatchStart] 판 시작(%d명) — 역할 배정"), Players.Num());
	GM->AssignRoles();

	// 역할 배정으로 좌석과 스탯(Health)이 확정된 뒤에 판을 연다.
	// StartMatch 가 초기 손패를 Health 만큼 나눠주므로 순서가 뒤바뀌면 안 된다.
	if (ABaamGameState* GS = GM->GetGameState<ABaamGameState>())
	{
		GS->StartMatch();
	}
	else
	{
		UE_LOG(LogBaamNet, Error, TEXT("[MatchStart] ABaamGameState 가 없어 턴을 시작하지 못했습니다."));
	}

	return true;
}

