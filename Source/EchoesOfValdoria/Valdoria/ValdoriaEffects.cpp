// Echoes of Valdoria - efeitos de jogo básicos.

#include "ValdoriaEffects.h"
#include "ValdoriaAttributeSet.h"
#include "ValdoriaGameplayTags.h"

static FGameplayModifierInfo MakeSetByCallerModifier(const FGameplayAttribute& Attribute, const FGameplayTag& Tag)
{
	FGameplayModifierInfo Mod;
	Mod.Attribute = Attribute;
	Mod.ModifierOp = EGameplayModOp::Additive;
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = Tag;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	return Mod;
}

UValdoriaDamageEffect::UValdoriaDamageEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(MakeSetByCallerModifier(UValdoriaAttributeSet::GetIncomingDamageAttribute(), ValdoriaTags::Data_Damage));
}

UValdoriaHealEffect::UValdoriaHealEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(MakeSetByCallerModifier(UValdoriaAttributeSet::GetIncomingHealAttribute(), ValdoriaTags::Data_Heal));
}
