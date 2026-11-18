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

	virtual void Tick(float DeltaSeconds) override;
	void ScheduleRevive();

protected:
	virtual void BeginPlay() override;
	void Revive();

	FVector StartLocation;
	float StrafeTime = 0.f;
	FTimerHandle ReviveTimer;
};
