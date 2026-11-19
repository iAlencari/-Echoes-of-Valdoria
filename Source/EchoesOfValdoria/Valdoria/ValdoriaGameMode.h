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

	virtual void OnHeroKilled(AValdoriaHeroCharacter* Victim, AValdoriaHeroCharacter* Killer);

	/** Cria um boneco de treino (usado também pelo painel de administrador). Kind: 0 básico, 1 armadura, 2 escudo, 3 andando. */
	AActor* SpawnDummyAt(const FVector& Location, const FRotator& Facing, uint8 Kind);

	/** Turn 44: a random open spot at street level anywhere on the map, at least MinDistance from every living
	 *  hero (deathmatch spawns, so nobody is born on top of the enemies). False if none was found. */
	bool FindSpreadSpawnPoint(FVector& OutLocation, float MinDistance = 1500.f);
	/** Respawn the player at a spread spot instead of the player start (deathmatch). */
	bool bSpreadRespawns = false;

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
	/** Admin panel test (-ValdoriaAdminTest): clicks every panel control and logs the state after each. */
	void RunAdminTestStep();
	void RunBeamTestStep();
	int32 BeamTestStep = 0;
	FVector LumeJumpStart = FVector::ZeroVector;
	int32 AdminTestStep = 0;
	TArray<FString> AdminTestQueue;
	int32 AutoTestStep = 0;
	FTimerHandle AutoTestTimer;

	/** Playtest observer (-ValdoriaPlaytest=<seconds> [-ValdoriaHero=<id>] [-ValdoriaAuto]): picks the hero, puts it on
	 *  autopilot, logs every hero's state, flags stuck / fallen heroes, kills and deaths per hero, screenshots, exits. */
	void RunPlaytestStep();
	int32 PlaytestStep = 0;
	int32 PlaytestSeconds = 0;
	FString PlaytestHero;
	FString PlaytestTag;
	TMap<FString, int32> PlaytestKills;
	TMap<FString, int32> PlaytestDeaths;
	TMap<FString, FVector> PlaytestLastPos;
	TMap<FString, int32> PlaytestStill;
	TSet<FString> PlaytestReported;

	FBox SpreadBounds = FBox(ForceInit);
	float SpreadFloorZ = 0.f;
	bool bSpreadReady = false;
};
