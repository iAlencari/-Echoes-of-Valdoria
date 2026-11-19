// Echoes of Valdoria - HUD desenhado em código (sem assets), no estilo das referências do dono:
// barra de vida segmentada na cor de cada herói, aliado ciano / inimigo vermelho com caveira,
// passiva + 3 habilidades + ultimate com moldura, munição grande com ícones de balas.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ValdoriaHUD.generated.h"

class AValdoriaHeroCharacter;
class UTexture2D;

UCLASS()
class ECHOESOFVALDORIA_API AValdoriaHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	/** Marcador de acerto + número de dano flutuante. */
	void AddHitMarker(const FVector& WorldLocation, float Damage, bool bHeadshot, bool bKill);
	/** Seta de dano recebido: mostra de onde veio o tiro (maior = mais dano). */
	void AddDamageIndicator(const FVector& FromLocation, float Damage);

	/** Mostra FPS, tempo de quadro e ping (canto superior esquerdo). */
	UPROPERTY(EditAnywhere, Category = "Valdoria")
	bool bShowStats = true;

	/** Vida por segmento da barra (estilo hero shooter). */
	UPROPERTY(EditAnywhere, Category = "Valdoria")
	float HealthPerSegment = 25.f;

	/** Medidor de dano (DPS) do painel de admin. */
	UPROPERTY(EditAnywhere, Category = "Valdoria")
	bool bShowDamageMeter = false;
	void ResetDamageMeter() { TotalDamage = 0.f; RecentDamage.Reset(); MeterStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f; }

	/** Chat: linhas recentes (comandos e respostas). */
	void AddChatLine(const FString& Line, const FLinearColor& Color = FLinearColor::White);
	bool bChatOpen = false;

	UPROPERTY(EditAnywhere, Category = "Valdoria")
	FLinearColor AllyColor = FLinearColor(0.2f, 0.85f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Valdoria")
	FLinearColor EnemyColor = FLinearColor(1.f, 0.22f, 0.2f);

protected:
	struct FDamageNumber
	{
		FVector WorldLocation;
		float Damage = 0.f;
		bool bHeadshot = false;
		float SpawnTime = 0.f;
	};
	TArray<FDamageNumber> DamageNumbers;
	struct FDamageIndicator { FVector From; float Damage; float Time; };
	TArray<FDamageIndicator> DamageIndicators;
	void DrawDamageFeedback(AValdoriaHeroCharacter* Hero, float W, float H);
	bool bWasUltReady = false;
	float UltReadyFlashTime = -10.f;
	float LastHitTime = -10.f;
	bool bLastHitHeadshot = false;
	bool bLastHitKill = false;
	float SmoothedFrameMs = 16.f;
	float TotalDamage = 0.f;
	float MeterStartTime = 0.f;
	TArray<TPair<float, float>> RecentDamage; // (tempo, dano)
	void DrawDamageMeter(float W, float H);
	float Scale = 1.f;
	struct FChatLine { FString Text; FLinearColor Color; float Time; };
	TArray<FChatLine> ChatLines;
	void DrawChat(float W, float H);

	void DrawRect(float X, float Y, float W, float H, const FLinearColor& Color);
	/** Desenha um ícone (textura do Data Asset). Retorna falso se não houver. */
	bool DrawIcon(const TSoftObjectPtr<UTexture2D>& Icon, float X, float Y, float Size, const FLinearColor& Tint = FLinearColor::White);
	void DrawStatusLine(AValdoriaHeroCharacter* Hero, float W, float H);
	/** Deathmatch: score, goal, timer and the result at the end. */
	void DrawModeScore(float W, float H);
	void DrawFrame(float X, float Y, float W, float H, const FLinearColor& Color, float Thickness = 2.f);
	void DrawLabel(const FString& Text, float X, float Y, const FLinearColor& Color, float TextScale = 1.f, bool bCenter = false, bool bLarge = false);
	void DrawSegmentedBar(float X, float Y, float W, float H, AValdoriaHeroCharacter* Hero, const FLinearColor& HealthColor);
	void DrawSkull(float Cx, float Cy, float Size, const FLinearColor& Color);

	void DrawCrosshairAndHits(float W, float H);
	void DrawStats();
	void DrawWorldHealthBars(AValdoriaHeroCharacter* Viewer);
	void DrawPlayerPanel(AValdoriaHeroCharacter* Hero, float W, float H);
	void DrawAbilities(AValdoriaHeroCharacter* Hero, float W, float H);
	/** Clock-style cooldown pie (Fraction 1 = full circle) and the small stack icons (turn 45). */
	void DrawClock(float Cx, float Cy, float Radius, float Fraction, const FLinearColor& Color);
	void DrawStackIcon(float X, float Y, float Size, const FString& Label, int32 Count, const FLinearColor& Color, bool bEnemyMark);
	void DrawAmmo(AValdoriaHeroCharacter* Hero, float W, float H);
	/** Minimap, top-left (turn 46): buildings, lanes, guardians, you, allies, and enemies revealed to your team. */
	void DrawMinimap(AValdoriaHeroCharacter* Hero, float W, float H);
	bool bShowMinimap = true;
	struct FMiniBlock { FBox2D Box; float Height; };
	TArray<FMiniBlock> MiniBlocks;
	FBox2D MiniBounds = FBox2D(ForceInit);
	bool bMiniBuilt = false;
};
