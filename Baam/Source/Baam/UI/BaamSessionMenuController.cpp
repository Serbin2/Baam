#include "UI/BaamSessionMenuController.h"

#include "UI/BaamSessionMenuWidget.h"
#include "Network/BaamNetLog.h"
#include "Network/Session/BaamSessionFlow.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/PackageName.h"
#include "TimerManager.h"

void UBaamSessionMenuController::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadConfig();

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UBaamSessionMenuController::HandlePostLoadMap);
}

void UBaamSessionMenuController::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	Super::Deinitialize();
}

void UBaamSessionMenuController::Toggle()
{
	if (Menu)
	{
		Close();
	}
	else
	{
		Open();
	}
}

void UBaamSessionMenuController::Open()
{
	UGameInstance* GI = GetGameInstance();
	APlayerController* PC = GI ? GI->GetFirstLocalPlayerController() : nullptr;
	if (!PC)
	{
		UE_LOG(LogBaamNet, Warning, TEXT("[SessionMenu] 로컬 PlayerController 없음"));
		return;
	}

	TSubclassOf<UBaamSessionMenuWidget> Class = MenuWidgetClass.LoadSynchronous();
	if (!Class)
	{
		Class = UBaamSessionMenuWidget::StaticClass();
		UE_LOG(LogBaamNet, Warning, TEXT("[SessionMenu] '%s' 로드 실패 — C++ 기본 트리 사용"),
			*MenuWidgetClass.ToString());
	}

	Menu = CreateWidget<UBaamSessionMenuWidget>(PC, Class);
	if (!Menu)
	{
		UE_LOG(LogBaamNet, Warning, TEXT("[SessionMenu] 위젯 생성 실패"));
		return;
	}

	// 입력 모드/커서는 위젯의 NativeConstruct 가 잡는다.
	Menu->AddToViewport(100);
	bOpen = true;
	UE_LOG(LogBaamNet, Log, TEXT("[SessionMenu] 열림"));
}

void UBaamSessionMenuController::Close()
{
	// 입력 모드 복원은 위젯의 NativeDestruct 가 한다.
	if (Menu)
	{
		Menu->RemoveFromParent();
		Menu = nullptr;
	}
	bOpen = false;
	UE_LOG(LogBaamNet, Log, TEXT("[SessionMenu] 닫음"));
}

bool UBaamSessionMenuController::IsGameLevel(const UWorld* World) const
{
	const UBaamSessionFlow* Flow = GetGameInstance() ? GetGameInstance()->GetSubsystem<UBaamSessionFlow>() : nullptr;
	if (!Flow || !World)
	{
		return false;
	}

	const FString GameLevel = FPackageName::GetShortName(Flow->GetGameLevelName());
	const FString Loaded = FPackageName::GetShortName(World->GetOutermost()->GetName());
	return !GameLevel.IsEmpty() && Loaded.Equals(GameLevel, ESearchCase::IgnoreCase);
}

void UBaamSessionMenuController::HandlePostLoadMap(UWorld* LoadedWorld)
{
	// 이전 월드와 함께 파괴됐다.
	Menu = nullptr;

	if (!bOpen || !LoadedWorld)
	{
		return;
	}

	if (IsGameLevel(LoadedWorld))
	{
		bOpen = false;
		UE_LOG(LogBaamNet, Log, TEXT("[SessionMenu] 게임 레벨 진입 — 복구하지 않음"));
		return;
	}

	// 이 시점엔 로컬 PlayerController 가 아직 없을 수 있다.
	LoadedWorld->GetTimerManager().SetTimer(RestoreTimer,
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (bOpen && !Menu)
			{
				Open();
			}
		}), 0.2f, /*bLoop=*/false);
}
