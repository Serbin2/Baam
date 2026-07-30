// 범용 카드 효과 실행기 — 카드 데이터(FBaamOutcomeEffects)에 적힌 효과 목록을 그대로 실행한다.
//
// 왜 범용인가:
//   카드마다 GA 를 만들면 "카드 1장 뽑고 피해 1 주고 한도 1 회복" 같은 조합마다 C++ 을 고쳐야 한다.
//   효과를 데이터로 표현하면 새 카드는 DT 행 추가만으로 끝난다(GDD §6.4 신규 카드 확장 전제).
//
// 사용법:
//   1) DT_BaamCard 의 OutcomeEffects 에 등급별 효과를 채운다
//   2) AbilityByCardId 에서 그 카드를 이 클래스로 매핑한다
//
// 판정은 하지 않는다 — 서버가 이미 끝냈고 Resolution.* 태그로 등급이 전달된다 (GDD §9.1).
// 어느 카드인지는 페이로드의 Card.Id.* 태그로 알아내 효과 목록을 조회한다.

#pragma once

#include "CoreMinimal.h"
#include "Player/Ability/BaamGameplayAbility.h"
#include "Game/BaamGameDataTypes.h"   // EBaamDiceOutcome / FBaamCardEffect
#include "GA_BaamCardEffects.generated.h"

class ABaamGameState;
class ABaamPlayerState;
class UAbilitySystemComponent;

UCLASS()
class BAAM_API UGA_BaamCardEffects : public UBaamGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_BaamCardEffects();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 힘 1당 피해 배율 증가분. GA_Bang 과 같은 규칙을 쓴다(두 경로의 피해가 어긋나면 안 된다).
	UPROPERTY(EditDefaultsOnly, Category = "Bang|Damage", meta = (ClampMin = "0.0"))
	float StrengthDamageMult = 0.25f;

	// 대상 지능 1당 경감되는 피해량.
	UPROPERTY(EditDefaultsOnly, Category = "Bang|Damage", meta = (ClampMin = "0.0"))
	float IntelligenceMitigation = 0.5f;

	/** 효과 목록을 순서대로 실행하고 사람이 읽을 요약을 채운다. */
	void ExecuteEffects(
		const TArray<FBaamCardEffect>& Effects,
		AActor* Avatar,
		AActor* TargetActor,
		UAbilitySystemComponent* SourceASC,
		TArray<FString>& OutSummary);

	// 개별 효과 — 성공하면 요약 문구를 돌려준다(빈 문자열이면 아무 일도 없었음).
	FString ApplyDamage(int32 Amount, AActor* Avatar, AActor* TargetActor, UAbilitySystemComponent* SourceASC);
	FString ApplyHeal(int32 Amount, UAbilitySystemComponent* SourceASC);
	FString ApplyDraw(int32 Amount, ABaamGameState* GS, ABaamPlayerState* SelfPS);
	FString ApplyTakeFromTarget(int32 Amount, bool bSteal, ABaamGameState* GS, ABaamPlayerState* SelfPS, ABaamPlayerState* TargetPS, AActor* Avatar);
	FString ApplyRestoreCardUse(int32 Amount, ABaamPlayerState* SelfPS);
	FString ApplyStatusEffect(const FBaamCardEffect& Effect, ABaamPlayerState* SelfPS, ABaamPlayerState* TargetPS);

	/** 판정 결과와 실행된 효과 요약을 알린다(연출·UI용). 서버에서 호출. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Bang")
	void OnCardEffectsResolved(EBaamDiceOutcome Outcome, const FString& Summary, AActor* Target);
};
