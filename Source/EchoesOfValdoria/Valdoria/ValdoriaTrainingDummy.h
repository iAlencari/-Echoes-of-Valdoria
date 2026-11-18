// Echoes of Valdoria - boneco de treino (alvo) que usa o mesmo sistema de dano.

#pragma once

#include "CoreMinimal.h"
#include "ValdoriaHeroCharacter.h"
#include "ValdoriaTrainingDummy.generated.h"

UENUM(BlueprintType)
enum class EValdoriaDummyKind : uint8
{
	Basic		UMETA(DisplayName = "Básico (só vida)"),
	Armored		UMETA(DisplayName = "Com armadura"),
	Shielded	UMETA(DisplayName = "Com escudo"),
	Strafing	UMETA(DisplayName = "Andando de lado")
};

UCLASS()
class ECHOESOFVALDORIA_API AValdoriaTrainingDummy : public AValdoriaHeroCharacter
{
	GENERATED_BODY()

public:
	AValdoriaTrainingDummy();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treino")
	EValdoriaDummyKind Kind = EValdoriaDummyKind::Basic;

	/** Volta à vida cheia depois deste tempo sem levar dano. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treino")
	float ResetDelay = 3.f;

	/** Bot atirador: 0 parado (só alvo), 1 fácil, 2 médio, 3 difícil. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treino")
	int32 BotDifficulty = 0;

	/** Faz o boneco andar de lado (alvo em movimento), qualquer que seja o tipo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treino")
	bool bForceStrafe = false;

	/** Não regenera a vida sozinho (para medir dano). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treino")
	bool bNoAutoHeal = false;

	/** Hunter (deathmatch): moves, keeps distance, strafes, jumps and targets the nearest enemy (bots too). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Treino")
	bool bHunter = false;

	virtual void Tick(float DeltaSeconds) override;
	virtual void ScheduleRevive();

protected:
	virtual void BeginPlay() override;
	void Revive();

	FVector StartLocation;
	float StrafeTime = 0.f;
	float NextBotShotTime = 0.f;
	float NextTargetPick = 0.f;
	TWeakObjectPtr<AValdoriaHeroCharacter> BotTarget;
	void TickBot(float DeltaSeconds);
	FTimerHandle ReviveTimer;
};
