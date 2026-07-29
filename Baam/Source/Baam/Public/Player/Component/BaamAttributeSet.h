// BANG! 스탯 (Prototype-Workflow.md §2 AttributeSet 표).
// 값 변경은 항상 서버에서 GE 로만 한다 (세터 직접 호출 금지 — 복제 어긋남).

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "BaamAttributeSet.generated.h"

// 접근자 4종 자동 생성 (Getter/ValueGetter/Setter/Initter). Team4 의 BaseAttributeSet 과 동일.
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class BAAM_API UBaamAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UBaamAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	// 생명력(=탄환). 손패 한도의 기준이기도 하다 (현재값 기준).
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Bang")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UBaamAttributeSet, Health)

	// 최대 생명력. 캐릭터별 3~4, 보안관 +1.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Bang")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UBaamAttributeSet, MaxHealth)

	// 무기 사거리. Colt 1 / Schofield 2 / Remington 3 / Rev.Carabine 4 / Winchester 5.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_WeaponRange, Category = "Bang")
	FGameplayAttributeData WeaponRange;
	ATTRIBUTE_ACCESSORS(UBaamAttributeSet, WeaponRange)

	// 거리 감소분 (Scope, Rose Doolan): 내가 남을 볼 때 -1.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DistanceReduction, Category = "Bang")
	FGameplayAttributeData DistanceReduction;
	ATTRIBUTE_ACCESSORS(UBaamAttributeSet, DistanceReduction)

	// 거리 증가분 (Mustang, Paul Regret): 남이 나를 볼 때 +1.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DistanceIncrease, Category = "Bang")
	FGameplayAttributeData DistanceIncrease;
	ATTRIBUTE_ACCESSORS(UBaamAttributeSet, DistanceIncrease)

	// 턴당 능동적으로 사용할 수 있는 카드 수 (GDD §7.1 확정 스탯). 초기값 2.
	//   - 버리기(Discard 페이즈)는 "카드 사용" 이 아니므로 소비하지 않는다.
	//   - 판정에 실패한 갈색 카드도 소비한다 (GDD §5.3).
	//   - 장비·캐릭터 능력·상태 효과가 GE 로 이 값을 올리거나 내릴 수 있다.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CardUseLimit, Category = "Bang")
	FGameplayAttributeData CardUseLimit;
	ATTRIBUTE_ACCESSORS(UBaamAttributeSet, CardUseLimit)

	// [비활성] 턴당 BANG! 사용 한도. GDD §3 에서 뱅! 전용 제한을 없앴다 — CardUseLimit 이 대신한다.
	//   선언만 유지하고 참조하지 않는다 (GDD §13.1 비활성 처리).
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BangLimit, Category = "Bang")
	FGameplayAttributeData BangLimit;
	ATTRIBUTE_ACCESSORS(UBaamAttributeSet, BangLimit)

	// BANG! 을 막는 데 필요한 Missed! 장수. Slab the Killer → 2.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MissedRequired, Category = "Bang")
	FGameplayAttributeData MissedRequired;
	ATTRIBUTE_ACCESSORS(UBaamAttributeSet, MissedRequired)

	// 턴 시작 뽑기 장수 (기본 2).
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DrawCount, Category = "Bang")
	FGameplayAttributeData DrawCount;
	ATTRIBUTE_ACCESSORS(UBaamAttributeSet, DrawCount)

	// 뽑기(Draw!) 판정 시 공개 장수 (Lucky Duke → 2장 중 선택).
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DrawBangCount, Category = "Bang")
	FGameplayAttributeData DrawBangCount;
	ATTRIBUTE_ACCESSORS(UBaamAttributeSet, DrawBangCount)

	// ── 능력치(RPG 스탯) 4종 ──────────────────────────────────────
	//  주사위 판정/데미지에 붙는 보정치. 캐릭터 Row 의 GE 로 세팅한다.
	//  힘   : 뱅 데미지 배율 (내가 줄 피해가 커진다)
	//  민첩 : 회피 판정 굴림에 가산 (내가 잘 피한다)
	//  지능 : 받는 데미지 경감 (내가 맞는 피해가 줄어든다)
	//  행운 : 모든 주사위 판정 굴림에 가산 (뱅·회피 공통, 좋은 눈이 잘 뜬다)

	// 힘 — 뱅 데미지 배율. 1당 GA_Bang 의 StrengthDamageMult 만큼 피해 증가.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Strength, Category = "Bang|Stat")
	FGameplayAttributeData Strength;
	ATTRIBUTE_ACCESSORS(UBaamAttributeSet, Strength)

	// 민첩 — 회피(Missed!) 판정 굴림에 가산 (행운과 별개로 추가).
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Agility, Category = "Bang|Stat")
	FGameplayAttributeData Agility;
	ATTRIBUTE_ACCESSORS(UBaamAttributeSet, Agility)

	// 지능 — 받는 데미지 경감. 1당 GA_Bang 의 IntelligenceMitigation 만큼 피해 감소.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Intelligence, Category = "Bang|Stat")
	FGameplayAttributeData Intelligence;
	ATTRIBUTE_ACCESSORS(UBaamAttributeSet, Intelligence)

	// 행운 — 모든 판정 굴림에 가산 (뱅 공격·회피 공통).
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Luck, Category = "Bang|Stat")
	FGameplayAttributeData Luck;
	ATTRIBUTE_ACCESSORS(UBaamAttributeSet, Luck)

protected:
	UFUNCTION() void OnRep_Health(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_WeaponRange(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_DistanceReduction(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_DistanceIncrease(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_CardUseLimit(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_BangLimit(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_MissedRequired(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_DrawCount(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_DrawBangCount(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_Strength(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_Agility(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_Intelligence(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_Luck(const FGameplayAttributeData& OldValue);
};
