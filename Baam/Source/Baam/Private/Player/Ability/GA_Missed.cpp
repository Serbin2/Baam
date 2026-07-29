#include "Player/Ability/GA_Missed.h"
#include "Player/Component/BaamAttributeSet.h"
#include "Game/BaamGameplayTags.h"
#include "Game/BaamGameMode.h"
#include "Game/BaamDebug.h"
#include "AbilitySystemComponent.h"

UGA_Missed::UGA_Missed()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(Bang::Ability::Missed.GetTag());
	// Reaction 성질 — 응답 창에서만 사용 (능동 사용 검증에서 걸러진다).
	AssetTags.AddTag(Bang::Card::Trait::Reaction.GetTag());
	SetAssetTags(AssetTags);

	ActivationBlockedTags.AddTag(Bang::State::Dead.GetTag());
}

void UGA_Missed::ActivateAbility(
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

	UAbilitySystemComponent* SourceASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const UBaamAttributeSet* SourceAS = SourceASC ? SourceASC->GetSet<UBaamAttributeSet>() : nullptr;
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	ABaamGameMode* GM = Avatar ? Avatar->GetWorld()->GetAuthGameMode<ABaamGameMode>() : nullptr;

	if (!SourceAS || !GM)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bang] GA_Missed — 서버 컨텍스트 없음(AS/GameMode). 중단."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 회피는 행운 + 민첩 을 굴림 보정으로 넣는다.
	const int32 DodgeBonus = FMath::RoundToInt(SourceAS->GetLuck() + SourceAS->GetAgility());
	int32 Roll = 0;
	const EBaamDiceOutcome Outcome = GM->RollForOutcome(Roll, /*Faces=*/0, /*RollBonus=*/DodgeBonus);

	// 회피는 성공/실패로만 압축: 성공·대성공 → 회피 성공.
	const bool bDodged =
		(Outcome == EBaamDiceOutcome::Success || Outcome == EBaamDiceOutcome::CriticalSuccess);

	// TODO(§3): bDodged 를 진행 중인 응답 요청(FBangResolutionRequest)에 제출 →
	//           성공이면 해당 BANG! 을 무효화한다. (MissedRequired 만큼 필요할 수 있음)
	BaamDebug::Screen(
		FString::Printf(TEXT("회피  %s → %s  (판정 %s, 눈%d, 행운+민첩%+d)"),
			Avatar ? *Avatar->GetName() : TEXT("?"),
			bDodged ? TEXT("성공") : TEXT("실패"),
			*ABaamGameMode::GetOutcomeText(Outcome).ToString(), Roll, DodgeBonus),
		bDodged ? FColor::Cyan : FColor::Red, /*Time=*/5.f);

	OnDodgeResolved(bDodged);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
