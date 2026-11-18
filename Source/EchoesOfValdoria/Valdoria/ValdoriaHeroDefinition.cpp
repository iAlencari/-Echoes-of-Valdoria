// Echoes of Valdoria - definição de personagem.

#include "ValdoriaHeroDefinition.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "ValdoriaGameplayAbility.h"

UValdoriaHeroDefinition::UValdoriaHeroDefinition()
{
	DisplayName = NSLOCTEXT("Valdoria", "RecruitName", "Recruta");
	Description = NSLOCTEXT("Valdoria", "RecruitDesc", "Personagem de teste com rifle automático, impulso e onda de choque.");
	Roles.Add(EValdoriaHeroRole::Warrior);
	// Mannequin do modelo Third Person da Unreal (apenas para testes).
	Mesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple")));
	AnimClass = TSoftClassPtr<UAnimInstance>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C")));

	// Animações gratuitas da Epic (plugin MoverExamples, esqueleto UE5 Mannequin).
	auto Anim = [](const TCHAR* Name)
	{
		const FString Path = FString::Printf(TEXT("/MoverExamples/Characters/Mannequins/Animations/Manny/%s.%s"), Name, Name);
		return TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Path));
	};
	Animations.CrouchIdle = Anim(TEXT("MM_Unarmed_Crouch_Idle"));
	Animations.CrouchWalkForward = Anim(TEXT("MM_Unarmed_Crouch_Walk_Fwd"));
	Animations.CrouchWalkBackward = Anim(TEXT("MM_Unarmed_Crouch_Walk_Bwd"));
	Animations.CrouchWalkLeft = Anim(TEXT("MM_Unarmed_Crouch_Walk_Left"));
	Animations.CrouchWalkRight = Anim(TEXT("MM_Unarmed_Crouch_Walk_Right"));
	// Animações CC0 da Quaternius (Universal Animation Library 1 e 2), convertidas para
	// o esqueleto do Manny por Scripts/retarget_ual.py.
	auto UAL = [](const TCHAR* Name)
	{
		const FString Path = FString::Printf(TEXT("/Game/Valdoria/Animations/Retarget/%s.%s"), Name, Name);
		return TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Path));
	};
	// A conversão automática (retarget) ainda deforma o corpo; enquanto isso, deslizar,
	// subir e escalar usam animações da Epic. Deitar fica com a natação (pedido do dono).
	Animations.Slide = Anim(TEXT("MM_Unarmed_Crouch_Entry"));
	Animations.Mantle = Anim(TEXT("VaultOver"));
	// Escalar parede e deitar: sem animação por enquanto (pedido do dono; as que havia
	// esticavam o corpo ou eram de natação). Preencha WallClimb/ProneIdle/ProneCrawl no
	// Data Asset quando houver animações boas.
	Animations.DashForward = Anim(TEXT("MM_Dash_Forward"));
	Animations.DashBackward = Anim(TEXT("MM_Dash_Backward"));
	Animations.DashLeft = Anim(TEXT("MM_Dash_Left"));
	Animations.DashRight = Anim(TEXT("MM_Dash_Right"));
}

FPrimaryAssetId UValdoriaHeroDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ValdoriaHero"), GetFName());
}

UValdoriaHeroDefinition* UValdoriaHeroDefinition::CreateFallbackRecruit(UObject* Outer)
{
	UValdoriaHeroDefinition* Def = NewObject<UValdoriaHeroDefinition>(Outer, TEXT("FallbackRecruit"));

	FValdoriaAbilityEntry Fire;
	Fire.DisplayName = NSLOCTEXT("Valdoria", "Fire", "Rifle");
	Fire.Slot = EValdoriaAbilitySlot::PrimaryFire;
	Fire.Description = NSLOCTEXT("Valdoria", "FireDesc", "Rifle automático de tiro instantâneo. 20 de dano por bala, dobro na cabeça; o dano cai com a distância. Pente de 30, recarrega no R.");
	Fire.AbilityClass = UValdoriaGA_FireHitscan::StaticClass();
	Def->Abilities.Add(Fire);

	// O impulso agora é do Shift (modo 3: segurar = correr, dois toques = impulso).
	// UValdoriaGA_Dash continua disponível para heróis que queiram impulso numa tecla de habilidade.
	Def->Movement.ShiftMode = EValdoriaShiftMode::SprintAndDash;
	Def->Movement.DashCharges = 2;
	Def->Movement.bCanWallClimb = true; // o Recruta testa tudo
	Def->Movement.MaxJumps = 1;
	Def->Movement.bCanProne = false; // X sem ação por enquanto (pedido do dono)

	FValdoriaAbilityEntry Heal;
	Heal.DisplayName = NSLOCTEXT("Valdoria", "Heal", "Pulso de cura");
	Heal.Slot = EValdoriaAbilitySlot::Ability2;
	Heal.Description = NSLOCTEXT("Valdoria", "HealDesc", "Pulso que cura você e os aliados por perto (80 de vida, raio de 8 m). Recarga de 12 s.");
	Heal.AbilityClass = UValdoriaGA_HealPulse::StaticClass();
	Heal.Cooldown = 12.f;
	Def->Abilities.Add(Heal);

	FValdoriaAbilityEntry Ult;
	Ult.DisplayName = NSLOCTEXT("Valdoria", "Shockwave", "Onda de choque");
	Ult.Slot = EValdoriaAbilitySlot::Ultimate;
	Ult.Description = NSLOCTEXT("Valdoria", "UltDesc", "Onda de choque ao seu redor: 150 de dano e empurra os inimigos num raio de 9 m. Carrega causando dano e com o tempo.");
	Ult.AbilityClass = UValdoriaGA_Shockwave::StaticClass();
	Def->Abilities.Add(Ult);

	// Rifle (Kael) do pacote de armas do Meshy, separado por Scripts/split_weapons.py.
	Def->WeaponMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Valdoria/Weapons/Merged/SM_Arma_Rifle.SM_Arma_Rifle")));
	Def->WeaponOffset = FTransform(FRotator(90.f, 0.f, 0.f), FVector(0.f, 0.f, 0.f), FVector(1.6f));
	Def->PassiveName = NSLOCTEXT("Valdoria", "RecruitPassive", "Treinamento básico");
	Def->PassiveDescription = NSLOCTEXT("Valdoria", "RecruitPassiveDesc", "Shift: segurar corre, dois toques dão um impulso (2 cargas). Ctrl agacha (correndo, desliza), X deita, Espaço sobe em beiradas e escala paredes.");
	return Def;
}
