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
#include "UI/BaamSessionMenuController.h"
#include "Game/BaamGameState.h"
#include "Game/BaamGameplayTags.h"

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
	if (WidgetTree)
	{
		BuildMissingWidgets();
	}
	return Super::RebuildWidget();
}

bool UBaamSessionMenuWidget::HasAllWidgets() const
{
	return StatusText && RoomCodeText && RoomNameInput && HostButton && JoinCodeInput
		&& JoinCodeButton && RefreshButton && ResultList && LobbyText
		&& ReadyButton && StartGameButton && LeaveButton;
}

void UBaamSessionMenuWidget::BuildMissingWidgets()
{
	if (HasAllWidgets())
	{
		return;
	}

	UPanelWidget* RootPanel = Cast<UPanelWidget>(WidgetTree->RootWidget);
	if (!RootPanel)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = Canvas;
		RootPanel = Canvas;
	}
	const bool bDesignerHasWidgets = RootPanel->GetChildrenCount() > 0;

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FallbackPanel"));
	Panel->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.75f));
	Panel->SetPadding(FMargin(12.f));

	if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(RootPanel->AddChild(Panel)))
	{
		// 디자이너 배치를 모르니, 위젯이 이미 있으면 화면 오른쪽 끝에 붙여 충돌을 피한다.
		PanelSlot->SetAnchors(bDesignerHasWidgets ? FAnchors(1.f, 0.f) : FAnchors(0.f, 0.f));
		PanelSlot->SetAlignment(bDesignerHasWidgets ? FVector2D(1.f, 0.f) : FVector2D::ZeroVector);
		PanelSlot->SetPosition(FVector2D(bDesignerHasWidgets ? -40.f : 40.f, 40.f));
		// 위젯이 늘어나도 목록이 밀려나지 않도록 여유를 둔다.
		PanelSlot->SetSize(FVector2D(440.f, bDesignerHasWidgets ? 420.f : 780.f));
	}

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("FallbackContent"));
	Panel->AddChild(Box);

	if (!StatusText)   { StatusText   = AddLabel(Box, TEXT("상태: 대기")); }
	if (!RoomCodeText) { RoomCodeText = AddLabel(Box, TEXT("방 코드: -")); }

	if (!RoomNameInput || !HostButton) { AddLabel(Box, TEXT("─ 호스트 ─")); }
	if (!RoomNameInput) { RoomNameInput = AddInput(Box, TEXT("방 이름(비우면 자동)"), TEXT("RoomNameInput")); }
	if (!HostButton)    { HostButton    = AddButton(Box, TEXT("방 만들기"), TEXT("HostButton")); }

	if (!JoinCodeInput || !JoinCodeButton || !RefreshButton) { AddLabel(Box, TEXT("─ 참여 ─")); }
	if (!JoinCodeInput)  { JoinCodeInput  = AddInput(Box, TEXT("방 코드 6자"), TEXT("JoinCodeInput")); }
	if (!JoinCodeButton) { JoinCodeButton = AddButton(Box, TEXT("코드로 참여"), TEXT("JoinCodeButton")); }
	if (!RefreshButton)  { RefreshButton  = AddButton(Box, TEXT("방 목록 새로고침"), TEXT("RefreshButton")); }

	if (!ResultList)
	{
		// 목록은 Fill 로 두면 위아래 고정 위젯이 늘어날 때 높이 0 으로 찌그러진다.
		// SizeBox 로 최소 높이를 보장해 항상 보이게 한다.
		ResultList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ResultList"));

		// 폭을 안 잡아주면 스크롤박스가 제 마음대로 커져 패널 밖으로 삐져나간다.
		USizeBox* ListBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ResultListBox"));
		ListBox->SetWidthOverride(400.f);
		ListBox->SetHeightOverride(180.f);
		ListBox->AddChild(ResultList);

		if (UVerticalBoxSlot* ListSlot = Box->AddChildToVerticalBox(ListBox))
		{
			ListSlot->SetPadding(FMargin(0.f, 6.f));
			ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}

	if (!LobbyText || !ReadyButton || !StartGameButton) { AddLabel(Box, TEXT("─ 진행 ─")); }
	if (!LobbyText)       { LobbyText       = AddLabel(Box, TEXT("대기 중")); }
	if (!ReadyButton)     { ReadyButton     = AddButton(Box, TEXT("준비"), TEXT("ReadyButton")); }
	if (!StartGameButton) { StartGameButton = AddButton(Box, TEXT("게임 시작 (호스트)"), TEXT("StartGameButton")); }
	if (!LeaveButton)     { LeaveButton     = AddButton(Box, TEXT("나가기"), TEXT("LeaveButton")); }
}

void UBaamSessionMenuWidget::BindButtonEvents()
{
	// 디자이너가 버튼만 올렸으면 라벨 텍스트는 버튼의 첫 자식에서 찾는다.
	if (!ReadyButtonText && ReadyButton)
	{
		ReadyButtonText = Cast<UTextBlock>(ReadyButton->GetChildAt(0));
	}

	if (HostButton)      { HostButton->OnClicked.AddUniqueDynamic(this, &UBaamSessionMenuWidget::HandleHostClicked); }
	if (JoinCodeButton)  { JoinCodeButton->OnClicked.AddUniqueDynamic(this, &UBaamSessionMenuWidget::HandleJoinCodeClicked); }
	if (RefreshButton)   { RefreshButton->OnClicked.AddUniqueDynamic(this, &UBaamSessionMenuWidget::HandleRefreshClicked); }
	if (ReadyButton)     { ReadyButton->OnClicked.AddUniqueDynamic(this, &UBaamSessionMenuWidget::HandleReadyClicked); }
	if (StartGameButton) { StartGameButton->OnClicked.AddUniqueDynamic(this, &UBaamSessionMenuWidget::HandleStartGameClicked); }
	if (LeaveButton)     { LeaveButton->OnClicked.AddUniqueDynamic(this, &UBaamSessionMenuWidget::HandleLeaveClicked); }
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

	BindButtonEvents();

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
	RefreshLobbyStatus();

	// 접속 인원은 통지가 없어 짧은 주기로 훑는다(러프 UI 한정).
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			LobbyRefreshTimer, this, &UBaamSessionMenuWidget::RefreshLobbyStatus, 0.5f, true);
	}
}

void UBaamSessionMenuWidget::RefreshLobbyStatus()
{
	IBaamSessionInterface* Session = GetSession();
	if (!Session)
	{
		return;
	}

	const bool bHost = Session->IsHost();
	const FBaamLobbyStatus Status = Session->GetLobbyStatus();

	FString Reason;
	const bool bCanStart = Session->CanStartGame(Reason);

	if (LobbyText)
	{
		// 최소 인원은 서버 설정이라 클라에서는 0 으로 온다 — 그때는 인원/준비만 보여준다.
		const FString Need = (Status.RequiredPlayers > 0)
			? FString::Printf(TEXT("/%d"), Status.RequiredPlayers) : FString();

		LobbyText->SetText(FText::FromString(Status.bMatchStarted
			? TEXT("판 진행 중")
			: FString::Printf(TEXT("접속 %d%s명 · 준비 %d명"),
				Status.CurrentPlayers, *Need, Status.ReadyPlayers)));
	}

	// 판이 시작되면(Phase.Play) 로비 UI 는 물러난다. PhaseTag 는 전원에게 복제되므로
	// 호스트·클라 모두 각자 닫힌다. 위젯이 자기 타이머 안에서 파괴되지 않도록 한 프레임 미룬다.
	const ABaamGameState* GS = GetWorld() ? GetWorld()->GetGameState<ABaamGameState>() : nullptr;
	if (!bClosedOnMatchStart && GS && GS->GetPhaseTag() == Bang::Phase::Play.GetTag())
	{
		bClosedOnMatchStart = true;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(CloseOnStartTimer,
				FTimerDelegate::CreateWeakLambda(this, [this]()
				{
					UGameInstance* GI = GetGameInstance();
					if (UBaamSessionMenuController* Menu =
						GI ? GI->GetSubsystem<UBaamSessionMenuController>() : nullptr)
					{
						UE_LOG(LogBaamNet, Log, TEXT("[SessionMenu] 판 시작 — 로비 UI 닫음"));
						Menu->Close();
					}
				}), 0.f, /*bLoop=*/false);
		}
	}

	// 준비는 참가자만 — 호스트는 항상 준비 상태다.
	if (ReadyButton)
	{
		ReadyButton->SetVisibility(bHost ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		ReadyButton->SetIsEnabled(!Status.bMatchStarted);
	}
	if (ReadyButtonText)
	{
		ReadyButtonText->SetText(FText::FromString(
			Session->IsLocalReady() ? TEXT("준비 해제") : TEXT("준비")));
	}

	// 시작은 호스트만, 전원 준비가 끝나야 눌린다.
	if (StartGameButton)
	{
		StartGameButton->SetVisibility(bHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		StartGameButton->SetIsEnabled(bCanStart);
	}
}

void UBaamSessionMenuWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LobbyRefreshTimer);
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

void UBaamSessionMenuWidget::HandleReadyClicked()
{
	if (IBaamSessionInterface* Session = GetSession())
	{
		Session->SetLocalReady(!Session->IsLocalReady());
		RefreshLobbyStatus();
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
