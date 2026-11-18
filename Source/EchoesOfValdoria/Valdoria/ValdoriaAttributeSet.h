// Echoes of Valdoria - atributos de combate (vida, armadura, escudo, ultimate).

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "ValdoriaAttributeSet.generated.h"

#define VALDORIA_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * Atributos replicados de cada personagem. O dano entra em IncomingDamage
 * (meta-atributo, não replicado) e é distribuído: escudo -> armadura -> vida.
 */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UValdoriaAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(BlueprintReadOnly, Category = "Atributos", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	VALDORIA_ATTRIBUTE_ACCESSORS(UValdoriaAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Atributos", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	VALDORIA_ATTRIBUTE_ACCESSORS(UValdoriaAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category = "Atributos", ReplicatedUsing = OnRep_Armor)
	FGameplayAttributeData Armor;
	VALDORIA_ATTRIBUTE_ACCESSORS(UValdoriaAttributeSet, Armor)

	UPROPERTY(BlueprintReadOnly, Category = "Atributos", ReplicatedUsing = OnRep_MaxArmor)
	FGameplayAttributeData MaxArmor;
	VALDORIA_ATTRIBUTE_ACCESSORS(UValdoriaAttributeSet, MaxArmor)

	UPROPERTY(BlueprintReadOnly, Category = "Atributos", ReplicatedUsing = OnRep_Shield)
	FGameplayAttributeData Shield;
	VALDORIA_ATTRIBUTE_ACCESSORS(UValdoriaAttributeSet, Shield)

	UPROPERTY(BlueprintReadOnly, Category = "Atributos", ReplicatedUsing = OnRep_MaxShield)
	FGameplayAttributeData MaxShield;
	VALDORIA_ATTRIBUTE_ACCESSORS(UValdoriaAttributeSet, MaxShield)

	UPROPERTY(BlueprintReadOnly, Category = "Atributos", ReplicatedUsing = OnRep_UltimateCharge)
	FGameplayAttributeData UltimateCharge;
	VALDORIA_ATTRIBUTE_ACCESSORS(UValdoriaAttributeSet, UltimateCharge)

	/** Multiplicador do dano causado (buffs, painel de admin). */
	UPROPERTY(BlueprintReadOnly, Category = "Atributos", ReplicatedUsing = OnRep_DamageMultiplier)
	FGameplayAttributeData DamageMultiplier;
	VALDORIA_ATTRIBUTE_ACCESSORS(UValdoriaAttributeSet, DamageMultiplier)

	/** Meta-atributo: dano recebido neste efeito. Só existe no servidor. */
	UPROPERTY(BlueprintReadOnly, Category = "Atributos")
	FGameplayAttributeData IncomingDamage;
	VALDORIA_ATTRIBUTE_ACCESSORS(UValdoriaAttributeSet, IncomingDamage)

	/** Meta-atributo: cura recebida neste efeito. */
	UPROPERTY(BlueprintReadOnly, Category = "Atributos")
	FGameplayAttributeData IncomingHeal;
	VALDORIA_ATTRIBUTE_ACCESSORS(UValdoriaAttributeSet, IncomingHeal)

protected:
	UFUNCTION() void OnRep_Health(const FGameplayAttributeData& Old);
	UFUNCTION() void OnRep_MaxHealth(const FGameplayAttributeData& Old);
	UFUNCTION() void OnRep_Armor(const FGameplayAttributeData& Old);
	UFUNCTION() void OnRep_MaxArmor(const FGameplayAttributeData& Old);
	UFUNCTION() void OnRep_Shield(const FGameplayAttributeData& Old);
	UFUNCTION() void OnRep_MaxShield(const FGameplayAttributeData& Old);
	UFUNCTION() void OnRep_UltimateCharge(const FGameplayAttributeData& Old);
	UFUNCTION() void OnRep_DamageMultiplier(const FGameplayAttributeData& Old);
};
