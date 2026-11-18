// Echoes of Valdoria - efeitos de jogo básicos (dano e cura instantâneos).

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ValdoriaEffects.generated.h"

/** Dano instantâneo. O valor vem por SetByCaller (Data.Damage). */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UValdoriaDamageEffect();
};

/** Cura instantânea. O valor vem por SetByCaller (Data.Heal). */
UCLASS()
class ECHOESOFVALDORIA_API UValdoriaHealEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UValdoriaHealEffect();
};
