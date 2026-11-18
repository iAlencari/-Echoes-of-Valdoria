// Echoes of Valdoria - loja de melhorias da partida (tecla B).
// Modelo: poucas categorias, cada item com níveis comprados em sequência, limite de itens,
// créditos ganhos durante a partida; tudo zera quando a partida (o mapa) recomeça.
// Os itens ficam num arquivo de dados (DA_Loja) para o dono balancear.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ValdoriaShop.generated.h"

UENUM(BlueprintType)
enum class EValdoriaShopCategory : uint8
{
	Defense		UMETA(DisplayName = "Defesa"),
	Utility		UMETA(DisplayName = "Utilidade"),
	Healing		UMETA(DisplayName = "Cura"),
	Damage		UMETA(DisplayName = "Dano")
};

/** O que um item melhora. Valores em fração (0.1 = 10%) ou pontos (vida). */
UENUM(BlueprintType)
enum class EValdoriaShopStat : uint8
{
	MaxHealth			UMETA(DisplayName = "Vida máxima (pontos)"),
	PhysicalResistance	UMETA(DisplayName = "Resistência física"),
	MagicResistance		UMETA(DisplayName = "Resistência mágica"),
	CooldownReduction	UMETA(DisplayName = "Redução de recarga"),
	MoveSpeed			UMETA(DisplayName = "Velocidade de movimento"),
	MagazineSize		UMETA(DisplayName = "Munição no pente"),
	HealingPower		UMETA(DisplayName = "Cura causada"),
	LifeSteal			UMETA(DisplayName = "Roubo de vida (dano vira cura)"),
	PhysicalPower		UMETA(DisplayName = "Poder físico"),
	MagicPower			UMETA(DisplayName = "Poder mágico"),
	HeadshotDamage		UMETA(DisplayName = "Dano na cabeça")
};

USTRUCT(BlueprintType)
struct FValdoriaShopItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EValdoriaShopCategory Category = EValdoriaShopCategory::Defense;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EValdoriaShopStat Stat = EValdoriaShopStat::MaxHealth;

	/** Preço de cada nível (o tamanho da lista = número de níveis). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TArray<int32> LevelCosts;

	/** Valor TOTAL do bônus em cada nível (não soma: o nível 3 vale o terceiro número). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TArray<float> LevelValues;
};

/** Bônus somados da loja, aplicados no herói. */
USTRUCT(BlueprintType)
struct FValdoriaShopBonus
{
	GENERATED_BODY()
	float MaxHealth = 0.f;
	float PhysicalResistance = 0.f;
	float MagicResistance = 0.f;
	float CooldownReduction = 0.f;
	float MoveSpeed = 0.f;
	float MagazineSize = 0.f;
	float HealingPower = 0.f;
	float LifeSteal = 0.f;
	float PhysicalPower = 0.f;
	float MagicPower = 0.f;
	float HeadshotDamage = 0.f;
};

/** Catálogo da loja e regras de créditos (Content/Valdoria/Shop/DA_Loja). */
UCLASS(BlueprintType)
class ECHOESOFVALDORIA_API UValdoriaShopCatalog : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loja")
	TArray<FValdoriaShopItem> Items;

	/** Quantos itens diferentes cada jogador pode ter ao mesmo tempo. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loja", meta = (ClampMin = "1"))
	int32 MaxItems = 4;

	/** Créditos no começo da partida. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Créditos")
	int32 StartingCredits = 400;

	/** Créditos ganhos por segundo, por ponto de dano causado e por abate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Créditos")
	float CreditsPerSecond = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Créditos")
	float CreditsPerDamage = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Créditos")
	int32 CreditsPerKill = 100;

	/** Preenche com os itens iniciais (Scripts/create_heroes.py usa isto). */
	UFUNCTION(BlueprintCallable, Category = "Loja")
	void ApplyDefaultItems();

	/** Soma dos bônus para os níveis comprados. */
	FValdoriaShopBonus ComputeBonus(const TMap<FName, int32>& Levels) const;

	const FValdoriaShopItem* FindItem(FName ItemId) const;
};
