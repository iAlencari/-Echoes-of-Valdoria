// Echoes of Valdoria - Filho de Nazaré "O Prometido" (turn 48). Kit: docs/ConceitosHerois.md (kit v2 + round 4).
// All numbers provisional (owner balances).
//  - CalmStorm (RMB, Acalmar a Tempestade): area; allies lose control effects, enemies are silenced.
//  - Chalice (1, Cálice e Carne): channel (press again to stop); 1% of his max HP every 0.25 s goes to the aimed ally,
//    never below 1%; the ally gets Ungido de Sangue for 7 s (renewed while fed).
//  - Baskets (2, Multiplicação dos Pães): drops baskets; an ally who walks over one heals over time.
//  - WalkOnWater (3, Caminhar sobre as Águas): dash, ignores slows and takes less damage for a short time.
//  - Pray (C, Rogai por Nós): kneels (cannot move or attack) and recovers health.
//  - Crucified (4, Crucificado): rises, immune to damage and control, heals allies around, pulses that heal and
//    cleanse, allies around only die to big hits; at the end he dies and comes back 3 s later.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdoriaGameplayAbility.h"
#include "ValdoriaNazare.generated.h"

class AValdoriaHeroCharacter;
class UStaticMeshComponent;

UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_CalmStorm : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_CalmStorm();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_Chalice : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_Chalice();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	/** Ends the channel and starts the cooldown (also called when the ally dies or walks away). */
	static void StopChannel(AValdoriaHeroCharacter* Hero, float Cooldown);
};

UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_Baskets : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_Baskets();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_WalkOnWater : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_WalkOnWater();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_Pray : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_Pray();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_Crucified : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_Crucified();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

/** A basket of bread and fish (Multiplicação dos Pães): the first ally hero who touches it heals over time. */
UCLASS()
class ECHOESOFVALDORIA_API AValdoriaBasket : public AActor
{
	GENERATED_BODY()
public:
	AValdoriaBasket();
	virtual void Tick(float DeltaSeconds) override;
	void Setup(AValdoriaHeroCharacter* InOwner, float InHealTotal, float InHealSeconds);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;

	TWeakObjectPtr<AValdoriaHeroCharacter> Giver;
	float HealTotal = 60.f;
	float HealSeconds = 4.f;
};
