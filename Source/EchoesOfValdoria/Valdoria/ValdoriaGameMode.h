// Echoes of Valdoria - modo de jogo de treino (campo de tiro com bonecos).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ValdoriaGameMode.generated.h"

class AValdoriaHeroCharacter;
class UValdoriaHeroDefinition;

UCLASS()
class ECHOESOFVALDORIA_API AValdoriaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AValdoriaGameMode();

	virtual void StartPlay() override;

	/** Herói usado ao entrar (vazio = Recruta de teste). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Valdoria")
	TObjectPtr<UValdoriaHeroDefinition> DefaultHero;

	/** Cria bonecos de treino na frente do ponto de nascimento. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Valdoria")
	bool bSpawnTrainingDummies = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Valdoria")
	float PlayerRespawnDelay = 4.f;

	void OnHeroKilled(AValdoriaHeroCharacter* Victim, AValdoriaHeroCharacter* Killer);

protected:
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
	void SpawnTrainingDummies();
	void RespawnPlayer(AController* Controller);

	/** Teste automático (-ValdoriaAutoTest): atira nos bonecos, usa habilidades,
	 *  registra o resultado no log, tira uma captura de tela e fecha o jogo. */
	void RunAutoTestStep();
	int32 AutoTestStep = 0;
	FTimerHandle AutoTestTimer;
};
