#include "Network/Session/BaamSessionFlow.h"
#include "Game/BaamGameInstance.h"
#include "Game/BaamMatchStartComponent.h"
#include "Network/BaamNetLog.h"
#include "Engine/World.h"

void UBaamSessionFlow::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadConfig();
	BindGameInstanceEvents();

	UE_LOG(LogBaamNet, Log, TEXT("[SessionFlow] Initialized (정원=%d)"), MaxPlayers);
}

void UBaamSessionFlow::Deinitialize()
{
	UnbindGameInstanceEvents();
	Super::Deinitialize();
}

UBaamGameInstance* UBaamSessionFlow::GetBaamGameInstance() const
{
	return Cast<UBaamGameInstance>(GetGameInstance());
}

void UBaamSessionFlow::BindGameInstanceEvents()
{
	if (UBaamGameInstance* GI = GetBaamGameInstance())
	{
		GI->OnCreateSessionComplete.AddUniqueDynamic(this, &UBaamSessionFlow::HandleCreateSessionComplete);
		GI->OnFindSessionsComplete.AddUniqueDynamic(this, &UBaamSessionFlow::HandleFindSessionsComplete);
		GI->OnJoinSessionComplete.AddUniqueDynamic(this, &UBaamSessionFlow::HandleJoinSessionComplete);
	}
	else
	{
		// DefaultEngine.ini 의 GameInstanceClass 가 UBaamGameInstance 가 아니면 세션 API 자체가 없다.
		UE_LOG(LogBaamNet, Warning, TEXT("[SessionFlow] GameInstance 가 UBaamGameInstance 가 아님 — 세션 바인딩 불가"));
	}
}

void UBaamSessionFlow::UnbindGameInstanceEvents()
{
	if (UBaamGameInstance* GI = GetBaamGameInstance())
	{
		GI->OnCreateSessionComplete.RemoveDynamic(this, &UBaamSessionFlow::HandleCreateSessionComplete);
		GI->OnFindSessionsComplete.RemoveDynamic(this, &UBaamSessionFlow::HandleFindSessionsComplete);
		GI->OnJoinSessionComplete.RemoveDynamic(this, &UBaamSessionFlow::HandleJoinSessionComplete);
	}
}

void UBaamSessionFlow::SetPhase(EBaamSessionPhase Phase, const FString& Message)
{
	UE_LOG(LogBaamNet, Log, TEXT("[SessionFlow] Phase=%d %s"), static_cast<int32>(Phase), *Message);
	OnSessionPhaseChanged.Broadcast(Phase, Message);
}

bool UBaamSessionFlow::IsHost() const
{
	if (const UWorld* World = GetWorld())
	{
		const ENetMode Mode = World->GetNetMode();
		return Mode == NM_ListenServer || Mode == NM_Standalone || Mode == NM_DedicatedServer;
	}
	return false;
}

FString UBaamSessionFlow::GetRoomCode() const
{
	// 호스트 프로세스에만 코드가 있다. 참가자에게 보여주려면 GameState 복제가 필요하다(미구현).
	const UBaamGameInstance* GI = GetBaamGameInstance();
	return GI ? GI->GetHostRoomCode() : FString();
}

// ── 호스트 의도 ──
void UBaamSessionFlow::HostCreateRoom(const FString& RoomName)
{
	UBaamGameInstance* GI = GetBaamGameInstance();
	if (!GI)
	{
		SetPhase(EBaamSessionPhase::Failed, TEXT("GameInstance 가 UBaamGameInstance 가 아님"));
		return;
	}

	// 이미 클라이언트로 다른 세션에 접속한 상태에서는 방 생성 금지 — 클라 월드에서는 리슨을 열 수 없다.
	if (const UWorld* World = GI->GetWorld(); World && World->GetNetMode() == NM_Client)
	{
		SetPhase(EBaamSessionPhase::Failed, TEXT("이미 다른 세션에 접속 중입니다"));
		return;
	}

	SetPhase(EBaamSessionPhase::Creating, TEXT("방 생성 중..."));
	// 성공 시 GameInstance 가 현재 월드에서 그대로 리슨을 연다(맵 이동 없음).
	GI->HostSession(MaxPlayers, /*bLAN=*/false, RoomName);
}

// ── 클라이언트 의도 ──
void UBaamSessionFlow::JoinRoomByCode(const FString& RoomCode)
{
	UBaamGameInstance* GI = GetBaamGameInstance();
	if (!GI)
	{
		SetPhase(EBaamSessionPhase::Failed, TEXT("GameInstance 가 UBaamGameInstance 가 아님"));
		return;
	}

	// 코드 정규화(공백 제거 + 대문자) — 호스트 광고값과 동일 규칙으로 매칭.
	PendingJoinCode = RoomCode.TrimStartAndEnd().ToUpper();
	if (PendingJoinCode.IsEmpty())
	{
		SetPhase(EBaamSessionPhase::Failed, TEXT("방 코드를 입력하세요"));
		return;
	}

	bWantsJoinAfterSearch = true;
	bWantsListAfterSearch = false; // 방찾기 검색과 동시 요청 시 마지막 의도만 유지
	SetPhase(EBaamSessionPhase::Searching, TEXT("세션 검색 중..."));
	GI->FindSessions(/*bLAN=*/false, /*MaxSearchResults=*/20);
}

// ── 방찾기(세션 브라우저) ──
void UBaamSessionFlow::RefreshSessionList(const FBaamSessionSearchFilter& Filter)
{
	UBaamGameInstance* GI = GetBaamGameInstance();
	if (!GI)
	{
		SetPhase(EBaamSessionPhase::Failed, TEXT("GameInstance 가 UBaamGameInstance 가 아님"));
		return;
	}

	PendingListFilter = Filter;
	bWantsListAfterSearch = true;
	bWantsJoinAfterSearch = false; // 코드 조인 검색과 동시 요청 시 마지막 의도만 유지
	SetPhase(EBaamSessionPhase::Searching, TEXT("세션 검색 중..."));
	// 클라 측 필터로 걸러질 것을 감안해 넉넉히 요청(최종 개수는 Filter.MaxResults 로 제한).
	GI->FindSessions(/*bLAN=*/false, /*MaxSearchResults=*/50);
}

void UBaamSessionFlow::JoinSessionByIndex(int32 ResultIndex)
{
	UBaamGameInstance* GI = GetBaamGameInstance();
	if (!GI)
	{
		SetPhase(EBaamSessionPhase::Failed, TEXT("GameInstance 가 UBaamGameInstance 가 아님"));
		return;
	}

	SetPhase(EBaamSessionPhase::Joining, TEXT("방 접속 중..."));
	// 인덱스 검증은 JoinFoundSession 에 위임 — 실패는 OnJoinSessionComplete(false) 로 돌아온다.
	GI->JoinFoundSession(ResultIndex);
}

void UBaamSessionFlow::BroadcastFilteredSessionList(bool bSearchSuccess)
{
	UBaamGameInstance* GI = GetBaamGameInstance();
	if (!bSearchSuccess || !GI)
	{
		SetPhase(EBaamSessionPhase::Failed, TEXT("세션 검색 실패"));
		OnSessionListReady.Broadcast(TArray<FBaamSessionSearchResult>());
		return;
	}

	const FString NameQuery = PendingListFilter.RoomNameQuery.TrimStartAndEnd();
	TArray<FBaamSessionSearchResult> Filtered;
	for (const FBaamSessionSearchResult& Item : GI->GetFoundSessionResults())
	{
		if (!NameQuery.IsEmpty() && !Item.RoomName.Contains(NameQuery))
		{
			continue;
		}
		if (PendingListFilter.bHideFull && Item.CurrentPlayers >= Item.MaxPlayers)
		{
			continue;
		}
		Filtered.Add(Item);
		if (PendingListFilter.MaxResults > 0 && Filtered.Num() >= PendingListFilter.MaxResults)
		{
			break;
		}
	}

	// 0개는 실패가 아니라 "빈 목록" — 위젯이 '방 없음' 표시를 담당한다.
	SetPhase(EBaamSessionPhase::Idle, FString::Printf(TEXT("방 %d개 검색됨"), Filtered.Num()));
	OnSessionListReady.Broadcast(Filtered);
}

// ── 공용 ──
void UBaamSessionFlow::LeaveSession()
{
	if (UBaamGameInstance* GI = GetBaamGameInstance())
	{
		GI->DestroyCurrentSession();
	}
	// 맵 이동이 없으므로 타이틀 복귀 트래블도 없다 — 파기 후 그대로 Phase.Lobby 로 되돌아간다.
	SetPhase(EBaamSessionPhase::Idle, TEXT("세션 종료"));
}

void UBaamSessionFlow::StartGame()
{
	UBaamGameInstance* GI = GetBaamGameInstance();
	if (!GI)
	{
		SetPhase(EBaamSessionPhase::Failed, TEXT("GameInstance 가 UBaamGameInstance 가 아님"));
		return;
	}

	if (!IsHost())
	{
		SetPhase(EBaamSessionPhase::Failed, TEXT("호스트만 게임을 시작할 수 있습니다"));
		return;
	}

	// 맵 이동 없이 제자리에서 시작한다(Prototype-Workflow.md §1.4).
	// 난입 차단은 StartMatch 안에서 처리된다.
	UBaamMatchStartComponent* Match = UBaamMatchStartComponent::Find(GetWorld());
	if (!Match)
	{
		SetPhase(EBaamSessionPhase::Failed, TEXT("게임모드에 MatchStart 컴포넌트가 없습니다"));
		return;
	}

	if (!Match->StartMatch())
	{
		SetPhase(EBaamSessionPhase::Failed, TEXT("판 시작 실패 — 인원 확인"));
		return;
	}

	SetPhase(EBaamSessionPhase::Hosting, TEXT("게임 시작"));
}

// ── UBaamGameInstance 콜백 ──
void UBaamSessionFlow::HandleCreateSessionComplete(bool bSuccess)
{
	SetPhase(bSuccess ? EBaamSessionPhase::Hosting : EBaamSessionPhase::Failed,
		bSuccess ? TEXT("방 생성 완료 — 리슨 시작") : TEXT("방 생성 실패"));
}

void UBaamSessionFlow::HandleFindSessionsComplete(bool bSuccess, int32 NumFound)
{
	// 방찾기 목록 갱신 경로 — 결과에 필터를 적용해 OnSessionListReady 로 통지.
	if (bWantsListAfterSearch)
	{
		bWantsListAfterSearch = false;
		BroadcastFilteredSessionList(bSuccess);
		return;
	}

	if (!bWantsJoinAfterSearch)
	{
		return; // 단순 검색(외부 직접 호출 등)은 무시.
	}
	bWantsJoinAfterSearch = false;

	UBaamGameInstance* GI = GetBaamGameInstance();
	if (!bSuccess || NumFound <= 0 || !GI)
	{
		SetPhase(EBaamSessionPhase::Failed, TEXT("접속 가능한 방이 없습니다"));
		return;
	}

	// 입력 코드와 광고된 방 코드가 일치하는 세션을 찾는다.
	int32 JoinIndex = INDEX_NONE;
	for (int32 i = 0; i < NumFound; ++i)
	{
		const FString FoundCode = GI->GetFoundSessionCode(i).TrimStartAndEnd().ToUpper();
		if (!FoundCode.IsEmpty() && FoundCode == PendingJoinCode)
		{
			JoinIndex = i;
			break;
		}
	}

	if (JoinIndex == INDEX_NONE)
	{
		SetPhase(EBaamSessionPhase::Failed, FString::Printf(TEXT("코드 '%s' 와 일치하는 방이 없습니다"), *PendingJoinCode));
		return;
	}

	SetPhase(EBaamSessionPhase::Joining, TEXT("방 접속 중..."));
	GI->JoinFoundSession(JoinIndex);
}

void UBaamSessionFlow::HandleJoinSessionComplete(bool bSuccess)
{
	SetPhase(bSuccess ? EBaamSessionPhase::Joined : EBaamSessionPhase::Failed,
		bSuccess ? TEXT("방 접속 완료") : TEXT("방 접속 실패"));
}
