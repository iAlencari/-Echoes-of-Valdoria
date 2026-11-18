// Echoes of Valdoria - definição de um personagem (um arquivo de dados por herói).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ValdoriaTypes.h"
#include "ValdoriaHeroDefinition.generated.h"

class USkeletalMesh;
class UAnimInstance;

/**
 * Tudo que diferencia um herói fica aqui: vida, escudo, armadura, velocidade,
 * arma, habilidades, ultimate e visual. Para balancear, edite o Data Asset no
 * editor (botão direito > Miscelânea > Data Asset > ValdoriaHeroDefinition).
 * Nada disso depende da função (role): a função é só uma etiqueta.
 */
UCLASS(BlueprintType)
class ECHOESOFVALDORIA_API UValdoriaHeroDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UValdoriaHeroDefinition();

	// --- Identidade
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identidade")
	FName HeroId = TEXT("recruta");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identidade")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identidade", meta = (MultiLine = true))
	FText Description;

	/** Uma ou mais funções (função mista é permitida). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identidade")
	TArray<EValdoriaHeroRole> Roles;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identidade")
	FLinearColor ThemeColor = FLinearColor(0.91f, 0.77f, 0.49f);

	// --- Sobrevivência
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sobrevivência", meta = (ClampMin = "1"))
	float MaxHealth = 225.f;

	/** Armadura: absorve dano e reduz cada acerto (ver ArmorDamageReduction). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sobrevivência", meta = (ClampMin = "0"))
	float MaxArmor = 0.f;

	/** Escudo: absorve dano primeiro e se regenera sozinho. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sobrevivência", meta = (ClampMin = "0"))
	float MaxShield = 0.f;

	/** Fração do dano de cada acerto que a armadura ignora (0.3 = 30%). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sobrevivência", meta = (ClampMin = "0", ClampMax = "0.9"))
	float ArmorDamageReduction = 0.3f;

	/** Escudo volta a encher após este tempo sem levar dano. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sobrevivência", meta = (ClampMin = "0"))
	float ShieldRegenDelay = 3.f;

	/** Escudo regenerado por segundo. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sobrevivência", meta = (ClampMin = "0"))
	float ShieldRegenRate = 30.f;

	/** Raio e altura da cápsula de colisão (hitbox do corpo), em cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sobrevivência")
	float CapsuleRadius = 42.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sobrevivência")
	float CapsuleHalfHeight = 96.f;

	// --- Movimento
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movimento", meta = (ClampMin = "50"))
	float MoveSpeed = 550.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movimento", meta = (ClampMin = "0"))
	float JumpVelocity = 520.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movimento", meta = (ClampMin = "0", ClampMax = "1"))
	float AirControl = 0.4f;

	/** Correr, agachar, deslizar, deitar, pulo duplo, subir em beirada, escalar parede. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movimento")
	FValdoriaMovementRules Movement;

	// --- Arma
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arma")
	FValdoriaWeaponStats Weapon;

	// --- Habilidades
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habilidades")
	TArray<FValdoriaAbilityEntry> Abilities;

	/** Carga necessária para a ultimate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habilidades", meta = (ClampMin = "1"))
	float UltimateCost = 1500.f;

	/** Carga de ultimate ganha por ponto de dano causado. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habilidades", meta = (ClampMin = "0"))
	float UltimateChargePerDamage = 1.f;

	/** Carga de ultimate ganha por segundo, sem fazer nada. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habilidades", meta = (ClampMin = "0"))
	float UltimateChargePerSecond = 5.f;

	// --- Visual
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TSoftClassPtr<UAnimInstance> AnimClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	float MeshScale = 1.f;

	/** Animações de agachar, deslizar, deitar, escalar e impulso. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FValdoriaAnimationSet Animations;

	/** Câmera por cima do ombro: distância e deslocamento lateral/altura (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Câmera")
	float CameraDistance = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Câmera")
	FVector CameraOffset = FVector(0.f, 55.f, 65.f);

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** Personagem padrão usado quando nenhum Data Asset foi escolhido (para testes). */
	static UValdoriaHeroDefinition* CreateFallbackRecruit(UObject* Outer);
};
