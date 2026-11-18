// Echoes of Valdoria - valores iniciais dos heróis prontos (Brasa, Kael, Iara e Vex).
// Isto só preenche os Data Assets na primeira vez (Scripts/create_heroes.py). Para
// balancear, edite os arquivos em Content/Valdoria/Heroes no editor, não este código.

#include "ValdoriaHeroDefinition.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "ValdoriaHeroAbilities.h"

#define LOCTEXT_NAMESPACE "ValdoriaHeroes"

namespace
{
	TSoftObjectPtr<UTexture2D> Icon(const TCHAR* Name)
	{
		return TSoftObjectPtr<UTexture2D>(FSoftObjectPath(FString::Printf(TEXT("/Game/Valdoria/UI/Icons/T_%s.T_%s"), Name, Name)));
	}

	TSoftObjectPtr<UStaticMesh> Weapon(const TCHAR* Name)
	{
		return TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(FString::Printf(TEXT("/Game/Valdoria/Weapons/Merged/SM_Arma_%s.SM_Arma_%s"), Name, Name)));
	}

	FValdoriaAbilityEntry& Add(UValdoriaHeroDefinition* Def, EValdoriaAbilitySlot Slot, TSubclassOf<UValdoriaGameplayAbility> Class,
		const FText& Name, const FText& Desc, const TCHAR* IconName = nullptr)
	{
		FValdoriaAbilityEntry& E = Def->Abilities.AddDefaulted_GetRef();
		E.Slot = Slot;
		E.AbilityClass = Class;
		E.DisplayName = Name;
		E.Description = Desc;
		if (IconName)
		{
			E.Icon = Icon(IconName);
		}
		return E;
	}

	const TCHAR* Manny = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple");
	const TCHAR* Quinn = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple");
}

TArray<FName> UValdoriaHeroDefinition::GetPresetIds()
{
	return { TEXT("brasa"), TEXT("kael"), TEXT("iara"), TEXT("vex"), TEXT("lume") };
}

UValdoriaHeroDefinition* UValdoriaHeroDefinition::CreatePreset(UObject* Outer, FName PresetId)
{
	UValdoriaHeroDefinition* Def = NewObject<UValdoriaHeroDefinition>(Outer);
	return Def->ApplyPreset(PresetId) ? Def : nullptr;
}

bool UValdoriaHeroDefinition::ApplyPreset(FName PresetId)
{
	const FString Id = PresetId.ToString().ToLower();
	Abilities.Reset();
	Roles.Reset();
	Weapon = FValdoriaWeaponStats();
	Movement = FValdoriaMovementRules();
	Movement.bCanProne = false; // X sem ação por enquanto
	Passive = FValdoriaPassiveRules();
	WeaponOffset = FTransform(FRotator(90.f, 0.f, 0.f), FVector::ZeroVector, FVector(1.6f));
	MaxArmor = MaxShield = 0.f;
	Locomotion = EValdoriaLocomotion::Walk;
	HoverHeight = 0.f;
	PhysicalPower = MagicPower = 1.f;
	PhysicalResistance = MagicResistance = 0.f;

	if (Id == TEXT("brasa"))
	{
		HeroId = TEXT("brasa");
		DisplayName = LOCTEXT("BrasaName", "Brasa");
		Description = LOCTEXT("BrasaDesc", "Vanguarda forjadora. Segura a linha de frente com o martelo e a barreira hexagonal e protege quem fica perto dela.");
		Roles.Add(EValdoriaHeroRole::Vanguard);
		ThemeColor = FLinearColor(1.f, 0.45f, 0.1f);
		MaxHealth = 400.f;
		MaxArmor = 200.f;
		ArmorDamageReduction = 0.3f;
		PhysicalResistance = 0.1f;
		MoveSpeed = 500.f;
		CapsuleRadius = 46.f;
		CapsuleHalfHeight = 100.f;
		MeshScale = 1.08f;
		Mesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(Manny));
		Movement.ShiftMode = EValdoriaShiftMode::Sprint;
		Movement.bCanWallClimb = false;
		Movement.bCanGlide = false;
		UltimateCost = 1800.f;

		Weapon.WeaponName = LOCTEXT("BrasaWeapon", "Martelo de Forja");
		Weapon.Damage = 70.f;
		Weapon.FireRate = 1.1f;
		Weapon.bAutomatic = true;
		Weapon.bUsesAmmo = false;
		Weapon.Range = 380.f;
		Weapon.SpreadDegrees = 100.f;
		WeaponMesh = ::Weapon(TEXT("Martelo"));

		PassiveName = LOCTEXT("BrasaPassive", "Coração de Forja");
		PassiveDescription = LOCTEXT("BrasaPassiveDesc", "Cada aliado a até 8 m reduz o dano que você recebe em 10% (máximo 30%). Além disso, 5% do dano que esses aliados levam vem para você no lugar deles.");
		PassiveIcon = Icon(TEXT("Brasa_Passiva"));
		Passive.Radius = 800.f;
		Passive.DamageReductionPerAlly = 0.1f;
		Passive.MaxDamageReduction = 0.3f;
		Passive.AllyDamageShare = 0.05f;

		Add(this, EValdoriaAbilitySlot::PrimaryFire, UValdoriaGA_FireMelee::StaticClass(), LOCTEXT("BrasaFire", "Martelo de Forja"),
			LOCTEXT("BrasaFireDesc", "Golpe largo na sua frente: 70 de dano físico em todos a até 3,8 m. Segure para continuar batendo. Não usa munição."));
		FValdoriaAbilityEntry& Shield = Add(this, EValdoriaAbilitySlot::SecondaryFire, UValdoriaGA_Barrier::StaticClass(), LOCTEXT("BrasaShield", "Escudo Hexagonal"),
			LOCTEXT("BrasaShieldDesc", "Ergue uma barreira de 4,5 m na sua frente que acompanha você por 6 s e segura 800 de dano. Tiros inimigos param nela; os do seu time passam. Recarga de 10 s."), TEXT("Brasa_BD"));
		Shield.Cooldown = 10.f; Shield.Magnitude = 800.f; Shield.Radius = 450.f; Shield.Duration = 6.f;
		FValdoriaAbilityEntry& Flare = Add(this, EValdoriaAbilitySlot::Ability1, UValdoriaGA_ThrowZone::StaticClass(), LOCTEXT("BrasaFlare", "Sinalizador"),
			LOCTEXT("BrasaFlareDesc", "Dispara um sinalizador que explode (40 de dano híbrido: metade físico, metade mágico) e deixa o chão em chamas por 4 s (20 de dano por segundo num raio de 3 m). Recarga de 9 s."), TEXT("Brasa_F"));
		Flare.Cooldown = 9.f; Flare.Magnitude = 40.f; Flare.Radius = 300.f; Flare.Duration = 4.f; Flare.SecondaryMagnitude = 20.f;
		Flare.DamageType = EValdoriaDamageType::Hybrid;
		FValdoriaAbilityEntry& Slam = Add(this, EValdoriaAbilitySlot::Ability2, UValdoriaGA_GroundSlam::StaticClass(), LOCTEXT("BrasaSlam", "Pisão Sísmico"),
			LOCTEXT("BrasaSlamDesc", "Bate no chão: 50 de dano físico num raio de 5 m, joga os inimigos para cima e os deixa 40% mais lentos por 2 s. Recarga de 8 s."), TEXT("Brasa_E"));
		Slam.Cooldown = 8.f; Slam.Magnitude = 50.f; Slam.Radius = 500.f; Slam.Duration = 2.f; Slam.SecondaryMagnitude = 0.4f;
		FValdoriaAbilityEntry& Ult = Add(this, EValdoriaAbilitySlot::Ultimate, UValdoriaGA_LeapSlam::StaticClass(), LOCTEXT("BrasaUlt", "Martelo Ardente"),
			LOCTEXT("BrasaUltDesc", "Salta para frente e, ao cair, explode: 200 de dano físico num raio de 8 m e arremessa os inimigos para longe."), TEXT("Brasa_Ult"));
		Ult.Magnitude = 200.f; Ult.Radius = 800.f;
		return true;
	}

	if (Id == TEXT("kael"))
	{
		HeroId = TEXT("kael");
		DisplayName = LOCTEXT("KaelName", "Kael Rook");
		Description = LOCTEXT("KaelDesc", "Guerreiro de longo alcance com braço mecânico. Rifle preciso, gancho para ganhar altura e um modo de precisão devastador.");
		Roles.Add(EValdoriaHeroRole::Warrior);
		ThemeColor = FLinearColor(0.15f, 0.75f, 0.7f);
		MaxHealth = 200.f;
		MoveSpeed = 560.f;
		Mesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(Manny));
		Movement.ShiftMode = EValdoriaShiftMode::SprintAndDash;
		Movement.DashCharges = 2;
		Movement.bCanWallClimb = true;
		UltimateCost = 1600.f;

		Weapon.WeaponName = LOCTEXT("KaelWeapon", "Rifle Rook");
		Weapon.Damage = 20.f;
		Weapon.FireRate = 9.f;
		Weapon.MagazineSize = 30;
		Weapon.ReserveAmmo = 90;
		Weapon.ReloadTime = 1.5f;
		Weapon.SpreadDegrees = 1.6f;
		Weapon.RecoilDegrees = 0.35f;
		WeaponMesh = ::Weapon(TEXT("Rifle"));

		PassiveName = LOCTEXT("KaelPassive", "Olho de Falcão");
		PassiveDescription = LOCTEXT("KaelPassiveDesc", "Parado ou agachado, a dispersão do rifle cai para 40%. Acertos na cabeça causam 25% a mais de dano.");
		PassiveIcon = Icon(TEXT("Kael_Passiva"));
		Passive.StillSpreadMultiplier = 0.4f;
		Passive.HeadshotBonus = 0.25f;

		Add(this, EValdoriaAbilitySlot::PrimaryFire, UValdoriaGA_FireHitscan::StaticClass(), LOCTEXT("KaelFire", "Rifle Rook"),
			LOCTEXT("KaelFireDesc", "Rifle automático de tiro instantâneo: 20 de dano físico por bala, dobro na cabeça. A mira sobe um pouco a cada tiro (recuo). Pente de 30, recarrega no R."));
		FValdoriaAbilityEntry& Arm = Add(this, EValdoriaAbilitySlot::SecondaryFire, UValdoriaGA_ConeBlast::StaticClass(), LOCTEXT("KaelArm", "Punho Galvânico"),
			LOCTEXT("KaelArmDesc", "Descarga elétrica do braço mecânico num cone de 6 m: 55 de dano híbrido, empurra e deixa 35% mais lento por 1,5 s. Recarga de 7 s."), TEXT("Kael_BD"));
		Arm.Cooldown = 7.f; Arm.Magnitude = 55.f; Arm.Radius = 600.f; Arm.Duration = 1.5f;
		Arm.DamageType = EValdoriaDamageType::Hybrid;
		FValdoriaAbilityEntry& Hook = Add(this, EValdoriaAbilitySlot::Ability1, UValdoriaGA_GrappleSelf::StaticClass(), LOCTEXT("KaelHook", "Gancho de Âncora"),
			LOCTEXT("KaelHookDesc", "Prende um gancho na parede ou teto mirado (até 25 m) e puxa você até lá. Só gasta a recarga se acertar algo. Recarga de 8 s."), TEXT("Kael_F"));
		Hook.Cooldown = 8.f; Hook.Radius = 2500.f; Hook.Magnitude = 2200.f;
		FValdoriaAbilityEntry& Stim = Add(this, EValdoriaAbilitySlot::Ability2, UValdoriaGA_Stim::StaticClass(), LOCTEXT("KaelStim", "Estimulante"),
			LOCTEXT("KaelStimDesc", "Cura 50 de vida, enche o pente na hora e deixa você 30% mais rápido por 4 s. Recarga de 12 s."), TEXT("Kael_E"));
		Stim.Cooldown = 12.f; Stim.Magnitude = 50.f; Stim.Duration = 4.f; Stim.SecondaryMagnitude = 0.3f;
		FValdoriaAbilityEntry& Ult = Add(this, EValdoriaAbilitySlot::Ultimate, UValdoriaGA_PrecisionMode::StaticClass(), LOCTEXT("KaelUlt", "Rifle de Elite"),
			LOCTEXT("KaelUltDesc", "Por 8 s o rifle vira uma arma de precisão: 6 tiros de 110 de dano, sem dispersão nem queda de dano, x2,5 na cabeça."), TEXT("Kael_Ult"));
		Ult.Magnitude = 110.f; Ult.Duration = 8.f; Ult.SecondaryMagnitude = 6.f;
		return true;
	}

	if (Id == TEXT("iara"))
	{
		HeroId = TEXT("iara");
		DisplayName = LOCTEXT("IaraName", "Iara");
		Description = LOCTEXT("IaraDesc", "Retaguarda das marés. Seus orbes de água ferem inimigos e curam aliados, e sua presença mantém o time de pé.");
		Roles.Add(EValdoriaHeroRole::Rearguard);
		ThemeColor = FLinearColor(0.2f, 0.85f, 0.8f);
		MaxHealth = 150.f;
		MaxShield = 50.f;
		MagicResistance = 0.1f;
		MoveSpeed = 550.f;
		Mesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(Quinn));
		Movement.ShiftMode = EValdoriaShiftMode::SprintAndDash;
		Movement.DashCharges = 1;
		Movement.bCanGlide = true;
		UltimateCost = 1700.f;
		UltimateChargePerSecond = 6.f;

		Weapon.WeaponName = LOCTEXT("IaraWeapon", "Cajado das Marés");
		Weapon.DamageType = EValdoriaDamageType::Magical;
		Weapon.Damage = 40.f;
		Weapon.FireRate = 2.2f;
		Weapon.MagazineSize = 24;
		Weapon.ReserveAmmo = 72;
		Weapon.ReloadTime = 1.6f;
		Weapon.SpreadDegrees = 0.f;
		Weapon.ProjectileSpeed = 3500.f;
		Weapon.ProjectileGravity = 0.15f;
		Weapon.AllyHeal = 35.f;
		WeaponMesh = ::Weapon(TEXT("CajadoA"));

		PassiveName = LOCTEXT("IaraPassive", "Maré Serena");
		PassiveDescription = LOCTEXT("IaraPassiveDesc", "Você e os aliados a até 8 m de você recuperam 6 de vida por segundo.");
		PassiveIcon = Icon(TEXT("Iara_Passiva"));
		// Anda sobre um disco de água, um pouco acima do chão (a hitbox sobe junto).
		Locomotion = EValdoriaLocomotion::RideEffect;
		HoverHeight = 14.f;
		Passive.Radius = 800.f;
		Passive.AuraHealPerSecond = 6.f;

		Add(this, EValdoriaAbilitySlot::PrimaryFire, UValdoriaGA_FireProjectile::StaticClass(), LOCTEXT("IaraFire", "Orbe d'Água"),
			LOCTEXT("IaraFireDesc", "Lança orbes de água que fazem uma leve curva: 40 de dano mágico em inimigos ou 35 de cura em aliados atingidos. Pente de 24."));
		FValdoriaAbilityEntry& Wave = Add(this, EValdoriaAbilitySlot::SecondaryFire, UValdoriaGA_ConeBlast::StaticClass(), LOCTEXT("IaraWave", "Onda"),
			LOCTEXT("IaraWaveDesc", "Onda num cone de 7 m: 30 de dano mágico e empurra os inimigos; cura 40 os aliados no caminho. Recarga de 8 s."), TEXT("Iara_BD"));
		Wave.Cooldown = 8.f; Wave.Magnitude = 30.f; Wave.Radius = 700.f; Wave.SecondaryMagnitude = 40.f;
		Wave.DamageType = EValdoriaDamageType::Magical;
		FValdoriaAbilityEntry& Crystal = Add(this, EValdoriaAbilitySlot::Ability1, UValdoriaGA_ThrowHealingZone::StaticClass(), LOCTEXT("IaraCrystal", "Cristal de Maré"),
			LOCTEXT("IaraCrystalDesc", "Arremessa um cristal que cura 40 ao bater e cria uma poça de 4 m que cura 30 por segundo por 5 s. Recarga de 12 s."), TEXT("Iara_F"));
		Crystal.Cooldown = 12.f; Crystal.Magnitude = 40.f; Crystal.Radius = 400.f; Crystal.Duration = 5.f; Crystal.SecondaryMagnitude = 30.f;
		Crystal.DamageType = EValdoriaDamageType::Magical;
		FValdoriaAbilityEntry& Lance = Add(this, EValdoriaAbilitySlot::Ability2, UValdoriaGA_PiercingBolt::StaticClass(), LOCTEXT("IaraLance", "Lança de Cristal"),
			LOCTEXT("IaraLanceDesc", "Raio de cristal que atravessa todos na linha (até 30 m): 60 de dano mágico nos inimigos e 60 de cura nos aliados. Recarga de 8 s."), TEXT("Iara_E"));
		Lance.Cooldown = 8.f; Lance.Magnitude = 60.f; Lance.Radius = 3000.f; Lance.SecondaryMagnitude = 60.f;
		Lance.DamageType = EValdoriaDamageType::Magical;
		FValdoriaAbilityEntry& Ult = Add(this, EValdoriaAbilitySlot::Ultimate, UValdoriaGA_Whirlpool::StaticClass(), LOCTEXT("IaraUlt", "Redemoinho"),
			LOCTEXT("IaraUltDesc", "Um redemoinho de 9 m segue você por 6 s: aliados dentro curam 50 por segundo; inimigos levam 15 de dano mágico por segundo e ficam 30% mais lentos."), TEXT("Iara_Ult"));
		Ult.Magnitude = 50.f; Ult.Radius = 900.f; Ult.Duration = 6.f; Ult.SecondaryMagnitude = 15.f;
		Ult.DamageType = EValdoriaDamageType::Magical;
		return true;
	}

	if (Id == TEXT("vex"))
	{
		HeroId = TEXT("vex");
		DisplayName = LOCTEXT("VexName", "Vex");
		Description = LOCTEXT("VexDesc", "Flanqueadora das sombras. Chega sem ser vista, puxa um alvo isolado e o finaliza pelas costas.");
		Roles.Add(EValdoriaHeroRole::Flanker);
		ThemeColor = FLinearColor(0.6f, 0.25f, 0.95f);
		MaxHealth = 150.f;
		MoveSpeed = 600.f;
		MeshScale = 0.95f;
		Mesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(Quinn));
		Movement.ShiftMode = EValdoriaShiftMode::SprintAndDash;
		Movement.DashCharges = 3;
		Movement.MaxJumps = 2;
		Movement.bCanWallClimb = true;
		UltimateCost = 1500.f;

		Weapon.WeaponName = LOCTEXT("VexWeapon", "Lâminas de Arremesso");
		Weapon.Damage = 35.f;
		Weapon.FireRate = 3.f;
		Weapon.MagazineSize = 8;
		Weapon.ReserveAmmo = 24;
		Weapon.ReloadTime = 1.3f;
		Weapon.SpreadDegrees = 0.f;
		Weapon.ProjectileSpeed = 7000.f;
		Weapon.ProjectileGravity = 0.3f;
		WeaponMesh = ::Weapon(TEXT("LaminasA"));

		PassiveName = LOCTEXT("VexPassive", "Golpe Furtivo");
		PassiveDescription = LOCTEXT("VexPassiveDesc", "Todo dano que você causa pelas costas do inimigo é 50% maior.");
		PassiveIcon = Icon(TEXT("Vex_Passiva"));
		Passive.BackstabMultiplier = 1.5f;

		Add(this, EValdoriaAbilitySlot::PrimaryFire, UValdoriaGA_FireProjectile::StaticClass(), LOCTEXT("VexFire", "Lâminas de Arremesso"),
			LOCTEXT("VexFireDesc", "Arremessa adagas rápidas que caem um pouco com a distância: 35 de dano físico, dobro na cabeça. 8 adagas por pente."));
		FValdoriaAbilityEntry& Lunge = Add(this, EValdoriaAbilitySlot::SecondaryFire, UValdoriaGA_LungeSlash::StaticClass(), LOCTEXT("VexLunge", "Corte Duplo"),
			LOCTEXT("VexLungeDesc", "Avança 7 m cortando: 50 de dano físico em quem estiver no caminho. Recarga de 6 s."), TEXT("Vex_BD"));
		Lunge.Cooldown = 6.f; Lunge.Magnitude = 50.f; Lunge.Radius = 700.f;
		FValdoriaAbilityEntry& Cloak = Add(this, EValdoriaAbilitySlot::Ability1, UValdoriaGA_Stealth::StaticClass(), LOCTEXT("VexCloak", "Manto Sombrio"),
			LOCTEXT("VexCloakDesc", "Invisibilidade parcial por 5 s e 20% mais rápida: os inimigos só veem você a menos de 6 m. Atacar ou usar outra habilidade revela. Recarga de 12 s."), TEXT("Vex_F"));
		Cloak.Cooldown = 12.f; Cloak.Duration = 5.f; Cloak.Radius = 600.f; Cloak.SecondaryMagnitude = 0.2f;
		FValdoriaAbilityEntry& Hook = Add(this, EValdoriaAbilitySlot::Ability2, UValdoriaGA_HookPull::StaticClass(), LOCTEXT("VexHook", "Gancho Sombrio"),
			LOCTEXT("VexHookDesc", "Lança um gancho (até 20 m): o inimigo atingido leva 30 de dano, é puxado até você e fica lento por 1 s. Recarga de 9 s."), TEXT("Vex_E"));
		Hook.Cooldown = 9.f; Hook.Magnitude = 30.f; Hook.Radius = 2000.f; Hook.SecondaryMagnitude = 1600.f;
		FValdoriaAbilityEntry& Ult = Add(this, EValdoriaAbilitySlot::Ultimate, UValdoriaGA_BladeDance::StaticClass(), LOCTEXT("VexUlt", "Dança das Lâminas"),
			LOCTEXT("VexUltDesc", "Golpeia até 5 inimigos à vista num raio de 12 m (100 de dano cada, +50% pelas costas) e aparece atrás do último."), TEXT("Vex_Ult"));
		Ult.Magnitude = 100.f; Ult.Radius = 1200.f; Ult.SecondaryMagnitude = 5.f;
		return true;
	}
	if (Id == TEXT("lume"))
	{
		HeroId = TEXT("lume");
		DisplayName = LOCTEXT("LumeName", "Lume");
		Description = LOCTEXT("LumeDesc", "Guardiã de luz que flutua sobre o chão. Seu feixe trava no alvo: queima inimigos ou cura aliados, e fica mais forte quanto mais tempo segura.");
		Roles.Add(EValdoriaHeroRole::Rearguard);
		Roles.Add(EValdoriaHeroRole::Warrior);
		ThemeColor = FLinearColor(1.f, 0.8f, 0.3f);
		MaxHealth = 175.f;
		MaxShield = 50.f;
		MagicResistance = 0.15f;
		MoveSpeed = 540.f;
		Mesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(Quinn));
		Locomotion = EValdoriaLocomotion::Float;
		HoverHeight = 30.f;
		Movement.ShiftMode = EValdoriaShiftMode::Sprint;
		Movement.bCanGlide = true;
		UltimateCost = 1600.f;

		Weapon.WeaponName = LOCTEXT("LumeWeapon", "Feixe Solar");
		Weapon.DamageType = EValdoriaDamageType::Magical;
		Weapon.Damage = 6.f;          // por pulso: 10 pulsos/s = 60 de dano por segundo
		Weapon.AllyHeal = 5.f;        // 50 de cura por segundo
		Weapon.FireRate = 10.f;
		Weapon.bAutomatic = true;
		Weapon.MagazineSize = 100;
		Weapon.ReserveAmmo = 0;
		Weapon.ReloadTime = 1.8f;
		Weapon.Range = 1500.f;
		Weapon.SpreadDegrees = 8.f;
		WeaponMesh = ::Weapon(TEXT("CajadoB"));

		PassiveName = LOCTEXT("LumePassive", "Foco Crescente");
		PassiveDescription = LOCTEXT("LumePassiveDesc", "Mantendo o feixe no mesmo alvo, o dano e a cura sobem aos poucos, até +50% depois de 3 s.");
		Passive.BeamRampMax = 0.5f;
		Passive.BeamRampTime = 3.f;

		Add(this, EValdoriaAbilitySlot::PrimaryFire, UValdoriaGA_FireBeam::StaticClass(), LOCTEXT("LumeFire", "Feixe Solar"),
			LOCTEXT("LumeFireDesc", "Segure: um feixe de 15 m trava no alvo mais perto da mira. Inimigos levam 60 de dano mágico por segundo; aliados recebem 50 de cura por segundo. Energia: 100 (10 s de feixe)."));
		FValdoriaAbilityEntry& Mirror = Add(this, EValdoriaAbilitySlot::SecondaryFire, UValdoriaGA_Reflect::StaticClass(), LOCTEXT("LumeMirror", "Espelho de Luz"),
			LOCTEXT("LumeMirrorDesc", "Por 2 s, projéteis inimigos que acertam você voltam para quem atirou, e metade do dano de tiros volta também. Você não ataca enquanto reflete. Recarga de 10 s."));
		Mirror.Cooldown = 10.f; Mirror.Duration = 2.f;
		FValdoriaAbilityEntry& Prism = Add(this, EValdoriaAbilitySlot::Ability1, UValdoriaGA_Absorb::StaticClass(), LOCTEXT("LumePrism", "Prisma Faminto"),
			LOCTEXT("LumePrismDesc", "Por 3 s, 60% do dano que você recebe some e vira carga de ultimate (3 de carga por ponto absorvido). Recarga de 12 s."));
		Prism.Cooldown = 12.f; Prism.Duration = 3.f; Prism.Magnitude = 0.6f; Prism.SecondaryMagnitude = 3.f;
		FValdoriaAbilityEntry& Phase = Add(this, EValdoriaAbilitySlot::Ability2, UValdoriaGA_Intangible::StaticClass(), LOCTEXT("LumePhase", "Forma de Luz"),
			LOCTEXT("LumePhaseDesc", "Por 2 s você vira luz: imune a dano físico (o mágico ainda acerta) e 30% mais rápida, mas sem atacar nem usar habilidades. Recarga de 14 s."));
		Phase.Cooldown = 14.f; Phase.Duration = 2.f; Phase.SecondaryMagnitude = 0.3f;
		Phase.DamageType = EValdoriaDamageType::Magical;
		FValdoriaAbilityEntry& Ult = Add(this, EValdoriaAbilitySlot::Ultimate, UValdoriaGA_WeaponSurge::StaticClass(), LOCTEXT("LumeUlt", "Aurora"),
			LOCTEXT("LumeUltDesc", "Por 6 s o feixe dobra de dano e de cura, não gasta energia e também acerta quem estiver a até 4 m do alvo."));
		Ult.Duration = 6.f; Ult.Radius = 400.f; Ult.SecondaryMagnitude = 2.f;
		return true;
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
