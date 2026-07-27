// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/BangCardView.h"
#include "BangHandWidget.generated.h"

class UBangCardWidget;
class UPanelWidget;

/**
 * 손패 위젯. FBangCardView 목록을 받아 카드 위젯을 채운다.
 *
 * WBP 서브클래스에 아래가 있어야 한다:
 *   Box_Cards (HorizontalBox 등 PanelWidget, 필수)
 * 그리고 디테일 패널에서 CardWidgetClass 에 카드 WBP 를 지정해야 한다.
 *
 * ── 게임 로직과의 연결 지점 ──
 *   들어오는 방향: SetHand()          — 로직이 손패를 밀어넣는다
 *   나가는 방향:   OnCardPlayRequested — UI 가 사용 의도를 알린다
 * 로직 파트는 이 두 개만 알면 된다.
 */
UCLASS(Abstract, Blueprintable)
class BAAM_API UBangHandWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 손패 전체를 교체한다. 서버에서 손패가 복제될 때마다 호출하면 된다. */
	UFUNCTION(BlueprintCallable, Category = "Bang|Hand")
	void SetHand(const TArray<FBangCardView>& InCards);

	UFUNCTION(BlueprintCallable, Category = "Bang|Hand")
	void ClearHand();

	/** 낙관적 제거용. 서버 확인 후 SetHand 로 덮어쓰는 것이 정석이다. */
	UFUNCTION(BlueprintCallable, Category = "Bang|Hand")
	bool RemoveCardByInstanceId(int32 InstanceId);

	UFUNCTION(BlueprintPure, Category = "Bang|Hand")
	int32 GetCardCount() const { return CardWidgets.Num(); }

	UFUNCTION(BlueprintPure, Category = "Bang|Hand")
	UBangCardWidget* FindCardWidget(int32 InstanceId) const;

	/**
	 * 카드가 플레이 존에 드롭되어 사용 요청이 발생했다.
	 * PlayerController 가 여기에 바인딩해 ServerRequestPlayCard 를 호출하면 된다.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Bang|Hand")
	FOnBangCardPlayRequested OnCardPlayRequested;

	/** 플레이 존이 드롭을 받으면 호출한다. 직접 호출로 드래그 없이 테스트할 수도 있다. */
	UFUNCTION(BlueprintCallable, Category = "Bang|Hand")
	void NotifyCardPlayRequested(const FBangCardView& Card);

	/** 게임 로직 없이 UI 만 테스트하기 위한 더미 손패. */
	UFUNCTION(BlueprintCallable, Category = "Bang|Hand|Debug")
	void FillWithDebugCards(int32 Count = 5);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> Box_Cards;

	UPROPERTY(EditAnywhere, Category = "Bang|Hand")
	TSubclassOf<UBangCardWidget> CardWidgetClass;

	UPROPERTY(EditAnywhere, Category = "Bang|Hand")
	FMargin CardPadding = FMargin(4.f, 0.f);

private:
	UPROPERTY()
	TArray<TObjectPtr<UBangCardWidget>> CardWidgets;
};
