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

	/** Cria um boneco de treino (usado também pelo painel de administrador). Kind: 0 básico, 1 armadura, 2 escudo, 3 andando. */
	AActor* SpawnDummyAt(const FVector& Location, const FRotator& Facing, uint8 Kind);

protected:
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
	void SpawnTrainingDummies();
	void RespawnPlayer(AController* Controller);

	/** Teste automático (-ValdoriaAutoTest): atira nos bonecos, usa habilidades,
	 *  registra o resultado no log, tira uma captura de tela e fecha o jogo. */
	void RunAutoTestStep();
	/** Teste de entrada (-ValdoriaInputTest): teclas de verdade pelo Slate + morrer duas vezes. */
	void RunInputTestStep();
	void PressKey(const FKey& Key, bool bDown);
	int32 InputTestStep = 0;
	FVector InputTestStartLoc = FVector::ZeroVector;
	int32 InputTestDeaths = 0;
	/** Teste dos heróis (-ValdoriaHeroTest): troca para cada herói e usa todas as habilidades num boneco. */
	void RunHeroTestStep();
	/** Teste de pose da arma (-ValdoriaWeaponPose): gira a arma do Kael e tira um print de cada. */
	void RunWeaponPoseStep();
	/** Flight test (-ValdoriaFlightTest): superhero and levitate styles, side screenshots. */
	void RunFlightTestStep();
	int32 FlightTestStep = 0;
	int32 WeaponPoseStep = 0;
	int32 HeroTestStep = 0;
	TWeakObjectPtr<class AValdoriaTrainingDummy> HeroTestEnemy;
	TWeakObjectPtr<class AValdoriaTrainingDummy> HeroTestAlly;
	FVector HeroTestHome = FVector::ZeroVector;
	FRotator HeroTestHomeRot = FRotator::ZeroRotator;
	int32 AutoTestStep = 0;
	FTimerHandle AutoTestTimer;
};
