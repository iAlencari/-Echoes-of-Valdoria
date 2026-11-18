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

UENUM(BlueprintType)
enum class EValdoriaSpotState : uint8
{
	Normal, SemiSealed, Sealed, Gone
};

UCLASS()
class ECHOESOFVALDORIA_API AValdoriaGuardian : public AValdoriaTrainingDummy
{
	GENERATED_BODY()
public:
	AValdoriaGuardian();
	virtual void Tick(float DeltaSeconds) override;
	virtual void ScheduleRevive() override;
	virtual void HandleDeath(AValdoriaHeroCharacter* Killer) override;

	// --- Sealing ritual
	EValdoriaSpotState State = EValdoriaSpotState::Normal;
	bool bRevived = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot|Seal")
	float SealSeconds = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot|Seal")
	float RevertSeconds = 8.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot|Seal")
	float RevertHealthFraction = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot|Seal")
	float RevivedRegenCap = 0.5f;
	/** Seconds until a semi-sealed guardian frees itself (0 = never; suggestion, off). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot|Seal")
	float SemiSealTimeout = 0.f;
	bool CanBeChanneledBy(const AValdoriaHeroCharacter* Hero) const;
	void CompleteSeal(AValdoriaHeroCharacter* By);
	void CompleteRevert(AValdoriaHeroCharacter* By);
	bool IsLinkedToFinal() const;

	// --- Final guardian (base): stronger for each linked spot guardian of its team
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot|Final")
	bool bFinalGuardian = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot|Final")
	float PerSpotHealth = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot|Final")
	float PerSpotDamage = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot|Final")
	float FinalBaseDamage = 4.f;

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

	/** Current buff value after the fade, times BuffMultiplier. */
	float GetCurrentBuff() const;

	/** Set by the MOBA mode (e.g. x3 on the last spot after early losses). */
	float BuffMultiplier = 1.f;
	int32 SpotIndex = 0;
	int32 LaneIndex = 0;

	/** Support mage (staff): area heal on a cooldown (heals the other guardian too) and an empower on
	 *  the lowest-health ally in the area (damage reduction, or reflection every other cast). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot|Mage")
	bool bSupportMage = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot|Mage")
	float MageHeal = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot|Mage")
	float MageHealCooldown = 8.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot|Mage")
	float MageEmpowerCooldown = 12.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spot|Mage")
	float MageEmpowerReduction = 0.4f;

	/** Call right after spawning: team and area (centre = current location). */
	void SetupSpot(uint8 Team, float Radius);

protected:
	virtual void BeginPlay() override;
	float BuffTimer = 0.f;
	float NextRiderStrike = 0.f;
	float NextMageHeal = 0.f;
	float NextMageEmpower = 0.f;
	int32 EmpowerCount = 0;
	float SemiSealedSince = 0.f;
	int32 LinkedSpots = -1;
	void Roar(bool bRage);
	void TickSpotState(float DeltaSeconds);
	void TickMage();
	void StrikeRiders();
};
