#include "Player/Component/BaamAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UBaamAttributeSet::UBaamAttributeSet()
{
	// 안전한 기본값 — 실제 값은 캐릭터 Row 의 DefaultAttributeGE 로 덮어쓴다.
	InitHealth(4.f);
	InitMaxHealth(4.f);
	InitWeaponRange(1.f);
	InitDistanceReduction(0.f);
	InitDistanceIncrease(0.f);
	InitBangLimit(1.f);
	InitMissedRequired(1.f);
	InitDrawCount(2.f);
	InitDrawBangCount(1.f);

	// 능력치 — 기본 0 (보정 없음). 실제 값은 캐릭터 Row 의 GE 로 세팅한다.
	InitStrength(0.f);
	InitAgility(0.f);
	InitIntelligence(0.f);
	InitLuck(0.f);
}

void UBaamAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// REPNOTIFY_Always: 같은 값으로 재적용돼도 OnRep 을 태워 UI 가 항상 갱신되도록.
	DOREPLIFETIME_CONDITION_NOTIFY(UBaamAttributeSet, Health,            COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaamAttributeSet, MaxHealth,         COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaamAttributeSet, WeaponRange,       COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaamAttributeSet, DistanceReduction, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaamAttributeSet, DistanceIncrease,  COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaamAttributeSet, BangLimit,         COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaamAttributeSet, MissedRequired,    COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaamAttributeSet, DrawCount,         COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaamAttributeSet, DrawBangCount,     COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaamAttributeSet, Strength,          COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaamAttributeSet, Agility,           COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaamAttributeSet, Intelligence,      COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaamAttributeSet, Luck,              COND_None, REPNOTIFY_Always);
}

void UBaamAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// 스탯은 음수가 되지 않는다. 거리 보정치도 0 미만은 무의미.
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
	else
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}

void UBaamAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// GE(피해/회복) 적용 후 Health 를 [0, MaxHealth] 로 최종 클램프.
	// 사망 판정(Health<=0 → State.Dead)은 캐릭터/룰 계층에서 이 값을 보고 처리한다.
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}
}

void UBaamAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaamAttributeSet, Health, OldValue);
}
void UBaamAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaamAttributeSet, MaxHealth, OldValue);
}
void UBaamAttributeSet::OnRep_WeaponRange(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaamAttributeSet, WeaponRange, OldValue);
}
void UBaamAttributeSet::OnRep_DistanceReduction(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaamAttributeSet, DistanceReduction, OldValue);
}
void UBaamAttributeSet::OnRep_DistanceIncrease(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaamAttributeSet, DistanceIncrease, OldValue);
}
void UBaamAttributeSet::OnRep_BangLimit(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaamAttributeSet, BangLimit, OldValue);
}
void UBaamAttributeSet::OnRep_MissedRequired(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaamAttributeSet, MissedRequired, OldValue);
}
void UBaamAttributeSet::OnRep_DrawCount(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaamAttributeSet, DrawCount, OldValue);
}
void UBaamAttributeSet::OnRep_DrawBangCount(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaamAttributeSet, DrawBangCount, OldValue);
}
void UBaamAttributeSet::OnRep_Strength(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaamAttributeSet, Strength, OldValue);
}
void UBaamAttributeSet::OnRep_Agility(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaamAttributeSet, Agility, OldValue);
}
void UBaamAttributeSet::OnRep_Intelligence(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaamAttributeSet, Intelligence, OldValue);
}
void UBaamAttributeSet::OnRep_Luck(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaamAttributeSet, Luck, OldValue);
}
