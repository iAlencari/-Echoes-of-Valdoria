// Echoes of Valdoria - 6v6 MOBA with 3 lanes and Guardian Spots instead of towers.
// Owner's design (CLAUDE.md): 3 spots per lane per team; the last spot has two guardians.
// Side lanes: long walk from the base to the first spot, spots close together.
// Middle lane: first spot closer to the base, more space between spots.
// The layout is generated between the two team starts (untagged PlayerStart = team 0,
// tag "Time2" = team 1), so it works on any map; all numbers are in the rules data asset.

#pragma once

#include "CoreMinimal.h"
#include "ValdoriaDeathmatchMode.h"
#include "ValdoriaGuardian.h"
#include "ValdoriaMobaMode.generated.h"

class AValdoriaGuardian;

/** One spot along a lane: where it sits and what its guardian(s) give. */
USTRUCT(BlueprintType)
struct FValdoriaSpotRule
{
	GENERATED_BODY()

	/** Distance from the team's base along the lane (0 = base, 1 = map centre). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spot")
	float SideLanePosition = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spot")
	float MidLanePosition = 0.5f;

	/** One entry per guardian at this spot (the last spot has two). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spot")
	TArray<EValdoriaSpotBuff> Buffs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spot")
	TArray<float> BuffValues;

	/** Buff full until this match time (s), fades to 0 at BuffFadeEnd. Fade end 0 = never fades. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spot")
	float BuffHoldUntil = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spot")
	float BuffFadeEnd = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spot")
	float GuardianHealth = 3000.f;
};

UCLASS(BlueprintType)
class ECHOESOFVALDORIA_API UValdoriaMobaRules : public UValdoriaModeRules
{
	GENERATED_BODY()
public:
	UValdoriaMobaRules();

	/** Heroes per team (the player counts in team 0). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MOBA", meta = (ClampMin = "1", ClampMax = "6"))
	int32 TeamSize = 6;

	/** Sideways distance (cm) of the side lanes from the middle lane. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MOBA")
	float SideLaneOffset = 2500.f;

	/** Spots from the outside in: [0] = spot 1 (first one the enemy meets), [2] = last spot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MOBA")
	TArray<FValdoriaSpotRule> Spots;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MOBA")
	float SpotAreaRadius = 1200.f;
};

UCLASS()
class ECHOESOFVALDORIA_API AValdoriaMobaMode : public AValdoriaGameMode
{
	GENERATED_BODY()
public:
	AValdoriaMobaMode();
	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnHeroKilled(AValdoriaHeroCharacter* Victim, AValdoriaHeroCharacter* Killer) override;

	const UValdoriaMobaRules* GetRules() const { return Rules; }
	/** Guardians still alive per team. */
	int32 GetGuardiansAlive(uint8 Team) const;
	int32 GetKills(uint8 Team) const { return Team == 0 ? Kills0 : Kills1; }
	float GetTimeLeft() const;
	/** 0 playing, 1 team 0 won, 2 team 1 won. */
	int32 GetResult() const { return Result; }

protected:
	void BuildLayout();
	FVector Ground(const FVector& Point) const;
	void EndMatch(uint8 Winner);

	UPROPERTY(Transient) TObjectPtr<UValdoriaMobaRules> Rules;
	TArray<TWeakObjectPtr<AValdoriaGuardian>> Guardians;
	FVector Base[2];
	int32 Kills0 = 0;
	int32 Kills1 = 0;
	int32 Result = 0;
	float MatchStart = 0.f;
	bool bAutoPlayer = false;
};
