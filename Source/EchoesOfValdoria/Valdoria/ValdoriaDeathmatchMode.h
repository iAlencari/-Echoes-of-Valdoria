// Echoes of Valdoria - free-for-all deathmatch ("mata-mata").
// Rules live in a data asset (UValdoriaModeRules) so the owner can balance them.
// Architecture for modes: one GameMode per mode, rules in a data asset. The 6v6 MOBA comes later.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ValdoriaGameMode.h"
#include "ValdoriaDeathmatchMode.generated.h"

class AValdoriaTrainingDummy;

/** Match rules for a mode (Content/Valdoria/Modes/DA_*). */
UCLASS(BlueprintType)
class ECHOESOFVALDORIA_API UValdoriaModeRules : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mode")
	FText DisplayName = NSLOCTEXT("ValdoriaModes", "Deathmatch", "Deathmatch");

	/** Kills needed to win. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mode", meta = (ClampMin = "1"))
	int32 ScoreToWin = 15;

	/** Match length in seconds (0 = no limit). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mode", meta = (ClampMin = "0"))
	float TimeLimit = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mode", meta = (ClampMin = "0"))
	float RespawnDelay = 3.f;

	/** Bots in the match (each one is its own team). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bots", meta = (ClampMin = "0", ClampMax = "11"))
	int32 BotCount = 5;

	/** 1 easy, 2 medium, 3 hard. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bots", meta = (ClampMin = "1", ClampMax = "3"))
	int32 BotDifficulty = 2;

	/** Heroes the bots pick from (preset ids). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bots")
	TArray<FName> BotHeroes = { TEXT("brasa"), TEXT("kael"), TEXT("iara"), TEXT("vex"), TEXT("lume") };
};

UCLASS()
class ECHOESOFVALDORIA_API AValdoriaDeathmatchMode : public AValdoriaGameMode
{
	GENERATED_BODY()
public:
	AValdoriaDeathmatchMode();
	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnHeroKilled(AValdoriaHeroCharacter* Victim, AValdoriaHeroCharacter* Killer) override;

	const UValdoriaModeRules* GetRules() const { return Rules; }
	int32 GetPlayerScore() const { return PlayerScore; }
	int32 GetBestBotScore() const;
	float GetTimeLeft() const;
	/** 0 playing, 1 player won, 2 player lost. */
	int32 GetResult() const { return Result; }

protected:
	void SpawnBots();
	void EndMatch(bool bPlayerWon);

	UPROPERTY(Transient) TObjectPtr<UValdoriaModeRules> Rules;
	TMap<uint8, int32> BotScores;
	TArray<TWeakObjectPtr<AValdoriaTrainingDummy>> Bots;
	int32 PlayerScore = 0;
	int32 Result = 0;
	float MatchStart = 0.f;
	bool bAutoPlayer = false;
};
