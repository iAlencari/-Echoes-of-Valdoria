// Echoes of Valdoria - controle do jogador.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ValdoriaPlayerController.generated.h"

UCLASS()
class ECHOESOFVALDORIA_API AValdoriaPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
};
