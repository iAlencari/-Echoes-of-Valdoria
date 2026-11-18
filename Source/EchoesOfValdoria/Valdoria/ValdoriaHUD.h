// Echoes of Valdoria - HUD de teste desenhado em código (sem assets).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ValdoriaHUD.generated.h"

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

	void DrawBar(float X, float Y, float W, float H, float Fraction, const FLinearColor& Color);
	void DrawLabel(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale = 1.f, bool bCenter = false);
};
