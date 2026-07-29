#include "Player/Effect/GE_Damage.h"
#include "Player/Component/BaamAttributeSet.h"
#include "Game/BaamGameplayTags.h"

UGE_Damage::UGE_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Health += SetByCaller(SetByCaller.Damage). 피해는 음수로 넣어 감소시킨다.
	FGameplayModifierInfo ModInfo;
	ModInfo.Attribute = UBaamAttributeSet::GetHealthAttribute();
	ModInfo.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat SBC;
	SBC.DataTag = Bang::SetByCaller::Damage.GetTag();
	ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(SBC);

	Modifiers.Add(ModInfo);
}
