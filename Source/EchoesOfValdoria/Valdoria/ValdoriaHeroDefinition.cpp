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
	// Pairar: parado e calmo (o corpo sobe, desce e inclina por código).
	Animations.Hover = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle")));
	// Andar e correr sem arma (modelo Third Person da Epic).
	auto Un = [](const TCHAR* Sub, const TCHAR* Name)
	{
		return TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(FString::Printf(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/%s%s.%s"), Sub, Name, Name)));
	};
	Animations.Idle = Un(TEXT(""), TEXT("MM_Idle"));
	Animations.WalkForward = Un(TEXT("Walk/"), TEXT("MF_Unarmed_Walk_Fwd"));
	Animations.WalkBackward = Un(TEXT("Walk/"), TEXT("MF_Unarmed_Walk_Bwd"));
	Animations.WalkLeft = Un(TEXT("Walk/"), TEXT("MF_Unarmed_Walk_Left"));
	Animations.WalkRight = Un(TEXT("Walk/"), TEXT("MF_Unarmed_Walk_Right"));
	Animations.JogForward = Un(TEXT("Jog/"), TEXT("MF_Unarmed_Jog_Fwd"));
	Animations.JogBackward = Un(TEXT("Jog/"), TEXT("MF_Unarmed_Jog_Bwd"));
	Animations.JogLeft = Un(TEXT("Jog/"), TEXT("MF_Unarmed_Jog_Left"));
	Animations.JogRight = Un(TEXT("Jog/"), TEXT("MF_Unarmed_Jog_Right"));
	Animations.FallLoop = Un(TEXT("Jump/"), TEXT("MM_Fall_Loop"));
	Animations.FireActions.Add(Un(TEXT("Attack/"), TEXT("MM_Attack_01")));
	Animations.AbilityAction = Un(TEXT("Attack/"), TEXT("MM_Attack_03"));
	Animations.UltimateAction = Un(TEXT("Attack/"), TEXT("MM_ChargedAttack"));
	Animations.Fly = Anim(TEXT("MM_Fly_Loop"));
	Animations.FlyForwardPose = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01")));
	Animations.TakeoffAction = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Jump.MM_Jump")));
	Animations.LandAction = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Land.MM_Land")));
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
	Def->Movement.bCanGlide = true;  // o Recruta testa planar e voar com combustível
	Def->Movement.FlightMode = EValdoriaFlightMode::Fuel;

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
	// The Recruit holds the rifle too: use the same rifle animations, socket and offset as Kael
	// (before this it punched with the rifle in hand).
	Def->ApplyPresetLooks(TEXT("kael"));
	Def->BodyWidth = Def->BodyDepth = Def->BodyHeight = 1.f;
	Def->PassiveName = NSLOCTEXT("Valdoria", "RecruitPassive", "Treinamento básico");
	Def->PassiveDescription = NSLOCTEXT("Valdoria", "RecruitPassiveDesc", "Shift: segurar corre, dois toques dão um impulso (2 cargas). Ctrl agacha (correndo, desliza), X deita, Espaço sobe em beiradas e escala paredes.");
	return Def;
}
