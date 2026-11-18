// Echoes of Valdoria - sons. Cada som toca com o tom um pouco diferente a cada vez
// (para não soar repetido) e com o volume de efeitos das Configurações.
// Os arquivos ficam em /Game/Valdoria/Audio/<categoria>; origem e licenças em CREDITS.md.

#pragma once

#include "CoreMinimal.h"

class USoundBase;
class UObject;

namespace ValdoriaAudio
{
	/** Carrega /Game/Valdoria/Audio/<Category>/<Name>. */
	ECHOESOFVALDORIA_API USoundBase* Load(const TCHAR* Category, const TCHAR* Name);

	/** Toca no mundo (com distância). PitchVariation 0.08 = até 8% mais grave ou agudo. */
	ECHOESOFVALDORIA_API void PlayAt(const UObject* WorldContext, USoundBase* Sound, const FVector& Location, float Volume = 1.f, float PitchVariation = 0.08f, float BasePitch = 1.f);

	/** Toca sem posição (interface, confirmações). */
	ECHOESOFVALDORIA_API void Play2D(const UObject* WorldContext, USoundBase* Sound, float Volume = 1.f, float PitchVariation = 0.05f, float BasePitch = 1.f);
}
