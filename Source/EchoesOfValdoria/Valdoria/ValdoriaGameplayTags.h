// Echoes of Valdoria - etiquetas (gameplay tags) nativas.

#pragma once

#include "NativeGameplayTags.h"

namespace ValdoriaTags
{
	/** Valor de dano passado para o efeito de dano (SetByCaller). */
	ECHOESOFVALDORIA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage);
	/** Valor de cura passado para o efeito de cura (SetByCaller). */
	ECHOESOFVALDORIA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Heal);
	/** Personagem morto. */
	ECHOESOFVALDORIA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);
	/** Recarregando a arma. */
	ECHOESOFVALDORIA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Reloading);
	/** Acerto na cabeça (vai no contexto do efeito de dano). */
	ECHOESOFVALDORIA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Hit_Headshot);
}
