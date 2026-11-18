// Echoes of Valdoria - definição de personagem.

#include "ValdoriaHeroDefinition.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"
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
	Animations.Slide = Anim(TEXT("MM_Unarmed_Crouch_Entry"));
	Animations.Mantle = Anim(TEXT("VaultOver"));
	Animations.DashForward = Anim(TEXT("MM_Dash_Forward"));
	Animations.DashBackward = Anim(TEXT("MM_Dash_Backward"));
	Animations.DashLeft = Anim(TEXT("MM_Dash_Left"));
	Animations.DashRight = Anim(TEXT("MM_Dash_Right"));
	// Deitar/rastejar e escalar parede: sem animação gratuita da Epic. Aguardando
	// aprovação do pacote CC0 (Quaternius Universal Animation Library).
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
	Fire.AbilityClass = UValdoriaGA_FireHitscan::StaticClass();
	Def->Abilities.Add(Fire);

	// O impulso agora é do Shift (modo 3: segurar = correr, dois toques = impulso).
	// UValdoriaGA_Dash continua disponível para heróis que queiram impulso numa tecla de habilidade.
	Def->Movement.ShiftMode = EValdoriaShiftMode::SprintAndDash;
	Def->Movement.DashCharges = 2;
	Def->Movement.bCanWallClimb = true; // o Recruta testa tudo
	Def->Movement.MaxJumps = 1;

	FValdoriaAbilityEntry Heal;
	Heal.DisplayName = NSLOCTEXT("Valdoria", "Heal", "Pulso de cura");
	Heal.Slot = EValdoriaAbilitySlot::Ability2;
	Heal.AbilityClass = UValdoriaGA_HealPulse::StaticClass();
	Heal.Cooldown = 12.f;
	Def->Abilities.Add(Heal);

	FValdoriaAbilityEntry Ult;
	Ult.DisplayName = NSLOCTEXT("Valdoria", "Shockwave", "Onda de choque");
	Ult.Slot = EValdoriaAbilitySlot::Ultimate;
	Ult.AbilityClass = UValdoriaGA_Shockwave::StaticClass();
	Def->Abilities.Add(Ult);

	return Def;
}
