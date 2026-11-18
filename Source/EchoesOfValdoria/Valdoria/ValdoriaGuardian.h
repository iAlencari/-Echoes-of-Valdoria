// Echoes of Valdoria - MOBA "spots": guardian monsters/bosses used instead of towers.
// Owner's design: each spot is a boss that moves inside its area, focuses the enemy hero who
// attacks an allied hero there, and gives its team a buff while it lives. Spot 1: 50% damage
// reduction during the first minute, fading to 0% at minute 5. Buffs for spots 2 and 3 are TBD.

#pragma once

#include "CoreMinimal.h"
#include "ValdoriaTrainingDummy.h"
#include "ValdoriaGuardian.generated.h"

UENUM(BlueprintType)
enum class EValdoriaSpotBuff : uint8
{
	DamageReduction	UMETA(DisplayName = "Damage reduction for allies"),
	DamageBonus		UMETA(DisplayName = "Extra damage for allies"),
	Regeneration	UMETA(DisplayName = "Health regeneration for allies (per second)")
};

/** How a guardian brings down enemies riding an Aurora Trail over its area. */
UENUM(BlueprintType)
enum class EValdoriaGuardianStyle : uint8
{
	Melee		UMETA(DisplayName = "Melee: quick charged leap that knocks the rider down"),
	Mage		UMETA(DisplayName = "Ranged magic: spell from the ground that pulls and roots"),
	Flyer		UMETA(DisplayName = "Flyer: grabs the rider and slams them down (stun)"),
	Thrower		UMETA(DisplayName = "Ranged physical: throws something to knock them down")
};

UCLASS()
class ECHOESOFVALDORIA_API AValdoriaGuardian : public AValdoriaTrainingDummy
{
	GENERATED_BODY()
public:
	AValdoriaGuardian();
	virtual void Tick(float DeltaSeconds) override;
	virtual void ScheduleRevive() override;

	/** Team this spot belongs to (its allies get the buff). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot")
	uint8 GuardTeam = 0;

	/** The guardian never leaves this radius (cm) around where it was placed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot")
	float AreaRadius = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot")
	float GuardianHealth = 3000.f;

	/** Seconds until it comes back after dying (0 = stays dead for the match). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot")
	float RespawnSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot")
	EValdoriaGuardianStyle Style = EValdoriaGuardianStyle::Melee;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot|Buff")
	EValdoriaSpotBuff Buff = EValdoriaSpotBuff::DamageReduction;

	/** Full buff value (0.5 = 50%; for Regeneration, health per second). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot|Buff")
	float BuffValue = 0.5f;

	/** Buff stays at full value until this match time (s), then fades to 0 at BuffFadeEnd. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot|Buff")
	float BuffHoldUntil = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot|Buff")
	float BuffFadeEnd = 300.f;

	/** Current buff value after the fade. */
	float GetCurrentBuff() const;

	/** Call right after spawning: team and area (centre = current location). */
	void SetupSpot(uint8 Team, float Radius);

protected:
	virtual void BeginPlay() override;
	float BuffTimer = 0.f;
	float NextRiderStrike = 0.f;
	void StrikeRiders();
};
