// BANG! 세션 공용 타입 — UBaamGameInstance(OSS 호출부)와 UBaamSessionFlow(UI 계층)가 공유한다.

#pragma once

#include "CoreMinimal.h"
#include "BaamSessionTypes.generated.h"

// 세션 진행 단계 — UI 로딩 표시/에러 토스트용.
UENUM(BlueprintType)
enum class EBaamSessionPhase : uint8
{
	Idle,
	Creating,   // 방 생성 중
	Hosting,    // 호스트 성공(리슨 시작)
	Searching,  // 세션 검색 중
	Joining,    // 조인 시도 중
	Joined,     // 조인 성공(클라 합류)
	Failed      // 실패(메시지 동반)
};

// 방찾기 검색 결과 한 건 — ResultIndex 는 직전 검색 캐시 기준(새 검색 시 무효).
USTRUCT(BlueprintType)
struct FBaamSessionSearchResult
{
	GENERATED_BODY()

	// 광고된 방 이름(미광고 시 호스트 표시명 폴백).
	UPROPERTY(BlueprintReadOnly, Category = "Baam|Session")
	FString RoomName;

	// 호스트 표시명(온라인 서비스 계정 표시명).
	UPROPERTY(BlueprintReadOnly, Category = "Baam|Session")
	FString HostDisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Baam|Session")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Baam|Session")
	int32 MaxPlayers = 0;

	// 검색 응답 왕복 핑(ms).
	UPROPERTY(BlueprintReadOnly, Category = "Baam|Session")
	int32 PingMs = 0;

	// 검색 캐시 인덱스 — JoinSessionByIndex 에 넘긴다.
	UPROPERTY(BlueprintReadOnly, Category = "Baam|Session")
	int32 ResultIndex = INDEX_NONE;

	// 빈자리 있고 판이 진행 중이 아님.
	UPROPERTY(BlueprintReadOnly, Category = "Baam|Session")
	bool bCanJoin = false;
};

// 방찾기 검색 필터 — 문자열 필터는 결과 수신 후 클라 측 적용(OSS 쿼리는 부분검색 미지원).
USTRUCT(BlueprintType)
struct FBaamSessionSearchFilter
{
	GENERATED_BODY()

	// 방 이름 부분 문자열(대소문자 무시, 비우면 전체).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Baam|Session")
	FString RoomNameQuery;

	// true 면 빈자리 있는 방만.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Baam|Session")
	bool bHideFull = false;

	// 필터 적용 후 최대 결과 수(0 이하 = 무제한).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Baam|Session")
	int32 MaxResults = 20;
};

// ── 세션 이벤트 델리게이트 ──

// 세션 비동기 결과 통지(호스트/조인 버튼 UI 갱신용).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBaamSessionBoolEvent, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBaamFindSessionsEvent, bool, bSuccess, int32, NumFound);

// 세션 단계 변화 통지(로딩 스피너/에러 표시용).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBaamSessionPhaseChanged, EBaamSessionPhase, Phase, const FString&, Message);

// 방찾기 목록 갱신 완료 통지(필터 적용 결과, 실패 시 빈 배열).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaamSessionListReady, const TArray<FBaamSessionSearchResult>&, Results);
