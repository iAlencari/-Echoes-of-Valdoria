// Echoes of Valdoria - biblioteca de habilidades reutilizáveis.
// Cada uma lê os números do Data Asset do herói (FValdoriaAbilityEntry):
//   Magnitude, Radius, Duration, SecondaryMagnitude, Cooldown e DamageType.
// O significado de cada número está no comentário da classe. Um herói novo pode
// usar qualquer combinação destas habilidades sem escrever código.

#pragma once

#include "CoreMinimal.h"
#include "ValdoriaGameplayAbility.h"
#include "ValdoriaHeroAbilities.generated.h"


// ---------------------------------------------------------------- ataques principais (botão esquerdo)

/** Golpe corpo a corpo em cone na frente (martelo, lâminas). Usa a arma: Damage, FireRate,
 *  Range = alcance, SpreadDegrees = abertura do cone em graus. */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_FireMelee : public UValdoriaGA_FireHitscan
{
	GENERATED_BODY()
public:
	UValdoriaGA_FireMelee();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

/** Atira um projétil (orbe, adaga, sinalizador). Usa a arma: Damage, FireRate, ProjectileSpeed,
 *  ProjectileGravity, SplashRadius, AllyHeal (cura aliados atingidos), RecoilDegrees. */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_FireProjectile : public UValdoriaGA_FireHitscan
{
	GENERATED_BODY()
public:
	UValdoriaGA_FireProjectile();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

// ---------------------------------------------------------------- defesa

/** Barreira na sua frente que acompanha você e bloqueia tiros inimigos.
 *  Magnitude = vida da barreira, Radius = largura (cm), Duration = segundos. */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_Barrier : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_Barrier();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

// ---------------------------------------------------------------- área

/** Arremessa algo que explode e deixa uma zona no chão.
 *  Magnitude = dano do impacto (ou cura, se bHealingZone), Radius = raio da zona,
 *  Duration = tempo da zona, SecondaryMagnitude = dano (ou cura) por segundo na zona. */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_ThrowZone : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_ThrowZone();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	/** Verdadeiro: a zona cura aliados. Falso: fere inimigos. */
	UPROPERTY(EditDefaultsOnly, Category = "Valdoria")
	bool bHealingZone = false;
	UPROPERTY(EditDefaultsOnly, Category = "Valdoria")
	float ThrowSpeed = 2600.f;
	UPROPERTY(EditDefaultsOnly, Category = "Valdoria")
	float ThrowGravity = 0.5f;
};

/** Zona de cura arremessada (mesma coisa que ThrowZone, já marcada como cura). */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_ThrowHealingZone : public UValdoriaGA_ThrowZone
{
	GENERATED_BODY()
public:
	UValdoriaGA_ThrowHealingZone();
};

/** Pancada no chão ao seu redor: Magnitude = dano, Radius = raio,
 *  SecondaryMagnitude = lentidão (0.4 = 40% mais lento), Duration = tempo da lentidão. */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_GroundSlam : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_GroundSlam();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

/** Salta para frente e explode ao cair: Magnitude = dano, Radius = raio. (Feita para ultimate.) */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_LeapSlam : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_LeapSlam();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

/** Rajada em cone na frente: Magnitude = dano e empurrão, Radius = alcance,
 *  SecondaryMagnitude = cura em aliados no cone, Duration = lentidão de 35% nos inimigos. */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_ConeBlast : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_ConeBlast();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	UPROPERTY(EditDefaultsOnly, Category = "Valdoria")
	float PushForce = 900.f;
	UPROPERTY(EditDefaultsOnly, Category = "Valdoria")
	float HalfAngleDegrees = 35.f;
};

/** Zona grande que segue você: Magnitude = cura por segundo nos aliados, Radius = raio,
 *  Duration = segundos, SecondaryMagnitude = dano por segundo nos inimigos (que ficam 30% lentos). */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_Whirlpool : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_Whirlpool();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

// ---------------------------------------------------------------- tiros especiais

/** Raio que atravessa todos no caminho: Magnitude = dano nos inimigos, Radius = alcance,
 *  SecondaryMagnitude = cura nos aliados atravessados. */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_PiercingBolt : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_PiercingBolt();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

/** Troca a arma por um modo de precisão: Magnitude = dano por tiro, Duration = segundos,
 *  SecondaryMagnitude = tiros no pente. Sem dispersão nem queda de dano. */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_PrecisionMode : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_PrecisionMode();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

/** Até N inimigos por perto levam um golpe cada, e você aparece atrás do último:
 *  Magnitude = dano por golpe, Radius = alcance, SecondaryMagnitude = número de alvos. */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_BladeDance : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_BladeDance();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

// ---------------------------------------------------------------- mobilidade

/** Gancho: puxa VOCÊ até a parede/teto mirado. Radius = alcance, Magnitude = força. */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_GrappleSelf : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_GrappleSelf();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

/** Gancho: puxa o INIMIGO mirado até você. Magnitude = dano, Radius = alcance,
 *  SecondaryMagnitude = força do puxão. */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_HookPull : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_HookPull();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

/** Avanço cortando: corre Radius cm para frente e acerta quem estiver no caminho (Magnitude). */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_LungeSlash : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_LungeSlash();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

/** Teleporte até o ponto mirado (Radius = alcance). Duration > 0: volta sozinho ao ponto de
 *  origem depois desse tempo (SecondaryMagnitude 1 = volta; 0 = não volta). */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_Teleport : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_Teleport();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

/** Ligação com o aliado mirado (Radius = alcance): SecondaryMagnitude 0 = você vai até ele
 *  (avanço), 1 = puxa o aliado até você, 2 = teleporta até ele. Magnitude = cura no aliado. */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_AllyLink : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_AllyLink();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

// ---------------------------------------------------------------- reforços

/** Estimulante: cura você (Magnitude), recarrega a arma e deixa mais rápido
 *  (SecondaryMagnitude 0.3 = +30%) por Duration segundos. */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_Stim : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_Stim();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

/** Invisibilidade parcial: some para os inimigos por Duration segundos, mas aparece quando
 *  algum inimigo está a menos de Radius cm. Atacar revela. SecondaryMagnitude = velocidade extra. */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_Stealth : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_Stealth();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

/** Rastro para deslizar (gelo, tinta, luz...): por Duration segundos você fica mais rápido
 *  (SecondaryMagnitude 0.5 = +50%) e deixa um caminho de Radius cm de largura que acelera os
 *  aliados que passarem por ele (o rastro dura mais 3 s). */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_SlideTrail : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_SlideTrail();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

/** Feixe contínuo (segure o botão): trava no alvo mais perto do centro da mira e, a cada
 *  pulso, fere inimigos (Damage) ou cura aliados (AllyHeal). Usa a arma: Damage e AllyHeal por
 *  pulso, FireRate = pulsos por segundo, Range = alcance, SpreadDegrees = cone para travar,
 *  SplashRadius > 0 = acerta também quem estiver perto do alvo. A passiva BeamRamp aumenta com o tempo. */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_FireBeam : public UValdoriaGA_FireHitscan
{
	GENERATED_BODY()
public:
	UValdoriaGA_FireBeam();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

/** Refletir por Duration segundos: projéteis inimigos voltam e metade do dano de tiros volta
 *  para quem atirou. Durante isso você não ataca (estado "Sem ataque"). */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_Reflect : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_Reflect();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

/** Absorver por Duration segundos: Magnitude (0.6 = 60%) do dano recebido some e vira carga de
 *  ultimate (SecondaryMagnitude pontos de carga por ponto de dano absorvido). */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_Absorb : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_Absorb();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

/** Intangível por Duration segundos: imune a dano físico (mágico ainda acerta), mais rápido
 *  (SecondaryMagnitude), mas sem atacar nem usar habilidades. */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_Intangible : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_Intangible();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

/** Arma turbinada por Duration segundos: dano e cura x SecondaryMagnitude, acerta em área
 *  (Radius) e sem gastar munição. (Feita para ultimate de quem usa feixe ou projétil.) */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_WeaponSurge : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_WeaponSurge();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

/** Portals: first use opens the entrance at the aimed spot, second use (within 10 s) opens the
 *  exit and links them. Duration = how long the pair lasts. SecondaryMagnitude: 0 = only your
 *  team can pass, 1 = everyone (enemies too). Radius = max distance to the aimed spot. */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_Portal : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_Portal();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
protected:
	TWeakObjectPtr<class AValdoriaPortal> PendingEntrance;
	float PendingUntil = 0.f;
};
