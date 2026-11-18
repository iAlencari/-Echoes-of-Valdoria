// Echoes of Valdoria - instância do jogo: tela de carregamento entre mapas e ao abrir o jogo.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Containers/Ticker.h"
#include "Containers/Ticker.h"
#include "ValdoriaGameInstance.generated.h"

class UTexture2D;
class SWidget;
class UAudioComponent;
struct FSlateBrush;

UCLASS()
class ECHOESOFVALDORIA_API UValdoriaGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	/** Nome bonito do mapa (ex.: NP_NovaPauliceia -> "Nova Paulicéia"). */
	static FString GetMapDisplayName(const FString& MapPath);

protected:
	void OnPreLoadMap(const FString& MapName);
	void OnPostLoadMap(UWorld* World);

	/** Tela com a arte do mapa, título, dica aleatória e um indicador girando. */
	TSharedRef<SWidget> BuildLoadingScreen(const FString& MapPath, bool bWithThrobber);
	void ShowIntroOverlay(const FString& MapPath);
	void HideIntroOverlay();

	UPROPERTY(Transient) TObjectPtr<UTexture2D> ImageNP;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> ImageTraining;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> LoadingMusic;

	TSharedPtr<FSlateBrush> BackgroundBrush;
	TSharedPtr<SWidget> IntroOverlay;
	float IntroStart = 0.f;
	FTSTicker::FDelegateHandle IntroTicker;
	FString PendingMap;
	/** Caps FPS while the window is not focused (BackgroundFpsLimit). */
	FTSTicker::FDelegateHandle FocusTicker;
	bool bThrottled = false;
	float SavedMaxFps = 0.f;
};
