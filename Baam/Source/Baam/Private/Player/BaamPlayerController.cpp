#include "Player/BaamPlayerController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GameplayAbilityTypes.h"   // FGameplayEventData

#include "Game/BaamCardLog.h"
#include "Game/BaamCardType.h"                // FBaamCardInstance
#include "Game/BaamDebug.h"
#include "Game/BaamCardViewLibrary.h"
#include "Game/BaamDataSubsystem.h"           // GetCardRow
#include "Game/BaamGameDataTypes.h"           // FBaamCardRow
#include "Game/BaamPlayerState.h"
#include "GameFramework/GameStateBase.h"      // PlayerArray
#include "UI/BangHandWidget.h"

ABaamPlayerController::ABaamPlayerController()
{
	// GA 는 AbilityByCardId(BP 디폴트에서 채움)로 조회한다. 하드코딩 폴백은 없앴다
	// (엉뚱한 GA 로 조용히 새는 버그 방지).
}

// ======================================================================================
//  손패 UI 배선
//
//  위젯 생성과 PlayerState 복제 도착은 순서가 보장되지 않는다.
//  클라에서는 보통 위젯이 먼저 만들어지고 PlayerState 가 나중에 오지만, 리슨서버 호스트는
//  반대다. 그래서 양쪽 진입점에서 모두 TryBindHandDelegate() 를 부른다.
// ======================================================================================

void ABaamPlayerController::SetHandWidget(UBangHandWidget* InHandWidget)
{
	HandWidget = InHandWidget;
	TryBindHandDelegate();
	RefreshHandWidget();
}

void ABaamPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// 클라: PlayerState 가 이제야 도착했다. 여기서 구독하지 않으면 첫 손패를 놓친다.
	TryBindHandDelegate();
	RefreshHandWidget();
}

void ABaamPlayerController::TryBindHandDelegate()
{
	ABaamPlayerState* PS = GetPlayerState<ABaamPlayerState>();
	if (!PS || BoundPlayerState.Get() == PS)
	{
		return;   // 아직 없거나 이미 같은 PlayerState 에 구독 중
	}

	// PlayerState 가 갈아끼워진 경우(재접속 등) 이전 구독을 정리한다.
	if (ABaamPlayerState* Previous = BoundPlayerState.Get())
	{
		Previous->OnHandChanged.RemoveDynamic(this, &ABaamPlayerController::HandleHandChanged);
	}

	PS->OnHandChanged.AddDynamic(this, &ABaamPlayerController::HandleHandChanged);
	BoundPlayerState = PS;

	UE_LOG(LogBaamCard, Verbose, TEXT("[PC] 손패 델리게이트 구독 — 좌석 %d"), PS->GetSeatIndex());
}

void ABaamPlayerController::HandleHandChanged()
{
	RefreshHandWidget();
}

void ABaamPlayerController::RefreshHandWidget()
{
	if (!HandWidget)
	{
		return;   // HUD 가 아직 SetHandWidget 을 부르지 않았다. 부르면 그때 다시 그린다.
	}

	const ABaamPlayerState* PS = GetPlayerState<ABaamPlayerState>();
	if (!PS)
	{
		HandWidget->ClearHand();
		return;
	}

	// 게임 로직 타입 → UI 뷰모델. 이 한 줄이 로직/UI 경계다.
	HandWidget->SetHand(UBaamCardViewLibrary::MakeHandViews(PS));

	// TODO(5.7): HandWidget->OnCardPlayRequested 를 RequestPlayCard 로 연결.
	//            bNeedsTarget 분기(대상 좌석 지정)가 필요해 STEP 5 에서 함께 처리한다.
}

void ABaamPlayerController::RequestPlayCard(FGameplayTag CardId, int32 TargetSeat)
{
	// 클라(요청 발신) 확인용. 이 줄이 안 뜨면 UI 바인딩이 RequestPlayCard 를 안 부르는 것.
	BaamDebug::Screen(
		FString::Printf(TEXT("[클라] 카드요청 보냄  %s  TargetSeat=%d  (→ 서버)"),
			CardId.IsValid() ? *CardId.ToString() : TEXT("(무효)"), TargetSeat),
		FColor(180, 180, 255), /*Time=*/5.f);

	// 로컬에서는 검증하지 않는다 (클라 표시는 힌트일 뿐, 권위는 서버). 그대로 중계.
	ServerRequestPlayCard(CardId, TargetSeat);
}

bool ABaamPlayerController::ServerRequestPlayCard_Validate(FGameplayTag CardId, int32 TargetSeat)
{
	// 형식 검증만 (악의적 페이로드 차단). 규칙 검증은 _Implementation 에서.
	return CardId.IsValid();
}

void ABaamPlayerController::ServerRequestPlayCard_Implementation(FGameplayTag CardId, int32 TargetSeat)
{
	HandlePlayCard(CardId, TargetSeat);
}

void ABaamPlayerController::HandlePlayCard(FGameplayTag CardId, int32 TargetSeat)
{
	// TODO(2단계): GameState 규칙 검증 — 내 턴? Phase.Play? 사거리? BangLimit?
	UE_LOG(LogTemp, Log, TEXT("[Bang] ServerRequestPlayCard 수신 — PC=%s Card=%s TargetSeat=%d"),
		*GetName(), *CardId.ToString(), TargetSeat);

	// 시전자의 ASC 를 얻는다 (현재 ASC 는 캐릭터 소유).
	UAbilitySystemComponent* ASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetPawn());
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bang] HandlePlayCard: 시전자 ASC 없음 (Pawn=%s)."),
			GetPawn() ? *GetPawn()->GetName() : TEXT("null"));
		return;
	}

	// ── 손패에 그 카드가 있는지 확인 (테스트 편의상 없어도 진행, 표시만 한다) ──
	// TODO(2단계): 없으면 거부. + 실제 사용 시 RemoveCardFromHand 로 첫 매칭 인스턴스를 뺀다.
	bool bInHand = false;
	if (const ABaamPlayerState* PS = GetPlayerState<ABaamPlayerState>())
	{
		for (const FBaamCardInstance& Card : PS->GetHand())
		{
			if (Card.CardId == CardId.GetTagName())
			{
				bInHand = true;
				break;
			}
		}
	}

	// ── CardId 태그 → GA 클래스 (맵을 "카드 종류"로 직접 조회) ──
	//  ⚠️ 맵(AbilityByCardId)의 키는 카드 태그(Card.Id.*)다 — 유저 설정에 맞춰 카드 기준 조회.
	//     (메커니즘 태그 Ability.* 로 거치지 않는다.)
	const TSubclassOf<UGameplayAbility>* Found = AbilityByCardId.Find(CardId);
	if (!Found || !*Found)
	{
		// 진단: 런타임에 맵에 실제로 뭐가 들었는지 그대로 찍는다.
		//   맵(0) 비어있음   → 빌드/BP 미반영
		//   키는 있는데 값 None → BP 에서 GA 클래스를 안 골랐음
		//   키가 다름          → 이 카드 태그가 맵에 없음
		FString Dump;
		for (const TPair<FGameplayTag, TSubclassOf<UGameplayAbility>>& Pair : AbilityByCardId)
		{
			Dump += FString::Printf(TEXT("%s=%s; "),
				*Pair.Key.ToString(),
				Pair.Value ? *Pair.Value->GetName() : TEXT("None"));
		}
		BaamDebug::Screen(
			FString::Printf(TEXT("[서버] GA 매핑 없음! 카드 %s 를 맵에서 못 찾음.  현재 맵(%d개): %s"),
				*CardId.ToString(), AbilityByCardId.Num(),
				Dump.IsEmpty() ? TEXT("(비어있음)") : *Dump),
			FColor::Red, 10.f);
		return;
	}
	TSubclassOf<UGameplayAbility> AbilityClass = *Found;

	// 메커니즘 태그(선택): 페이로드/로그용. DT Row 의 AbilityEventTag — 없어도 진행한다.
	FGameplayTag EventTag;
	{
		const UGameInstance* GI = GetGameInstance();
		const UBaamDataSubsystem* Data = GI ? GI->GetSubsystem<UBaamDataSubsystem>() : nullptr;
		if (const FBaamCardRow* Row = Data ? Data->GetCardRow(CardId) : nullptr)
		{
			EventTag = Row->AbilityEventTag;
		}
	}

	// ── 대상/시전자 페이로드 구성 (뱅처럼 TargetSeat 이 있는 카드는 대상 폰을 넘긴다) ──
	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = GetPawn();
	if (AActor* TargetActor = ResolveTargetActor(TargetSeat))
	{
		Payload.Target = TargetActor;
	}

	BaamDebug::Screen(
		FString::Printf(TEXT("[서버] 카드처리  %s  손패:%s  메커니즘[%s] → %s  대상:%s"),
			*CardId.ToString(),
			bInHand ? TEXT("있음") : TEXT("없음"),
			EventTag.IsValid() ? *EventTag.ToString() : TEXT("-"),
			*AbilityClass->GetName(),
			Payload.Target.Get() ? *Payload.Target->GetName() : TEXT("없음")),
		FColor(180, 180, 255), /*Time=*/5.f);

	// 카드 주도 발동 (md §1.1): 서버 권위로 즉석 부여+1회 발동. 페이로드가 GA 의 TriggerEventData 로 전달된다.
	FGameplayAbilitySpec Spec(AbilityClass, /*Level=*/1, /*InputID=*/INDEX_NONE, /*SourceObject=*/this);
	ASC->GiveAbilityAndActivateOnce(Spec, &Payload);
}

AActor* ABaamPlayerController::ResolveTargetActor(int32 TargetSeat) const
{
	if (TargetSeat == INDEX_NONE)
	{
		return nullptr;
	}

	const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GS)
	{
		return nullptr;
	}

	for (APlayerState* PS : GS->PlayerArray)
	{
		const ABaamPlayerState* BPS = Cast<ABaamPlayerState>(PS);
		if (BPS && BPS->GetSeatIndex() == TargetSeat)
		{
			return PS->GetPawn();
		}
	}
	return nullptr;
}
