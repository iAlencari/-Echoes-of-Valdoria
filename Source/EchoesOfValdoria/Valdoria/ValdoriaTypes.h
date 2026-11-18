// Echoes of Valdoria - tipos compartilhados do hero shooter.

#pragma once

#include "CoreMinimal.h"
#include "ValdoriaTypes.generated.h"

class UValdoriaGameplayAbility;
class UAnimSequenceBase;
class UTexture2D;

/**
 * Tipo de dano. Físico = armas e golpes; Mágico = cajados e magias; Híbrido = parte de cada
 * (a fração mágica fica em HybridMagicShare). O Poder Físico/Mágico de quem ataca aumenta o
 * dano e a Resistência Física/Mágica de quem apanha reduz.
 */
UENUM(BlueprintType)
enum class EValdoriaDamageType : uint8
{
	Physical	UMETA(DisplayName = "Físico"),
	Magical		UMETA(DisplayName = "Mágico"),
	Hybrid		UMETA(DisplayName = "Híbrido (físico + mágico)")
};

/** Como o herói se locomove no chão (visual e altura da hitbox). */
UENUM(BlueprintType)
enum class EValdoriaLocomotion : uint8
{
	Walk		UMETA(DisplayName = "Andar (passos)"),
	Float		UMETA(DisplayName = "Flutuar (sem passos, pairando; use HoverHeight)"),
	RideEffect	UMETA(DisplayName = "Andar sobre um efeito (mágico ou tecnológico; veja RideEffect*; use HoverHeight)")
};

/** Estados de controle (no alvo ou no próprio herói durante uma habilidade). */
UENUM(BlueprintType)
enum class EValdoriaStatus : uint8
{
	Silenced			UMETA(DisplayName = "Silenciado (sem habilidades, pode atirar)"),
	Rooted				UMETA(DisplayName = "Enraizado (não anda, pode atirar)"),
	Stunned				UMETA(DisplayName = "Atordoado (não faz nada)"),
	CannotAttack		UMETA(DisplayName = "Não pode atacar"),
	CannotUseAbilities	UMETA(DisplayName = "Não pode usar habilidades")
};

/** Funções (roles). São só etiquetas: os atributos vêm de cada personagem. */
UENUM(BlueprintType)
enum class EValdoriaHeroRole : uint8
{
	Vanguard	UMETA(DisplayName = "Vanguarda (tanque)"),
	Warrior		UMETA(DisplayName = "Guerreiro (dano)"),
	Rearguard	UMETA(DisplayName = "Retaguarda (suporte)"),
	Flanker		UMETA(DisplayName = "Flanqueador (assassino)")
};

/** Em qual botão a habilidade fica. O valor vira o InputID do GAS. */
UENUM(BlueprintType)
enum class EValdoriaAbilitySlot : uint8
{
	None		UMETA(DisplayName = "Nenhum"),
	PrimaryFire	UMETA(DisplayName = "Tiro principal (botão esquerdo)"),
	SecondaryFire UMETA(DisplayName = "Tiro secundário (botão direito)"),
	Ability1	UMETA(DisplayName = "Habilidade 1 (F)"),
	Ability2	UMETA(DisplayName = "Habilidade 2 (E)"),
	Ultimate	UMETA(DisplayName = "Ultimate (Q ou X)"),
	Ability3	UMETA(DisplayName = "Habilidade 3 (C)")
};

/** Como o herói voa. */
UENUM(BlueprintType)
enum class EValdoriaFlightMode : uint8
{
	None = 0		UMETA(DisplayName = "Não voa"),
	Fuel = 1		UMETA(DisplayName = "Combustível (segurar Espaço no ar)"),
	Ability = 2		UMETA(DisplayName = "Habilidade (voa por um tempo ao usar)")
};

/** O que o Shift faz para este personagem. */
UENUM(BlueprintType)
enum class EValdoriaShiftMode : uint8
{
	None = 0			UMETA(DisplayName = "0 - Nada"),
	Sprint = 1			UMETA(DisplayName = "1 - Correr (segurar)"),
	Dash = 2			UMETA(DisplayName = "2 - Impulso"),
	SprintAndDash = 3	UMETA(DisplayName = "3 - Segurar = correr, dois toques = impulso")
};

/** Números da arma principal. Tudo editável no Data Asset do personagem. */
USTRUCT(BlueprintType)
struct FValdoriaWeaponStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arma")
	FText WeaponName = NSLOCTEXT("Valdoria", "DefaultWeapon", "Rifle de treino");

	/** Dano por tiro (antes de queda por distância e de crítico). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arma", meta = (ClampMin = "0"))
	float Damage = 20.f;

	/** Multiplicador ao acertar a cabeça. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arma", meta = (ClampMin = "1"))
	float HeadshotMultiplier = 2.f;

	/** Tiros por segundo. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arma", meta = (ClampMin = "0.1"))
	float FireRate = 8.f;

	/** Segurar o botão continua atirando? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arma")
	bool bAutomatic = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arma", meta = (ClampMin = "1"))
	int32 MagazineSize = 30;

	/** Munição reserva mostrada no HUD (0 = infinita). Por enquanto só visual. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arma", meta = (ClampMin = "0"))
	int32 ReserveAmmo = 0;

	/** Segundos para recarregar. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arma", meta = (ClampMin = "0"))
	float ReloadTime = 1.5f;

	/** Alcance máximo em centímetros (100 = 1 metro). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arma", meta = (ClampMin = "100"))
	float Range = 8000.f;

	/** Dispersão em graus (0 = perfeito). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arma", meta = (ClampMin = "0"))
	float SpreadDegrees = 0.6f;

	/** A partir desta distância (cm) o dano começa a cair. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arma|Queda de dano")
	float FalloffStart = 2500.f;

	/** Nesta distância (cm) o dano chega ao mínimo. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arma|Queda de dano")
	float FalloffEnd = 4500.f;

	/** Fração do dano que sobra no fim da queda (0.5 = metade). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arma|Queda de dano", meta = (ClampMin = "0", ClampMax = "1"))
	float FalloffMinMultiplier = 0.5f;

	// --- Tipo de ataque (o tipo de tiro é a habilidade do botão esquerdo; estes são os números)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arma|Tipo")
	EValdoriaDamageType DamageType = EValdoriaDamageType::Physical;

	/** Fração mágica quando o tipo é Híbrido (0.5 = metade de cada). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arma|Tipo", meta = (ClampMin = "0", ClampMax = "1"))
	float HybridMagicShare = 0.5f;

	/** Usa munição? Desligado = arma corpo a corpo (martelo, lâminas na mão). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arma|Tipo")
	bool bUsesAmmo = true;

	/** Recuo: quanto a mira sobe a cada tiro, em graus. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arma|Recuo", meta = (ClampMin = "0"))
	float RecoilDegrees = 0.f;

	/** Projétil: velocidade (cm/s) e gravidade (0 = reto, 1 = cai como objeto). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arma|Projétil", meta = (ClampMin = "100"))
	float ProjectileSpeed = 6000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arma|Projétil", meta = (ClampMin = "0"))
	float ProjectileGravity = 0.f;

	/** Explosão ao bater (raio em cm, 0 = sem explosão). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arma|Projétil", meta = (ClampMin = "0"))
	float SplashRadius = 0.f;

	/** Cura em aliados atingidos pelo projétil (0 = não cura). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arma|Projétil", meta = (ClampMin = "0"))
	float AllyHeal = 0.f;
};

/**
 * Passiva do herói: todos os números ficam aqui; o que for 0 (ou 1 nos multiplicadores)
 * não faz nada. Assim cada herói liga só a parte dele, e dá para misturar.
 */
USTRUCT(BlueprintType)
struct FValdoriaPassiveRules
{
	GENERATED_BODY()

	/** Distância (cm) que conta como "perto" para as partes de aliados e aura. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passiva", meta = (ClampMin = "0"))
	float Radius = 800.f;

	/** Redução de dano recebido por aliado perto (0.1 = 10% por aliado). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passiva|Aliados", meta = (ClampMin = "0", ClampMax = "1"))
	float DamageReductionPerAlly = 0.f;

	/** Teto da redução acima. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passiva|Aliados", meta = (ClampMin = "0", ClampMax = "0.9"))
	float MaxDamageReduction = 0.f;

	/** Parte do dano que aliados perto levam e que vem para você (0.05 = 5%). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passiva|Aliados", meta = (ClampMin = "0", ClampMax = "1"))
	float AllyDamageShare = 0.f;

	/** Cura por segundo em você e nos aliados perto. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passiva|Aura", meta = (ClampMin = "0"))
	float AuraHealPerSecond = 0.f;

	/** Dispersão parado ou agachado = dispersão x este valor (1 = sem efeito). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passiva|Mira", meta = (ClampMin = "0", ClampMax = "1"))
	float StillSpreadMultiplier = 1.f;

	/** Dano extra em acertos na cabeça (0.25 = +25%). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passiva|Mira", meta = (ClampMin = "0"))
	float HeadshotBonus = 0.f;

	/** Feixe: dano/cura extra máximo ao manter o feixe no mesmo alvo (0.5 = +50%). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passiva|Feixe", meta = (ClampMin = "0"))
	float BeamRampMax = 0.f;

	/** Segundos no mesmo alvo para chegar ao máximo acima. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passiva|Feixe", meta = (ClampMin = "0.1"))
	float BeamRampTime = 3.f;

	/** Dano pelas costas = dano x este valor (1 = sem efeito). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Passiva|Furtivo", meta = (ClampMin = "1"))
	float BackstabMultiplier = 1.f;
};

/**
 * O que o personagem sabe fazer ao se mover. Cada herói liga/desliga e ajusta
 * no próprio Data Asset (ex.: um tanque que não deita, um flanqueador que escala).
 */
USTRUCT(BlueprintType)
struct FValdoriaMovementRules
{
	GENERATED_BODY()

	// --- Shift: 0 nada, 1 correr, 2 impulso, 3 segurar = correr + dois toques = impulso
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shift")
	EValdoriaShiftMode ShiftMode = EValdoriaShiftMode::SprintAndDash;

	/** Velocidade correndo = velocidade normal x este valor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shift|Correr", meta = (ClampMin = "1"))
	float SprintMultiplier = 1.45f;

	/** Pode atirar enquanto corre? (se não, atirar interrompe a corrida) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shift|Correr")
	bool bCanFireWhileSprinting = false;

	/** Cargas de impulso (1 a 5). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shift|Impulso", meta = (ClampMin = "1", ClampMax = "5"))
	int32 DashCharges = 2;

	/** Força do impulso. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shift|Impulso", meta = (ClampMin = "0"))
	float DashImpulse = 1700.f;

	/** Segundos para recuperar UMA carga de impulso. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shift|Impulso", meta = (ClampMin = "0.1"))
	float DashRechargeTime = 4.f;

	/** No modo 3: tempo máximo entre os dois toques no Shift (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shift|Impulso", meta = (ClampMin = "0.1", ClampMax = "1"))
	float DoubleTapWindow = 0.3f;

	// --- Agachar (Ctrl, segurar)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agachar")
	bool bCanCrouch = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agachar", meta = (EditCondition = "bCanCrouch"))
	float CrouchSpeed = 300.f;

	/** Meia-altura da cápsula agachado (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Agachar", meta = (EditCondition = "bCanCrouch"))
	float CrouchHalfHeight = 62.f;

	// --- Deslizar (Ctrl enquanto corre)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deslizar")
	bool bCanSlide = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deslizar", meta = (EditCondition = "bCanSlide"))
	float SlideImpulse = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deslizar", meta = (EditCondition = "bCanSlide"))
	float SlideDuration = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deslizar", meta = (EditCondition = "bCanSlide"))
	float SlideCooldown = 1.2f;

	// --- Deitar (X, alterna)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deitar")
	bool bCanProne = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deitar", meta = (EditCondition = "bCanProne"))
	float ProneSpeed = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deitar", meta = (EditCondition = "bCanProne"))
	float ProneHalfHeight = 42.f; // não pode ser menor que o raio da cápsula

	// --- Pulo
	/** Quantos pulos seguidos (1 = normal, 2 = pulo duplo). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pulo", meta = (ClampMin = "1", ClampMax = "5"))
	int32 MaxJumps = 1;

	// --- Subir em beirada (Espaço perto de um muro baixo)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escalar")
	bool bCanMantle = true;

	/** Altura máxima (cm, medida dos pés) de uma beirada que dá para subir. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escalar", meta = (EditCondition = "bCanMantle"))
	float MaxMantleHeight = 200.f;

	/** Duração da subida (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escalar", meta = (EditCondition = "bCanMantle"))
	float MantleDuration = 0.35f;

	// --- Planar (segurar Espaço caindo)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planar")
	bool bCanGlide = false;

	/** Velocidade máxima de queda planando (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planar", meta = (EditCondition = "bCanGlide", ClampMin = "0"))
	float GlideFallSpeed = 180.f;

	/** Controle no ar planando (0 a 1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planar", meta = (EditCondition = "bCanGlide", ClampMin = "0", ClampMax = "1"))
	float GlideAirControl = 0.8f;

	// --- Voar
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voar")
	EValdoriaFlightMode FlightMode = EValdoriaFlightMode::None;

	/** Velocidade voando (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voar", meta = (ClampMin = "100"))
	float FlightSpeed = 900.f;

	/** Combustível: segundos de voo com o tanque cheio. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voar", meta = (ClampMin = "0.1"))
	float FlightFuelSeconds = 3.f;

	/** Combustível recuperado por segundo no chão. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voar", meta = (ClampMin = "0"))
	float FlightFuelRegen = 1.f;

	/** Modo habilidade: quanto tempo voa ao usar (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voar", meta = (ClampMin = "0.1"))
	float FlightAbilityDuration = 6.f;

	// --- Escalar parede (segurar Espaço de frente para uma parede alta)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escalar")
	bool bCanWallClimb = false;

	/** Velocidade subindo a parede (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escalar", meta = (EditCondition = "bCanWallClimb"))
	float WallClimbSpeed = 450.f;

	/** Quanto tempo aguenta subindo antes de soltar (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escalar", meta = (EditCondition = "bCanWallClimb"))
	float MaxWallClimbTime = 1.6f;
};

/**
 * Animações extras de movimento de um herói (tocadas por cima do Animation Blueprint,
 * no slot "DefaultSlot"). Servem para qualquer malha no esqueleto padrão da Unreal
 * (UE5 Mannequin / Manny), inclusive os heróis gerados por IA. Vazio = sem animação.
 */
USTRUCT(BlueprintType)
struct FValdoriaAnimationSet
{
	GENERATED_BODY()

	/** Nome do slot no Animation Blueprint onde as animações tocam. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações")
	FName SlotName = TEXT("DefaultSlot");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Agachar")
	TSoftObjectPtr<UAnimSequenceBase> CrouchIdle;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Agachar")
	TSoftObjectPtr<UAnimSequenceBase> CrouchWalkForward;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Agachar")
	TSoftObjectPtr<UAnimSequenceBase> CrouchWalkBackward;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Agachar")
	TSoftObjectPtr<UAnimSequenceBase> CrouchWalkLeft;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Agachar")
	TSoftObjectPtr<UAnimSequenceBase> CrouchWalkRight;

	/** Deslizando (se vazio, usa CrouchIdle). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Deslizar")
	TSoftObjectPtr<UAnimSequenceBase> Slide;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Deitar")
	TSoftObjectPtr<UAnimSequenceBase> ProneIdle;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Deitar")
	TSoftObjectPtr<UAnimSequenceBase> ProneCrawl;

	/** Subir em beirada (toca uma vez). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Escalar")
	TSoftObjectPtr<UAnimSequenceBase> Mantle;
	/** Escalando parede (repete). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Escalar")
	TSoftObjectPtr<UAnimSequenceBase> WallClimb;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Impulso")
	TSoftObjectPtr<UAnimSequenceBase> DashForward;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Impulso")
	TSoftObjectPtr<UAnimSequenceBase> DashBackward;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Impulso")
	TSoftObjectPtr<UAnimSequenceBase> DashLeft;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Impulso")
	TSoftObjectPtr<UAnimSequenceBase> DashRight;

	// --- Animador próprio (ValdoriaAnimInstance): andar, correr, cair e ações por cima.
	/** Ligado: usa o animador em C++ com as animações abaixo (o AnimClass do herói é ignorado). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Base")
	bool bUseValdoriaAnimator = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Base")
	TSoftObjectPtr<UAnimSequenceBase> Idle;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Base")
	TSoftObjectPtr<UAnimSequenceBase> WalkForward;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Base")
	TSoftObjectPtr<UAnimSequenceBase> WalkBackward;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Base")
	TSoftObjectPtr<UAnimSequenceBase> WalkLeft;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Base")
	TSoftObjectPtr<UAnimSequenceBase> WalkRight;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Base")
	TSoftObjectPtr<UAnimSequenceBase> JogForward;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Base")
	TSoftObjectPtr<UAnimSequenceBase> JogBackward;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Base")
	TSoftObjectPtr<UAnimSequenceBase> JogLeft;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Base")
	TSoftObjectPtr<UAnimSequenceBase> JogRight;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Base")
	TSoftObjectPtr<UAnimSequenceBase> FallLoop;
	/** Acima desta velocidade (cm/s) usa correr; abaixo, andar. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Base")
	float WalkRunSpeed = 320.f;
	/** Velocidades em que as animações de andar/correr foram feitas (ajusta o ritmo dos passos). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Base")
	float WalkAnimSpeed = 230.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Base")
	float JogAnimSpeed = 500.f;

	/** Ataque principal: tocadas em sequência (combo), da cintura para cima. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Ações")
	TArray<TSoftObjectPtr<UAnimSequenceBase>> FireActions;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Ações")
	float FireActionRate = 1.f;
	/** Ao usar habilidade (botão direito, F, E) e ultimate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Ações")
	TSoftObjectPtr<UAnimSequenceBase> AbilityAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Ações")
	TSoftObjectPtr<UAnimSequenceBase> UltimateAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Ações")
	TSoftObjectPtr<UAnimSequenceBase> ReloadAction;

	/** Pairando (locomoção Flutuar) e voando. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Voo")
	TSoftObjectPtr<UAnimSequenceBase> Hover;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animações|Voo")
	TSoftObjectPtr<UAnimSequenceBase> Fly;
};

/** Uma habilidade concedida ao personagem. */
USTRUCT(BlueprintType)
struct FValdoriaAbilityEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habilidade")
	FText DisplayName;

	/** Explicação mostrada na tela de ajuda (tecla K). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habilidade", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habilidade")
	EValdoriaAbilitySlot Slot = EValdoriaAbilitySlot::Ability1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habilidade")
	TSubclassOf<UValdoriaGameplayAbility> AbilityClass;

	/** Recarga em segundos (0 = usa a da própria habilidade). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habilidade", meta = (ClampMin = "0"))
	float Cooldown = 0.f;

	/** Valor principal da habilidade (dano, cura, força do impulso...). 0 = padrão da habilidade. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habilidade")
	float Magnitude = 0.f;

	/** Raio/alcance em cm, quando fizer sentido. 0 = padrão. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habilidade")
	float Radius = 0.f;

	/** Duração em segundos (barreira, zona, invisibilidade, lentidão...). 0 = padrão. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habilidade")
	float Duration = 0.f;

	/** Segundo valor; o significado depende da habilidade (está na descrição dela). 0 = padrão. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habilidade")
	float SecondaryMagnitude = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habilidade")
	EValdoriaDamageType DamageType = EValdoriaDamageType::Physical;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habilidade", meta = (ClampMin = "0", ClampMax = "1"))
	float HybridMagicShare = 0.5f;

	/** Ícone no HUD. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habilidade")
	TSoftObjectPtr<UTexture2D> Icon;
};
