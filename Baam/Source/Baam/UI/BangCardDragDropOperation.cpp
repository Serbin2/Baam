// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/BangCardDragDropOperation.h"
#include "UI/BangCardWidget.h"

void UBangCardDragDropOperation::Drop(const FPointerEvent& PointerEvent)
{
	RestoreSourceWidget();
	Super::Drop(PointerEvent);
}

void UBangCardDragDropOperation::DragCancelled(const FPointerEvent& PointerEvent)
{
	RestoreSourceWidget();
	Super::DragCancelled(PointerEvent);
}

void UBangCardDragDropOperation::RestoreSourceWidget()
{
	if (UBangCardWidget* Source = SourceCardWidget.Get())
	{
		Source->SetBeingDragged(false);
	}
}
