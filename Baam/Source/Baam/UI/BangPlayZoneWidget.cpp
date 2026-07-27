// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/BangPlayZoneWidget.h"
#include "UI/BangCardDragDropOperation.h"
#include "UI/BangHandWidget.h"
#include "Components/Border.h"

void UBangPlayZoneWidget::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);

	if (const UBangCardDragDropOperation* CardOp = Cast<UBangCardDragDropOperation>(InOperation))
	{
		SetHighlight(CardOp->CardView.bPlayable ? HoverColor : RejectColor);
	}
}

void UBangPlayZoneWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	SetHighlight(IdleColor);
}

bool UBangPlayZoneWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	SetHighlight(IdleColor);

	UBangCardDragDropOperation* CardOp = Cast<UBangCardDragDropOperation>(InOperation);
	if (!CardOp || !CardOp->CardView.IsValid())
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	// 클라이언트 힌트로 명백히 불가한 카드는 요청조차 보내지 않는다.
	// 최종 판정은 언제나 서버가 한다.
	if (!CardOp->CardView.bPlayable)
	{
		return true;
	}

	// 정식 경로는 원본 핸드 위젯이다. 플레이 존 델리게이트는 보조 훅.
	if (UBangHandWidget* Hand = CardOp->SourceHandWidget.Get())
	{
		Hand->NotifyCardPlayRequested(CardOp->CardView);
	}
	OnCardDropped.Broadcast(CardOp->CardView);

	return true;
}

void UBangPlayZoneWidget::SetHighlight(const FLinearColor& Color)
{
	if (Border_Highlight)
	{
		Border_Highlight->SetBrushColor(Color);
	}
}
