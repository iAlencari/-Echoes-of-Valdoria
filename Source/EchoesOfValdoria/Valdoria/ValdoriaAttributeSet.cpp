// Echoes of Valdoria - atributos de combate.

#include "ValdoriaAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
#include "ValdoriaGameplayTags.h"
#include "ValdoriaHeroCharacter.h"

UValdoriaAttributeSet::UValdoriaAttributeSet()
{
	InitHealth(225.f);
	InitMaxHealth(225.f);
	InitArmor(0.f);
	InitMaxArmor(0.f);
	InitShield(0.f);
	InitMaxShield(0.f);
	InitUltimateCharge(0.f);
	InitDamageMultiplier(1.f);
	InitIncomingDamage(0.f);
	InitIncomingHeal(0.f);
}

void UValdoriaAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UValdoriaAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UValdoriaAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UValdoriaAttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UValdoriaAttributeSet, MaxArmor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UValdoriaAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UValdoriaAttributeSet, MaxShield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UValdoriaAttributeSet, UltimateCharge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UValdoriaAttributeSet, DamageMultiplier, COND_None, REPNOTIFY_Always);
}

void UValdoriaAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetArmorAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxArmor());
	}
	else if (Attribute == GetShieldAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxShield());
	}
	else if (Attribute == GetUltimateChargeAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
	}
}

void UValdoriaAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	AValdoriaHeroCharacter* Target = Cast<AValdoriaHeroCharacter>(GetOwningActor());
	const FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
	AValdoriaHeroCharacter* Source = Cast<AValdoriaHeroCharacter>(Context.GetOriginalInstigator());

	if (Data.EvaluatedData.Attribute == GetIncomingHealAttribute())
	{
		const float Heal = GetIncomingHeal();
		SetIncomingHeal(0.f);
		if (Heal > 0.f && GetHealth() > 0.f)
		{
			SetHealth(FMath::Clamp(GetHealth() + Heal, 0.f, GetMaxHealth()));
		}
		return;
	}

	if (Data.EvaluatedData.Attribute != GetIncomingDamageAttribute())
	{
		return;
	}

	float Damage = GetIncomingDamage();
	SetIncomingDamage(0.f);
	if (Damage <= 0.f || GetHealth() <= 0.f || (Target && Target->IsInvulnerable()))
	{
		return;
	}
	const float TotalDamage = Damage;

	// 1) Escudo absorve primeiro.
	const float ShieldAbsorb = FMath::Min(GetShield(), Damage);
	SetShield(GetShield() - ShieldAbsorb);
	Damage -= ShieldAbsorb;

	// 2) Armadura: reduz parte do acerto e absorve o resto até acabar.
	if (Damage > 0.f && GetArmor() > 0.f)
	{
		const float Reduction = Target ? Target->GetArmorDamageReduction() : 0.3f;
		const float Reduced = Damage * (1.f - Reduction);
		const float ArmorAbsorb = FMath::Min(GetArmor(), Reduced);
		SetArmor(GetArmor() - ArmorAbsorb);
		// O que a armadura não aguentou passa com o valor original proporcional.
		Damage = (Reduced - ArmorAbsorb) / FMath::Max(0.1f, 1.f - Reduction);
	}

	// 3) Vida.
	if (Damage > 0.f)
	{
		SetHealth(FMath::Clamp(GetHealth() - Damage, 0.f, GetMaxHealth()));
	}

	const bool bHeadshot = Data.EffectSpec.GetDynamicAssetTags().HasTagExact(ValdoriaTags::Hit_Headshot);

	if (Source && Source != Target)
	{
		Source->OnDamageDealt(Target, TotalDamage, bHeadshot);
	}
	if (Target)
	{
		Target->OnDamageTaken(Source, TotalDamage, bHeadshot);
		if (GetHealth() <= 0.f)
		{
			Target->HandleDeath(Source);
		}
	}
}

void UValdoriaAttributeSet::OnRep_Health(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(UValdoriaAttributeSet, Health, Old); }
void UValdoriaAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(UValdoriaAttributeSet, MaxHealth, Old); }
void UValdoriaAttributeSet::OnRep_Armor(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(UValdoriaAttributeSet, Armor, Old); }
void UValdoriaAttributeSet::OnRep_MaxArmor(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(UValdoriaAttributeSet, MaxArmor, Old); }
void UValdoriaAttributeSet::OnRep_Shield(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(UValdoriaAttributeSet, Shield, Old); }
void UValdoriaAttributeSet::OnRep_MaxShield(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(UValdoriaAttributeSet, MaxShield, Old); }
void UValdoriaAttributeSet::OnRep_UltimateCharge(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(UValdoriaAttributeSet, UltimateCharge, Old); }
void UValdoriaAttributeSet::OnRep_DamageMultiplier(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(UValdoriaAttributeSet, DamageMultiplier, Old); }
