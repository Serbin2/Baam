// BANG! 세션 규약 — UI·게임플레이는 세션 구현을 모르고 이 인터페이스에만 의존한다.
// 획득: Cast<IBaamSessionInterface>(GetGameInstance()->GetSubsystem<UBaamSessionFlow>())

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Network/Session/BaamSessionTypes.h"
#include "BaamSessionInterface.generated.h"

UINTERFACE(MinimalAPI)
class UBaamSessionInterface : public UInterface
{
	GENERATED_BODY()
};

class BAAM_API IBaamSessionInterface
{
	GENERATED_BODY()

public:
	// ── 의도(intent) ──
	// 방 생성. RoomName 이 비면 호스트 닉네임 기반 기본값을 광고한다.
	virtual void HostCreateRoom(const FString& RoomName) = 0;

	// 방 코드 6자로 접속(검색 → 코드 매칭 → 조인).
	virtual void JoinRoomByCode(const FString& RoomCode) = 0;

	// 방 목록 갱신. 완료는 GetSessionListReadyEvent() 로 통지된다.
	virtual void RefreshSessionList(const FBaamSessionSearchFilter& Filter) = 0;

	// 직전 검색 결과의 ResultIndex 세션에 조인.
	virtual void JoinSessionByIndex(int32 ResultIndex) = 0;

	// 세션 파기(호스트/클라 공통).
	virtual void LeaveSession() = 0;

	// 게스트 전용. 로컬 플레이어의 Ready 를 서버에 올린다(호스트가 부르면 무시된다).
	virtual void SetLocalReady(bool bReady) = 0;

	// 호스트 전용. 접속한 전원을 게임 레벨로 함께 이동시킨다(심리스 ServerTravel).
	// 호스트를 제외한 전원이 Ready 여야 출발한다 — 아니면 Failed 단계로 떨어진다.
	// 이동 후에도 리슨을 유지하므로 세션과 방 코드는 그대로 살아 있다.
	virtual void StartGame() = 0;

	// ── 조회 ──
	// 이 인스턴스가 서버 권위인가.
	virtual bool IsHost() const = 0;

	// 현재 방 코드. 호스트가 아니면 빈 문자열.
	virtual FString GetRoomCode() const = 0;

	// 로컬 플레이어의 Ready 상태.
	virtual bool IsLocalReady() const = 0;

	// 호스트 제외 Ready 인원 / 게스트 총원. 로비 표시용(서버·클라 공통).
	virtual void GetReadyCounts(int32& OutReady, int32& OutGuests) const = 0;

	// ── 통지 ──
	// 단계 변화(Creating/Hosting/Searching/Joining/Joined/Failed). UI 가 여기에 바인딩한다.
	virtual FOnBaamSessionPhaseChanged& GetSessionPhaseChangedEvent() = 0;

	// 방 목록 갱신 완료(필터 적용 결과, 실패 시 빈 배열).
	virtual FOnBaamSessionListReady& GetSessionListReadyEvent() = 0;
};
