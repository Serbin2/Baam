// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/BangCardDragDropOperation.h"
#include "UI/BangCardWidget.h"

void UBangCardDragDropOperation::Drop_Implementation(const FPointerEvent& PointerEvent)
{
	RestoreSourceWidget();
	Super::Drop_Implementation(PointerEvent);
}

void UBangCardDragDropOperation::DragCancelled_Implementation(const FPointerEvent& PointerEvent)
{
	RestoreSourceWidget();
	Super::DragCancelled_Implementation(PointerEvent);
}

void UBangCardDragDropOperation::RestoreSourceWidget()
{
	if (UBangCardWidget* Source = SourceCardWidget.Get())
	{
		Source->SetBeingDragged(false);
	}
}
