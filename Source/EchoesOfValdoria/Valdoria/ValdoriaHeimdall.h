// Echoes of Valdoria - Heimdall's abilities (turn 47). Kit and numbers: docs/ConceitosHerois.md (all provisional).
//  - RevealPulse: reveals enemies in a radius (or the whole map) to the team for Duration (Sentinela, O Olho que Tudo Vê).
//  - BifrostCall (3): charge, grab the first enemy hero (passes through minions), throw it up and away; the nearest
//    Bifrost catches it and carries it helpless toward its own base for Duration; then it cannot take that bridge
//    for 8 s. Missing = the full cooldown anyway.
//  - Gjallarhorn (4): horn for 1.5 s (control immune, 30% less damage, slower, no attacks); the area moves with him;
//    at the end everyone inside (allies, enemies, minions; never guardians) jumps along a rainbow arc to the aimed
//    spot. Range grows near the Bifrosts. (First version: the spot is aimed with the crosshair; the big-map picker
//    comes next.)

#pragma once

#include "CoreMinimal.h"
#include "ValdoriaGameplayAbility.h"
#include "ValdoriaHeimdall.generated.h"

UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_RevealPulse : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_RevealPulse();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_BifrostCall : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_BifrostCall();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGA_Gjallarhorn : public UValdoriaGameplayAbility
{
	GENERATED_BODY()
public:
	UValdoriaGA_Gjallarhorn();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	/** Jump range from a spot: 40 m + up to 60 m per Bifrost within 30 m (closer = more), max 300 m. */
	static float RangeFrom(const UWorld* World, const FVector& Where);
};
