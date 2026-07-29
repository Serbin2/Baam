// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/BangCardView.h"
#include "BangCardWidget.generated.h"

class UTextBlock;
class UBorder;
class UBangHandWidget;

/**
 * 카드 1장 위젯. 이름 + 설명을 표시하고, 좌클릭 드래그를 시작한다.
 *
 * WBP 서브클래스에 아래 이름의 위젯이 있어야 한다:
 *   Text_CardName        (TextBlock, 필수)
 *   Text_CardDescription (TextBlock, 필수)
 *   Border_Root          (Border, 선택 - 드래그 중 디밍에 사용)
 */
UCLASS(Abstract, Blueprintable)
class BAAM_API UBangCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Bang|Card")
	void SetCardView(const FBangCardView& InCardView);

	UFUNCTION(BlueprintPure, Category = "Bang|Card")
	const FBangCardView& GetCardView() const { return CardView; }

	UFUNCTION(BlueprintPure, Category = "Bang|Card")
	int32 GetInstanceId() const { return CardView.InstanceId; }

	/** 이 카드를 소유한 핸드 위젯. 핸드가 카드를 만들 때 설정한다. */
	// 정의는 .cpp 에 있다 - TWeakObjectPtr 대입은 완전한 타입을 요구한다.
	void SetOwningHand(UBangHandWidget* InHand);

	/** 드래그 중인 원본 카드를 흐리게 표시한다. 드롭/취소 시 false 로 복구된다. */
	UFUNCTION(BlueprintCallable, Category = "Bang|Card")
	void SetBeingDragged(bool bInBeingDragged);

	/**
	 * 드래그를 잠근다. 대상 좌석 선택을 기다리는 동안 카드를 또 던지는 것을 막는다.
	 * 잠금은 손패 위젯이 일괄로 건다 — 개별 카드에서 직접 부르지 말 것.
	 */
	UFUNCTION(BlueprintCallable, Category = "Bang|Card")
	void SetDragLocked(bool bInLocked);

	UFUNCTION(BlueprintPure, Category = "Bang|Card")
	bool IsDragLocked() const { return bDragLocked; }

	/**
	 * true 면 드래그 비주얼용 복제본이므로 입력을 받지 않는다.
	 * 복제본이 다시 드래그를 시작하는 무한 재귀를 막는다.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Bang|Card")
	bool bIsDragVisual = false;

protected:
	virtual void NativePreConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

	/** 표시할 카드 데이터. 디자이너에서 값을 넣으면 프리뷰에 그대로 반영된다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bang|Card", meta = (ExposeOnSpawn = true))
	FBangCardView CardView;

	/** 비워두면 자기 자신의 클래스로 드래그 비주얼을 만든다. */
	UPROPERTY(EditAnywhere, Category = "Bang|Card")
	TSubclassOf<UBangCardWidget> DragVisualClass;

	UPROPERTY(EditAnywhere, Category = "Bang|Card")
	FLinearColor NormalTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Bang|Card")
	FLinearColor DraggedTint = FLinearColor(1.f, 1.f, 1.f, 0.25f);

	/** 사용할 수 없는 카드(bPlayable == false)의 색조. */
	UPROPERTY(EditAnywhere, Category = "Bang|Card")
	FLinearColor UnplayableTint = FLinearColor(0.4f, 0.4f, 0.4f, 1.f);

	/** 대상 선택 대기 등으로 손패 전체가 잠긴 상태의 색조. */
	UPROPERTY(EditAnywhere, Category = "Bang|Card")
	FLinearColor LockedTint = FLinearColor(0.3f, 0.3f, 0.35f, 1.f);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CardName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CardDescription;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_Root;

private:
	void RefreshVisual();

	UPROPERTY()
	TWeakObjectPtr<UBangHandWidget> OwningHand;

	bool bBeingDragged = false;
	bool bDragLocked = false;
};
