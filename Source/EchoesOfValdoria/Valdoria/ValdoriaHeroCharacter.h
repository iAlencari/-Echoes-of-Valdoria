// Echoes of Valdoria - personagem jogável genérico. Tudo que é específico de
// cada herói vem do UValdoriaHeroDefinition (Data Asset).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpecHandle.h"
#include "ValdoriaTypes.h"
#include "ValdoriaShop.h"
#include "ValdoriaHeroCharacter.generated.h"

class UAbilitySystemComponent;
class UValdoriaAttributeSet;
class UValdoriaHeroDefinition;
class UAnimSequenceBase;
class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogValdoria, Log, All);

UCLASS()
class ECHOESOFVALDORIA_API AValdoriaHeroCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AValdoriaHeroCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void Tick(float DeltaSeconds) override;

	// --- Herói
	/** Data Asset do herói. Vazio = "Recruta" de teste criado em código. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_HeroDefinition, Category = "Herói")
	TObjectPtr<UValdoriaHeroDefinition> HeroDefinition;

	/** Troca o herói em tempo real (servidor). */
	UFUNCTION(BlueprintCallable, Category = "Herói")
	void SetHeroDefinition(UValdoriaHeroDefinition* NewDefinition);

	const UValdoriaHeroDefinition* GetHeroDefinition() const { return ActiveDefinition; }
	const FValdoriaAbilityEntry* FindAbilityEntry(EValdoriaAbilitySlot Slot) const;
	const FValdoriaWeaponStats& GetWeaponStats() const;
	float GetArmorDamageReduction() const;

	/** Time: inimigos são todos com TeamId diferente. 255 = alvo de treino. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Herói")
	uint8 TeamId = 0;

	bool IsEnemy(const AActor* Other) const;

	// --- Arma
	bool CanFireWeapon() const;
	void ConsumeAmmo();
	void GetAimRay(FVector& OutStart, FVector& OutDirection) const;
	static bool IsHeadshot(const FHitResult& Hit);
	int32 GetAmmo() const { return CurrentAmmo; }
	bool IsReloading() const { return bReloading; }
	void StartReload();
	/** Enche o pente na hora (estimulante, renascer). */
	void RefillAmmo() { bReloading = false; CurrentAmmo = GetWeaponStats().MagazineSize; }

	// --- Recargas e ultimate
	float GetCooldownRemaining(EValdoriaAbilitySlot Slot) const;
	void StartCooldown(EValdoriaAbilitySlot Slot, float Seconds);
	bool IsUltimateReady() const;
	float GetUltimatePercent() const;
	void ConsumeUltimate();
	void AddUltimateCharge(float Amount);

	// --- Vida e morte
	bool IsDead() const { return bIsDead; }
	/** God mode (admin) or timed damage immunity (Crucificado, standing up after a saved lethal hit). */
	bool IsInvulnerable() const;
	virtual void HandleDeath(AValdoriaHeroCharacter* Killer);
	virtual void OnDamageTaken(AValdoriaHeroCharacter* DamageInstigator, float Damage, bool bHeadshot);
	virtual void OnDamageDealt(AValdoriaHeroCharacter* Victim, float Damage, bool bHeadshot);
	float GetHealth() const;
	float GetMaxHealth() const;
	float GetArmor() const;
	float GetMaxArmor() const;
	float GetShield() const;
	float GetMaxShield() const;

	// --- Efeitos (cosméticos)
	void PlayShotEffects(const FVector& Impact, bool bHit);
	void PlayAreaEffect(const FVector& Center, float Radius, FLinearColor Color);

	// --- Painel de administrador (só servidor)
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Admin")
	bool bGodMode = false;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Admin")
	bool bNoCooldowns = false;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Admin")
	bool bInfiniteAmmo = false;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Admin")
	bool bInfiniteDashes = false;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Admin")
	float AdminSpeedMultiplier = 1.f;
	/** Flight fuel never runs out (admin). */
	UPROPERTY(BlueprintReadOnly, Category = "Admin")
	bool bInfiniteFlight = false;
	/** Test only (-ValdoriaPoseGallery): forces this base animation, ignoring the Data Asset. */
	TSoftObjectPtr<UAnimSequenceBase> DebugBaseAnim;
	/** Admin: normal flight (hold Space in the air, Ctrl goes down), infinite, for any hero. Collides with the world. */
	UPROPERTY(BlueprintReadOnly, Category = "Admin")
	bool bAdminFlight = false;
	/** Admin override of air control (negative = use the hero's value). */
	float AdminAirControl = -1.f;
	void AdminSetSpeedMultiplier(float Multiplier);
	float GetDamageMultiplier() const;
	void AdminSetDamageMultiplier(float Multiplier);
	void AdminFillUltimate();
	void AdminRestoreAll();

	/** Troca a arma visível (painel de admin). */
	void SetWeaponMesh(UStaticMesh* NewWeapon);

	/** Câmera em primeira pessoa (tecla V alterna). */
	void SetFirstPerson(bool bEnable);
	bool IsFirstPerson() const { return bFirstPerson; }

	/** Voar e atravessar paredes (painel de admin). */
	void SetNoclip(bool bEnable);
	bool IsNoclip() const { return bNoclip; }

	/** Aplica dano em outro personagem pelo GAS, como se fosse um tiro deste (bots usam isto). */
	void DealDamageTo(AActor* Target, float Damage, bool bHeadshot = false);

	/** Volta ao estado inicial (vida cheia, munição, sem recargas). */
	void ResetCombatState();

	// --- Dano por tipo, cura e efeitos (habilidades e projéteis usam isto; só servidor)
	/**
	 * Dano com tipo: aplica Poder Físico/Mágico de quem ataca, resistências e passivas
	 * (cabeça, costas, proteção de aliado) e então chama DealDamageTo. Também acerta barreiras
	 * inimigas. Retorna o dano final.
	 */
	float DealTypedDamage(AActor* Target, float BaseDamage, EValdoriaDamageType Type, float MagicShare = 0.5f, bool bHeadshot = false);
	void HealTarget(AActor* Target, float Amount);
	/** Fica mais lento: Fraction 0.3 = 30% mais devagar, por Seconds. */
	void ApplySlow(float Fraction, float Seconds);
	/** Fica mais rápido: Fraction 0.3 = 30% mais rápido, por Seconds. */
	void ApplySpeedBuff(float Fraction, float Seconds);
	/** Aplica um estado de controle por Seconds (o maior tempo vale). */
	void ApplyStatus(EValdoriaStatus Status, float Seconds);
	bool HasStatus(EValdoriaStatus Status) const;
	/** Pode usar este botão agora (considerando atordoado, silêncio etc.)? */
	bool CanUseSlot(EValdoriaAbilitySlot Slot) const;
	/** Invisibilidade parcial: some para os inimigos até chegar a RevealDistance deles. */
	void StartStealth(float Seconds, float RevealDistance, EValdoriaAbilitySlot FromSlot);
	void BreakStealth();
	bool IsStealthActive() const { return bStealthActive; }
	/** Invisível agora (ativo e sem inimigo perto). */
	bool IsHiddenByStealth() const { return bStealthActive && !bStealthRevealed; }
	/** Troca os números da arma por um tempo (ex.: ultimate de precisão). */
	void SetWeaponOverride(const FValdoriaWeaponStats& Stats, float Seconds);
	bool HasWeaponOverride() const { return bHasWeaponOverride; }
	/** Salta para frente e explode ao cair. */
	void StartLeapSlam(float Damage, float Radius, EValdoriaDamageType Type);
	/** Mira sobe (recuo). */
	void ApplyRecoil(float Degrees);
	/** Dispersão atual x (passiva de mira parada). */
	float GetSpreadMultiplier() const;
	/** Barreiras do próprio time não bloqueiam os próprios tiros. */
	void IgnoreFriendlyBarriers(FCollisionQueryParams& Params) const;
	void IgnoreFriendlyBarriers(UPrimitiveComponent* Component) const;
	const FValdoriaPassiveRules& GetPassive() const;
	float GetPassiveDamageReduction() const { return PassiveReduction; }
	int32 GetAlliesNearby() const { return AlliesNearby; }
	float GetPhysicalPower() const;
	float GetMagicPower() const;
	float GetPhysicalResistance() const;
	float GetMagicResistance() const;
	// --- Defesas especiais (habilidades)
	/** Refletir: projéteis inimigos voltam e metade do dano de tiros volta para quem atirou. */
	void StartReflect(float Seconds) { ReflectEndTime = GetWorld()->GetTimeSeconds() + Seconds; }
	bool IsReflecting() const { return GetWorld()->GetTimeSeconds() < ReflectEndTime; }
	/** Absorver: Fraction do dano recebido é anulado e vira carga de ultimate (x ChargePerPoint). */
	void StartAbsorb(float Seconds, float Fraction, float ChargePerPoint) { AbsorbEndTime = GetWorld()->GetTimeSeconds() + Seconds; AbsorbFraction = Fraction; AbsorbChargePerPoint = ChargePerPoint; }
	bool IsAbsorbing() const { return GetWorld()->GetTimeSeconds() < AbsorbEndTime; }
	/** Intangível: imune a dano físico (o mágico ainda acerta). */
	void StartIntangible(float Seconds) { IntangibleEndTime = GetWorld()->GetTimeSeconds() + Seconds; }
	bool IsIntangible() const { return GetWorld()->GetTimeSeconds() < IntangibleEndTime; }
	/** Feixe contínuo: alvo travado e há quanto tempo. */
	TWeakObjectPtr<AActor> BeamTarget;
	float BeamLockTime = 0.f;

	/** Bônus da loja (a loja do jogador chama isto ao comprar e ao renascer). */
	void SetShopBonus(const FValdoriaShopBonus& Bonus);
	/** Autopilot: the hero plays by itself with its real kit (bots, and /auto for the player). Skill 0..1 = aim. */
	void SetAutopilot(bool bEnable, float Skill = 0.6f);
	bool IsAutopilot() const { return bAutopilot; }
	/** Autopilot stays within Radius of Center (MOBA spot guardians). Radius 0 = free. */
	void SetLeash(const FVector& Center, float Radius) { LeashCenter = Center; LeashRadius = Radius; }
	/** With nobody in sight, the autopilot walks these points in order (MOBA lanes). */
	// --- Aurora Trails (MOBA lane rails)
	bool TryMountRail();
	void StopRiding();
	bool IsRiding() const { return RideRail.IsValid(); }
	/** Riding the vertical part of a Bifrost (rising at the spawn). */
	bool bRideVertical = false;
	// --- Elements (Kay), hold-to-cast ultimate, forms (Ariel) and vigor (Lume) - turn 44
	int32 CurrentElement = 0;
	float ElementSwitchReadyTime = 0.f;
	float UltHoldStart = -1.f;
	void SwitchElement();
	int32 GetCurrentElement() const { return CurrentElement; }
	FLinearColor GetElementColor() const;
	FText GetElementName() const;
	EValdoriaDamageType GetElementDamageType(EValdoriaDamageType Fallback) const;
	/** 0..1 while the ultimate key is held on a hold-to-cast ultimate; -1 when not holding. */
	float GetUltimateHoldProgress() const;
	float GetElementSwitchRemaining() const;
	void TickUltimateHold();
	/** Swaps to another hero definition (Ariel's other form) next tick, keeping health %, ultimate charge and cooldowns. */
	void SwapToForm(FName FormId);
	float Vigor = -1.f;
	float VigorLastSpend = -10.f;
	void TickVigor(float DeltaSeconds);
	float GetVigorPercent() const;

	/** Vex telekinetic dash (turn 44): flies to Target (follows it if it moves, gives up after 1.5 s). Enemy: push, damage
	 *  and a brief stagger on arrival. Ally: just arrives; Shield (behind her during the flight) is removed on arrival. */
	void StartTelekineticDash(AValdoriaHeroCharacter* Target, float Speed, float Damage, float Push, AActor* Shield);
	bool IsTelekineticDashing() const { return TkDashTarget.IsValid(); }
	void TickTelekineticDash();
	TWeakObjectPtr<AValdoriaHeroCharacter> TkDashTarget;
	TWeakObjectPtr<AActor> TkDashShield;
	float TkDashEnd = 0.f;
	float TkDashSpeed = 2500.f;
	float TkDashDamage = 0.f;
	float TkDashPush = 0.f;
	/** Autopilot pathfinding (turn 44): direction of the next navmesh path point towards Goal (straight line if there
	 *  is no navmesh). Re-paths every 0.6 s or when the goal moves more than 4 m. */
	FVector AutopilotNavDirection(const FVector& Goal);
	TArray<FVector> NavPath;
	int32 NavPathIndex = 0;
	float NavRepathTime = 0.f;
	FVector NavGoal = FVector::ZeroVector;
	FVector AutopilotRoamGoal = FVector::ZeroVector;
	float AutopilotRoamTime = 0.f;
	/** Falling below this height kills the hero (computed from the lowest player start, -30 m). */
	float FallKillZ = TNumericLimits<float>::Lowest();
	void TickFallOutOfMap();
	/** Invisible map limits (owner, turn 44): a box around the map's geometry. Sides block like walls; the sky
	 *  is a soft ceiling where flying slows down to a stop. Computed once from the level's static meshes. */
	void TickMapBounds();
	FBox MapBounds = FBox(ForceInit);
	float MapCeilingZ = 0.f;
	bool bMapBoundsReady = false;
	static constexpr float MapBoundsPadding = 500.f;   // cm past the outermost geometry (fallback when there is no ground)
	static constexpr float MapGroundInset = 60.f;      // turn 45: cm INSIDE the edge of the ground pieces (no walking off the floor)
	static constexpr float MapCeilingAbove = 2500.f;   // cm above the highest geometry
	static constexpr float MapCeilingSoft = 1500.f;    // slow-down band under the ceiling
	FString GetRideLaneName() const;
	// --- Seal / reverse-seal (hold G next to a defeated spot guardian)
	class AValdoriaGuardian* FindChannelTarget() const;
	float GetChannelProgress() const { return ChannelTarget.IsValid() ? ChannelProgress : 0.f; }
	FString GetChannelSubtitle() const;
	bool bInteractHeld = false;
	/** Future surfer hero: can ride enemy trails too. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Herói")
	bool bCanRideAnyRail = false;
	/** Combat states: general = hit/healed someone; passive = took damage or control from an enemy. */
	bool IsInPassiveCombat(float Seconds) const;
	/** Real HP per second regenerated out of combat (owner: 1 HP every 0.5 s). */
	float OutOfCombatRegenHP = 2.f;

	// ---------------- Turn 46: unit kinds, levels, match stats, V punch ----------------
	bool IsMinionUnit() const;
	bool IsGuardianUnit() const;
	/** A real hero (player or bot hero), not a minion, guardian or training target. */
	bool IsHeroUnit() const;

	/** MOBA only: levels 1-20, 1 point per level for abilities 1-4 (see docs/RegrasCombate.md). */
	bool bUsesLevels = false;
	int32 Level = 1;
	float XP = 0.f;
	int32 AbilityPoints = 0;
	int32 ElevationPoints = 0;
	/** Ranks of abilities 1, 2, 3 and the supreme (0 = locked, max 5). */
	int32 AbilityRanks[4] = { 0, 0, 0, 0 };
	static int32 RankIndex(EValdoriaAbilitySlot Slot);
	int32 GetAbilityRank(EValdoriaAbilitySlot Slot) const;
	bool IsSlotLocked(EValdoriaAbilitySlot Slot) const;
	bool CanUpgrade(EValdoriaAbilitySlot Slot) const;
	bool TryUpgrade(EValdoriaAbilitySlot Slot);
	/** Damage/heal multiplier and cooldown multiplier from the rank (rank 1 = data values). */
	float GetRankPowerMultiplier(EValdoriaAbilitySlot Slot) const;
	float GetRankCooldownMultiplier(EValdoriaAbilitySlot Slot) const;
	float XPToNextLevel() const;
	void GainXP(float Amount);
	void AutoSpendPoints();

	/** Match scoreboard (Tab). Damage, healing and damage taken in real HP. */
	struct FMatchStats { int32 Kills = 0; int32 Deaths = 0; int32 Assists = 0; int32 Seals = 0; float Damage = 0.f; float Healing = 0.f; float DamageTaken = 0.f; float Gold = 0.f; };
	FMatchStats Stats;
	/** Who hit this hero recently (assists): attacker -> time. */
	TMap<TWeakObjectPtr<AValdoriaHeroCharacter>, float> RecentDamagers;
	/** Death recap (hold Y, owner): damage received, by attacker and source. */
	struct FDamageLogEntry { FString Attacker; FString Source; float Amount = 0.f; float Time = 0.f; bool bAttackerIsHero = false; };
	TArray<FDamageLogEntry> DamageLog;
	struct FDeathRecap { bool bValid = false; FString Killer; FString KillerSource; float Total = 0.f; TArray<FDamageLogEntry> Lines; };
	FDeathRecap LastDeathRecap;
	/** Name of what is dealing damage right now (set around ability/punch damage); empty = weapon. */
	FString DamageSourceLabel;
	FString GetUnitName() const;

	/** Level, XP, ranks and stats, carried to the new body when the player respawns. */
	struct FProgress { bool bValid = false; bool bUsesLevels = false; int32 Level = 1; float XP = 0.f; int32 AbilityPoints = 0; int32 ElevationPoints = 0; int32 Ranks[4] = { 0, 0, 0, 0 }; FMatchStats Stats; FDeathRecap Recap; };
	FProgress TakeProgress() const;
	void ApplyProgress(const FProgress& P);

	/** V: melee punch, every hero (owner). Damage in design units, from the hero data. */
	void TryMeleePunch();
	float NextPunchTime = 0.f;
	/** Temporary bonuses (buffs such as Ungido de Sangue): vampirism = basic attack and V, life steal = abilities. */
	float BonusVampirism = 0.f;
	float BonusLifeSteal = 0.f;

	// Turn 48 (O Nazareno and generic).
	float DamageImmuneUntil = 0.f;
	/** Ungido de Sangue (Cálice e Carne): 10% less damage, +10% life steal, +20% vampirism, +5% speed (speed buff),
	 *  10% shorter cooldowns, 3x out-of-combat regeneration. */
	float UngidoUntil = 0.f;
	bool IsUngido() const;
	float SlowImmuneUntil = 0.f;
	/** Crucificado: while set, a lethal hit only kills if it dealt more than 30% of max health (once). */
	float LastStandGuardUntil = 0.f;
	bool bLastStandUsed = false;
	/** True when the hit was absorbed: health stays at 1, 1 s immune and on the floor. */
	bool TrySurviveLethal(float HitDamage);
	/** Next respawn delay (seconds) instead of the mode formula; < 0 = use the formula. */
	float RespawnOverride = -1.f;
	/** Cálice e Carne channel. */
	TWeakObjectPtr<AValdoriaHeroCharacter> ChaliceTarget;
	FTimerHandle ChaliceTimer;
	/** Removes control effects and slows. */
	void Cleanse();
	bool IsFireHeld() const { return bFireHeld; }
	/** Ariel: the form-swap arrival bonus (shield / speed) can only happen again after this time. */
	float NextFormBonusTime = 0.f;
	/** Kay: the fire/air jump in the air was used (back on landing). */
	bool bElementJumpUsed = false;
	/** Deus do Velho Testamento: finishes Victim, credit to the ally who hurt it most, heal to the nearest ally. */
	void ExecuteEnemy(AValdoriaHeroCharacter* Victim);
	/** Damage already in real units (no stat scale, no multipliers). */
	void DealRawDamageTo(AActor* Target, float RealDamage);

	// Turn 47 (Heimdall and generic): control immunity, timed damage reduction, forced Bifrost ride.
	float CCImmuneUntil = 0.f;
	float BuffReduction = 0.f;
	float BuffReductionUntil = 0.f;
	/** Heimdall's passive near a Bifrost (off it). */
	float RailGuardReduction = 0.f;
	bool IsRailGuardian() const;
	/** Put this hero on a rail at that distance and direction (shared by mounting and forced rides). */
	void MountRailAt(class AValdoriaLaneRail* Rail, float Along, float Sign);
	/** Chamado da Bifrost: the nearest rail within MaxDistance catches this hero and carries it toward its own base for
	 *  Seconds, helpless (no attacks, no abilities, cannot get off). Returns false if no rail is close. */
	bool StartForcedRide(float Seconds, float MaxDistance);
	bool IsForcedRiding() const;
	float ForcedRideUntil = 0.f;
	/** Rails this hero may not take again until the time (8 s after a forced ride ends). */
	TMap<TWeakObjectPtr<class AValdoriaLaneRail>, float> RailLockouts;
	bool bLevelsChecked = false;
	/** World time when this hero comes back (for the HUD). */
	float RespawnAt = 0.f;
	bool bUltUpgradePress = false;
	/** Kills, deaths, assists and XP when this unit dies (turn 46). */
	void AwardDeath(AValdoriaHeroCharacter* Killer);

	bool IsInCombat(float Seconds) const { return GetWorld()->GetTimeSeconds() - LastCombatTime < Seconds; }
	float LastHarmedTime = -100.f;
	float LastCombatTime = -100.f;
	void SetAutopilotRoute(const TArray<FVector>& Points) { AutopilotRoute = Points; AutopilotRouteIndex = 0; }
	void RestartAutopilotRoute() { AutopilotRouteIndex = 0; }
	void SetAutopilotPace(float RepathSeconds, float PickSeconds) { NavRepathInterval = RepathSeconds; AutopilotPickInterval = PickSeconds; }
	/** Team buff from a MOBA spot: 0 damage reduction, 1 damage bonus, 2 regeneration per second. Lasts Seconds. */
	void ApplyTeamBuff(uint8 Type, float Value, float Seconds);
	float GetTeamDamageReduction() const;
	float GetTeamDamageBonus() const;
	float GetTeamRegen() const;
	// ---- Stacks (owner, turn 45). A stack lives on the hero that RECEIVES it and remembers who caused it.
	// HUD: your own stacks sit small above your abilities; stacks an enemy put on you sit smaller with a red mark;
	// stacks you put on someone else show next to their health bar, only for you.
	struct FValdoriaStack
	{
		TWeakObjectPtr<AValdoriaHeroCharacter> Source;
		FName Key;
		FString Label;          // 1-3 letters drawn on the small icon
		FLinearColor Color = FLinearColor::White;
		int32 Count = 1;
		float EndTime = 0.f;    // 0 = until removed
	};
	/** Sets the count of a stack (Count <= 0 removes it). Duration 0 = stays until changed. */
	void SetStack(AValdoriaHeroCharacter* Source, FName Key, const FString& Label, FLinearColor Color, int32 Count, float Duration = 0.f);
	/** Adds to a stack, capped at MaxCount, and refreshes its duration. */
	void AddStack(AValdoriaHeroCharacter* Source, FName Key, const FString& Label, FLinearColor Color, int32 Add = 1, int32 MaxCount = 99, float Duration = 0.f);
	void RemoveStack(AValdoriaHeroCharacter* Source, FName Key) { SetStack(Source, Key, FString(), FLinearColor::White, 0); }
	/** Live stacks (expired ones are skipped by the HUD and cleaned on the next change). */
	const TArray<FValdoriaStack>& GetStacks() const { return Stacks; }
	bool IsStackLive(const FValdoriaStack& Stack) const { return Stack.Count > 0 && (Stack.EndTime <= 0.f || Stack.EndTime > GetWorld()->GetTimeSeconds()); }
	TArray<FValdoriaStack> Stacks;

	/** Anti-heal: healing this hero receives is cut by Fraction for Seconds. */
	void ApplyAntiHeal(float Fraction, float Seconds) { AntiHealValue = FMath::Max(Fraction, GetWorld()->GetTimeSeconds() < AntiHealEnd ? AntiHealValue : 0.f); AntiHealEnd = GetWorld()->GetTimeSeconds() + Seconds; }
	/** (1 + healing received from items) x (1 - active anti-heal). */
	float GetHealingReceivedMultiplier() const { return (1.f + ShopBonus.HealingReceived) * (1.f - (GetWorld()->GetTimeSeconds() < AntiHealEnd ? FMath::Clamp(AntiHealValue, 0.f, 0.9f) : 0.f)); }
	/** Overheal: temporary shield from healing past full health; absorbs damage first and fades over time. */
	float Overheal = 0.f;
	float OverhealDecayRate = 0.f;
	void AddOverheal(float Amount, float Cap, float DecayPerSecond);
	/** Who hit this hero last, and when (guardians defend allies under attack). */
	TWeakObjectPtr<AValdoriaHeroCharacter> LastAttacker;
	float LastAttackedTime = -100.f;
	/** Reaplica as teclas das habilidades (ao trocar o esquema nas Configurações). */
	void RefreshKeyScheme();
	/** Animação de base agora (o animador em C++ pergunta isto a cada quadro). */
	UAnimSequenceBase* PickBaseAnimation(bool& bOutLoop, float& OutRate) const;
	/** Toca o gesto do botão usado (tiro/golpe em combo, habilidade, ultimate). */
	void PlayActionForSlot(EValdoriaAbilitySlot Slot);
	void PlayReloadAnimation();
	/** Skating on a created trail (SlideTrail): no steps, knees bent, arms out, leaning forward. */
	void StartSkating(float Seconds) { SkateEndTime = GetWorld()->GetTimeSeconds() + Seconds; }
	bool IsSkating() const { return GetWorld() && GetWorld()->GetTimeSeconds() < SkateEndTime; }
	void PlaySlotSound(EValdoriaAbilitySlot Slot);
	const FValdoriaShopBonus& GetShopBonus() const { return ShopBonus; }

	int32 GetDashCharges() const { return DashChargesLeft; }
	bool IsSprinting() const { return bIsSprinting; }
	bool IsProne() const { return bIsProne; }
	bool IsSliding() const { return bSliding; }
	bool IsWallClimbing() const { return bWallClimbing; }
	bool IsMantling() const { return bMantling; }
	bool IsGliding() const { return bGliding; }
	bool IsFlying() const { return bFlying; }
	float GetFlightFuelPercent() const;
	/** Voo por habilidade: voa por Seconds (0 = usa o tempo do Data Asset). */
	void StartAbilityFlight(float Seconds = 0.f);

	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;

	UFUNCTION() void OnRep_HeroDefinition();
	UFUNCTION() void OnRep_IsDead();

	/** Lê o Data Asset e aplica tudo: atributos, movimento, câmera, malha e habilidades. */
	void ApplyHeroDefinition();
	void GrantAbilities();
	void BuildDefaultInput();
	void TryActivateSlot(EValdoriaAbilitySlot Slot);
	void FinishReload();
	void EnterRagdoll();
	void LeaveRagdoll();

	// Entrada
	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	/** Gamepad right stick: a rate (degrees per second), not a mouse delta. */
	void Input_LookStick(const FInputActionValue& Value);
	bool IsCursorFree() const;
	void Input_FirePressed();
	void Input_FireReleased();
	void Input_Secondary();
	void Input_Ability1();
	void Input_Ability2();
	void Input_Ability3();
	void Input_AbilityExtra();
	/** Key 4 held / released (heroes whose ultimate is charged by holding, e.g. Kay). */
	void Input_UltimateReleased();
	void Input_Ultimate();
	void Input_Reload();
	void Input_JumpPressed();
	void Input_JumpReleased();
	void Input_SprintPressed();
	void Input_SprintReleased();
	void Input_CrouchPressed();
	void Input_CrouchReleased();
	void Input_Prone();

	// --- Movimento avançado (regras por herói em FValdoriaMovementRules)
	const FValdoriaMovementRules& GetMovementRules() const;
	void UpdateMovementSpeed();
	void SetSprinting(bool bNewSprinting);
	void SetProne(bool bNewProne);
	void TryDash();
	UFUNCTION(Server, Reliable) void ServerDash(FVector_NetQuantizeNormal Direction);
	void PerformDash(const FVector& Direction);
	int32 DashChargesLeft = 0;
	float DashRechargeProgress = 0.f;
	float LastShiftPressTime = -10.f;
	void StartSlide();
	void EndSlide();
	bool FindMantleTarget(FVector& OutTarget) const;
	bool IsFacingWall(float Distance) const;
	void StartMantle(const FVector& Target);
	void TickMovementFeatures(float DeltaSeconds);
	UAnimSequenceBase* PickMovementAnimation(bool& bOutLoop) const;
	void UpdateMovementAnimation();
	void PlayDashAnimation(const FVector& Direction);
	TWeakObjectPtr<UAnimSequenceBase> CurrentMovementAnim;
	virtual void Landed(const FHitResult& Hit) override;

	UFUNCTION(Server, Reliable) void ServerSetSprinting(bool bNewSprinting);
	UFUNCTION(Server, Reliable) void ServerSetProne(bool bNewProne);
	UFUNCTION(Server, Reliable) void ServerSetJumpHeld(bool bHeld);
	UFUNCTION(Server, Reliable) void ServerStartMantle(FVector_NetQuantize Target);
	UFUNCTION(Server, Reliable) void ServerStartSlide();
	UFUNCTION() void OnRep_IsProne();

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Movimento")
	bool bIsSprinting = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsProne, BlueprintReadOnly, Category = "Movimento")
	bool bIsProne = false;

	bool bNoclip = false;
	bool bGliding = false;
	bool bFlying = false;
	float FlightFuel = -1.f;
	float AbilityFlightEnd = 0.f;
	/** Vex (turn 48): attacking in the air pauses the flight (hovering still) until this time, then she flies on. */
	float FlightPauseUntil = 0.f;
	void StopFlight();
	bool bFirstPerson = false;
	bool bJumpHeld = false;
	bool bCrouchHeld = false;
	bool bPendingRecrouch = false;
	bool bSliding = false;
	float SlideEndTime = 0.f;
	float LastSlideTime = -100.f;
	float WallClimbTimeUsed = 0.f;
	bool bWallClimbing = false;
	bool bMantling = false;
	float MantleElapsed = 0.f;
	FVector MantleStart = FVector::ZeroVector;
	FVector MantleTarget = FVector::ZeroVector;

	UPROPERTY(Transient) TObjectPtr<UInputAction> SprintAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> CrouchAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> ProneAction;

	UFUNCTION(Server, Reliable) void ServerReload();
	UFUNCTION(NetMulticast, Unreliable) void MulticastShotEffects(FVector_NetQuantize Impact, bool bHit);
	UFUNCTION(NetMulticast, Unreliable) void MulticastAreaEffect(FVector_NetQuantize Center, float Radius, FLinearColor Color);
	UFUNCTION(Client, Unreliable) void ClientNotifyHit(FVector_NetQuantize Location, float Damage, bool bHeadshot, bool bKill);

	void DrawShot(const FVector& Impact, bool bHit) const;
	void DrawArea(const FVector& Center, float Radius, FLinearColor Color) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componentes")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componentes")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componentes")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componentes")
	TObjectPtr<UStaticMeshComponent> WeaponComponent;

	/** Turn 49: visible body when the hero definition has a VisualMesh (the Mannequin is then hidden). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componentes")
	TObjectPtr<USkeletalMeshComponent> VisualMesh;

	/** Item in the other hand (shield, second dagger). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Valdoria")
	TObjectPtr<UStaticMeshComponent> OffhandComponent;

	/** Disco mágico sob os pés (locomoção "andar sobre efeito"). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componentes")
	TObjectPtr<UStaticMeshComponent> RideDisc;
	/** Per-foot discs (bRideEffectPerFoot). They stay where the foot planted (absolute location). */
	UPROPERTY(VisibleAnywhere, Category = "Componentes")
	TObjectPtr<UStaticMeshComponent> FootDiscL;
	UPROPERTY(VisibleAnywhere, Category = "Componentes")
	TObjectPtr<UStaticMeshComponent> FootDiscR;
	float FootDiscAlpha[2] = { 0.f, 0.f };
	void TickFootDiscs(float DeltaSeconds);
	/** Turn 49: with a VRoid body the weapons follow the VISIBLE hands (the Mannequin's hands are elsewhere). */
	void TickVisualAttachments();
	FVector JumpLogLast = FVector::ZeroVector;
	FVector JumpLogPelvis = FVector::ZeroVector;
	FVector VisualBaseRel[2] = { FVector::ZeroVector, FVector::ZeroVector };
	FVector VisualWrittenRel[2] = { FVector(1e9f), FVector(1e9f) };

	UPROPERTY()
	TObjectPtr<UValdoriaAttributeSet> Attributes;

	/** Definição em uso (o Data Asset, ou o Recruta de teste). */
	UPROPERTY(Transient)
	TObjectPtr<UValdoriaHeroDefinition> ActiveDefinition;

	// Entrada criada em código (não depende de arquivos do editor).
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> DefaultMapping;
	UPROPERTY(Transient) TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> LookAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> LookStickAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> FireAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> SecondaryAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Ability1Action;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Ability2Action;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Ability3Action;
	UPROPERTY(Transient) TObjectPtr<UInputAction> AbilityExtraAction;
	void MapAbilityKeys();
	UPROPERTY(Transient) TObjectPtr<UInputAction> UltimateAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool bIsDead = false;

	UPROPERTY(Replicated)
	int32 CurrentAmmo = 30;

	UPROPERTY(Replicated)
	bool bReloading = false;

	TMap<EValdoriaAbilitySlot, float> CooldownEndTimes;
	TArray<FGameplayAbilitySpecHandle> GrantedAbilities;
	FTimerHandle ReloadTimer;
	float LastDamageTakenTime = -100.f;
	bool bFireHeld = false;
	bool bSecondaryHeld = false;
	bool bAimGliding = false;
	bool bWasFireHold = false;
	void Input_SecondaryReleased();
	bool bAbilitiesGranted = false;

	// Efeitos temporários e passivas
	void TickStatusEffects(float DeltaSeconds);
	void UpdateStealthVisibility();
	void DoLeapSlamImpact();
	TMap<EValdoriaStatus, float> StatusEndTimes;
	FValdoriaShopBonus ShopBonus;
	int32 ComboIndex = 0;
	void TickFootsteps(float DeltaSeconds);
	void TickFlightPose(float DeltaSeconds);
	bool bWasFlyingPose = false;
	bool bLandAfterFlight = false;
	void TickAutopilot(float DeltaSeconds);
	bool bAutopilot = false;
	float AutopilotSkill = 0.6f;
	FRotator AutopilotAim = FRotator::ZeroRotator;
	TWeakObjectPtr<AValdoriaHeroCharacter> AutopilotTarget;
	float AutopilotNextPick = 0.f;
	float AutopilotNextAbility = 0.f;
	float AutopilotStrafe = 0.f;
	FVector LeashCenter = FVector::ZeroVector;
	float LeashRadius = 0.f;
	TArray<FVector> AutopilotRoute;
	void TickRiding(float DeltaSeconds);
	TWeakObjectPtr<class AValdoriaLaneRail> RideRail;
	float RideDistance = 0.f;
	float RideSpeed = 1800.f;
	/** +1 = towards the end of the trail, -1 = back to the base. */
	float RideSign = 1.f;
	/** Forward input (W) while riding, used to turn around. */
	float RideForwardInput = 0.f;
	float JumpPressTime = -10.f;
	bool bTriedMountThisPress = false;
	void TickChannel(float DeltaSeconds);
	TWeakObjectPtr<class AValdoriaGuardian> ChannelTarget;
	float ChannelProgress = 0.f;
	UPROPERTY(Transient) TObjectPtr<UInputAction> InteractAction;
	void Input_InteractPressed() { bInteractHeld = true; }
	void Input_InteractReleased() { bInteractHeld = false; }
	int32 AutopilotRouteIndex = 0;
	/** Stuck rescue (turn 45): where the bot was at the last check; no progress in 3 s = skip to the next route point. */
	FVector AutopilotStuckPos = FVector::ZeroVector;
	float AutopilotStuckCheck = 0.f;
	int32 AutopilotStuckStrikes = 0;
	/** How often the autopilot re-plans its path and re-picks a target (turn 45: minions use slower values, FPS). */
	float NavRepathInterval = 0.6f;
	float AutopilotPickInterval = 0.5f;
	float TeamBuffValue[3] = { 0.f, 0.f, 0.f };
	float TeamBuffEnd[3] = { 0.f, 0.f, 0.f };
	float AntiHealValue = 0.f;
	float AntiHealEnd = 0.f;
	float SkateEndTime = 0.f;
	bool bWasSkating = false;
	float FlightLeanAlpha = 0.f;
	float StepDistance = 0.f;
	float LastFireSoundTime = -10.f;
	void TickHover(float DeltaSeconds);
	FVector HoverMeshBase = FVector::ZeroVector;
	float ReflectEndTime = 0.f;
	float AbsorbEndTime = 0.f;
	float AbsorbFraction = 0.f;
	float AbsorbChargePerPoint = 0.f;
	float IntangibleEndTime = 0.f;
	/** Arma do Data Asset com os bônus da loja (pente maior...). */
	FValdoriaWeaponStats BoostedWeapon;
	void RebuildBoostedWeapon();
	float SlowFraction = 0.f;
	float SlowEndTime = 0.f;
	float BuffFraction = 0.f;
	float BuffEndTime = 0.f;
	bool bStealthActive = false;
	bool bStealthRevealed = false;
	float StealthEndTime = 0.f;
	float StealthRevealDistance = 600.f;
	EValdoriaAbilitySlot StealthSlot = EValdoriaAbilitySlot::None;
	bool bHasWeaponOverride = false;
	FValdoriaWeaponStats WeaponOverride;
	float WeaponOverrideEndTime = 0.f;
	int32 SavedAmmo = 0;
	bool bPendingSlam = false;
	float SlamDamage = 0.f;
	float SlamRadius = 0.f;
	EValdoriaDamageType SlamType = EValdoriaDamageType::Physical;
	float SlamTimeout = 0.f;
	float PassiveTimer = 0.f;
	float PassiveReduction = 0.f;
	int32 AlliesNearby = 0;
	FTransform DefaultMeshRelativeTransform;

	friend class AValdoriaGameMode;
	friend class AValdoriaHUD;
};
