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

	/** Index (in Buffs) of the guardian that is a support mage (-1 = none). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spot")
	int32 MageGuardianIndex = -1;
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

	// --- Comeback rules (owner's design)
	/** "Early" = before this match time (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Comeback")
	float EarlyWindow = 300.f;
	/** If a team's spot 1 falls early, that team keeps its buff x this, constant, until EarlyWindow. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Comeback")
	float Spot1EarlyFallMultiplier = 2.f;
	/** If spots 1 and 2 of a lane fall early, that lane's last spot buffs x this until LastSpotBoostUntil. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Comeback")
	float LastSpotMultiplier = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Comeback")
	float LastSpotBoostUntil = 600.f;

	// --- Final guardian (in the base, behind the spawn). The match is won by bringing it down.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Final Guardian")
	float FinalGuardianHealth = 8000.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Final Guardian")
	float FinalGuardianArea = 1800.f;
	/** Damage multiplier with no spot linked; each linked spot adds PerSpotDamage on top. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Final Guardian")
	float FinalBaseDamage = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Final Guardian")
	float PerSpotHealth = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Final Guardian")
	float PerSpotDamage = 0.15f;

	// --- Sealing ritual
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Seal")
	float SealSeconds = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Seal")
	float RevertSeconds = 8.f;

	/** Lane names (left, middle, right). Nova Paulicéia uses famous Brazilian avenues. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bifrost")
	TArray<FString> LaneNames = { TEXT("Av. Ipiranga"), TEXT("Av. Paulista"), TEXT("Av. Rebouças") };

	/** Calm trail colour per lane (enemy trails get a red accent). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bifrost")
	TArray<FLinearColor> LaneColors = { FLinearColor(0.35f, 0.75f, 1.f), FLinearColor(0.75f, 0.55f, 1.f), FLinearColor(0.4f, 1.f, 0.7f) };

	/** How far each trail reaches without minions (0.5 = the middle of the map). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bifrost")
	float TrailReach = 0.5f;

	/** Height of the horizontal part over the lane, and of the vertical start at the base (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bifrost")
	float TrailHeight = 700.f;
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
	TWeakObjectPtr<AValdoriaGuardian> FinalGuardian[2];
	FVector Base[2];
	int32 Kills0 = 0;
	int32 Kills1 = 0;
	int32 Result = 0;
	float MatchStart = 0.f;
	bool bAutoPlayer = false;
	/** Match time when each guardian fell: key = team*100 + lane*10 + spot. */
	TMap<int32, float> FallTimes;
	float ComebackTimer = 0.f;
	void TickComeback();
};
