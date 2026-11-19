// Echoes of Valdoria - valores iniciais dos heróis prontos (Brasa, Kael, Iara e Vex).
// Isto só preenche os Data Assets na primeira vez (Scripts/create_heroes.py). Para
// balancear, edite os arquivos em Content/Valdoria/Heroes no editor, não este código.

#include "ValdoriaHeroDefinition.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Sound/SoundBase.h"
#include "ValdoriaHeroAbilities.h"
#include "ValdoriaHeimdall.h"

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
	return { TEXT("brasa"), TEXT("kael"), TEXT("iara"), TEXT("vex"), TEXT("lume"), TEXT("kay"), TEXT("ariel"), TEXT("ariel_faisca"), TEXT("heimdall") };
}

UValdoriaHeroDefinition* UValdoriaHeroDefinition::CreatePreset(UObject* Outer, FName PresetId)
{
	UValdoriaHeroDefinition* Def = NewObject<UValdoriaHeroDefinition>(Outer);
	if (!Def->ApplyPreset(PresetId))
	{
		return nullptr;
	}
	Def->ApplyPresetLooks(PresetId);
	return Def;
}

bool UValdoriaHeroDefinition::ApplyPreset(FName PresetId)
{
	Modify();
	const FString Id = PresetId.ToString().ToLower();
	Abilities.Reset();
	Roles.Reset();
	Weapon = FValdoriaWeaponStats();
	Movement = FValdoriaMovementRules();
	Movement.bCanProne = true; // X deita (animações do pacote Free Crawl, turno 44); desligue por herói no Data Asset
	Passive = FValdoriaPassiveRules();
	WeaponOffset = FTransform(FRotator(90.f, 0.f, 0.f), FVector::ZeroVector, FVector(1.6f));
	MaxArmor = MaxShield = 0.f;
	Locomotion = EValdoriaLocomotion::Walk;
	HoverHeight = 0.f;
	PhysicalPower = MagicPower = 1.f;
	PhysicalResistance = MagicResistance = 0.f;
	ElementCount = 0; ElementNames.Reset(); ElementColors.Reset(); ElementDamageTypes.Reset();
	UltimateHoldSeconds = 0.f; AlternateFormId = NAME_None;
	bRideEffectPerFoot = false;
	bRailGuardian = false;
	ClairvoyanceRadius = 0.f;

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
		// Turn 45: ability 3 (standard keys 1-2-3). Numbers provisional, for the owner to balance.
		FValdoriaAbilityEntry& Chain = Add(this, EValdoriaAbilitySlot::Ability3, UValdoriaGA_HookPull::StaticClass(), LOCTEXT("BrasaChain", "Corrente de Forja"),
			LOCTEXT("BrasaChainDesc", "Lança uma corrente no inimigo mirado (até 15 m): 30 de dano físico e o puxa até você. Recarga de 12 s."));
		Chain.Cooldown = 12.f; Chain.Magnitude = 30.f; Chain.Radius = 1500.f; Chain.SecondaryMagnitude = 1400.f;
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
		FValdoriaAbilityEntry& Grenade = Add(this, EValdoriaAbilitySlot::Ability3, UValdoriaGA_ThrowZone::StaticClass(), LOCTEXT("KaelGrenade", "Granada de Pulso"),
			LOCTEXT("KaelGrenadeDesc", "Granada que explode ao bater: 50 de dano híbrido num raio de 3 m e deixa um campo elétrico por 3 s (15 de dano por segundo). Recarga de 11 s."));
		Grenade.Cooldown = 11.f; Grenade.Magnitude = 50.f; Grenade.Radius = 300.f; Grenade.Duration = 3.f; Grenade.SecondaryMagnitude = 15.f;
		Grenade.DamageType = EValdoriaDamageType::Hybrid;
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
		FValdoriaAbilityEntry& Current = Add(this, EValdoriaAbilitySlot::Ability3, UValdoriaGA_AllyLink::StaticClass(), LOCTEXT("IaraCurrent", "Correnteza"),
			LOCTEXT("IaraCurrentDesc", "Uma correnteza puxa o aliado mirado (até 20 m) para perto de você e o cura em 60. Recarga de 14 s."));
		Current.Cooldown = 14.f; Current.Magnitude = 60.f; Current.Radius = 2000.f; Current.SecondaryMagnitude = 1.f;
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
		Description = LOCTEXT("VexDesc", "Flanqueadora telecinética. Suas adagas flutuam às costas e voam ao comando da mente; ela levita sobre o campo e escolhe de onde atacar.");
		Roles.Add(EValdoriaHeroRole::Flanker);
		ThemeColor = FLinearColor(0.6f, 0.25f, 0.95f);
		MaxHealth = 150.f;
		MoveSpeed = 600.f;
		MeshScale = 0.95f;
		Mesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(Quinn));
		Movement.ShiftMode = EValdoriaShiftMode::SprintAndDash;
		Movement.DashCharges = 3;
		Movement.MaxJumps = 1;            // holding Space in the air = telekinetic flight
		// Owner (turn 44): she walks on small telekinetic floors she creates under her feet, slightly above the ground
		// (that is why she is silent). Same "walk on an effect" locomotion as Iara's water disc, purple and low.
		Locomotion = EValdoriaLocomotion::RideEffect;
		HoverHeight = 8.f;
		RideEffectColor = FLinearColor(0.55f, 0.25f, 1.f);
		RideEffectScale = FVector(0.3f, 0.3f, 0.02f);  // one disc per foot (owner, turn 45)
		bRideEffectPerFoot = true;
		Movement.bCanWallClimb = false;
		// Owner (turn 44): unlimited flight, but no attack nor abilities while flying (all her telekinesis holds her up).
		Movement.FlightMode = EValdoriaFlightMode::Fuel;
		Movement.bUnlimitedFlight = true;
		Movement.bFlightBlocksActions = true;
		Movement.FlightSpeed = 650.f;     // a little slower than running, so flight is for repositioning (proposal)
		UltimateCost = 1500.f;

		// Telekinetic daggers: no ammo, no reload; slower rate to balance (owner). Numbers are provisional.
		Weapon.WeaponName = LOCTEXT("VexWeapon", "Adagas Telecinéticas");
		Weapon.Damage = 45.f;
		Weapon.FireRate = 1.25f;
		Weapon.bUsesAmmo = false;
		Weapon.MagazineSize = 1;
		Weapon.ReserveAmmo = 0;
		Weapon.SpreadDegrees = 0.f;
		Weapon.ProjectileSpeed = 4500.f;
		Weapon.Range = 2500.f;
		WeaponMesh = ::Weapon(TEXT("AdagaA"));

		// Owner (turn 44): the passive is silence - she makes no noise while moving.
		PassiveName = LOCTEXT("VexPassive", "Passos Silenciosos");
		PassiveDescription = LOCTEXT("VexPassiveDesc", "Você não faz barulho ao se mover: passos, saltos e voo são silenciosos para todos.");
		PassiveIcon = Icon(TEXT("Vex_Passiva"));
		Passive.BackstabMultiplier = 1.f;
		bSilentMovement = true;

		Add(this, EValdoriaAbilitySlot::PrimaryFire, UValdoriaGA_TelekineticDagger::StaticClass(), LOCTEXT("VexFire", "Adagas Telecinéticas"),
			LOCTEXT("VexFireDesc", "Lança uma adaga com a mente (45 de dano físico) e, no mesmo gesto, puxa de volta a anterior, que acerta quem estiver no caminho de volta (60% do dano). A adaga crava no primeiro inimigo ou parede e volta sozinha depois de 4 s. Sem munição e sem recarga."));
		// Owner (turn 44): right click and ultimate become telekinetic and ranged.
		FValdoriaAbilityEntry& Lunge = Add(this, EValdoriaAbilitySlot::SecondaryFire, UValdoriaGA_PiercingBolt::StaticClass(), LOCTEXT("VexLunge", "Corte Telecinético"),
			LOCTEXT("VexLungeDesc", "Lança as duas adagas juntas numa linha reta de 15 m que atravessa todos: 50 de dano físico em cada inimigo no caminho. Recarga de 6 s."), TEXT("Vex_BD"));
		Lunge.Cooldown = 6.f; Lunge.Magnitude = 50.f; Lunge.Radius = 1500.f; Lunge.SecondaryMagnitude = 0.f; Lunge.DamageType = EValdoriaDamageType::Physical;
		FValdoriaAbilityEntry& Cloak = Add(this, EValdoriaAbilitySlot::Ability1, UValdoriaGA_Stealth::StaticClass(), LOCTEXT("VexCloak", "Manto Sombrio"),
			LOCTEXT("VexCloakDesc", "Invisibilidade parcial por 5 s e 20% mais rápida: os inimigos só veem você a menos de 6 m. Atacar ou usar outra habilidade revela. Recarga de 12 s."), TEXT("Vex_F"));
		Cloak.Cooldown = 12.f; Cloak.Duration = 5.f; Cloak.Radius = 600.f; Cloak.SecondaryMagnitude = 0.2f;
		// Owner (turn 44): pull/push did not fit an assassin. Now she flies to the aimed enemy or ally.
		FValdoriaAbilityEntry& Hook = Add(this, EValdoriaAbilitySlot::Ability2, UValdoriaGA_TelekineticDash::StaticClass(), LOCTEXT("VexHook", "Salto Telecinético"),
			LOCTEXT("VexHookDesc", "Mire um inimigo ou aliado (até 18 m) e voe até ele. Inimigo: você o empurra, causa 40 de dano físico e ele perde o equilíbrio por 0,4 s. Aliado: uma mão leva você e a outra cria às suas costas um pequeno escudo telecinético redondo (250 de vida) que dura até você chegar. Sem alvo na mira, nada é gasto. Recarga de 8 s."), TEXT("Vex_E"));
		Hook.Cooldown = 8.f; Hook.Magnitude = 40.f; Hook.Radius = 1800.f; Hook.SecondaryMagnitude = 250.f;
		FValdoriaAbilityEntry& Blink = Add(this, EValdoriaAbilitySlot::Ability3, UValdoriaGA_Teleport::StaticClass(), LOCTEXT("VexBlink", "Passo Sombrio"),
			LOCTEXT("VexBlinkDesc", "Teleporta até o ponto mirado (até 12 m). Depois de 3 s você volta sozinha ao ponto de partida: entre, ataque e saia. Recarga de 12 s."));
		Blink.Cooldown = 12.f; Blink.Radius = 1200.f; Blink.Duration = 3.f; Blink.SecondaryMagnitude = 1.f;
		FValdoriaAbilityEntry& Ult = Add(this, EValdoriaAbilitySlot::Ultimate, UValdoriaGA_TelekineticStorm::StaticClass(), LOCTEXT("VexUlt", "Tempestade de Lâminas"),
			LOCTEXT("VexUltDesc", "Até 5 adagas voam de uma vez, uma para cada inimigo à vista num raio de 12 m (100 de dano físico cada). Elas cravam e voltam sozinhas, acertando de novo no caminho (60%)."), TEXT("Vex_Ult"));
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
		Movement.ShiftMode = EValdoriaShiftMode::SprintAndDash; // owner (turn 45): Shift runs and dashes, paid with vigor (only Lume uses vigor)
		Movement.DashCharges = 2;
		Movement.bUsesVigor = true;
		Movement.bCanGlide = true;
		Movement.GlideFallSpeed = 60.f;   // owner: she sinks very slowly while Space is held
		Movement.GlideAirControl = 1.f;
		JumpVelocity = 820.f;             // owner: longer jump
		AirControl = 0.8f;
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
		Weapon.LifeSteal = 0.25f;    // draining beam (owner): placeholder values for balancing
		Weapon.OverhealMax = 0.3f;
		Weapon.OverhealDecay = 8.f;

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
		FValdoriaAbilityEntry& Pulse = Add(this, EValdoriaAbilitySlot::Ability3, UValdoriaGA_HealPulse::StaticClass(), LOCTEXT("LumePulse", "Pulso de Aurora"),
			LOCTEXT("LumePulseDesc", "Um pulso de luz cura você e os aliados a até 7 m em 60. Recarga de 14 s."));
		Pulse.Cooldown = 14.f; Pulse.Magnitude = 60.f; Pulse.Radius = 700.f;
		FValdoriaAbilityEntry& Ult = Add(this, EValdoriaAbilitySlot::Ultimate, UValdoriaGA_WeaponSurge::StaticClass(), LOCTEXT("LumeUlt", "Aurora"),
			LOCTEXT("LumeUltDesc", "Por 6 s o feixe dobra de dano e de cura, não gasta energia e também acerta quem estiver a até 4 m do alvo."));
		Ult.Duration = 6.f; Ult.Radius = 400.f; Ult.SecondaryMagnitude = 2.f;
		return true;
	}

	// Kay [Agent Kay] (owner's hero, turn 45): non-binary, no weapon, bends fire, earth, water and air.
	// Only the BASE for now: element switch (tap 4, 20 s cooldown), Devastação (hold 4 for 3 s) and an
	// elemental basic attack. Abilities 1/2/3, the right-click ability and the passive come from the owner later.
	if (Id == TEXT("kay"))
	{
		HeroId = TEXT("kay");
		DisplayName = LOCTEXT("KayName", "Kay");
		Description = LOCTEXT("KayDesc", "Agente que dobra os quatro elementos com as mãos, sem arma. Toque no 4 para trocar entre fogo, terra, água e ar; segure o 4 para a Devastação.");
		Roles.Add(EValdoriaHeroRole::Warrior);
		ThemeColor = FLinearColor(0.75f, 0.75f, 0.85f);
		MaxHealth = 200.f;
		MoveSpeed = 560.f;
		Mesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(Manny));
		Movement.ShiftMode = EValdoriaShiftMode::SprintAndDash;
		Movement.DashCharges = 1;
		UltimateCost = 1700.f;

		ElementCount = 4;
		ElementNames = { LOCTEXT("KayFire", "Fogo"), LOCTEXT("KayEarth", "Terra"), LOCTEXT("KayWater", "Água"), LOCTEXT("KayAir", "Ar") };
		ElementColors = { FLinearColor(1.f, 0.35f, 0.08f), FLinearColor(0.55f, 0.38f, 0.18f), FLinearColor(0.15f, 0.55f, 1.f), FLinearColor(0.8f, 0.95f, 1.f) };
		ElementDamageTypes = { EValdoriaDamageType::Magical, EValdoriaDamageType::Physical, EValdoriaDamageType::Magical, EValdoriaDamageType::Hybrid };
		ElementSwitchCooldown = 20.f;
		UltimateHoldSeconds = 3.f;
		UltimateCancelCooldown = 3.f;

		// No weapon: the hands throw an elemental bolt (the element decides the damage type and the colour).
		Weapon.WeaponName = LOCTEXT("KayWeapon", "Sopro Elemental");
		Weapon.DamageType = EValdoriaDamageType::Magical;
		Weapon.Damage = 32.f;
		Weapon.FireRate = 3.f;
		Weapon.bAutomatic = true;
		Weapon.bUsesAmmo = false;
		Weapon.SpreadDegrees = 0.5f;
		Weapon.ProjectileSpeed = 5000.f;
		Weapon.Range = 3000.f;
		WeaponMesh.Reset();

		PassiveName = LOCTEXT("KayPassive", "Passiva (a definir)");
		PassiveDescription = LOCTEXT("KayPassiveDesc", "O dono do projeto vai definir a passiva de Kay.");

		Add(this, EValdoriaAbilitySlot::PrimaryFire, UValdoriaGA_FireProjectile::StaticClass(), LOCTEXT("KayFireAtk", "Sopro Elemental"),
			LOCTEXT("KayFireAtkDesc", "Lança um sopro do elemento atual: 32 de dano. Fogo e água: mágico; terra: físico; ar: híbrido. Sem munição."));
		FValdoriaAbilityEntry& Ult = Add(this, EValdoriaAbilitySlot::Ultimate, UValdoriaGA_Devastation::StaticClass(), LOCTEXT("KayUlt", "Devastação"),
			LOCTEXT("KayUltDesc", "Segure o 4 por 3 s com a suprema carregada: explosão de 9 m com 150 de dano do elemento atual. Fogo: o chão queima. Terra: atordoa por 2 s. Água: 50% mais lentos por 2 s. Ar: joga os inimigos para cima. Soltar antes de completar cancela e dá 3 s de recarga. Toque rápido no 4: troca de elemento (recarga de 20 s)."));
		Ult.Magnitude = 150.f; Ult.Radius = 900.f; Ult.Duration = 2.f; Ult.SecondaryMagnitude = 0.5f;
		Ult.DamageType = EValdoriaDamageType::Magical;
		return true;
	}

	// Ariel (created by Claude, turn 45): bigender, inspired by a gender-shifting character. C swaps the form.
	// Owner: refer to Ariel by name (or "de Ariel"); refer to Kay only by name, never o/a.
	// Forma Rocha (masculine): close-range bruiser. Forma Faísca (feminine): agile ranged energy.
	if (Id == TEXT("heimdall"))
	{
		// Turn 47: owner's hero, kit decided with Claude (docs/ConceitosHerois.md). Placeholder look: mannequin + hammer
		// until the axe mesh is imported. All numbers provisional (owner balances).
		HeroId = TEXT("heimdall");
		DisplayName = LOCTEXT("HeimdallName", "Heimdall");
		Description = LOCTEXT("HeimdallDesc", "O Onividente. Guardião da Bifrost: arremessa machados de longe, enxerga o que ninguém vê e usa as pontes de arco-íris como arma.");
		Roles.Add(EValdoriaHeroRole::Warrior);
		ThemeColor = FLinearColor(0.95f, 0.8f, 0.35f);
		MaxHealth = 250.f;
		MaxArmor = 25.f;
		MoveSpeed = 560.f;
		MeshScale = 1.08f;
		Mesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(Manny));
		Movement.ShiftMode = EValdoriaShiftMode::Sprint;
		UltimateCost = 1800.f;
		bRailGuardian = true;
		ClairvoyanceRadius = 2000.f; // hears enemies within 20 m

		Weapon.WeaponName = LOCTEXT("HeimdallWeapon", "Machados de Gjallar");
		Weapon.DamageType = EValdoriaDamageType::Physical;
		Weapon.Damage = 48.f;
		Weapon.FireRate = 1.6f;
		Weapon.MagazineSize = 8;
		Weapon.ReserveAmmo = 64;
		Weapon.ReloadTime = 1.8f;
		Weapon.SpreadDegrees = 0.f;
		Weapon.Range = 3000.f;
		Weapon.ProjectileSpeed = 3200.f;
		Weapon.ProjectileGravity = 0.35f; // thrown axes fly in an arc
		WeaponMesh = ::Weapon(TEXT("Martelo"));

		PassiveName = LOCTEXT("HeimdallPassive", "Guardião da Ponte");
		PassiveDescription = LOCTEXT("HeimdallPassiveDesc", "Anda em qualquer Bifrost e nunca cai dela quando é atingido, mas lá em cima causa 30% menos dano. Perto de uma Bifrost (fora dela) recebe 15% menos dano, e os aliados perto dele andam 20% mais rápido nas pontes. Ouve inimigos a até 20 m mesmo atrás de paredes e na neblina, e vê todos que estão em alguma Bifrost.");

		Add(this, EValdoriaAbilitySlot::PrimaryFire, UValdoriaGA_FireProjectile::StaticClass(), LOCTEXT("HeimdallFire", "Machados"),
			LOCTEXT("HeimdallFireDesc", "Arremessa machados que voam em arco: 48 de dano físico. 8 por carga."));
		FValdoriaAbilityEntry& Leagues = Add(this, EValdoriaAbilitySlot::SecondaryFire, UValdoriaGA_PiercingBolt::StaticClass(), LOCTEXT("HeimdallRmb", "Cem Léguas"),
			LOCTEXT("HeimdallRmbDesc", "Arremesso longo e certeiro que atravessa todos na linha (até 40 m): 70 de dano físico. Recarga de 9 s."));
		Leagues.Cooldown = 9.f; Leagues.Magnitude = 70.f; Leagues.Radius = 4000.f; Leagues.DamageType = EValdoriaDamageType::Physical;
		FValdoriaAbilityEntry& Sentinel = Add(this, EValdoriaAbilitySlot::Ability1, UValdoriaGA_RevealPulse::StaticClass(), LOCTEXT("HeimdallSentinel", "Sentinela"),
			LOCTEXT("HeimdallSentinelDesc", "Um pulso revela todos os inimigos a até 40 m por 4 s, para o time todo (minimapa e através da neblina). Recarga de 16 s."));
		Sentinel.Cooldown = 16.f; Sentinel.Radius = 4000.f; Sentinel.Duration = 4.f;
		FValdoriaAbilityEntry& Horse = Add(this, EValdoriaAbilitySlot::Ability2, UValdoriaGA_LungeSlash::StaticClass(), LOCTEXT("HeimdallHorse", "Gulltoppr"),
			LOCTEXT("HeimdallHorseDesc", "Investida curta no cavalo espectral de crina dourada: 40 de dano físico em quem estiver no caminho. Recarga de 10 s."));
		Horse.Cooldown = 10.f; Horse.Magnitude = 40.f; Horse.Radius = 900.f;
		FValdoriaAbilityEntry& Call = Add(this, EValdoriaAbilitySlot::Ability3, UValdoriaGA_BifrostCall::StaticClass(), LOCTEXT("HeimdallCall", "Chamado da Bifrost"),
			LOCTEXT("HeimdallCallDesc", "Investida que atravessa minions e agarra o primeiro herói inimigo: 40 de dano mágico, ele é arremessado para cima e a Bifrost mais próxima o leva indefeso por 3 s em direção à base dele. Depois ele fica 8 s sem poder pegar aquela ponte. Errar gasta a recarga inteira (40 s)."));
		Call.Cooldown = 40.f; Call.Magnitude = 40.f; Call.Radius = 1600.f; Call.Duration = 3.f; Call.DamageType = EValdoriaDamageType::Magical;
		FValdoriaAbilityEntry& Eye = Add(this, EValdoriaAbilitySlot::AbilityExtra, UValdoriaGA_RevealPulse::StaticClass(), LOCTEXT("HeimdallEye", "O Olho que Tudo Vê"),
			LOCTEXT("HeimdallEyeDesc", "Por 3 s, todos os inimigos do mapa aparecem para o seu time. Recarga de 90 s."));
		Eye.Cooldown = 90.f; Eye.Radius = 0.f; Eye.Duration = 3.f;
		FValdoriaAbilityEntry& Horn = Add(this, EValdoriaAbilitySlot::Ultimate, UValdoriaGA_Gjallarhorn::StaticClass(), LOCTEXT("HeimdallUlt", "Gjallarhorn"),
			LOCTEXT("HeimdallUltDesc", "Toca o berrante por 1,5 s (imune a controle, 30% menos dano, mais lento). Todos na área em volta dele (aliados, inimigos e minions; nunca guardiões) viajam juntos por um arco-íris até o ponto mirado. Alcance: 40 m, mais até 60 m por Bifrost perto dele (na base, o mapa todo)."));
		Horn.Radius = 450.f; Horn.Duration = 1.5f; Horn.SecondaryMagnitude = 0.3f;
		return true;
	}
	if (Id == TEXT("ariel"))
	{
		HeroId = TEXT("ariel");
		AlternateFormId = TEXT("ariel_faisca");
		DisplayName = LOCTEXT("ArielName", "Ariel (Rocha)");
		Description = LOCTEXT("ArielDesc", "Ariel é bigênero e troca de forma no C. Forma Rocha: pele de pedra, punhos pesados, segura a frente. A vida, a suprema e as recargas passam para a outra forma.");
		Roles.Add(EValdoriaHeroRole::Vanguard);
		ThemeColor = FLinearColor(0.85f, 0.5f, 0.2f);
		MaxHealth = 325.f;
		MaxArmor = 100.f;
		ArmorDamageReduction = 0.3f;
		PhysicalResistance = 0.15f;
		MoveSpeed = 520.f;
		MeshScale = 1.05f;
		Mesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(Manny));
		Movement.ShiftMode = EValdoriaShiftMode::Sprint;
		UltimateCost = 1700.f;

		Weapon.WeaponName = LOCTEXT("ArielWeapon", "Punhos de Rocha");
		Weapon.Damage = 55.f;
		Weapon.FireRate = 1.4f;
		Weapon.bAutomatic = true;
		Weapon.bUsesAmmo = false;
		Weapon.Range = 300.f;
		Weapon.SpreadDegrees = 80.f;
		WeaponMesh.Reset();

		PassiveName = LOCTEXT("ArielPassive", "Duas Metades");
		PassiveDescription = LOCTEXT("ArielPassiveDesc", "Trocar de forma mantém a vida (em porcentagem), a suprema e as recargas. Ao virar Forma Rocha, Ariel ganha 60 de escudo temporário; ao virar Forma Faísca, ganha 25% de velocidade por 2 s. Forma Rocha: 15% menos dano físico.");

		Add(this, EValdoriaAbilitySlot::PrimaryFire, UValdoriaGA_FireMelee::StaticClass(), LOCTEXT("ArielFire", "Punhos de Rocha"),
			LOCTEXT("ArielFireDesc", "Soco largo: 55 de dano físico em todos a até 3 m na sua frente. Segure para continuar batendo."));
		FValdoriaAbilityEntry& Lunge = Add(this, EValdoriaAbilitySlot::SecondaryFire, UValdoriaGA_LungeSlash::StaticClass(), LOCTEXT("ArielLunge", "Investida"),
			LOCTEXT("ArielLungeDesc", "Avança alguns metros e acerta quem estiver no caminho: 45 de dano físico. Recarga de 7 s."));
		Lunge.Cooldown = 7.f; Lunge.Magnitude = 45.f; Lunge.Radius = 600.f;
		FValdoriaAbilityEntry& Slam = Add(this, EValdoriaAbilitySlot::Ability1, UValdoriaGA_GroundSlam::StaticClass(), LOCTEXT("ArielSlam", "Tremor"),
			LOCTEXT("ArielSlamDesc", "Bate no chão: 45 de dano físico num raio de 4,5 m e deixa os inimigos 35% mais lentos por 2 s. Recarga de 9 s."));
		Slam.Cooldown = 9.f; Slam.Magnitude = 45.f; Slam.Radius = 450.f; Slam.Duration = 2.f; Slam.SecondaryMagnitude = 0.35f;
		FValdoriaAbilityEntry& Stim = Add(this, EValdoriaAbilitySlot::Ability2, UValdoriaGA_Stim::StaticClass(), LOCTEXT("ArielStim", "Pele de Pedra"),
			LOCTEXT("ArielStimDesc", "Recupera 80 de vida e fica 20% mais rápido por 3 s. Recarga de 14 s."));
		Stim.Cooldown = 14.f; Stim.Magnitude = 80.f; Stim.Duration = 3.f; Stim.SecondaryMagnitude = 0.2f;
		FValdoriaAbilityEntry& Wall = Add(this, EValdoriaAbilitySlot::Ability3, UValdoriaGA_Barrier::StaticClass(), LOCTEXT("ArielWall", "Muralha de Pedra"),
			LOCTEXT("ArielWallDesc", "Ergue uma parede de pedra de 4 m na sua frente por 4 s que segura 500 de dano. Recarga de 12 s."));
		Wall.Cooldown = 12.f; Wall.Magnitude = 500.f; Wall.Radius = 400.f; Wall.Duration = 4.f;
		FValdoriaAbilityEntry& Swap = Add(this, EValdoriaAbilitySlot::AbilityExtra, UValdoriaGA_FormSwap::StaticClass(), LOCTEXT("ArielSwap", "Trocar de Forma"),
			LOCTEXT("ArielSwapDesc", "Vira a Forma Faísca (feminina, à distância e ágil). Recarga de 4 s."));
		Swap.Cooldown = 4.f;
		// Passive bonus ON ARRIVING in this form (read by SwapToForm): Magnitude = temporary shield, SecondaryMagnitude = speed for 2 s.
		Swap.Magnitude = 60.f;
		FValdoriaAbilityEntry& Ult = Add(this, EValdoriaAbilitySlot::Ultimate, UValdoriaGA_LeapSlam::StaticClass(), LOCTEXT("ArielUlt", "Queda de Montanha"),
			LOCTEXT("ArielUltDesc", "Salta para frente e cai como uma rocha: 180 de dano físico num raio de 7 m e arremessa os inimigos."));
		Ult.Magnitude = 180.f; Ult.Radius = 700.f;
		return true;
	}
	if (Id == TEXT("ariel_faisca"))
	{
		HeroId = TEXT("ariel_faisca");
		AlternateFormId = TEXT("ariel");
		DisplayName = LOCTEXT("ArielFName", "Ariel (Faísca)");
		Description = LOCTEXT("ArielFDesc", "Ariel é bigênero e troca de forma no C. Forma Faísca: energia elétrica à distância, rápida e frágil. A vida, a suprema e as recargas passam para a outra forma.");
		Roles.Add(EValdoriaHeroRole::Warrior);
		ThemeColor = FLinearColor(0.35f, 0.75f, 1.f);
		MaxHealth = 200.f;
		MagicResistance = 0.1f;
		MoveSpeed = 600.f;
		MeshScale = 0.97f;
		Mesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(Quinn));
		Movement.ShiftMode = EValdoriaShiftMode::SprintAndDash;
		Movement.DashCharges = 2;
		Movement.bCanWallClimb = true;
		UltimateCost = 1700.f;

		Weapon.WeaponName = LOCTEXT("ArielFWeapon", "Faíscas");
		Weapon.DamageType = EValdoriaDamageType::Magical;
		Weapon.Damage = 24.f;
		Weapon.FireRate = 5.f;
		Weapon.bAutomatic = true;
		Weapon.MagazineSize = 30;
		Weapon.ReserveAmmo = 90;
		Weapon.ReloadTime = 1.4f;
		Weapon.SpreadDegrees = 1.f;
		Weapon.ProjectileSpeed = 6000.f;
		Weapon.Range = 2500.f;
		WeaponMesh.Reset();

		PassiveName = LOCTEXT("ArielFPassive", "Duas Metades");
		PassiveDescription = LOCTEXT("ArielFPassiveDesc", "Trocar de forma mantém a vida (em porcentagem), a suprema e as recargas. Ao virar Forma Faísca, Ariel ganha 25% de velocidade por 2 s; ao virar Forma Rocha, ganha 60 de escudo temporário. Forma Faísca: dois avanços.");

		Add(this, EValdoriaAbilitySlot::PrimaryFire, UValdoriaGA_FireProjectile::StaticClass(), LOCTEXT("ArielFFire", "Faíscas"),
			LOCTEXT("ArielFFireDesc", "Dispara faíscas rápidas das mãos: 24 de dano mágico cada. Pente de 30."));
		FValdoriaAbilityEntry& Cone = Add(this, EValdoriaAbilitySlot::SecondaryFire, UValdoriaGA_ConeBlast::StaticClass(), LOCTEXT("ArielFCone", "Descarga"),
			LOCTEXT("ArielFConeDesc", "Descarga elétrica num cone de 5 m: 40 de dano mágico, empurra e deixa 35% mais lento por 1 s. Recarga de 7 s."));
		Cone.Cooldown = 7.f; Cone.Magnitude = 40.f; Cone.Radius = 500.f; Cone.Duration = 1.f; Cone.DamageType = EValdoriaDamageType::Magical;
		FValdoriaAbilityEntry& Dash = Add(this, EValdoriaAbilitySlot::Ability1, UValdoriaGA_Dash::StaticClass(), LOCTEXT("ArielFDash", "Relâmpago"),
			LOCTEXT("ArielFDashDesc", "Dispara na direção em que você anda. Recarga de 6 s."));
		Dash.Cooldown = 6.f; Dash.Magnitude = 2400.f;
		FValdoriaAbilityEntry& Bolt = Add(this, EValdoriaAbilitySlot::Ability2, UValdoriaGA_PiercingBolt::StaticClass(), LOCTEXT("ArielFBolt", "Arco Voltaico"),
			LOCTEXT("ArielFBoltDesc", "Raio que atravessa todos na linha (até 25 m): 65 de dano mágico. Recarga de 9 s."));
		Bolt.Cooldown = 9.f; Bolt.Magnitude = 65.f; Bolt.Radius = 2500.f; Bolt.SecondaryMagnitude = 0.f; Bolt.DamageType = EValdoriaDamageType::Magical;
		FValdoriaAbilityEntry& Field = Add(this, EValdoriaAbilitySlot::Ability3, UValdoriaGA_ThrowZone::StaticClass(), LOCTEXT("ArielFField", "Campo Elétrico"),
			LOCTEXT("ArielFFieldDesc", "Arremessa uma esfera de faíscas: 30 de dano mágico ao bater e um campo de 3,5 m por 4 s (15 de dano por segundo). Recarga de 12 s."));
		Field.Cooldown = 12.f; Field.Magnitude = 30.f; Field.Radius = 350.f; Field.Duration = 4.f; Field.SecondaryMagnitude = 15.f;
		Field.DamageType = EValdoriaDamageType::Magical;
		FValdoriaAbilityEntry& Swap = Add(this, EValdoriaAbilitySlot::AbilityExtra, UValdoriaGA_FormSwap::StaticClass(), LOCTEXT("ArielFSwap", "Trocar de Forma"),
			LOCTEXT("ArielFSwapDesc", "Vira a Forma Rocha (masculina, corpo a corpo e resistente). Recarga de 4 s."));
		Swap.Cooldown = 4.f;
		// Passive bonus ON ARRIVING in this form (read by SwapToForm): Magnitude = temporary shield, SecondaryMagnitude = speed for 2 s.
		Swap.SecondaryMagnitude = 0.25f;
		FValdoriaAbilityEntry& Ult = Add(this, EValdoriaAbilitySlot::Ultimate, UValdoriaGA_WeaponSurge::StaticClass(), LOCTEXT("ArielFUlt", "Tempestade"),
			LOCTEXT("ArielFUltDesc", "Por 6 s as faíscas dobram de dano, não gastam munição e explodem num raio de 3 m."));
		Ult.Duration = 6.f; Ult.Radius = 300.f; Ult.SecondaryMagnitude = 2.f;
		return true;
	}
	return false;
}



// ------------------------------------------------------------------ sons dos heróis prontos

static void ApplyPresetSounds(UValdoriaHeroDefinition* Def, const FString& Id)
{
	auto Snd = [](const TCHAR* Category, const TCHAR* Name)
	{
		return TSoftObjectPtr<USoundBase>(FSoftObjectPath(FString::Printf(TEXT("/Game/Valdoria/Audio/%s/%s.%s"), Category, Name, Name)));
	};
	auto SetSound = [Def](EValdoriaAbilitySlot Slot, const TSoftObjectPtr<USoundBase>& Sound)
	{
		for (FValdoriaAbilityEntry& E : Def->Abilities)
		{
			if (E.Slot == Slot)
			{
				E.Sound = Sound;
			}
		}
	};
	Def->FireSounds.Reset();
	Def->FootstepVolume = 0.5f;
	Def->ReloadSound = Snd(TEXT("UI"), TEXT("metalLatch"));
	using S = EValdoriaAbilitySlot;
	if (Id == TEXT("brasa"))
	{
		Def->FireSounds = { Snd(TEXT("Melee"), TEXT("melee_sound")), Snd(TEXT("Melee"), TEXT("sword_sound")) };
		Def->FireVolume = 0.8f;
		Def->FootstepVolume = 0.7f;
		SetSound(S::SecondaryFire, Snd(TEXT("SciFi"), TEXT("forceField_000")));
		SetSound(S::Ability1, Snd(TEXT("Magic"), TEXT("flamethrower")));
		SetSound(S::Ability3, Snd(TEXT("UI"), TEXT("metalClick")));
		SetSound(S::Ability2, Snd(TEXT("SciFi"), TEXT("lowFrequency_explosion_000")));
		SetSound(S::Ultimate, Snd(TEXT("SciFi"), TEXT("explosionCrunch_000")));
	}
	else if (Id == TEXT("kael"))
	{
		Def->FireSounds = { Snd(TEXT("Shots"), TEXT("rifle")) };
		Def->FireVolume = 0.45f;
		SetSound(S::SecondaryFire, Snd(TEXT("Magic"), TEXT("zap2")));
		SetSound(S::Ability1, Snd(TEXT("Melee"), TEXT("swish_3")));
		SetSound(S::Ability3, Snd(TEXT("SciFi"), TEXT("explosionCrunch_000")));
		SetSound(S::Ability2, Snd(TEXT("Magic"), TEXT("heal")));
		SetSound(S::Ultimate, Snd(TEXT("SciFi"), TEXT("laserLarge_000")));
	}
	else if (Id == TEXT("iara"))
	{
		Def->FireSounds = { Snd(TEXT("Magic"), TEXT("water")) };
		Def->FireVolume = 0.5f;
		SetSound(S::SecondaryFire, Snd(TEXT("Magic"), TEXT("wind2")));
		SetSound(S::Ability1, Snd(TEXT("Magic"), TEXT("magicdrop")));
		SetSound(S::Ability3, Snd(TEXT("Magic"), TEXT("water")));
		SetSound(S::Ability2, Snd(TEXT("Magic"), TEXT("spell")));
		SetSound(S::Ultimate, Snd(TEXT("Magic"), TEXT("enchant")));
	}
	else if (Id == TEXT("vex"))
	{
		Def->FireSounds = { Snd(TEXT("Melee"), TEXT("swish_2")), Snd(TEXT("Melee"), TEXT("swish_4")) };
		Def->FireVolume = 0.6f;
		Def->FootstepVolume = 0.f; // passive: silent movement (owner, turn 44)
		Def->bSilentMovement = true;
		SetSound(S::SecondaryFire, Snd(TEXT("Melee"), TEXT("sword_sound")));
		SetSound(S::Ability1, Snd(TEXT("Magic"), TEXT("curse")));
		SetSound(S::Ability2, Snd(TEXT("Melee"), TEXT("Bow")));
		SetSound(S::Ability3, Snd(TEXT("Magic"), TEXT("teleport")));
		SetSound(S::Ultimate, Snd(TEXT("Magic"), TEXT("forcepulse")));
	}
	else if (Id == TEXT("lume"))
	{
		Def->FireSounds = { Snd(TEXT("SciFi"), TEXT("laserSmall_000")), Snd(TEXT("SciFi"), TEXT("laserSmall_001")), Snd(TEXT("SciFi"), TEXT("laserSmall_002")) };
		Def->FireVolume = 0.25f;
		Def->FootstepVolume = 0.f; // flutua: sem passos
		SetSound(S::SecondaryFire, Snd(TEXT("SciFi"), TEXT("forceField_001")));
		SetSound(S::Ability1, Snd(TEXT("SciFi"), TEXT("forceField_002")));
		SetSound(S::Ability3, Snd(TEXT("Magic"), TEXT("heal")));
		SetSound(S::Ability2, Snd(TEXT("Magic"), TEXT("teleport")));
		SetSound(S::Ultimate, Snd(TEXT("Magic"), TEXT("warp")));
	}
	else if (Id == TEXT("kay"))
	{
		Def->FireSounds = { Snd(TEXT("Magic"), TEXT("spell")), Snd(TEXT("Magic"), TEXT("wind2")) };
		Def->FireVolume = 0.45f;
		SetSound(S::Ultimate, Snd(TEXT("SciFi"), TEXT("explosionCrunch_000")));
	}
	else if (Id == TEXT("ariel"))
	{
		Def->FireSounds = { Snd(TEXT("Melee"), TEXT("melee_sound")) };
		Def->FireVolume = 0.7f;
		Def->FootstepVolume = 0.65f;
		SetSound(S::SecondaryFire, Snd(TEXT("Melee"), TEXT("swish_3")));
		SetSound(S::Ability1, Snd(TEXT("SciFi"), TEXT("lowFrequency_explosion_000")));
		SetSound(S::Ability3, Snd(TEXT("SciFi"), TEXT("forceField_000")));
		SetSound(S::Ability2, Snd(TEXT("Magic"), TEXT("heal")));
		SetSound(S::AbilityExtra, Snd(TEXT("Magic"), TEXT("warp")));
		SetSound(S::Ultimate, Snd(TEXT("SciFi"), TEXT("explosionCrunch_000")));
	}
	else if (Id == TEXT("ariel_faisca"))
	{
		Def->FireSounds = { Snd(TEXT("Magic"), TEXT("zap2")) };
		Def->FireVolume = 0.4f;
		SetSound(S::SecondaryFire, Snd(TEXT("Magic"), TEXT("zap2")));
		SetSound(S::Ability1, Snd(TEXT("Magic"), TEXT("teleport")));
		SetSound(S::Ability3, Snd(TEXT("Magic"), TEXT("zap2")));
		SetSound(S::Ability2, Snd(TEXT("SciFi"), TEXT("laserLarge_000")));
		SetSound(S::AbilityExtra, Snd(TEXT("Magic"), TEXT("warp")));
		SetSound(S::Ultimate, Snd(TEXT("Magic"), TEXT("enchant")));
	}
}

// ------------------------------------------------------------------ aparência (animações e corpo)

bool UValdoriaHeroDefinition::ApplyPresetLooks(FName PresetId)
{
	Modify(); // marca o arquivo como alterado (senão o script não salva)
	const FString Id = PresetId.ToString().ToLower();
	AddMissingPresetAbilities(PresetId);
	ApplyPresetSounds(this, Id);
	auto A = [](const TCHAR* Folder, const TCHAR* Name)
	{
		return TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(FString::Printf(TEXT("/Game/Characters/Mannequins/Anims/%s/%s.%s"), Folder, Name, Name)));
	};
	auto Attack = [&A](const TCHAR* Name) { return A(TEXT("Unarmed/Attack"), Name); };
	FValdoriaAnimationSet& S = Animations;
	S.bUseValdoriaAnimator = true;
	S.FireActions.Reset();
	ApplyMovementAnimationPack(); // same movement clips for every ready-made hero
	// Prone (X) is per hero now (owner, turn 45): Scripts/create_heroes.py CAN_PRONE / VALDORIA_NO_PRONE.

	// Grips from the owner's concept art: Brasa hammer (right) + shield (left forearm), Kael rifle,
	// Iara and Lume a staff in the right hand, Vex one curved dagger in each hand.
	auto Grip = [](const FRotator& R, const FVector& L, float Scale) { return FTransform(R, L, FVector(Scale)); };
	OffhandMesh.Reset();
	OffhandSocket = TEXT("hand_l");
	OffhandOffset = FTransform::Identity;
	bWeaponsFloat = false;
	bSilentMovement = false; // Vex sets it again below
	if (Id != TEXT("kael"))
	{
		WeaponSocket = TEXT("hand_r");
	}
	if (Id == TEXT("brasa"))
	{
		WeaponMesh = ::Weapon(TEXT("Martelo"));
		WeaponOffset = Grip(FRotator(180.f, 0.f, 0.f), FVector(-30.f, 0.f, 0.f), 3.f); // handle end in the fist, head down
		OffhandMesh = ::Weapon(TEXT("EscudoA"));
		OffhandOffset = Grip(FRotator(0.f, 0.f, 90.f), FVector::ZeroVector, 2.7f); // face forward on the left arm
	}
	else if (Id == TEXT("iara"))
	{
		WeaponMesh = ::Weapon(TEXT("CajadoA"));
		WeaponOffset = Grip(FRotator(120.f, 0.f, 0.f), FVector(-12.f, 0.f, 21.f), 3.2f); // diagonal, crystal end low in front
	}
	else if (Id == TEXT("vex"))
	{
		// Owner (turn 44): the daggers float behind her back by telekinesis (they bob; see bWeaponsFloat).
		// spine_03 axes: first guess, tune with -ValdoriaWeaponPose + Saved/grip_variants.txt.
		bWeaponsFloat = true;
		WeaponSocket = TEXT("spine_03");
		OffhandSocket = TEXT("spine_03");
		WeaponMesh = ::Weapon(TEXT("AdagaA"));
		WeaponOffset = Grip(FRotator(0.f, 0.f, 25.f), FVector(5.f, 30.f, 18.f), 1.6f);
		bSilentMovement = true;
		bRideEffectPerFoot = true;                     // owner (turn 45): a disc under each foot, not one board
		RideEffectScale = FVector(0.3f, 0.3f, 0.02f);
		OffhandMesh = ::Weapon(TEXT("AdagaB"));
		OffhandOffset = Grip(FRotator(0.f, 0.f, -25.f), FVector(5.f, 30.f, -18.f), 1.6f);
	}
	else if (Id == TEXT("lume"))
	{
		WeaponMesh = ::Weapon(TEXT("CajadoB"));
		WeaponOffset = Grip(FRotator(120.f, 0.f, 0.f), FVector(-12.f, 0.f, 21.f), 3.4f);
	}
	else if (Id == TEXT("kay") || Id == TEXT("ariel") || Id == TEXT("ariel_faisca"))
	{
		WeaponMesh.Reset(); // bare hands: Kay bends the elements, Ariel fights with fists or sparks
	}

	if (Id == TEXT("brasa"))
	{
		// Mulher de corpo largo e forte (tanque), proporções naturais.
		Mesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(Quinn));
		MeshScale = 1.1f;
		BodyWidth = 1.2f; BodyDepth = 1.15f; BodyHeight = 1.03f;
		S.FireActions = { Attack(TEXT("MM_Attack_01")), Attack(TEXT("MM_Attack_02")), Attack(TEXT("MM_Attack_03")) };
		S.FireActionRate = 1.3f;
		S.AbilityAction = Attack(TEXT("MM_Attack_02"));
		S.UltimateAction = Attack(TEXT("MM_ChargedAttack"));
		return true;
	}
	if (Id == TEXT("kael"))
	{
		// Rifle: andar, correr, cair, atirar e recarregar com a arma nas mãos.
		BodyWidth = 1.02f; BodyDepth = 1.f; BodyHeight = 1.03f;
		// As animações de rifle da Epic seguram a arma no soquete HandGrip_R do esqueleto.
		WeaponSocket = TEXT("HandGrip_R");
		WeaponOffset = FTransform(FRotator(0.f, 90.f, 0.f), FVector::ZeroVector, FVector(1.5f));
		S.Idle = A(TEXT("Rifle"), TEXT("MF_Rifle_Idle_ADS"));
		S.WalkForward = A(TEXT("Rifle/Walk"), TEXT("MF_Rifle_Walk_Fwd"));
		S.WalkBackward = A(TEXT("Rifle/Walk"), TEXT("MF_Rifle_Walk_Bwd"));
		S.WalkLeft = A(TEXT("Rifle/Walk"), TEXT("MF_Rifle_Walk_Left"));
		S.WalkRight = A(TEXT("Rifle/Walk"), TEXT("MF_Rifle_Walk_Right"));
		S.JogForward = A(TEXT("Rifle/Jog"), TEXT("MF_Rifle_Jog_Fwd"));
		S.JogBackward = A(TEXT("Rifle/Jog"), TEXT("MF_Rifle_Jog_Bwd"));
		S.JogLeft = A(TEXT("Rifle/Jog"), TEXT("MF_Rifle_Jog_Left"));
		S.JogRight = A(TEXT("Rifle/Jog"), TEXT("MF_Rifle_Jog_Right"));
		S.FallLoop = A(TEXT("Rifle/Jump"), TEXT("MM_Rifle_Jump_Fall_Loop"));
		S.FireActions = { A(TEXT("Rifle"), TEXT("MM_Rifle_Fire")) };
		S.FireActionRate = 1.f;
		S.ReloadAction = A(TEXT("Rifle"), TEXT("MM_Rifle_Reload"));
		S.AbilityAction = A(TEXT("Rifle"), TEXT("MM_Rifle_Equip"));
		S.UltimateAction = A(TEXT("Rifle"), TEXT("MM_Rifle_Equip"));
		return true;
	}
	if (Id == TEXT("iara"))
	{
		BodyWidth = 0.97f; BodyDepth = 1.04f; BodyHeight = 0.98f;
		S.FireActions = { Attack(TEXT("MM_Attack_02")) };
		S.FireActionRate = 1.6f;
		S.AbilityAction = Attack(TEXT("MM_Attack_03"));
		S.UltimateAction = Attack(TEXT("MM_ChargedAttack"));
		return true;
	}
	if (Id == TEXT("vex"))
	{
		// Corpo esguio e reto.
		BodyWidth = 0.9f; BodyDepth = 0.92f; BodyHeight = 1.f;
		// Telekinetic throw: one hand gesture (upper body); flying = upright, arms down, legs hanging (owner).
		S.FireActions = { Attack(TEXT("MM_Attack_02")) };
		S.FireActionRate = 1.5f;
		S.FlightStyle = EValdoriaFlightStyle::Levitate;
		S.Hover = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/StruggleToAscension/Animations/UE5/AS_STA_Air_idle07.AS_STA_Air_idle07")));
		Movement.bUnlimitedFlight = true;      // existing DA gets the flight rules on VALDORIA_APPLY_LOOKS too
		Movement.bFlightBlocksActions = true;
		Movement.FlightMode = EValdoriaFlightMode::Fuel;
		S.AbilityAction = Attack(TEXT("MM_Attack_03"));
		S.UltimateAction = Attack(TEXT("MM_ChargedAttack"));
		return true;
	}
	if (Id == TEXT("lume"))
	{
		BodyWidth = 1.f; BodyDepth = 1.f; BodyHeight = 1.04f;
		HoverBob = 6.f;
		HoverLean = 14.f;
		Movement.ShiftMode = EValdoriaShiftMode::SprintAndDash; // owner (turn 45): vigor + Shift, existing DA gets it on VALDORIA_APPLY_LOOKS
		Movement.DashCharges = FMath::Max(Movement.DashCharges, 2);
		Movement.bUsesVigor = true;
		// Owner (turn 44): Lume can jump, and holding Space in the air makes her sink very slowly (calm hover pose).
		Movement.bCanGlide = true;
		Movement.GlideFallSpeed = 60.f;   // cm/s, very slow
		Movement.GlideAirControl = 1.f;
		Movement.MaxJumps = FMath::Max(Movement.MaxJumps, 1);
		JumpVelocity = 820.f;             // owner: longer jump (default 520-ish); with the slow glide she travels far
		AirControl = 0.8f;                // steer the jump forward
		// Owner's hover video: upright, legs almost straight, toes pointing down (pose gallery #5);
		// sinking with Space: knees a little bent (#3). Fab "Struggle To Ascension".
		S.Hover = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/StruggleToAscension/Animations/UE5/AS_STA_Ground_idle05_to_idle06_AirLoop.AS_STA_Ground_idle05_to_idle06_AirLoop")));
		S.Glide = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/StruggleToAscension/Animations/UE5/AS_STA_Air_idle06.AS_STA_Air_idle06")));
		// Turn 45 (owner: she was stiff in the air): rising = arms lifting (Ascension), falling = legs hanging.
		// No side-glide clips: the Fab ones are belly-down skydives; TickHover banks her body instead.
		S.AirRise = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/StruggleToAscension/Animations/UE5/AS_STA_idle_Ascension.AS_STA_idle_Ascension")));
		S.AirFall = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/StruggleToAscension/Animations/UE5/AS_STA_Air_idle07.AS_STA_Air_idle07")));
		S.GlideLeft.Reset();
		S.GlideRight.Reset();
		// Feixe contínuo: sem golpe a cada pulso; habilidades com gesto.
		S.FireHoldPose = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(TEXT("/Game/FreeAnimationsPack2/Animations/AS_LevitatingFireballCharge2.AS_LevitatingFireballCharge2")));
		S.FireHoldPoseTime = 2.f;
		S.AbilityAction = Attack(TEXT("MM_Attack_03"));
		S.UltimateAction = Attack(TEXT("MM_ChargedAttack"));
		return true;
	}
	if (Id == TEXT("kay"))
	{
		// Androgynous, slim (owner: born male, short black hair, heterochromia; hair and eyes wait for the real model).
		BodyWidth = 0.92f; BodyDepth = 0.95f; BodyHeight = 1.01f;
		S.FireActions = { Attack(TEXT("MM_Attack_02")), Attack(TEXT("MM_Attack_03")) };
		S.FireActionRate = 1.8f;
		S.AbilityAction = Attack(TEXT("MM_Attack_03"));
		S.UltimateAction = Attack(TEXT("MM_ChargedAttack"));
		return true;
	}
	if (Id == TEXT("ariel"))
	{
		BodyWidth = 1.12f; BodyDepth = 1.08f; BodyHeight = 1.02f; // Forma Rocha: broad
		S.FireActions = { Attack(TEXT("MM_Attack_01")), Attack(TEXT("MM_Attack_02")) };
		S.FireActionRate = 1.4f;
		S.AbilityAction = Attack(TEXT("MM_Attack_02"));
		S.UltimateAction = Attack(TEXT("MM_ChargedAttack"));
		return true;
	}
	if (Id == TEXT("ariel_faisca"))
	{
		BodyWidth = 0.93f; BodyDepth = 0.95f; BodyHeight = 1.f;  // Forma Faísca: light
		S.FireActions = { Attack(TEXT("MM_Attack_02")) };
		S.FireActionRate = 1.8f;
		S.AbilityAction = Attack(TEXT("MM_Attack_03"));
		S.UltimateAction = Attack(TEXT("MM_ChargedAttack"));
		return true;
	}
	return false;
}

#undef LOCTEXT_NAMESPACE

int32 UValdoriaHeroDefinition::AddMissingPresetAbilities(FName PresetId)
{
	UValdoriaHeroDefinition* Fresh = NewObject<UValdoriaHeroDefinition>(GetTransientPackage());
	if (!Fresh->ApplyPreset(PresetId))
	{
		return 0;
	}
	int32 Added = 0;
	for (const FValdoriaAbilityEntry& Entry : Fresh->Abilities)
	{
		const bool bHas = Abilities.ContainsByPredicate([&Entry](const FValdoriaAbilityEntry& E) { return E.Slot == Entry.Slot && E.ElementSet == Entry.ElementSet; });
		if (!bHas)
		{
			Modify();
			Abilities.Add(Entry);
			++Added;
		}
	}
	// Passive text written later (e.g. Ariel's arrival bonus): only fill it when the asset still has the old preset text.
	if (PassiveName.IsEmpty())
	{
		PassiveName = Fresh->PassiveName;
		PassiveDescription = Fresh->PassiveDescription;
	}
	return Added;
}
