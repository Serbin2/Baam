// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "UI/BangCardView.h"
#include "BangCardDragDropOperation.generated.h"

class UBangCardWidget;
class UBangHandWidget;

/** 손패에서 끌고 있는 카드 1장을 나르는 드래그 페이로드. */
UCLASS()
class BAAM_API UBangCardDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Bang|Drag")
	FBangCardView CardView;

	/** 드래그를 시작한 원본 카드 위젯. 드래그가 끝나면 하이라이트를 복구해야 한다. */
	UPROPERTY()
	TWeakObjectPtr<UBangCardWidget> SourceCardWidget;

	/** 사용 요청을 최종적으로 브로드캐스트할 핸드 위젯. */
	UPROPERTY()
	TWeakObjectPtr<UBangHandWidget> SourceHandWidget;

	virtual void Drop(const FPointerEvent& PointerEvent) override;
	virtual void DragCancelled(const FPointerEvent& PointerEvent) override;

private:
	/** 성공/취소 어느 쪽이든 원본 위젯의 드래그 표시를 되돌린다. */
	void RestoreSourceWidget();
};
