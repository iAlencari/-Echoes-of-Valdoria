// Echoes of Valdoria - controle do jogador: mouse, menu de pausa (Esc) e Alt para soltar o cursor.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ValdoriaPlayerController.generated.h"

class SWidget;

UCLASS()
class ECHOESOFVALDORIA_API AValdoriaPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	bool IsPauseMenuOpen() const { return PauseWidget.IsValid(); }

	/** Esc: abre/fecha o menu de pausa. */
	void TogglePauseMenu();

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

	FReply OnResumeClicked();
	FReply OnRestartClicked();
	FReply OnQuitClicked();

	TSharedPtr<SWidget> PauseWidget;
	bool bAltHeld = false;
};
