// Echoes of Valdoria - controle do jogador.

#include "ValdoriaPlayerController.h"

void AValdoriaPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		// Mouse preso no centro e escondido durante o jogo (evita o cursor
		// "travado" num canto da tela).
		FInputModeGameOnly Mode;
		Mode.SetConsumeCaptureMouseDown(false);
		SetInputMode(Mode);
		bShowMouseCursor = false;
	}
}
