// Echoes of Valdoria - MOBA spots (guardian bosses).

#include "ValdoriaGuardian.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "ValdoriaAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "ValdoriaHeroDefinition.h"
#include "ValdoriaWorldActors.h"
#include "ValdoriaAudio.h"

AValdoriaGuardian::AValdoriaGuardian()
{
	bHunter = true;
	BotDifficulty = 0; // fights with the autopilot, not fake shots
}

void AValdoriaGuardian::BeginPlay()
{
	Super::BeginPlay();
	TeamId = GuardTeam;
	// Big and tough: Brasa's kit (hammer, slam), larger body, lots of health.
	SetHeroDefinition(UValdoriaHeroDefinition::CreatePreset(this, TEXT("brasa")));
	GetMesh()->SetRelativeScale3D(GetMesh()->GetRelativeScale3D() * 1.5f);
	if (HasAuthority())
	{
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetMaxHealthAttribute(), GuardianHealth);
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetHealthAttribute(), GuardianHealth);
	}
	bNoAutoHeal = true;
	SetAutopilot(true, 0.6f);
	SetLeash(GetActorLocation(), AreaRadius);
}

void AValdoriaGuardian::ScheduleRevive()
{
	if (RespawnSeconds > 0.f)
	{
		FTimerHandle Timer;
		GetWorldTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			Revive();
			AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetHealthAttribute(), GuardianHealth);
		}), RespawnSeconds, false);
	}
}

float AValdoriaGuardian::GetCurrentBuff() const
{
	const float T = GetWorld()->GetTimeSeconds();
	if (BuffFadeEnd <= 0.f || T <= BuffHoldUntil)
	{
		return BuffValue * BuffMultiplier;
	}
	const float Fade = FMath::Clamp((T - BuffHoldUntil) / FMath::Max(1.f, BuffFadeEnd - BuffHoldUntil), 0.f, 1.f);
	return BuffValue * (1.f - Fade) * BuffMultiplier;
}

void AValdoriaGuardian::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TickSpotState(DeltaSeconds);
	if (HasAuthority() && !IsDead() && State == EValdoriaSpotState::Normal)
	{
		StrikeRiders();
		if (bSupportMage)
		{
			TickMage();
		}
	}
	BuffTimer += DeltaSeconds;
	if (!HasAuthority() || IsDead() || State != EValdoriaSpotState::Normal || bFinalGuardian || BuffTimer < 0.5f)
	{
		return;
	}
	BuffTimer = 0.f;
	// The buff lasts 1 s on each ally and is refreshed while the guardian lives.
	const float Value = GetCurrentBuff();
	for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
	{
		if (*It != this && It->TeamId == GuardTeam && !It->IsDead() && !It->IsA<AValdoriaGuardian>())
		{
			It->ApplyTeamBuff(static_cast<uint8>(Buff), Value, 1.f);
		}
	}
}

void AValdoriaGuardian::SetupSpot(uint8 Team, float Radius)
{
	GuardTeam = Team;
	TeamId = Team;
	AreaRadius = Radius;
	StartLocation = GetActorLocation();
	SetLeash(GetActorLocation(), AreaRadius);
	if (HasAuthority())
	{
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetMaxHealthAttribute(), GuardianHealth);
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetHealthAttribute(), GuardianHealth);
	}
}
void AValdoriaGuardian::StrikeRiders()
{
	// Enemies riding a trail over this area get brought down (owner's design, per guardian style).
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < NextRiderStrike)
	{
		return;
	}
	for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
	{
		AValdoriaHeroCharacter* Rider = *It;
		if (!Rider->IsRiding() || !IsEnemy(Rider) || FVector::Dist2D(Rider->GetActorLocation(), StartLocation) > AreaRadius)
		{
			continue;
		}
		NextRiderStrike = Now + 2.5f;
		AValdoriaFx::Beam(GetWorld(), GetActorLocation() + FVector(0, 0, 80.f), Rider->GetActorLocation(), FLinearColor(1.f, 0.3f, 0.2f), 8.f, 0.3f);
		Rider->StopRiding();
		DealTypedDamage(Rider, 40.f, Style == EValdoriaGuardianStyle::Mage ? EValdoriaDamageType::Magical : EValdoriaDamageType::Physical);
		switch (Style)
		{
		case EValdoriaGuardianStyle::Melee:
			LaunchCharacter(FVector(0.f, 0.f, 900.f), false, true); // quick charged leap
			Rider->LaunchCharacter(FVector(0.f, 0.f, -1500.f), true, true);
			Rider->ApplyStatus(EValdoriaStatus::Stunned, 0.8f);
			break;
		case EValdoriaGuardianStyle::Mage:
			Rider->LaunchCharacter((GetActorLocation() - Rider->GetActorLocation()).GetSafeNormal() * 1400.f, true, true);
			Rider->ApplyStatus(EValdoriaStatus::Rooted, 1.5f);
			break;
		case EValdoriaGuardianStyle::Flyer:
		case EValdoriaGuardianStyle::Thrower:
			Rider->LaunchCharacter(FVector(0.f, 0.f, -1800.f), true, true);
			Rider->ApplyStatus(EValdoriaStatus::Stunned, 1.2f);
			break;
		}
		ValdoriaAudio::PlayAt(this, ValdoriaAudio::Load(TEXT("Impacts"), TEXT("impactPunch_heavy_000")), Rider->GetActorLocation(), 0.9f);
		return;
	}
}
void AValdoriaGuardian::TickMage()
{
	const float Now = GetWorld()->GetTimeSeconds();
	// Area heal: every ally (the other guardian included) inside the area.
	if (Now >= NextMageHeal)
	{
		bool bAnyHurt = false;
		for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
		{
			if (It->TeamId == GuardTeam && !It->IsDead() && It->GetHealth() < It->GetMaxHealth()
				&& FVector::Dist2D(It->GetActorLocation(), StartLocation) <= AreaRadius)
			{
				bAnyHurt = true;
				HealTarget(*It, MageHeal); // healing items/anti-heal apply here too
			}
		}
		if (bAnyHurt)
		{
			NextMageHeal = Now + MageHealCooldown;
			PlayAreaEffect(StartLocation, AreaRadius, FLinearColor(0.4f, 1.f, 0.6f));
			ValdoriaAudio::PlayAt(this, ValdoriaAudio::Load(TEXT("Magic"), TEXT("heal")), GetActorLocation(), 0.9f);
		}
	}
	// Empower: the ally with the lowest health fraction in the area.
	if (Now >= NextMageEmpower)
	{
		AValdoriaHeroCharacter* Weakest = nullptr;
		float Lowest = 0.95f;
		for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
		{
			if (It->TeamId != GuardTeam || It->IsDead() || FVector::Dist2D(It->GetActorLocation(), StartLocation) > AreaRadius)
			{
				continue;
			}
			const float Frac = It->GetHealth() / FMath::Max(1.f, It->GetMaxHealth());
			if (Frac < Lowest)
			{
				Lowest = Frac;
				Weakest = *It;
			}
		}
		if (Weakest)
		{
			NextMageEmpower = Now + MageEmpowerCooldown;
			// Alternates: damage reduction, then damage reflection.
			if (EmpowerCount++ % 2 == 0)
			{
				Weakest->ApplyTeamBuff(0, MageEmpowerReduction, 4.f);
			}
			else
			{
				Weakest->StartReflect(2.f);
			}
			AValdoriaFx::Beam(GetWorld(), GetActorLocation() + FVector(0, 0, 90.f), Weakest->GetActorLocation(), FLinearColor(1.f, 0.85f, 0.4f), 6.f, 0.4f);
			ValdoriaAudio::PlayAt(this, ValdoriaAudio::Load(TEXT("Magic"), TEXT("blessing")), Weakest->GetActorLocation(), 0.8f);
		}
	}
}
// ------------------------------------------------------------------ sealing ritual (owner's lore)
// A defeated spot guardian does not die: it becomes semi-sealed (no health, helpless). An enemy
// hero out of passive combat holds the seal (hand out, Latin incantation) for SealSeconds and the
// guardian is sealed for good (rage roar). Its own team can undo it with the reverse incantation
// (RevertSeconds): it comes back with a little health and regenerates up to half while out of
// combat; if it falls a second time it fades away for good (roar of pain). Either way the final
// guardian loses the share of power this spot gave it. Roars and shockwaves are cosmetic only.

void AValdoriaGuardian::HandleDeath(AValdoriaHeroCharacter* Killer)
{
	if (bFinalGuardian || State != EValdoriaSpotState::Normal || bRevived)
	{
		// The final guardian, or a guardian that was already brought back once: gone for good.
		if (State == EValdoriaSpotState::Normal && bRevived)
		{
			Roar(false);
		}
		State = bFinalGuardian ? State : EValdoriaSpotState::Gone;
		Super::HandleDeath(Killer);
		return;
	}
	// First defeat: semi-sealed. Helpless and untouchable until someone seals or frees it.
	State = EValdoriaSpotState::SemiSealed;
	SemiSealedSince = GetWorld()->GetTimeSeconds();
	bGodMode = true;
	SetAutopilot(false);
	ApplyStatus(EValdoriaStatus::Stunned, 100000.f);
	PlayAreaEffect(GetActorLocation(), 300.f, FLinearColor(0.7f, 0.7f, 1.f));
}

bool AValdoriaGuardian::CanBeChanneledBy(const AValdoriaHeroCharacter* Hero) const
{
	return State == EValdoriaSpotState::SemiSealed && Hero && !Hero->IsDead();
}

void AValdoriaGuardian::CompleteSeal(AValdoriaHeroCharacter* By)
{
	if (State != EValdoriaSpotState::SemiSealed)
	{
		return;
	}
	State = EValdoriaSpotState::Sealed;
	bGodMode = false;
	Roar(true);
	// Seal mark: a red circle on the ground where the guardian stood.
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AValdoriaZone* Mark = GetWorld()->SpawnActor<AValdoriaZone>(AValdoriaZone::StaticClass(), StartLocation - FVector(0, 0, 100.f), FRotator::ZeroRotator, P))
	{
		Mark->Setup(this, 220.f, 100000.f, 0.f, 0.f, EValdoriaDamageType::Magical, 0.f, FLinearColor(0.9f, 0.1f, 0.1f));
	}
	Super::HandleDeath(By);
}

void AValdoriaGuardian::CompleteRevert(AValdoriaHeroCharacter* By)
{
	if (State != EValdoriaSpotState::SemiSealed)
	{
		return;
	}
	State = EValdoriaSpotState::Normal;
	bRevived = true;
	bGodMode = false;
	StatusEndTimes.Reset();
	AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetHealthAttribute(), GetMaxHealth() * RevertHealthFraction);
	SetAutopilot(true, 0.6f);
	PlayAreaEffect(GetActorLocation(), 400.f, FLinearColor(0.5f, 1.f, 0.7f));
}

void AValdoriaGuardian::Roar(bool bRage)
{
	// Heard across the whole map (2D) and seen as a big shockwave; purely cosmetic.
	AValdoriaFx::Ring(GetWorld(), GetActorLocation(), 2500.f, bRage ? FLinearColor(1.f, 0.2f, 0.1f) : FLinearColor(0.6f, 0.6f, 1.f), 1.2f);
	ValdoriaAudio::Play2D(this, ValdoriaAudio::Load(TEXT("SciFi"), bRage ? TEXT("explosionCrunch_000") : TEXT("lowFrequency_explosion_000")),
		1.f, 0.f, bRage ? 0.7f : 0.5f);
}

bool AValdoriaGuardian::IsLinkedToFinal() const
{
	return !bFinalGuardian && (State == EValdoriaSpotState::Normal || State == EValdoriaSpotState::SemiSealed);
}

void AValdoriaGuardian::TickSpotState(float DeltaSeconds)
{
	const float Now = GetWorld()->GetTimeSeconds();
	// Brought back: regenerates up to RevivedRegenCap of its health while out of passive combat.
	if (State == EValdoriaSpotState::Normal && bRevived && !IsInPassiveCombat(3.f) && GetHealth() < GetMaxHealth() * RevivedRegenCap)
	{
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetHealthAttribute(),
			FMath::Min(GetMaxHealth() * RevivedRegenCap, GetHealth() + GetMaxHealth() * 0.01f * DeltaSeconds));
	}
	// Optional (my suggestion, off by default): a semi-sealed guardian frees itself after a while.
	if (State == EValdoriaSpotState::SemiSealed && SemiSealTimeout > 0.f && Now - SemiSealedSince > SemiSealTimeout)
	{
		CompleteRevert(nullptr);
	}
	// Final guardian: stronger for every spot guardian of its team still linked to it.
	if (bFinalGuardian && HasAuthority() && !IsDead())
	{
		int32 Linked = 0;
		for (TActorIterator<AValdoriaGuardian> It(GetWorld()); It; ++It)
		{
			Linked += (It->GuardTeam == GuardTeam && It->IsLinkedToFinal()) ? 1 : 0;
		}
		if (Linked != LinkedSpots)
		{
			LinkedSpots = Linked;
			const float NewMax = GuardianHealth + PerSpotHealth * Linked;
			const float Frac = GetHealth() / FMath::Max(1.f, GetMaxHealth());
			AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetMaxHealthAttribute(), NewMax);
			AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetHealthAttribute(), NewMax * Frac);
			AdminSetDamageMultiplier(FinalBaseDamage * (1.f + PerSpotDamage * Linked));
		}
	}
}
