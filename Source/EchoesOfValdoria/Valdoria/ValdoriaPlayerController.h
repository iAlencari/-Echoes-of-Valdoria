// Echoes of Valdoria - controle do jogador: mouse, menu de pausa (Esc), Alt para soltar o
// cursor e painel de administrador (F1).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ValdoriaPlayerController.generated.h"

class SWidget;
class AValdoriaHeroCharacter;
class UValdoriaHeroDefinition;

UCLASS()
class ECHOESOFVALDORIA_API AValdoriaPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	bool IsPauseMenuOpen() const { return PauseWidget.IsValid(); }
	bool IsAdminPanelOpen() const { return AdminWidget.IsValid(); }
	void DebugSetAdminMode(bool bOn) { bAdminMode = bOn; }
	void DebugOpenSettings() { OpenSettings(); }

	/** Esc: abre/fecha o menu de pausa. */
	void TogglePauseMenu();

	/** F1: abre/fecha o painel de administrador (o jogo continua rodando). */
	void ToggleAdminPanel();

	/** Tela de ajuda das habilidades (segurar K). */
	void HelpPressed();
	void HelpReleased();
	bool IsHelpOpen() const { return HelpWidget.IsValid(); }

	/** Inimigo/personagem sob a mira (o que o painel edita). */
	AValdoriaHeroCharacter* PickAimedCharacter() const;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void OpenPauseMenu();
	void ClosePauseMenu();

	/** Segurar Alt: solta o cursor (para arrastar a janela ou usar outro programa). */
	void FreeCursorPressed();
	void FreeCursorReleased();

	void ApplyGameInput();
	void ApplyCursorInput();
	void RefreshInputMode();

	FReply OnResumeClicked();
	FReply OnRestartClicked();
	FReply OnQuitClicked();

	// --- Painel de administrador (ValdoriaAdminPanel.cpp)
	void OpenAdminPanel();
	void CloseAdminPanel();
	TSharedRef<SWidget> BuildAdminPanel();
	AValdoriaHeroCharacter* GetHero() const;
	FVector GetAimPoint() const;
	void AdminTeleport(const FVector& Location);
	void AdminSetTargetAttribute(int32 Which, float Value);

	TSharedRef<SWidget> BuildHelpScreen();
	TSharedPtr<SWidget> HelpWidget;

	// --- Configurações (ValdoriaSettingsMenu.cpp)
	void OpenSettings();
	void CloseSettings();
	TSharedRef<SWidget> BuildSettingsScreen();
	void ApplyOptionsToGame();
	TSharedPtr<SWidget> SettingsWidget;

	/** Modo administrador: ligado pelo menu de pausa; só com ele o F1 abre o painel. */
	bool bAdminMode = false;

	TSharedPtr<SWidget> PauseWidget;
	TSharedPtr<SWidget> AdminWidget;
	TWeakObjectPtr<AValdoriaHeroCharacter> AdminTarget;
	TArray<TWeakObjectPtr<UValdoriaHeroDefinition>> AdminHeroList;
	TArray<FString> AdminMapList;
	FVector SavedPositions[3];
	FRotator SavedRotations[3];
	bool bSavedPosition[3] = { false, false, false };
	int32 AdminBotDifficulty = 0;
	bool bAltHeld = false;
};
