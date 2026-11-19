// Echoes of Valdoria - definição de um personagem (um arquivo de dados por herói).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ValdoriaTypes.h"
#include "ValdoriaHeroDefinition.generated.h"

class USkeletalMesh;
class UStaticMesh;
class UAnimInstance;
class UTexture2D;
class USoundBase;

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

	/** Passiva: nome e explicação (tela de ajuda, tecla K). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habilidades")
	FText PassiveName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habilidades", meta = (MultiLine = true))
	FText PassiveDescription;

	/** Números da passiva (distância, porcentagens). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habilidades")
	FValdoriaPassiveRules Passive;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habilidades")
	TSoftObjectPtr<UTexture2D> PassiveIcon;

	// --- Poder e resistências (a loja e itens somam a estes valores)
	/** Multiplica o dano físico causado (1 = normal). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poder", meta = (ClampMin = "0"))
	float PhysicalPower = 1.f;

	/** Multiplica o dano mágico causado (1 = normal). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poder", meta = (ClampMin = "0"))
	float MagicPower = 1.f;

	/** Reduz o dano físico recebido (0.2 = 20% a menos). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poder", meta = (ClampMin = "0", ClampMax = "0.9"))
	float PhysicalResistance = 0.f;

	/** Reduz o dano mágico recebido (0.2 = 20% a menos). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Poder", meta = (ClampMin = "0", ClampMax = "0.9"))
	float MagicResistance = 0.f;

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

	/** Tamanho do modelo. Para heróis baixos (anões) ou grandes, ajuste junto com a cápsula
	 *  (CapsuleRadius = largura, CapsuleHalfHeight = metade da altura) para a hitbox acompanhar. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	float MeshScale = 1.f;

	/** Proporções do corpo (1 = normal): largura dos ombros/quadril, profundidade (peito/costas) e
	 *  altura. Para corpos diferentes; ajuste a cápsula (hitbox) junto. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Corpo", meta = (ClampMin = "0.5", ClampMax = "2"))
	float BodyWidth = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Corpo", meta = (ClampMin = "0.5", ClampMax = "2"))
	float BodyDepth = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Corpo", meta = (ClampMin = "0.5", ClampMax = "2"))
	float BodyHeight = 1.f;

	/** Flutuar: quanto o corpo sobe e desce (cm) e quanto inclina ao se mover (graus). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Corpo", meta = (ClampMin = "0"))
	float HoverBob = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Corpo", meta = (ClampMin = "0"))
	float HoverLean = 12.f;

	/** Flutua sempre esta altura (cm) acima do chão (não é voo). A malha, que é a hitbox dos
	 *  tiros e da cabeça, sobe junto. Some esta altura em CapsuleHalfHeight/2 se quiser a cápsula maior. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "0"))
	float HoverHeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	EValdoriaLocomotion Locomotion = EValdoriaLocomotion::Walk;

	/** Visual de "andar sobre efeito": malha (vazio = disco), escala e cor (preto = cor do herói).
	 *  Pode ser água, gelo, placas antigravidade, um campo de energia... */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<UStaticMesh> RideEffectMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FVector RideEffectScale = FVector(1.3f, 1.3f, 0.03f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FLinearColor RideEffectColor = FLinearColor::Black;
	/** One small disc under EACH foot, made just before the foot lands and fading after it lifts
	 *  (owner, turn 45: Vex; one disc for both feet looked like a board). RideEffectScale = one disc. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	bool bRideEffectPerFoot = false;

	/** Modelo 3D da arma (malha estática) e onde ela fica presa na malha do herói. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Arma")
	TSoftObjectPtr<UStaticMesh> WeaponMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Arma")
	FName WeaponSocket = TEXT("hand_r");

	// --- Elements (Kay, turn 44): tap the ultimate key (4) to switch element, hold it to cast the ultimate.
	/** Number of elements (0 = none; Kay = 4: fire, earth, water, air). Abilities with ElementSet = n belong to element n. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elementos")
	int32 ElementCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elementos")
	TArray<FText> ElementNames;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elementos")
	TArray<FLinearColor> ElementColors;
	/** Damage type of the basic attack per element (fire/water magical, earth physical, air hybrid by default). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elementos")
	TArray<EValdoriaDamageType> ElementDamageTypes;
	/** Cooldown of the element switch (tap on the ultimate key). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elementos")
	float ElementSwitchCooldown = 20.f;
	/** Hold the ultimate key this long to cast the ultimate (0 = normal press). Releasing early cancels. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elementos")
	float UltimateHoldSeconds = 0.f;
	/** Cooldown given when the hold is released before it completes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Elementos")
	float UltimateCancelCooldown = 3.f;

	// --- Forms (Ariel, turn 44): an ability swaps to another hero definition (the other form), keeping health % and charge.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Formas")
	FName AlternateFormId;

	/** Weapons float by telekinesis (Vex): they bob gently around their socket instead of being held. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Arma")
	bool bWeaponsFloat = false;

	/** Posição, rotação e escala da arma em relação ao osso/soquete. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Arma")
	FTransform WeaponOffset;

	/** Second item in the other hand (Brasa's shield, Vex's second dagger). Empty = none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Arma")
	TSoftObjectPtr<UStaticMesh> OffhandMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Arma")
	FName OffhandSocket = TEXT("hand_l");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Arma")
	FTransform OffhandOffset;

	/** Sons do ataque principal (vários = sorteia um) e da recarga. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sons")
	TArray<TSoftObjectPtr<USoundBase>> FireSounds;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sons")
	float FireVolume = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sons")
	TSoftObjectPtr<USoundBase> ReloadSound;
	/** Passos: volume (0 = sem passos, por exemplo para quem flutua). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sons")
	float FootstepVolume = 0.5f;

	/** Passive-style silence (Vex): no footsteps and no movement sounds at all, whatever FootstepVolume says. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sons")
	bool bSilentMovement = false;

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

	/**
	 * Preenche este arquivo com os valores iniciais de um herói pronto: "brasa", "kael",
	 * "iara" ou "vex" (Scripts/create_heroes.py usa isto para criar os Data Assets; depois
	 * é só editar no editor). Retorna falso se o nome não existir.
	 */
	UFUNCTION(BlueprintCallable, Category = "Herói")
	bool ApplyPreset(FName PresetId);

	/** Nomes dos heróis prontos. */
	/** Só a aparência de um herói pronto (animações e corpo), sem mexer no balanceamento. */
	UFUNCTION(BlueprintCallable, Category = "Herói")
	bool ApplyPresetLooks(FName PresetId);
	/** Adds the preset abilities whose slot (and element) this asset does not have yet, keeping everything the owner
	 *  balanced (turn 45: ability 3 for every hero). Called by ApplyPresetLooks. Returns how many were added. */
	int32 AddMissingPresetAbilities(FName PresetId);

	/** Movement animations (crouch, slide, mantle, wall climb, prone with transitions) from the owner's Fab packs. */
	void ApplyMovementAnimationPack();

	static TArray<FName> GetPresetIds();

	/** Cria um herói pronto em memória (usado se o Data Asset ainda não existir). */
	static UValdoriaHeroDefinition* CreatePreset(UObject* Outer, FName PresetId);
};
