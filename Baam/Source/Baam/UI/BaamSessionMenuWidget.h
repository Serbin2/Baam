// BANG! 세션 러프 UI — 방 생성 / 코드 조인 / 방 목록을 최소 위젯으로 노출한다.
// 디자인 애셋 없이 C++ 에서 UMG 트리를 직접 만든다. WBP 를 이 클래스로 리페어런트해도
// 디자이너 트리가 비어 있는 동안은 이 C++ 트리가 뜨고, 디자이너가 위젯을 하나라도 올리면
// 그쪽이 우선한다(판정은 RebuildWidget). 로직은 어느 쪽이든 그대로 쓰인다.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Network/Session/BaamSessionTypes.h"
#include "BaamSessionMenuWidget.generated.h"

class IBaamSessionInterface;
class UBaamSessionMenuWidget;
class UButton;
class UEditableTextBox;
class UScrollBox;
class UTextBlock;
class UVerticalBox;

// 방 목록 한 줄의 클릭 대상. UButton::OnClicked 가 인자를 넘기지 못해 행마다 하나씩 둔다.
UCLASS()
class UBaamSessionRowHandler : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UBaamSessionMenuWidget> Menu;

	UPROPERTY()
	int32 ResultIndex = INDEX_NONE;

	UFUNCTION()
	void HandleClicked();
};

UCLASS()
class BAAM_API UBaamSessionMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 목록 행에서 호출된다(UBaamSessionRowHandler).
	void JoinByIndex(int32 ResultIndex);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	IBaamSessionInterface* GetSession() const;

	// 디자이너 트리가 없을 때만 호출된다.
	void BuildDefaultTree();
	UTextBlock* AddLabel(UVerticalBox* Parent, const FString& Text, int32 FontSize = 12);
	UButton* AddButton(UVerticalBox* Parent, const FString& Label, FName WidgetName);
	UEditableTextBox* AddInput(UVerticalBox* Parent, const FString& HintText, FName WidgetName);

	void RefreshHeader();

	// Ready 는 PlayerState 복제라 별도 통지가 없다 — 로비에서만 짧은 주기로 갱신한다.
	void RefreshReadyStatus();

	UFUNCTION()
	void HandleReadyClicked();

	UFUNCTION()
	void HandleHostClicked();
	UFUNCTION()
	void HandleJoinCodeClicked();
	UFUNCTION()
	void HandleRefreshClicked();
	UFUNCTION()
	void HandleStartGameClicked();
	UFUNCTION()
	void HandleLeaveClicked();

	UFUNCTION()
	void HandlePhaseChanged(EBaamSessionPhase Phase, const FString& Message);
	UFUNCTION()
	void HandleSessionListReady(const TArray<FBaamSessionSearchResult>& Results);

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RoomCodeText;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> RoomNameInput;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> JoinCodeInput;

	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> ResultList;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ReadyText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ReadyButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> StartGameButton;

	FTimerHandle ReadyRefreshTimer;

	// 목록 행 핸들러 — 목록을 다시 만들 때까지 살아 있어야 클릭이 동작한다.
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBaamSessionRowHandler>> RowHandlers;
};
