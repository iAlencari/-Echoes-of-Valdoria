// Echoes of Valdoria - HUD desenhado em código (sem assets), no estilo das referências do dono:
// barra de vida segmentada na cor de cada herói, aliado ciano / inimigo vermelho com caveira,
// passiva + 3 habilidades + ultimate com moldura, munição grande com ícones de balas.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ValdoriaHUD.generated.h"

class AValdoriaHeroCharacter;

UCLASS()
class ECHOESOFVALDORIA_API AValdoriaHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	/** Marcador de acerto + número de dano flutuante. */
	void AddHitMarker(const FVector& WorldLocation, float Damage, bool bHeadshot, bool bKill);

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
	void DrawFrame(float X, float Y, float W, float H, const FLinearColor& Color, float Thickness = 2.f);
	void DrawLabel(const FString& Text, float X, float Y, const FLinearColor& Color, float TextScale = 1.f, bool bCenter = false, bool bLarge = false);
	void DrawSegmentedBar(float X, float Y, float W, float H, AValdoriaHeroCharacter* Hero, const FLinearColor& HealthColor);
	void DrawSkull(float Cx, float Cy, float Size, const FLinearColor& Color);

	void DrawCrosshairAndHits(float W, float H);
	void DrawStats();
	void DrawWorldHealthBars(AValdoriaHeroCharacter* Viewer);
	void DrawPlayerPanel(AValdoriaHeroCharacter* Hero, float W, float H);
	void DrawAbilities(AValdoriaHeroCharacter* Hero, float W, float H);
	void DrawAmmo(AValdoriaHeroCharacter* Hero, float W, float H);
};
