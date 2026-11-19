// Echoes of Valdoria - global balance multipliers per game mode (turn 48, owner).
//
// Read from Config/Multiplicadores.ini (plain text, edited by the owner). Every value is a multiplier (1.0 = normal).
// A section [Todos] applies to every mode; a section [Modo N] applies only to mode N; both multiply together.
// Anything missing (a line, a whole section or the file itself) counts as 1.0.
//
// Mode ids (keep in sync with the lobby list in ValdoriaLobby.cpp and with the comments in the ini):
//   0 = Campo de Treino   1 = Treino em Nova Pauliceia   2 = Mata-mata   3 = Guardian Spots (MOBA)

#pragma once

#include "CoreMinimal.h"

class UWorld;

enum class EValdoriaMult : uint8
{
	Damage,          // all damage dealt by heroes, minions and bots
	PhysicalDamage,  // physical part of a hit
	MagicalDamage,   // magical part of a hit
	AttackDamage,    // basic attack (left mouse) and the V punch
	AbilityDamage,   // abilities and passives
	Healing,         // every heal (abilities, regen, vampirism and life steal heals)
	Vampirism,       // heal rate from basic attack / V punch
	LifeSteal,       // heal rate from abilities
	MaxHealth,       // hero max health
	MaxArmor,        // hero max armour
	MaxShield,       // hero max shield
	GuardianHealth,  // guardian spot health
	Cooldown,        // ability cooldowns (0.5 = half the time)
	UltimateCharge,  // ultimate charge gained
	MoveSpeed,       // movement speed
	Gold,            // credits / gold earned
	Experience,      // XP earned (MOBA levels)
	RespawnTime,     // time dead before coming back
	Count
};

namespace ValdoriaMultipliers
{
	/** Mode id of the running match (see the table above). */
	int32 GetModeId(const UWorld* World);
	/** [Todos] x [Modo N] for the current mode; 1.0 when not set. Never negative. */
	float Get(const UWorld* World, EValdoriaMult Which);
	/** Re-read the ini (admin panel / chat). */
	void Reload();
	/** Key name used in the ini for a multiplier. */
	const TCHAR* KeyName(EValdoriaMult Which);
}
