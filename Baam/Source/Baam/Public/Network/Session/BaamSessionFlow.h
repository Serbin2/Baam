// BANG! 세션 플로우 — UI 와 UBaamGameInstance 사이의 단일 바인딩 계층.
// UI 는 IBaamSessionInterface 로만 접근한다. 레벨 트래블은 다루지 않는다.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Network/Session/BaamSessionTypes.h"
#include "Network/Session/BaamSessionInterface.h"
#include "BaamSessionFlow.generated.h"

class UBaamGameInstance;

UCLASS(Config = Game)
class BAAM_API UBaamSessionFlow : public UGameInstanceSubsystem, public IBaamSessionInterface
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ── IBaamSessionInterface ──
	UFUNCTION(BlueprintCallable, Category = "Baam|Session")
	virtual void HostCreateRoom(const FString& RoomName) override;

	UFUNCTION(BlueprintCallable, Category = "Baam|Session")
	virtual void JoinRoomByCode(const FString& RoomCode) override;

	UFUNCTION(BlueprintCallable, Category = "Baam|Session")
	virtual void RefreshSessionList(const FBaamSessionSearchFilter& Filter) override;

	UFUNCTION(BlueprintCallable, Category = "Baam|Session")
	virtual void JoinSessionByIndex(int32 ResultIndex) override;

	UFUNCTION(BlueprintCallable, Category = "Baam|Session")
	virtual void LeaveSession() override;

	UFUNCTION(BlueprintPure, Category = "Baam|Session")
	virtual bool IsHost() const override;

	UFUNCTION(BlueprintPure, Category = "Baam|Session")
	virtual FString GetRoomCode() const override;

	virtual FOnBaamSessionPhaseChanged& GetSessionPhaseChangedEvent() override { return OnSessionPhaseChanged; }
	virtual FOnBaamSessionListReady& GetSessionListReadyEvent() override { return OnSessionListReady; }

	// ── UI 단계 통지(BP 바인딩용 — C++ 은 인터페이스의 Get*Event 로 접근) ──
	UPROPERTY(BlueprintAssignable, Category = "Baam|Session")
	FOnBaamSessionPhaseChanged OnSessionPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Baam|Session")
	FOnBaamSessionListReady OnSessionListReady;

protected:
	// 방 정원(BANG! 기본판 4~7인). DefaultGame.ini [/Script/Baam.BaamSessionFlow] 로 덮어쓸 수 있다.
	// 실제 클램프는 GameInstance 에서 ABaamGameMode::Min/MaxPlayers 로 한 번 더.
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Baam|Session")
	int32 MaxPlayers = 7;

private:
	UBaamGameInstance* GetBaamGameInstance() const;
	void BindGameInstanceEvents();
	void UnbindGameInstanceEvents();

	// UBaamGameInstance 세션 콜백.
	UFUNCTION()
	void HandleCreateSessionComplete(bool bSuccess);
	UFUNCTION()
	void HandleFindSessionsComplete(bool bSuccess, int32 NumFound);
	UFUNCTION()
	void HandleJoinSessionComplete(bool bSuccess);

	// 단계 통지 헬퍼.
	void SetPhase(EBaamSessionPhase Phase, const FString& Message = FString());

	// 검색 완료 → PendingListFilter 적용 → OnSessionListReady 브로드캐스트.
	void BroadcastFilteredSessionList(bool bSearchSuccess);

	// 검색 후행 의도 두 갈래(코드 조인 / 목록 갱신) — 겹치면 마지막 의도만 유지한다.
	// 조인 대기 중 코드(검색 완료 후 매칭에 사용).
	FString PendingJoinCode;
	bool bWantsJoinAfterSearch = false;

	// 방찾기 목록 갱신 대기 필터(검색 완료 후 클라 측 적용).
	FBaamSessionSearchFilter PendingListFilter;
	bool bWantsListAfterSearch = false;
};
