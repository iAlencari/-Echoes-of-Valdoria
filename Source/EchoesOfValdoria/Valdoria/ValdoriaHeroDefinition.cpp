// Echoes of Valdoria - definição de personagem.

#include "ValdoriaHeroDefinition.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "ValdoriaGameplayAbility.h"
#include "ValdoriaHeroAbilities.h"

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
	// Fab packs added by the owner (Free Animation Set, Free Crawl Animation; Fab Standard License).
	auto Pack = [](const TCHAR* Path) { return TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Path)); };
	Animations.Glide = Pack(TEXT("/Game/FreeAnimationSet/Animations/Manny/Gliding_Forward_Anim.Gliding_Forward_Anim"));
	Animations.GlideLeft = Pack(TEXT("/Game/FreeAnimationSet/Animations/Manny/Gliding_Left_Anim.Gliding_Left_Anim"));   // turn 45: banking while gliding sideways
	Animations.GlideRight = Pack(TEXT("/Game/FreeAnimationSet/Animations/Manny/Gliding_Right_Anim.Gliding_Right_Anim"));
	// RidePose: Dive_Forward from the pack is a head-first vertical dive (looked wrong on the Bifrost); empty = fall loop + lean.
	ApplyMovementAnimationPack(); // overrides crouch/slide/mantle/prone/wall climb with the better Fab clips
	Animations.FireActions.Add(Un(TEXT("Attack/"), TEXT("MM_Attack_01")));
	Animations.AbilityAction = Un(TEXT("Attack/"), TEXT("MM_Attack_03"));
	Animations.UltimateAction = Un(TEXT("Attack/"), TEXT("MM_ChargedAttack"));
	// Fly (hovering while flying) is set by ApplyMovementAnimationPack (was MM_Fly_Loop, arms out).
	Animations.FlyForwardPose = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01")));
	Animations.TakeoffAction = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Jump.MM_Jump")));
	Animations.LandAction = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Land.MM_Land")));
}

void UValdoriaHeroDefinition::ApplyMovementAnimationPack()
{
	// All clips below use the UE5 Mannequin skeleton (checked: compatible with SK_Mannequin, no root motion).
	// FAL = Fab "Free Animation Library", FC = Fab "Free Crawl Animation" (Fab Standard License, owner's library).
	auto FAL = [](const TCHAR* SubPath, const TCHAR* Name)
	{
		return TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(FString::Printf(TEXT("/Game/FreeAnimationLibrary/Animations/%s/%s.%s"), SubPath, Name, Name)));
	};
	auto FC = [](const TCHAR* SubPath, const TCHAR* Name)
	{
		return TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(FString::Printf(TEXT("/Game/Free_Crawl_Animation/Animations/%s/%s.%s"), SubPath, Name, Name)));
	};
	FValdoriaAnimationSet& S = Animations;
	// Crouch: real crouch-walk loops in four directions.
	S.CrouchIdle = FAL(TEXT("Crouch"), TEXT("anim_Crouch_Idle"));
	S.CrouchWalkForward = FAL(TEXT("Crouch"), TEXT("anim_Crouch_Fwd_Loop"));
	S.CrouchWalkBackward = FAL(TEXT("Crouch"), TEXT("anim_Crouch_Bwd_Loop"));
	S.CrouchWalkLeft = FAL(TEXT("Crouch"), TEXT("anim_Crouch_Left_Loop"));
	S.CrouchWalkRight = FAL(TEXT("Crouch"), TEXT("anim_Crouch_Right_Loop"));
	// A real slide (before: the crouch-entry clip) and a proper mantle / wall climb.
	S.Slide = FAL(TEXT("Slide"), TEXT("anim_Slide_L"));
	S.Mantle = FAL(TEXT("Mantle"), TEXT("anim_Mantle_1M_R"));
	S.WallClimb = FAL(TEXT("Climbing_Animations/Climbing"), TEXT("anim_Climb_Up"));
	// Prone: idle + crawl in every direction, with lie-down and get-up transitions.
	S.ProneIdle = FC(TEXT("Idle"), TEXT("anim_Prone_Idle"));
	S.ProneCrawl = FC(TEXT("Loop"), TEXT("anim_Prone_Fwd_Loop_L"));
	S.ProneCrawlBackward = FAL(TEXT("UnarmedProne"), TEXT("anim_Prone_Bwd_Loop_R"));
	S.ProneCrawlLeft = FAL(TEXT("UnarmedProne"), TEXT("anim_Prone_Left_loop"));
	S.ProneCrawlRight = FAL(TEXT("UnarmedProne"), TEXT("anim_Prone_Right_Loop"));
	S.ProneEnter = FC(TEXT("TransitionToProne"), TEXT("anim_Stand_To_Prone"));
	S.ProneExit = FC(TEXT("TransitionToProne"), TEXT("anim_Prone_To_Stand"));

	// Air poses chosen in the pose gallery (Fab "Struggle To Ascension", "Free Animation Set"):
	auto STA = [](const TCHAR* Name)
	{
		return TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(FString::Printf(TEXT("/Game/StruggleToAscension/Animations/UE5/%s.%s"), Name, Name)));
	};
	S.Fly = STA(TEXT("AS_STA_Air_idle07"));  // hovering while flying: legs hanging, calm (was the arms-out fly loop)
	// Turn 45 (owner: more flight poses): moving = knees bent, diving = head-first dive from the Free Animation Set.
	S.FlyMove = STA(TEXT("AS_STA_Air_idle06"));
	S.FlyDown = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/FreeAnimationSet/Animations/Manny/Dive_Downward_A_Anim.Dive_Downward_A_Anim")));
	// Bifrost: horizontal = glide pose leaning forward; vertical (rising at the spawn) = upright, toes pointing down.
	// The Fab glide clip is a belly-down skydive (looked lying down); legs-hanging air idle + 28 degrees forward lean instead.
	S.RidePose = STA(TEXT("AS_STA_Air_idle07"));
	S.RideVerticalPose = STA(TEXT("AS_STA_Ground_idle05_to_idle06_AirLoop"));
	S.RideLean = 28.f;
}

FPrimaryAssetId UValdoriaHeroDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ValdoriaHero"), GetFName());
}

UValdoriaHeroDefinition* UValdoriaHeroDefinition::CreateFallbackRecruit(UObject* Outer)
{
	UValdoriaHeroDefinition* Def = NewObject<UValdoriaHeroDefinition>(Outer, TEXT("FallbackRecruit"));
	Def->Movement.bAimGlide = true; // owner: the Recruit falls slower while aiming/shooting in the air

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
	Def->Movement.bCanProne = true; // X deita com a animação do pacote Free Crawl (Fab); o dono já baixou
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

	// Test bench for library abilities: portals on F, slide trail on C.
	FValdoriaAbilityEntry Portal;
	Portal.DisplayName = NSLOCTEXT("Valdoria", "Portal", "Portal");
	Portal.Slot = EValdoriaAbilitySlot::Ability1;
	Portal.Description = NSLOCTEXT("Valdoria", "PortalDesc", "First press opens the entrance where you aim, second press (within 10 s) opens the exit. Your team walks through to the other side for 12 s. Cooldown 14 s.");
	Portal.AbilityClass = UValdoriaGA_Portal::StaticClass();
	Portal.Cooldown = 14.f;
	Def->Abilities.Add(Portal);
	FValdoriaAbilityEntry Trail;
	Trail.DisplayName = NSLOCTEXT("Valdoria", "Trail", "Light Trail");
	Trail.Slot = EValdoriaAbilitySlot::Ability3;
	Trail.Description = NSLOCTEXT("Valdoria", "TrailDesc", "For 4 s you skate 50% faster and leave a trail that speeds up allies who touch it. Cooldown 12 s.");
	Trail.AbilityClass = UValdoriaGA_SlideTrail::StaticClass();
	Trail.Cooldown = 12.f;
	Def->Abilities.Add(Trail);

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
