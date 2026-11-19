// Echoes of Valdoria - MOBA spots (guardian bosses).

#include "ValdoriaGuardian.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "ValdoriaAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "ValdoriaHeroDefinition.h"
#include "ValdoriaWorldActors.h"
#include "ValdoriaAudio.h"
#include "ValdoriaPlayerController.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

AValdoriaGuardian::AValdoriaGuardian()
{
	// Protection sphere and the energy link to the final guardian (shown while semi-sealed).
	ShieldSphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldSphere"));
	ShieldSphere->SetupAttachment(RootComponent);
	ShieldSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShieldSphere->SetCastShadow(false);
	ShieldSphere->SetRelativeScale3D(FVector(3.2f * 2.f)); // turn 45: guardians are twice as big
	ShieldSphere->SetVisibility(false);
	LinkLine = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LinkLine"));
	LinkLine->SetupAttachment(RootComponent);
	LinkLine->SetUsingAbsoluteLocation(true);
	LinkLine->SetUsingAbsoluteRotation(true);
	LinkLine->SetUsingAbsoluteScale(true);
	LinkLine->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LinkLine->SetCastShadow(false);
	LinkLine->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	ShieldSphere->SetStaticMesh(SphereMesh.Object);
	LinkLine->SetStaticMesh(CylMesh.Object);
	bHunter = true;
	BotDifficulty = 0; // fights with the autopilot, not fake shots
}

void AValdoriaGuardian::BeginPlay()
{
	Super::BeginPlay();
	TeamId = GuardTeam;
	// Big and tough: Brasa's kit (hammer, slam), larger body, lots of health.
	// Turn 45 (owner): twice the old size (was 1.5x Brasa, now 3x) so guardians read as bosses; the hitbox and the
	// hammer reach grow with the body. The definition is a private copy, safe to edit.
	if (UValdoriaHeroDefinition* Def = UValdoriaHeroDefinition::CreatePreset(this, TEXT("brasa")))
	{
		Def->MeshScale *= GuardianScale;
		Def->CapsuleRadius *= GuardianScale;
		Def->CapsuleHalfHeight *= GuardianScale;
		Def->Weapon.Range *= FMath::Sqrt(GuardianScale) * 1.4f;
		Def->DisplayName = NSLOCTEXT("Valdoria", "GuardianName", "Guardião");
		SetHeroDefinition(Def);
	}
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
	TickSemiSeal(DeltaSeconds);
	if (HasAuthority() && !IsDead() && State == EValdoriaSpotState::Normal)
	{
		StrikeRiders();
		if (bSupportMage || bInheritedMage)
		{
			TickMage();
		}
	}
	BuffTimer += DeltaSeconds;
	if (!HasAuthority() || IsDead() || State != EValdoriaSpotState::Normal || BuffTimer < 0.5f)
	{
		return;
	}
	BuffTimer = 0.f;
	if (bFinalGuardian)
	{
		TickInherited();
		return;
	}
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
		// A final guardian takes turns with the styles inherited from its linked spots.
		const EValdoriaGuardianStyle Use = (bFinalGuardian && InheritedStyles.Num() > 0) ? InheritedStyles[RiderStrikeCount++ % InheritedStyles.Num()] : Style;
		AValdoriaFx::Beam(GetWorld(), GetActorLocation() + FVector(0, 0, 80.f), Rider->GetActorLocation(), FLinearColor(1.f, 0.3f, 0.2f), 8.f, 0.3f);
		Rider->StopRiding();
		DealTypedDamage(Rider, 40.f, Use == EValdoriaGuardianStyle::Mage ? EValdoriaDamageType::Magical : EValdoriaDamageType::Physical);
		switch (Use)
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
void AValdoriaGuardian::TickInherited()
{
	// Final guardian: inherits the kit of every spot guardian of its team that is still standing
	// (owner's design). Rider-strike styles, the support mage's heal/empower, and a share of each
	// spot's buff on itself. Each part goes away as soon as that spot is semi-sealed, sealed or gone.
	InheritedStyles.Reset();
	bInheritedMage = false;
	if (!bInheritSpotKits)
	{
		return;
	}
	for (TActorIterator<AValdoriaGuardian> It(GetWorld()); It; ++It)
	{
		AValdoriaGuardian* Spot = *It;
		if (Spot == this || Spot->bFinalGuardian || Spot->GuardTeam != GuardTeam || Spot->State != EValdoriaSpotState::Normal || Spot->IsDead())
		{
			continue;
		}
		InheritedStyles.AddUnique(Spot->Style);
		bInheritedMage |= Spot->bSupportMage;
		if (InheritBuffShare > 0.f)
		{
			ApplyTeamBuff(static_cast<uint8>(Spot->Buff), Spot->GetCurrentBuff() * InheritBuffShare, 1.f);
		}
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
			if (Killer)
			{
				GiveTeamCredits(Killer->TeamId, CreditsGone, 0.f);
			}
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
	if (Killer)
	{
		GiveTeamCredits(Killer->TeamId, CreditsSemiSeal, 0.f);
	}
}

void AValdoriaGuardian::GiveTeamCredits(uint8 Team, float Near, float Far) const
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AValdoriaPlayerController* PC = Cast<AValdoriaPlayerController>(It->Get());
		const AValdoriaHeroCharacter* H = PC ? Cast<AValdoriaHeroCharacter>(PC->GetPawn()) : nullptr;
		if (!H || H->TeamId != Team)
		{
			continue;
		}
		const float Amount = FVector::Dist2D(H->GetActorLocation(), StartLocation) <= AreaRadius ? Near : Far;
		if (Amount > 0.f)
		{
			PC->AddCredits(Amount);
		}
	}
}

void AValdoriaGuardian::TickFinalGate()
{
	if (!IsFinalGated())
	{
		return;
	}
	bGodMode = true; // immune (hits show IMMUNE) but keeps fighting
	// Open when, in any lane of this team, every guardian of the last spot is sealed or gone.
	TMap<int32, int32> LastSpot;
	for (TActorIterator<AValdoriaGuardian> It(GetWorld()); It; ++It)
	{
		if (!It->bFinalGuardian && It->GuardTeam == GuardTeam)
		{
			int32& Max = LastSpot.FindOrAdd(It->LaneIndex, -1);
			Max = FMath::Max(Max, It->SpotIndex);
		}
	}
	for (const TPair<int32, int32>& Lane : LastSpot)
	{
		bool bBroken = true;
		for (TActorIterator<AValdoriaGuardian> It(GetWorld()); It && bBroken; ++It)
		{
			if (!It->bFinalGuardian && It->GuardTeam == GuardTeam && It->LaneIndex == Lane.Key && It->SpotIndex == Lane.Value)
			{
				bBroken = It->State == EValdoriaSpotState::Sealed || It->State == EValdoriaSpotState::Gone;
			}
		}
		if (bBroken)
		{
			bGateOpen = true;
			bGodMode = false;
			Roar(true); // the base is open: heard across the map
			UE_LOG(LogValdoria, Display, TEXT("MOBA guardiao final do time %d ficou vulneravel (lane %d quebrada)"), GuardTeam, Lane.Key);
			return;
		}
	}
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
	if (By)
	{
		GiveTeamCredits(By->TeamId, CreditsSealNear, CreditsSealTeam);
	}
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
	GiveTeamCredits(GuardTeam, CreditsRevertNear, CreditsRevertTeam);
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
	if (State == EValdoriaSpotState::Normal && bRevived && !bNoRegen && !IsInPassiveCombat(3.f) && GetHealth() < GetMaxHealth() * RevivedRegenCap)
	{
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetHealthAttribute(),
			FMath::Min(GetMaxHealth() * RevivedRegenCap, GetHealth() + GetMaxHealth() * 0.01f * DeltaSeconds));
	}
	// Optional (my suggestion, off by default): a semi-sealed guardian frees itself after a while.
	if (State == EValdoriaSpotState::SemiSealed && SemiSealTimeout > 0.f && Now - SemiSealedSince > SemiSealTimeout)
	{
		SelfRevert();
	}
	// Final guardian: stronger for every spot guardian of its team still linked to it.
	if (bFinalGuardian && HasAuthority() && !IsDead())
	{
		TickFinalGate();
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

// ------------------------------------------------------------------ semi-seal: shared progress, timeout, visuals

bool AValdoriaGuardian::AddChannel(AValdoriaHeroCharacter* By, float Amount)
{
	if (State != EValdoriaSpotState::SemiSealed || !By)
	{
		return false;
	}
	// Several heroes of the same side channelling together add up (faster ritual).
	const bool bRevert = By->TeamId == GuardTeam;
	float& Progress = bRevert ? RevertProgress : SealProgress;
	Progress += Amount;
	LastChannelTime = GetWorld()->GetTimeSeconds();
	if (Progress >= 1.f)
	{
		SealProgress = RevertProgress = 0.f;
		bRevert ? CompleteRevert(By) : CompleteSeal(By);
		return true;
	}
	return false;
}

void AValdoriaGuardian::TickSemiSeal(float DeltaSeconds)
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastChannelTime > 0.5f)
	{
		SealProgress = RevertProgress = 0.f; // nobody channelling: the ritual starts over
	}
	// Recovering after a self-revert: immune for a short moment.
	if (State == EValdoriaSpotState::Normal && Now < RecoverUntil)
	{
		bGodMode = true;
	}
	else if (State == EValdoriaSpotState::Normal && bGodMode && Now >= RecoverUntil && RecoverUntil > 0.f)
	{
		bGodMode = false;
		RecoverUntil = 0.f;
		ApplyStatus(EValdoriaStatus::Stunned, 0.01f);
		StatusEndTimes.Reset();
		SetAutopilot(true, 0.6f);
	}

	// Protection sphere (~30% opaque). Viewer's enemies: amber -> bright red; allies: blue -> bright green.
	// Also shown on a gated (immune) final guardian, so everyone sees it cannot be hurt yet.
	const bool bShow = State == EValdoriaSpotState::SemiSealed || (State == EValdoriaSpotState::Normal && Now < RecoverUntil) || IsFinalGated();
	ShieldSphere->SetVisibility(bShow);
	LinkLine->SetVisibility(bShow && FinalLink.IsValid());
	if (!bShow)
	{
		return;
	}
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const AValdoriaHeroCharacter* Viewer = PC ? Cast<AValdoriaHeroCharacter>(PC->GetPawn()) : nullptr;
	const bool bAllyView = Viewer && Viewer->TeamId == GuardTeam;
	const float T = SemiSealTimeout > 0.f ? FMath::Clamp((Now - SemiSealedSince) / SemiSealTimeout, 0.f, 1.f) : 0.f;
	const FLinearColor Color = bAllyView
		? FMath::Lerp(FLinearColor(0.15f, 0.4f, 1.f), FLinearColor(0.2f, 1.f, 0.3f) * 2.f, T)
		: FMath::Lerp(FLinearColor(1.f, 0.65f, 0.1f), FLinearColor(1.f, 0.05f, 0.05f) * 2.f, T);
	if (!ShieldMaterial)
	{
		ShieldMaterial = ValdoriaMakeGlowMaterial(this, Color, 0.3f);
		ShieldSphere->SetMaterial(0, ShieldMaterial);
		LinkLine->SetMaterial(0, ValdoriaMakeGlowMaterial(this, FLinearColor(0.6f, 0.8f, 1.f), 0.35f));
	}
	if (ShieldMaterial)
	{
		ShieldMaterial->SetVectorParameterValue(TEXT("Cor"), Color * 0.35f);
	}
	// Line on the ground from the final guardian through every spot of this lane (base side first)
	// up to this semi-sealed one, with energy pulses travelling along it (cosmetic only).
	LinkLine->SetVisibility(false);
	if (AValdoriaGuardian* Final = FinalLink.Get())
	{
		TArray<AValdoriaGuardian*> Chain;
		for (TActorIterator<AValdoriaGuardian> It(GetWorld()); It; ++It)
		{
			if (*It != this && !It->bFinalGuardian && It->GuardTeam == GuardTeam && It->LaneIndex == LaneIndex && It->SpotIndex > SpotIndex)
			{
				Chain.Add(*It);
			}
		}
		Chain.Sort([](const AValdoriaGuardian& A, const AValdoriaGuardian& B) { return A.SpotIndex > B.SpotIndex; });
		TArray<FVector> Path = { Final->StartLocation - FVector(0, 0, 100.f) };
		for (AValdoriaGuardian* G : Chain)
		{
			Path.Add(G->StartLocation - FVector(0, 0, 100.f));
		}
		Path.Add(StartLocation - FVector(0, 0, 100.f));
		PulseTimer += DeltaSeconds;
		if (PulseTimer > 0.3f)
		{
			PulseTimer = 0.f;
			float Total = 0.f;
			for (int32 i = 1; i < Path.Num(); ++i)
			{
				AValdoriaFx::Beam(GetWorld(), Path[i - 1], Path[i], FLinearColor(0.6f, 0.8f, 1.f), 5.f, 0.35f);
				Total += FVector::Dist(Path[i - 1], Path[i]);
			}
			// Pulse: a small burst moving from the final guardian towards this one.
			PulsePhase = FMath::Fmod(PulsePhase + 0.06f, 1.f);
			float Want = PulsePhase * Total;
			for (int32 i = 1; i < Path.Num(); ++i)
			{
				const float Seg = FVector::Dist(Path[i - 1], Path[i]);
				if (Want <= Seg)
				{
					AValdoriaFx::Burst(GetWorld(), FMath::Lerp(Path[i - 1], Path[i], Want / FMath::Max(1.f, Seg)) + FVector(0, 0, 20.f), 70.f, Color, 0.35f);
					break;
				}
				Want -= Seg;
			}
		}
	}
}

void AValdoriaGuardian::SelfRevert()
{
	// Nobody sealed it in time: it comes back by itself with half its health (no further regen),
	// immune for a short recovery.
	State = EValdoriaSpotState::Normal;
	bRevived = true;
	bNoRegen = true;
	AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetHealthAttribute(), GetMaxHealth() * 0.5f);
	RecoverUntil = GetWorld()->GetTimeSeconds() + 2.5f;
	PlayAreaEffect(GetActorLocation(), 400.f, FLinearColor(0.5f, 1.f, 0.7f));
}