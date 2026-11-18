// Echoes of Valdoria - tipos compartilhados do hero shooter.

#pragma once

#include "CoreMinimal.h"
#include "ValdoriaTypes.generated.h"

class UValdoriaGameplayAbility;
class UAnimSequenceBase;

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
	Ability1	UMETA(DisplayName = "Habilidade 1 (Shift)"),
	Ability2	UMETA(DisplayName = "Habilidade 2 (E)"),
	Ultimate	UMETA(DisplayName = "Ultimate (Q)")
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
};
