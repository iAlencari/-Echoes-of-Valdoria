// Echoes of Valdoria - etiquetas (gameplay tags) nativas.

#include "ValdoriaGameplayTags.h"

namespace ValdoriaTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage, "Data.Damage", "Valor de dano (SetByCaller)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Heal, "Data.Heal", "Valor de cura (SetByCaller)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead, "State.Dead", "Personagem morto");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Reloading, "State.Reloading", "Recarregando a arma");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Hit_Headshot, "Hit.Headshot", "Acerto na cabeça");
}
