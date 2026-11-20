// Echoes of Valdoria - turn 46: unit kinds, MOBA levels/XP, match stats (Tab) and the V punch.
// Rules (owner): 20 levels, 1 point per level, only abilities 1-4 are upgraded; abilities 1-3 have 5 ranks
// (unlock + 4 upgrades), the supreme unlocks at level 5 and upgrades at 9, 13, 16 and 20. Elevation points at
// levels 10 and 20 (effects come later, per hero). XP and gold numbers are provisional (owner balances).

#include "ValdoriaHeroCharacter.h"
#include "ValdoriaMultipliers.h"
#include "ValdoriaHeroDefinition.h"
#include "ValdoriaTrainingDummy.h"
#include "ValdoriaGuardian.h"
#include "ValdoriaAnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"

namespace
{
	const int32 UltRankLevels[5] = { 5, 9, 13, 16, 20 };
	constexpr int32 MaxHeroLevel = 20;
	constexpr float PowerPerRank = 0.06f;    // +6% damage/heal per upgrade (Claude, provisional)
	constexpr float CooldownPerRank = 0.05f; // -5% cooldown per upgrade (Claude, provisional)
}

bool AValdoriaHeroCharacter::IsMinionUnit() const
{
	const AValdoriaTrainingDummy* D = Cast<AValdoriaTrainingDummy>(this);
	return D && D->bMinion;
}

bool AValdoriaHeroCharacter::IsGuardianUnit() const
{
	return IsA<AValdoriaGuardian>();
}

bool AValdoriaHeroCharacter::IsHeroUnit() const
{
	return TeamId != 255 && !IsMinionUnit() && !IsGuardianUnit();
}

int32 AValdoriaHeroCharacter::RankIndex(EValdoriaAbilitySlot Slot)
{
	switch (Slot)
	{
	case EValdoriaAbilitySlot::Ability1: return 0;
	case EValdoriaAbilitySlot::Ability2: return 1;
	case EValdoriaAbilitySlot::Ability3: return 2;
	case EValdoriaAbilitySlot::Ultimate: return 3;
	default: return INDEX_NONE;
	}
}

int32 AValdoriaHeroCharacter::GetAbilityRank(EValdoriaAbilitySlot Slot) const
{
	const int32 I = RankIndex(Slot);
	return (!bUsesLevels || I == INDEX_NONE) ? 1 : AbilityRanks[I];
}

bool AValdoriaHeroCharacter::IsSlotLocked(EValdoriaAbilitySlot Slot) const
{
	return bUsesLevels && RankIndex(Slot) != INDEX_NONE && GetAbilityRank(Slot) <= 0;
}

bool AValdoriaHeroCharacter::CanUpgrade(EValdoriaAbilitySlot Slot) const
{
	const int32 I = RankIndex(Slot);
	if (!bUsesLevels || I == INDEX_NONE || AbilityPoints <= 0 || AbilityRanks[I] >= 5)
	{
		return false;
	}
	if (Slot == EValdoriaAbilitySlot::Ultimate)
	{
		return Level >= UltRankLevels[AbilityRanks[I]];
	}
	// Abilities 1-3: rank up to (level + 1) / 2, so one ability cannot be maxed at level 5 (Claude's rule).
	return AbilityRanks[I] < (Level + 1) / 2;
}

bool AValdoriaHeroCharacter::TryUpgrade(EValdoriaAbilitySlot Slot)
{
	if (!CanUpgrade(Slot))
	{
		return false;
	}
	++AbilityRanks[RankIndex(Slot)];
	--AbilityPoints;
	return true;
}

float AValdoriaHeroCharacter::GetRankPowerMultiplier(EValdoriaAbilitySlot Slot) const
{
	return 1.f + PowerPerRank * FMath::Max(0, GetAbilityRank(Slot) - 1);
}

float AValdoriaHeroCharacter::GetRankCooldownMultiplier(EValdoriaAbilitySlot Slot) const
{
	return 1.f - CooldownPerRank * FMath::Max(0, GetAbilityRank(Slot) - 1);
}

float AValdoriaHeroCharacter::XPToNextLevel() const
{
	return 120.f + 60.f * (Level - 1); // turn 47 (owner): double, growing each level; level 20 after ~12500 XP
}

void AValdoriaHeroCharacter::GainXP(float Amount)
{
	if (!bUsesLevels || Amount <= 0.f || Level >= MaxHeroLevel)
	{
		return;
	}
	XP += Amount * ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::Experience);
	bool bLevelled = false;
	while (Level < MaxHeroLevel && XP >= XPToNextLevel())
	{
		XP -= XPToNextLevel();
		++Level;
		++AbilityPoints;
		if (Level == 10 || Level == 20)
		{
			++ElevationPoints;
		}
		bLevelled = true;
	}
	if (Level >= MaxHeroLevel)
	{
		XP = 0.f;
	}
	if (bLevelled && !IsPlayerControlled())
	{
		AutoSpendPoints();
	}
}

void AValdoriaHeroCharacter::AutoSpendPoints()
{
	// Bots: supreme whenever allowed, otherwise the lowest of 1/2/3 (ties: 1, then 2, then 3).
	static const EValdoriaAbilitySlot Order[3] = { EValdoriaAbilitySlot::Ability1, EValdoriaAbilitySlot::Ability2, EValdoriaAbilitySlot::Ability3 };
	for (int32 Guard = 0; Guard < 25 && AbilityPoints > 0; ++Guard)
	{
		if (TryUpgrade(EValdoriaAbilitySlot::Ultimate))
		{
			continue;
		}
		EValdoriaAbilitySlot Best = EValdoriaAbilitySlot::None;
		for (EValdoriaAbilitySlot S : Order)
		{
			if (CanUpgrade(S) && (Best == EValdoriaAbilitySlot::None || GetAbilityRank(S) < GetAbilityRank(Best)))
			{
				Best = S;
			}
		}
		if (Best == EValdoriaAbilitySlot::None || !TryUpgrade(Best))
		{
			break;
		}
	}
}

AValdoriaHeroCharacter::FProgress AValdoriaHeroCharacter::TakeProgress() const
{
	FProgress P;
	P.bValid = true;
	P.bUsesLevels = bUsesLevels;
	P.Level = Level;
	P.XP = XP;
	P.AbilityPoints = AbilityPoints;
	P.ElevationPoints = ElevationPoints;
	FMemory::Memcpy(P.Ranks, AbilityRanks, sizeof(AbilityRanks));
	P.Stats = Stats;
	P.Recap = LastDeathRecap;
	return P;
}

void AValdoriaHeroCharacter::ApplyProgress(const FProgress& P)
{
	bUsesLevels = P.bUsesLevels;
	Level = P.Level;
	XP = P.XP;
	AbilityPoints = P.AbilityPoints;
	ElevationPoints = P.ElevationPoints;
	FMemory::Memcpy(AbilityRanks, P.Ranks, sizeof(AbilityRanks));
	Stats = P.Stats;
	LastDeathRecap = P.Recap;
}

void AValdoriaHeroCharacter::TryMeleePunch()
{
	UWorld* World = GetWorld();
	if (!World || bIsDead || !ActiveDefinition || World->GetTimeSeconds() < NextPunchTime || IsRiding())
	{
		return;
	}
	NextPunchTime = World->GetTimeSeconds() + 0.8f;
	if (UValdoriaAnimInstance* Anim = GetMesh() ? Cast<UValdoriaAnimInstance>(GetMesh()->GetAnimInstance()) : nullptr)
	{
		// Turn 48 (owner: the V animation looked broken for everyone): play the real punch on the upper body, like the
		// other attacks, instead of a frozen frame (SetUpperPose must be fed every frame, so a single call popped).
		static int32 PunchIndex = 0;
		static const TCHAR* Punches[] = {
			TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01"),
			TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02.MM_Attack_02") };
		PunchIndex = (PunchIndex + 1) % 2; // alternate the two punches
		Anim->PlayAction(LoadObject<UAnimSequenceBase>(nullptr, Punches[PunchIndex]), 1.5f);
	}
	if (!HasAuthority())
	{
		return;
	}
	// Nearest enemy in a short cone in front (2 m), physical damage; vampirism applies (owner: LMB and V).
	const FVector Fwd = GetActorForwardVector().GetSafeNormal2D();
	AValdoriaHeroCharacter* Best = nullptr;
	float BestDist = 220.f;
	for (TActorIterator<AValdoriaHeroCharacter> It(World); It; ++It)
	{
		if (!IsEnemy(*It) || It->IsDead())
		{
			continue;
		}
		const FVector To = It->GetActorLocation() - GetActorLocation();
		const float D = To.Size2D();
		if (D < BestDist && FMath::Abs(To.Z) < 150.f && FVector::DotProduct(To.GetSafeNormal2D(), Fwd) > 0.5f)
		{
			BestDist = D;
			Best = *It;
		}
	}
	if (Best)
	{
		DamageSourceLabel = TEXT("Soco (V)");
		DealTypedDamage(Best, ActiveDefinition->MeleeDamage, EValdoriaDamageType::Physical, 0.f, false);
		DamageSourceLabel.Reset();
	}
}

void AValdoriaHeroCharacter::AwardDeath(AValdoriaHeroCharacter* Killer)
{
	UWorld* World = GetWorld();
	const float Now = World->GetTimeSeconds();
	AValdoriaHeroCharacter* KillerHero = (Killer && Killer != this && Killer->IsHeroUnit()) ? Killer : nullptr;
	if (IsMinionUnit())
	{
		// Minion XP: the killer gets 40, allied heroes within 15 m get 20 (provisional).
		if (KillerHero)
		{
			KillerHero->GainXP(40.f);
		}
		for (TActorIterator<AValdoriaHeroCharacter> It(World); It; ++It)
		{
			if (*It != KillerHero && It->IsHeroUnit() && !It->IsDead() && IsEnemy(*It)
				&& FVector::Dist2D(It->GetActorLocation(), GetActorLocation()) < 1500.f)
			{
				It->GainXP(20.f);
			}
		}
		return;
	}
	if (!IsHeroUnit())
	{
		RecentDamagers.Reset();
		return; // guardians: XP comes from the spot rules (semi-seal / seal)
	}
	++Stats.Deaths;
	// Death recap: damage of the last 10 s, grouped by attacker + source, biggest first.
	{
		FDeathRecap R;
		R.bValid = true;
		R.Killer = Killer && Killer != this ? Killer->GetUnitName() : FString(TEXT("Queda / ambiente"));
		TMap<FString, int32> Index;
		for (const FDamageLogEntry& E : DamageLog)
		{
			if (Now - E.Time > 10.f)
			{
				continue;
			}
			const FString Key = E.Attacker + TEXT("|") + E.Source;
			if (int32* I = Index.Find(Key))
			{
				R.Lines[*I].Amount += E.Amount;
			}
			else
			{
				Index.Add(Key, R.Lines.Add(E));
			}
			R.Total += E.Amount;
		}
		if (DamageLog.Num() > 0)
		{
			R.KillerSource = DamageLog.Last().Source;
		}
		R.Lines.Sort([](const FDamageLogEntry& A, const FDamageLogEntry& B) { return A.Amount > B.Amount; });
		LastDeathRecap = R;
		DamageLog.Reset();
	}
	if (KillerHero)
	{
		++KillerHero->Stats.Kills;
		KillerHero->GainXP(200.f);
		KillerHero->Stats.Gold += 100.f * ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::Gold);
	}
	// Assists: enemy heroes who hit this hero in the last 10 s (not the killer).
	for (const TPair<TWeakObjectPtr<AValdoriaHeroCharacter>, float>& P : RecentDamagers)
	{
		AValdoriaHeroCharacter* A = P.Key.Get();
		if (A && A != KillerHero && A->IsHeroUnit() && IsEnemy(A) && Now - P.Value < 10.f)
		{
			++A->Stats.Assists;
			A->GainXP(100.f);
			A->Stats.Gold += 50.f * ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::Gold);
		}
	}
	RecentDamagers.Reset();
}

FString AValdoriaHeroCharacter::GetUnitName() const
{
	if (IsMinionUnit())
	{
		return TEXT("Minion");
	}
	if (IsGuardianUnit())
	{
		return TEXT("Guardião");
	}
	const FString Who = ActiveDefinition && !ActiveDefinition->DisplayName.IsEmpty() ? ActiveDefinition->DisplayName.ToString() : TEXT("Recruta");
	return IsPlayerControlled() ? Who + TEXT(" (você)") : Who;
}
