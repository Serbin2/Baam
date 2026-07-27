// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/BangCardWidget.h"
#include "UI/BangCardDragDropOperation.h"
#include "UI/BangHandWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"

void UBangCardWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshVisual();
}

void UBangCardWidget::SetCardView(const FBangCardView& InCardView)
{
	CardView = InCardView;
	RefreshVisual();
}

void UBangCardWidget::SetBeingDragged(bool bInBeingDragged)
{
	if (bBeingDragged == bInBeingDragged)
	{
		return;
	}
	bBeingDragged = bInBeingDragged;
	RefreshVisual();
}

void UBangCardWidget::RefreshVisual()
{
	if (Text_CardName)
	{
		Text_CardName->SetText(CardView.DisplayName);
	}
	if (Text_CardDescription)
	{
		Text_CardDescription->SetText(CardView.Description);
	}
	if (Border_Root)
	{
		const FLinearColor Tint =
			bBeingDragged        ? DraggedTint :
			!CardView.bPlayable  ? UnplayableTint :
			                       NormalTint;
		Border_Root->SetBrushColor(Tint);
	}
}

FReply UBangCardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 드래그 비주얼 복제본은 입력을 받지 않는다.
	if (bIsDragVisual)
	{
		return FReply::Unhandled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// 즉시 드래그하지 않고 임계 거리를 넘어야 드래그로 승격된다.
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UBangCardWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (bIsDragVisual || !CardView.IsValid())
	{
		return;
	}

	UBangCardDragDropOperation* Operation = Cast<UBangCardDragDropOperation>(
		UWidgetBlueprintLibrary::CreateDragDropOperation(UBangCardDragDropOperation::StaticClass()));
	if (!Operation)
	{
		return;
	}

	Operation->CardView         = CardView;
	Operation->SourceCardWidget = this;
	Operation->SourceHandWidget = OwningHand;
	Operation->Pivot            = EDragPivot::MouseDown;

	// 커서를 따라다닐 복제 위젯. DragVisualClass 가 없으면 자기 클래스를 쓴다.
	UClass* VisualClass = DragVisualClass ? DragVisualClass.Get() : GetClass();
	if (UBangCardWidget* Visual = CreateWidget<UBangCardWidget>(this, VisualClass))
	{
		Visual->bIsDragVisual = true;
		Visual->SetCardView(CardView);
		Operation->DefaultDragVisual = Visual;
	}

	SetBeingDragged(true);
	OutOperation = Operation;
}
