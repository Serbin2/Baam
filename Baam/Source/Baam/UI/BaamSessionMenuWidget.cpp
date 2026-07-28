#include "UI/BaamSessionMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Network/BaamNetLog.h"
#include "Network/Session/BaamSessionFlow.h"
#include "Network/Session/BaamSessionInterface.h"

namespace
{
	FString PhaseToText(EBaamSessionPhase Phase)
	{
		switch (Phase)
		{
		case EBaamSessionPhase::Creating:  return TEXT("방 생성 중");
		case EBaamSessionPhase::Hosting:   return TEXT("호스팅 중");
		case EBaamSessionPhase::Searching: return TEXT("검색 중");
		case EBaamSessionPhase::Joining:   return TEXT("접속 중");
		case EBaamSessionPhase::Joined:    return TEXT("접속 완료");
		case EBaamSessionPhase::Failed:    return TEXT("실패");
		default:                           return TEXT("대기");
		}
	}
}

void UBaamSessionRowHandler::HandleClicked()
{
	if (Menu)
	{
		Menu->JoinByIndex(ResultIndex);
	}
}

IBaamSessionInterface* UBaamSessionMenuWidget::GetSession() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UBaamSessionFlow>() : nullptr;
}

TSharedRef<SWidget> UBaamSessionMenuWidget::RebuildWidget()
{
	// WBP 를 새로 만들면 에디터가 빈 CanvasPanel 을 루트로 넣어둔다. 그래서 "루트 없음"이 아니라
	// "디자이너가 아직 아무것도 안 올림"을 기준으로 판단해야 C++ 트리가 뜬다.
	// 디자이너가 자식을 하나라도 넣는 순간 이쪽은 손을 뗀다.
	if (WidgetTree)
	{
		const UPanelWidget* RootPanel = Cast<UPanelWidget>(WidgetTree->RootWidget);
		const bool bDesignerTreeEmpty = !WidgetTree->RootWidget || (RootPanel && RootPanel->GetChildrenCount() == 0);
		if (bDesignerTreeEmpty)
		{
			BuildDefaultTree();
		}
	}
	return Super::RebuildWidget();
}

void UBaamSessionMenuWidget::BuildDefaultTree()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = Canvas;

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.75f));
	Panel->SetPadding(FMargin(12.f));

	if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel))
	{
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f));
		PanelSlot->SetAlignment(FVector2D::ZeroVector);
		PanelSlot->SetPosition(FVector2D(40.f, 40.f));
		// 위젯이 늘어나도 목록이 밀려나지 않도록 여유를 둔다.
		PanelSlot->SetSize(FVector2D(440.f, 780.f));
	}

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Content"));
	Panel->AddChild(Box);

	AddLabel(Box, TEXT("BAAM 세션"), 18);
	StatusText   = AddLabel(Box, TEXT("상태: 대기"));
	RoomCodeText = AddLabel(Box, TEXT("방 코드: -"));

	AddLabel(Box, TEXT("─ 호스트 ─"));
	RoomNameInput = AddInput(Box, TEXT("방 이름(비우면 자동)"), TEXT("RoomNameInput"));
	if (UButton* HostButton = AddButton(Box, TEXT("방 만들기"), TEXT("HostButton")))
	{
		HostButton->OnClicked.AddDynamic(this, &UBaamSessionMenuWidget::HandleHostClicked);
	}

	AddLabel(Box, TEXT("─ 참여 ─"));
	JoinCodeInput = AddInput(Box, TEXT("방 코드 6자"), TEXT("JoinCodeInput"));
	if (UButton* JoinButton = AddButton(Box, TEXT("코드로 참여"), TEXT("JoinCodeButton")))
	{
		JoinButton->OnClicked.AddDynamic(this, &UBaamSessionMenuWidget::HandleJoinCodeClicked);
	}
	if (UButton* RefreshButton = AddButton(Box, TEXT("방 목록 새로고침"), TEXT("RefreshButton")))
	{
		RefreshButton->OnClicked.AddDynamic(this, &UBaamSessionMenuWidget::HandleRefreshClicked);
	}

	// 목록은 Fill 로 두면 위아래 고정 위젯이 늘어날 때 높이 0 으로 찌그러진다.
	// SizeBox 로 최소 높이를 보장해 항상 보이게 한다.
	ResultList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ResultList"));

	USizeBox* ListBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ResultListBox"));
	ListBox->SetMinDesiredHeight(160.f);
	ListBox->AddChild(ResultList);

	if (UVerticalBoxSlot* ListSlot = Box->AddChildToVerticalBox(ListBox))
	{
		ListSlot->SetPadding(FMargin(0.f, 6.f));
		ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	AddLabel(Box, TEXT("─ 준비 ─"));
	ReadyText = AddLabel(Box, TEXT("Ready: -"));
	ReadyButton = AddButton(Box, TEXT("Ready 토글"), TEXT("ReadyButton"));
	if (ReadyButton)
	{
		ReadyButton->OnClicked.AddDynamic(this, &UBaamSessionMenuWidget::HandleReadyClicked);
	}

	StartGameButton = AddButton(Box, TEXT("게임 시작 (호스트)"), TEXT("StartGameButton"));
	if (StartGameButton)
	{
		StartGameButton->OnClicked.AddDynamic(this, &UBaamSessionMenuWidget::HandleStartGameClicked);
	}

	if (UButton* LeaveButton = AddButton(Box, TEXT("나가기"), TEXT("LeaveButton")))
	{
		LeaveButton->OnClicked.AddDynamic(this, &UBaamSessionMenuWidget::HandleLeaveClicked);
	}
}

UTextBlock* UBaamSessionMenuWidget::AddLabel(UVerticalBox* Parent, const FString& Text, int32 FontSize)
{
	UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Block->SetText(FText::FromString(Text));

	FSlateFontInfo Font = Block->GetFont();
	Font.Size = FontSize;
	Block->SetFont(Font);

	if (UVerticalBoxSlot* BoxSlot = Parent->AddChildToVerticalBox(Block))
	{
		BoxSlot->SetPadding(FMargin(0.f, 3.f));
	}
	return Block;
}

UButton* UBaamSessionMenuWidget::AddButton(UVerticalBox* Parent, const FString& Label, FName WidgetName)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), WidgetName);

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Text->SetText(FText::FromString(Label));
	Text->SetJustification(ETextJustify::Center);
	Button->AddChild(Text);

	if (UVerticalBoxSlot* BoxSlot = Parent->AddChildToVerticalBox(Button))
	{
		BoxSlot->SetPadding(FMargin(0.f, 4.f));
	}
	return Button;
}

UEditableTextBox* UBaamSessionMenuWidget::AddInput(UVerticalBox* Parent, const FString& HintText, FName WidgetName)
{
	UEditableTextBox* Input = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), WidgetName);
	Input->SetHintText(FText::FromString(HintText));

	if (UVerticalBoxSlot* BoxSlot = Parent->AddChildToVerticalBox(Input))
	{
		BoxSlot->SetPadding(FMargin(0.f, 3.f));
	}
	return Input;
}

void UBaamSessionMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 입력 모드는 이 위젯이 직접 챙긴다. Exec 에만 두면 다른 경로로 띄웠을 때
	// (BP, spawn_widget_in_pie 등) 마우스가 게임 입력에 남아 버튼을 못 누른다.
	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
		PC->SetShowMouseCursor(true);
	}

	if (IBaamSessionInterface* Session = GetSession())
	{
		Session->GetSessionPhaseChangedEvent().AddUniqueDynamic(this, &UBaamSessionMenuWidget::HandlePhaseChanged);
		Session->GetSessionListReadyEvent().AddUniqueDynamic(this, &UBaamSessionMenuWidget::HandleSessionListReady);
	}
	else
	{
		UE_LOG(LogBaamNet, Warning, TEXT("[SessionMenu] BaamSessionFlow 서브시스템 없음 — 버튼이 동작하지 않는다"));
	}

	RefreshHeader();
	RefreshReadyStatus();

	// PlayerState 복제에는 통지가 없어서 짧은 주기로 훑는다(러프 UI 한정).
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ReadyRefreshTimer, this, &UBaamSessionMenuWidget::RefreshReadyStatus, 0.5f, true);
	}
}

void UBaamSessionMenuWidget::RefreshReadyStatus()
{
	IBaamSessionInterface* Session = GetSession();
	if (!Session)
	{
		return;
	}

	const bool bHost = Session->IsHost();

	if (ReadyText)
	{
		int32 Ready = 0;
		int32 Guests = 0;
		Session->GetReadyCounts(Ready, Guests);

		ReadyText->SetText(FText::FromString(bHost
			? FString::Printf(TEXT("Ready: %d/%d (호스트 — 전원 준비 시 시작 가능)"), Ready, Guests)
			: FString::Printf(TEXT("Ready: %d/%d — 내 상태: %s"), Ready, Guests,
				Session->IsLocalReady() ? TEXT("준비됨") : TEXT("대기"))));
	}

	// 호스트에게는 Ready 버튼이, 게스트에게는 시작 버튼이 의미 없다.
	if (ReadyButton)
	{
		ReadyButton->SetVisibility(bHost ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (StartGameButton)
	{
		StartGameButton->SetVisibility(bHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UBaamSessionMenuWidget::HandleReadyClicked()
{
	if (IBaamSessionInterface* Session = GetSession())
	{
		Session->SetLocalReady(!Session->IsLocalReady());
		RefreshReadyStatus();
	}
}

void UBaamSessionMenuWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReadyRefreshTimer);
	}

	if (IBaamSessionInterface* Session = GetSession())
	{
		Session->GetSessionPhaseChangedEvent().RemoveDynamic(this, &UBaamSessionMenuWidget::HandlePhaseChanged);
		Session->GetSessionListReadyEvent().RemoveDynamic(this, &UBaamSessionMenuWidget::HandleSessionListReady);
	}

	// 열 때 바꿨으니 닫을 때 되돌린다.
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}

	Super::NativeDestruct();
}

void UBaamSessionMenuWidget::RefreshHeader()
{
	if (!RoomCodeText)
	{
		return;
	}

	IBaamSessionInterface* Session = GetSession();
	const FString Code = Session ? Session->GetRoomCode() : FString();
	RoomCodeText->SetText(FText::FromString(
		Code.IsEmpty() ? TEXT("방 코드: -") : FString::Printf(TEXT("방 코드: %s"), *Code)));
}

void UBaamSessionMenuWidget::HandleHostClicked()
{
	if (IBaamSessionInterface* Session = GetSession())
	{
		const FString RoomName = RoomNameInput ? RoomNameInput->GetText().ToString() : FString();
		Session->HostCreateRoom(RoomName);
	}
}

void UBaamSessionMenuWidget::HandleJoinCodeClicked()
{
	if (IBaamSessionInterface* Session = GetSession())
	{
		const FString Code = JoinCodeInput ? JoinCodeInput->GetText().ToString() : FString();
		Session->JoinRoomByCode(Code);
	}
}

void UBaamSessionMenuWidget::HandleRefreshClicked()
{
	if (IBaamSessionInterface* Session = GetSession())
	{
		Session->RefreshSessionList(FBaamSessionSearchFilter());
	}
}

void UBaamSessionMenuWidget::HandleStartGameClicked()
{
	if (IBaamSessionInterface* Session = GetSession())
	{
		Session->StartGame();
	}
}

void UBaamSessionMenuWidget::HandleLeaveClicked()
{
	if (IBaamSessionInterface* Session = GetSession())
	{
		Session->LeaveSession();
	}
}

void UBaamSessionMenuWidget::JoinByIndex(int32 ResultIndex)
{
	if (IBaamSessionInterface* Session = GetSession())
	{
		Session->JoinSessionByIndex(ResultIndex);
	}
}

void UBaamSessionMenuWidget::HandlePhaseChanged(EBaamSessionPhase Phase, const FString& Message)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(
			FString::Printf(TEXT("상태: %s — %s"), *PhaseToText(Phase), *Message)));
	}
	RefreshHeader();
}

void UBaamSessionMenuWidget::HandleSessionListReady(const TArray<FBaamSessionSearchResult>& Results)
{
	if (!ResultList)
	{
		return;
	}

	ResultList->ClearChildren();
	RowHandlers.Reset();

	if (Results.Num() == 0)
	{
		UTextBlock* Empty = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Empty->SetText(FText::FromString(TEXT("검색된 방 없음")));
		ResultList->AddChild(Empty);
		return;
	}

	for (const FBaamSessionSearchResult& Item : Results)
	{
		UBaamSessionRowHandler* Handler = NewObject<UBaamSessionRowHandler>(this);
		Handler->Menu = this;
		Handler->ResultIndex = Item.ResultIndex;
		RowHandlers.Add(Handler);

		UButton* Row = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		Row->OnClicked.AddDynamic(Handler, &UBaamSessionRowHandler::HandleClicked);

		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(FText::FromString(FString::Printf(TEXT("%s  (%d/%d)  %dms  %s"),
			*Item.RoomName, Item.CurrentPlayers, Item.MaxPlayers, Item.PingMs,
			Item.bCanJoin ? TEXT("[참여]") : TEXT("[입장 불가]"))));
		Text->SetJustification(ETextJustify::Center);
		Row->AddChild(Text);

		ResultList->AddChild(Row);
	}
}
