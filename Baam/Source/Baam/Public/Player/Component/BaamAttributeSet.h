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

	// 턴당 BANG! 사용 한도. Volcanic / Willy the Kid → 큰 수.
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

protected:
	UFUNCTION() void OnRep_Health(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_WeaponRange(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_DistanceReduction(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_DistanceIncrease(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_BangLimit(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_MissedRequired(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_DrawCount(const FGameplayAttributeData& OldValue);
	UFUNCTION() void OnRep_DrawBangCount(const FGameplayAttributeData& OldValue);
};
