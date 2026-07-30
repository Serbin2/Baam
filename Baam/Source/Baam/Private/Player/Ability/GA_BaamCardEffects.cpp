#include "Player/Ability/GA_BaamCardEffects.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Game/BaamCardLog.h"
#include "Game/BaamCardType.h"
#include "Game/BaamDataSubsystem.h"
#include "Game/BaamDebug.h"
#include "Game/BaamDiceComponent.h"
#include "Game/BaamGameState.h"
#include "Game/BaamGameplayTags.h"
#include "Game/BaamPlayerState.h"
#include "Player/Component/BaamAttributeSet.h"
#include "Player/Effect/GE_Damage.h"
#include "Player/Effect/GE_Heal.h"

UGA_BaamCardEffects::UGA_BaamCardEffects()
{
	ActivationBlockedTags.AddTag(Bang::State::Dead.GetTag());
}

namespace
{
	/** 페이로드에 실린 Card.Id.* 태그를 찾는다. 어느 카드가 이 GA 를 발동했는지 알아내는 용도. */
	FGameplayTag FindCardIdTag(const FGameplayTagContainer& Tags)
	{
		static const FGameplayTag CardIdRoot = FGameplayTag::RequestGameplayTag(TEXT("Card.Id"));
		for (const FGameplayTag& Tag : Tags)
		{
			if (Tag.MatchesTag(CardIdRoot) && Tag != CardIdRoot)
			{
				return Tag;
			}
		}
		return FGameplayTag();
	}
}

void UGA_BaamCardEffects::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	UAbilitySystemComponent* SourceASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

	if (!TriggerEventData)
	{
		UE_LOG(LogBaamCard, Warning,
			TEXT("[Bang] GA_BaamCardEffects: 페이로드가 없습니다 — 카드 경로로 발동해야 합니다."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ── 어느 카드인가 ──
	const FGameplayTag CardIdTag = FindCardIdTag(TriggerEventData->InstigatorTags);
	const UWorld* World = Avatar ? Avatar->GetWorld() : nullptr;
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	const UBaamDataSubsystem* Data = GI ? GI->GetSubsystem<UBaamDataSubsystem>() : nullptr;
	const FBaamCardRow* Row = (Data && CardIdTag.IsValid()) ? Data->GetCardRow(CardIdTag) : nullptr;

	if (!Row)
	{
		UE_LOG(LogBaamCard, Warning,
			TEXT("[Bang] GA_BaamCardEffects: 카드 Row 를 찾지 못했습니다 (CardId 태그=%s). "
			     "HandlePlayCard 가 Card.Id 태그를 실어 보내는지 확인하세요."),
			CardIdTag.IsValid() ? *CardIdTag.ToString() : TEXT("없음"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ── 판정 등급 수신 (판정은 서버가 이미 끝냈다 — GDD §9.1) ──
	const EBaamDiceOutcome Outcome =
		UBaamDiceComponent::TagsToOutcome(TriggerEventData->InstigatorTags, EBaamDiceOutcome::Success);

	const TArray<FBaamCardEffect>& Effects = Row->OutcomeEffects.ForOutcome(Outcome);

	AActor* TargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());

	TArray<FString> Summary;
	ExecuteEffects(Effects, Avatar, TargetActor, SourceASC, Summary);

	const FString SummaryText = Summary.IsEmpty() ? TEXT("효과 없음") : FString::Join(Summary, TEXT(" + "));

	BaamDebug::Screen(
		FString::Printf(TEXT("%s  %s → [%s] %s"),
			*Row->DisplayName.ToString(),
			Avatar ? *Avatar->GetName() : TEXT("?"),
			*UBaamDiceComponent::GetOutcomeText(Outcome).ToString(),
			*SummaryText),
		Summary.IsEmpty() ? FColor(160, 160, 160) : FColor(120, 220, 255), /*Time=*/6.f);

	OnCardEffectsResolved(Outcome, SummaryText, TargetActor);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_BaamCardEffects::ExecuteEffects(
	const TArray<FBaamCardEffect>& Effects,
	AActor* Avatar,
	AActor* TargetActor,
	UAbilitySystemComponent* SourceASC,
	TArray<FString>& OutSummary)
{
	const UWorld* World = Avatar ? Avatar->GetWorld() : nullptr;
	ABaamGameState* GS = World ? World->GetGameState<ABaamGameState>() : nullptr;

	ABaamPlayerState* SelfPS   = GetBaamPlayerState(Avatar);
	ABaamPlayerState* TargetPS = GetBaamPlayerState(TargetActor);

	//	목록 순서대로 실행한다. 순서가 결과를 바꿀 수 있으므로(뽑기 → 한도 회복 등)
	//	DT 에 적힌 순서를 그대로 지킨다.
	for (const FBaamCardEffect& Effect : Effects)
	{
		//	수치 0 은 "이 등급에서는 이 효과 없음" 으로 본다.
		//	단 ApplyStatus 는 예외다 — 강제 판정 상태는 수치를 쓰지 않으므로 0 이 정상이다.
		if (Effect.Amount <= 0 && Effect.Op != EBaamCardEffectOp::ApplyStatus)
		{
			continue;
		}

		FString Line;
		switch (Effect.Op)
		{
		case EBaamCardEffectOp::DamageTarget:
			Line = ApplyDamage(Effect.Amount, Avatar, TargetActor, SourceASC);
			break;

		case EBaamCardEffectOp::HealSelf:
			Line = ApplyHeal(Effect.Amount, SourceASC);
			break;

		case EBaamCardEffectOp::DrawSelf:
			Line = ApplyDraw(Effect.Amount, GS, SelfPS);
			break;

		case EBaamCardEffectOp::DiscardTargetRandom:
			Line = ApplyTakeFromTarget(Effect.Amount, /*bSteal=*/false, GS, SelfPS, TargetPS, Avatar);
			break;

		case EBaamCardEffectOp::StealTargetRandom:
			Line = ApplyTakeFromTarget(Effect.Amount, /*bSteal=*/true, GS, SelfPS, TargetPS, Avatar);
			break;

		case EBaamCardEffectOp::RestoreCardUse:
			Line = ApplyRestoreCardUse(Effect.Amount, SelfPS);
			break;

		case EBaamCardEffectOp::ApplyStatus:
			Line = ApplyStatusEffect(Effect, SelfPS, TargetPS);
			break;

		default:
			break;
		}

		if (!Line.IsEmpty())
		{
			OutSummary.Add(Line);
		}
	}
}

FString UGA_BaamCardEffects::ApplyDamage(int32 Amount, AActor* Avatar, AActor* TargetActor, UAbilitySystemComponent* SourceASC)
{
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!SourceASC || !TargetASC)
	{
		return FString();
	}

	//	피해는 카드 데이터가 전부 결정한다.
	//
	//	[비활성] 힘 배율 / 대상 지능 경감은 쓰지 않는다 (GDD §7.1 / §13.1).
	//	  확정 스탯은 CardUseLimit 하나뿐이고, §7.2 도 초기 프로토타입에서는 스탯을 최소로
	//	  두라고 권한다. 실제로 지능 1(= round(1 × 0.5) = 1)이 피해 1 을 통째로 상쇄해
	//	  "성공했는데 피해 0" 이 되는 문제가 있었다 — 카드 수치가 곧 피해여야
	//	  §10 확률·효과 표시와 §11 밸런스 지표가 성립한다.
	//	  StrengthDamageMult / IntelligenceMitigation 프로퍼티는 선언만 남는다.
	//
	//	약점 포착(Status.NextAttack.DamageBonus)은 스탯이 아니라 카드 효과이므로 유지한다.
	int32 Final = Amount;
	int32 Bonus = 0;
	if (ABaamPlayerState* SelfPS = GetBaamPlayerState(Avatar))
	{
		SelfPS->ConsumePendingStatus(Bang::Status::NextAttack::DamageBonus.GetTag(), Bonus);
		Final += Bonus;
	}

	UE_LOG(LogBaamCard, Log, TEXT("[피해] 카드%d%s → 최종 %d"),
		Amount,
		(Bonus > 0) ? *FString::Printf(TEXT(" + 약점포착%d"), Bonus) : TEXT(""),
		Final);

	if (Final <= 0)
	{
		return FString::Printf(TEXT("피해 0 (카드 수치 %d)"), Amount);
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(this);
	Context.AddInstigator(Avatar, Avatar);

	const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(UGE_Damage::StaticClass(), 1.f, Context);
	if (!Spec.IsValid())
	{
		return FString();
	}

	//	피해는 음수로 넣는다(Health 가산 = 감소).
	Spec.Data->SetSetByCallerMagnitude(Bang::SetByCaller::Damage.GetTag(), -static_cast<float>(Final));
	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);

	return FString::Printf(TEXT("피해 %d"), Final);
}

FString UGA_BaamCardEffects::ApplyHeal(int32 Amount, UAbilitySystemComponent* SourceASC)
{
	if (!SourceASC)
	{
		return FString();
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(this);

	const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(UGE_Heal::StaticClass(), 1.f, Context);
	if (!Spec.IsValid())
	{
		return FString();
	}

	//	MaxHealth 클램프는 AttributeSet 이 처리한다.
	Spec.Data->SetSetByCallerMagnitude(Bang::SetByCaller::Heal.GetTag(), static_cast<float>(Amount));
	SourceASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

	return FString::Printf(TEXT("회복 %d"), Amount);
}

FString UGA_BaamCardEffects::ApplyDraw(int32 Amount, ABaamGameState* GS, ABaamPlayerState* SelfPS)
{
	if (!GS || !SelfPS)
	{
		return FString();
	}

	int32 Drawn = 0;
	for (int32 i = 0; i < Amount; ++i)
	{
		FBaamCardInstance Card;
		if (!GS->DrawFromDeck(Card))
		{
			break;   // 덱과 버린 패가 모두 비었다
		}
		SelfPS->AddCardToHand(Card);
		++Drawn;
	}

	return (Drawn > 0) ? FString::Printf(TEXT("%d장 뽑기"), Drawn) : FString();
}

FString UGA_BaamCardEffects::ApplyTakeFromTarget(int32 Amount, bool bSteal,
	ABaamGameState* GS, ABaamPlayerState* SelfPS, ABaamPlayerState* TargetPS, AActor* Avatar)
{
	UBaamDiceComponent* Dice = UBaamDiceComponent::Get(Avatar);
	if (!GS || !TargetPS || !Dice)
	{
		return FString();
	}

	int32 Moved = 0;
	for (int32 i = 0; i < Amount; ++i)
	{
		//	손패는 매 반복마다 다시 읽는다 — 앞 반복에서 한 장이 빠졌다.
		const TArray<FBaamCardInstance>& Hand = TargetPS->GetHand();
		if (Hand.IsEmpty())
		{
			break;
		}

		const int32 Index = Dice->RandomIndex(Hand.Num());
		if (!Hand.IsValidIndex(Index))
		{
			break;
		}

		//	제거 전에 값으로 복사 — 제거하면 위 참조가 흔들린다.
		const int32 InstanceId = Hand[Index].InstanceId;

		FBaamCardInstance Removed;
		if (!TargetPS->RemoveCardFromHand(InstanceId, Removed))
		{
			break;
		}

		if (bSteal && SelfPS)
		{
			SelfPS->AddCardToHand(Removed);
		}
		else
		{
			GS->PushToDiscard(Removed);
		}
		++Moved;
	}

	return (Moved > 0)
		? FString::Printf(TEXT("상대 %d장 %s"), Moved, bSteal ? TEXT("훔침") : TEXT("버림"))
		: FString();
}

FString UGA_BaamCardEffects::ApplyRestoreCardUse(int32 Amount, ABaamPlayerState* SelfPS)
{
	if (!SelfPS)
	{
		return FString();
	}

	//	이 카드 자신의 사용 횟수는 GA 발동 전에 이미 계상돼 있다(HandlePlayCard).
	//	그래서 1 회복하면 "이 카드는 사용 횟수를 쓰지 않은 셈" 이 된다.
	const int32 Restored = SelfPS->DecrementCardsUsedThisTurn(Amount);
	return (Restored > 0) ? FString::Printf(TEXT("사용한도 %d 회복"), Restored) : FString();
}

FString UGA_BaamCardEffects::ApplyStatusEffect(const FBaamCardEffect& Effect,
	ABaamPlayerState* SelfPS, ABaamPlayerState* TargetPS)
{
	if (!Effect.StatusTag.IsValid())
	{
		UE_LOG(LogBaamCard, Warning, TEXT("[Bang] ApplyStatus: StatusTag 가 비어 있습니다(DT 확인)."));
		return FString();
	}

	ABaamPlayerState* Recipient = Effect.bToTarget ? TargetPS : SelfPS;
	if (!Recipient)
	{
		UE_LOG(LogBaamCard, Warning, TEXT("[Bang] ApplyStatus: %s 를 받을 대상이 없습니다."),
			*Effect.StatusTag.ToString());
		return FString();
	}

	//	중첩은 AddPendingStatus 가 거부한다. 여기까지 왔다면 HandlePlayCard 의 사전 검사를
	//	통과했다는 뜻이므로 정상적으로는 실패하지 않는다.
	if (!Recipient->AddPendingStatus(Effect.StatusTag, Effect.Amount))
	{
		return FString();
	}

	return FString::Printf(TEXT("%s%s 부여"),
		Effect.bToTarget ? TEXT("상대에게 ") : TEXT(""),
		*Effect.StatusTag.GetTagName().ToString());
}
