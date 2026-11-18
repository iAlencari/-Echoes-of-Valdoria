// Echoes of Valdoria - base de todas as habilidades (GAS).

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ValdoriaTypes.h"
#include "ValdoriaGameplayAbility.generated.h"

class AValdoriaHeroCharacter;

/**
 * Base das habilidades. A recarga é guardada no personagem (por slot) e vem do
 * Data Asset do herói, para o balanceamento ficar só nos dados.
 */
UCLASS(Abstract)
class ECHOESOFVALDORIA_API UValdoriaGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UValdoriaGameplayAbility();

	/** Recarga padrão (usada se o Data Asset não definir uma). */
	UPROPERTY(EditDefaultsOnly, Category = "Valdoria")
	float DefaultCooldown = 6.f;

	UPROPERTY(EditDefaultsOnly, Category = "Valdoria")
	float DefaultMagnitude = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Valdoria")
	float DefaultRadius = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Valdoria")
	float DefaultDuration = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Valdoria")
	float DefaultSecondary = 0.f;

	/** Tipo de dano padrão (o Data Asset pode trocar). */
	UPROPERTY(EditDefaultsOnly, Category = "Valdoria")
	EValdoriaDamageType DefaultDamageType = EValdoriaDamageType::Physical;

	/** Gasta a carga de ultimate inteira ao usar. */
	UPROPERTY(EditDefaultsOnly, Category = "Valdoria")
	bool bIsUltimate = false;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

protected:
	AValdoriaHeroCharacter* GetHero() const;
	EValdoriaAbilitySlot GetSlot() const;
	float GetEffectiveCooldown() const;
	float GetMagnitude() const;
	float GetRadius() const;
	float GetDuration() const;
	float GetSecondary() const;
	EValdoriaDamageType GetDamageType() const;
	float GetMagicShare() const;
	/** Cor do herói (efeitos). */
	FLinearColor GetThemeColor() const;
	/** Entrada da habilidade no Data Asset (pode ser nula). */
	const FValdoriaAbilityEntry* GetEntry() const;

	/** Inicia a recarga e, se for ultimate, consome a carga. Chame ao usar. */
	void StartCooldownAndCost();

	/** Aplica dano em outro personagem pelo GAS (só tem efeito no servidor). */
	void ApplyDamageTo(AActor* Target, float Damage, bool bHeadshot = false) const;
};

/** Tiro hitscan (instantâneo) da arma principal, com dispersão, queda de dano e crítico. */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_FireHitscan : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_FireHitscan();
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

/** Impulso rápido na direção do movimento (mobilidade). Magnitude = força. */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_Dash : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_Dash();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

/** Pulso de cura em área para aliados e para si. Magnitude = cura, Radius = raio. */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_HealPulse : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_HealPulse();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

/** Ultimate de teste: onda de choque em área que causa dano e empurra. */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_Shockwave : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_Shockwave();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
