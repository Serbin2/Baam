#include "Player/Effect/GE_Heal.h"
#include "Player/Component/BaamAttributeSet.h"
#include "Game/BaamGameplayTags.h"

UGE_Heal::UGE_Heal()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Health += SetByCaller(SetByCaller.Heal). 회복은 양수.
	FGameplayModifierInfo ModInfo;
	ModInfo.Attribute = UBaamAttributeSet::GetHealthAttribute();
	ModInfo.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat SBC;
	SBC.DataTag = Bang::SetByCaller::Heal.GetTag();
	ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(SBC);

	Modifiers.Add(ModInfo);
}
