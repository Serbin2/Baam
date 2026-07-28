// BANG! 세션 메뉴 수명 관리 — 토글과 트래블 복구를 맡는다.
// 트래블은 뷰포트 위젯을 파괴하므로 열려 있었는지 기억했다가 맵 로드 후 되살린다.
// 게임 레벨에 도착하면 되살리지 않는다(메뉴는 로비 전용).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BaamSessionMenuController.generated.h"

class UBaamSessionMenuWidget;

UCLASS(Config = Game)
class BAAM_API UBaamSessionMenuController : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Baam|UI")
	void Toggle();

	UFUNCTION(BlueprintCallable, Category = "Baam|UI")
	void Open();

	UFUNCTION(BlueprintCallable, Category = "Baam|UI")
	void Close();

protected:
	// 지정한 WBP 를 띄운다. 로드 실패 시 C++ 기본 트리로 폴백.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Baam|UI")
	TSoftClassPtr<UBaamSessionMenuWidget> MenuWidgetClass =
		TSoftClassPtr<UBaamSessionMenuWidget>(FSoftObjectPath(TEXT("/Game/Network/UI/WBP_BaamSessionMenu.WBP_BaamSessionMenu_C")));

private:
	void HandlePostLoadMap(UWorld* LoadedWorld);

	// 게임 레벨이면 메뉴를 되살리지 않는다.
	bool IsGameLevel(const UWorld* World) const;

	UPROPERTY(Transient)
	TObjectPtr<UBaamSessionMenuWidget> Menu;

	bool bOpen = false;

	FDelegateHandle PostLoadMapHandle;
	FTimerHandle RestoreTimer;
};
