// Echoes of Valdoria - Heimdall's abilities (turn 47). See ValdoriaHeimdall.h.

#include "ValdoriaHeimdall.h"
#include "ValdoriaHeroCharacter.h"
#include "ValdoriaLaneRail.h"
#include "ValdoriaVision.h"
#include "ValdoriaAudio.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"

#define VALDORIA_BEGIN() \
	AValdoriaHeroCharacter* Hero = GetHero(); \
	if (!Hero || !CommitAbility(Handle, ActorInfo, ActivationInfo)) \
	{ \
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); \
		return; \
	}
#define VALDORIA_END() EndAbility(Handle, ActorInfo, ActivationInfo, true, false)

// ------------------------------------------------------------------ RevealPulse

UValdoriaGA_RevealPulse::UValdoriaGA_RevealPulse()
{
	DefaultCooldown = 16.f;
	DefaultRadius = 4000.f;
	DefaultDuration = 4.f;
}

void UValdoriaGA_RevealPulse::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	const float Radius = GetRadius(); // <= 0: the whole map
	if (UValdoriaVisionSubsystem* Vision = Hero->GetWorld()->GetSubsystem<UValdoriaVisionSubsystem>())
	{
		for (TActorIterator<AValdoriaHeroCharacter> It(Hero->GetWorld()); It; ++It)
		{
			if (Hero->IsEnemy(*It) && !It->IsDead() && It->TeamId != 255
				&& (Radius <= 0.f || FVector::Dist2D(It->GetActorLocation(), Hero->GetActorLocation()) <= Radius))
			{
				Vision->RevealUnit(*It, Hero->TeamId, GetDuration());
			}
		}
	}
	Hero->PlayAreaEffect(Hero->GetActorLocation(), Radius > 0.f ? FMath::Min(Radius, 1500.f) : 1500.f, GetThemeColor());
	VALDORIA_END();
}

// ------------------------------------------------------------------ BifrostCall

UValdoriaGA_BifrostCall::UValdoriaGA_BifrostCall()
{
	DefaultCooldown = 40.f;
	DefaultRadius = 1600.f; // charge length
	DefaultDuration = 3.f;  // helpless ride
	DefaultMagnitude = 40.f;
}

void UValdoriaGA_BifrostCall::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost(); // missing still costs the full cooldown (owner)
	const FVector Fwd = FRotator(0.f, Hero->GetControlRotation().Yaw, 0.f).Vector();
	const float ChargeTime = 0.55f;
	const float Speed = GetRadius() / ChargeTime;
	Hero->LaunchCharacter(Fwd * Speed + FVector(0.f, 0.f, 120.f), true, true);
	ValdoriaAudio::PlayAt(Hero, ValdoriaAudio::Load(TEXT("SciFi"), TEXT("forceField_000")), Hero->GetActorLocation(), 0.8f);
	const float Damage = GetMagnitude();
	const float RideSeconds = GetDuration();
	TWeakObjectPtr<AValdoriaHeroCharacter> WeakHero = Hero;
	TSharedRef<float> Elapsed = MakeShared<float>(0.f);
	TSharedRef<FTimerHandle> Timer = MakeShared<FTimerHandle>();
	Hero->GetWorld()->GetTimerManager().SetTimer(*Timer, FTimerDelegate::CreateWeakLambda(Hero, [WeakHero, Elapsed, Timer, Fwd, Damage, RideSeconds, ChargeTime]()
	{
		AValdoriaHeroCharacter* H = WeakHero.Get();
		*Elapsed += 0.05f;
		if (!H || H->IsDead() || *Elapsed > ChargeTime)
		{
			if (H)
			{
				H->GetWorld()->GetTimerManager().ClearTimer(*Timer);
			}
			return;
		}
		// First enemy HERO touched (minions are passed through).
		for (TActorIterator<AValdoriaHeroCharacter> It(H->GetWorld()); It; ++It)
		{
			AValdoriaHeroCharacter* Victim = *It;
			if (!H->IsEnemy(Victim) || Victim->IsDead() || Victim->IsMinionUnit() || Victim->IsGuardianUnit() || FVector::Dist(Victim->GetActorLocation(), H->GetActorLocation()) > 220.f)
			{
				continue;
			}
			H->GetWorld()->GetTimerManager().ClearTimer(*Timer);
			H->GetCharacterMovement()->StopMovementImmediately();
			H->DamageSourceLabel = TEXT("Chamado da Bifrost");
			H->DealTypedDamage(Victim, Damage, EValdoriaDamageType::Magical, 1.f, false);
			H->DamageSourceLabel.Reset();
			// Thrown up and away (not a teleport); the nearest Bifrost "calls" the victim at the top of the arc.
			Victim->ApplyStatus(EValdoriaStatus::Stunned, 0.8f);
			Victim->LaunchCharacter(Fwd * 700.f + FVector(0.f, 0.f, 1300.f), true, true);
			TWeakObjectPtr<AValdoriaHeroCharacter> WeakVictim = Victim;
			FTimerHandle Catch;
			H->GetWorld()->GetTimerManager().SetTimer(Catch, FTimerDelegate::CreateWeakLambda(Victim, [WeakVictim, RideSeconds]()
			{
				if (AValdoriaHeroCharacter* V = WeakVictim.Get(); V && !V->IsDead())
				{
					V->StartForcedRide(RideSeconds, 3500.f);
				}
			}), 0.7f, false);
			return;
		}
	}), 0.05f, true);
	VALDORIA_END();
}

// ------------------------------------------------------------------ Gjallarhorn

UValdoriaGA_Gjallarhorn::UValdoriaGA_Gjallarhorn()
{
	bIsUltimate = true;
	DefaultCooldown = 1.f;
	DefaultRadius = 450.f;   // area around Heimdall
	DefaultDuration = 1.5f;  // horn
	DefaultSecondary = 0.3f; // damage reduction while blowing
}

float UValdoriaGA_Gjallarhorn::RangeFrom(const UWorld* World, const FVector& Where)
{
	float Range = 4000.f;
	for (TActorIterator<AValdoriaLaneRail> It(World); It; ++It)
	{
		float Along = 0.f;
		const float D = It->DistanceToPoint(Where, Along);
		Range += 6000.f * FMath::Clamp(1.f - D / 3000.f, 0.f, 1.f);
	}
	return FMath::Min(Range, 30000.f);
}

void UValdoriaGA_Gjallarhorn::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	const float Horn = GetDuration();
	const float Area = GetRadius();
	UWorld* World = Hero->GetWorld();
	// Blowing the horn: control immune, less damage taken, slower, no attacks (owner).
	Hero->CCImmuneUntil = World->GetTimeSeconds() + Horn;
	Hero->BuffReduction = GetSecondary();
	Hero->BuffReductionUntil = World->GetTimeSeconds() + Horn;
	Hero->ApplySlow(0.4f, Horn);
	Hero->ApplyStatus(EValdoriaStatus::CannotAttack, Horn);
	Hero->PlayAreaEffect(Hero->GetActorLocation(), Area, FLinearColor(1.f, 0.85f, 0.4f));
	ValdoriaAudio::PlayAt(Hero, ValdoriaAudio::Load(TEXT("SciFi"), TEXT("lowFrequency_explosion_000")), Hero->GetActorLocation(), 1.f);
	TWeakObjectPtr<AValdoriaHeroCharacter> WeakHero = Hero;
	FTimerHandle Timer;
	World->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(Hero, [WeakHero, Area]()
	{
		AValdoriaHeroCharacter* H = WeakHero.Get();
		if (!H || H->IsDead())
		{
			return;
		}
		UWorld* W = H->GetWorld();
		const FVector From = H->GetActorLocation();
		// Landing spot: where the crosshair points, clamped to the range, snapped to a walkable street.
		FVector Start, Dir;
		H->GetAimRay(Start, Dir);
		const float Range = RangeFrom(W, From);
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ValdoriaHorn), false, H);
		FVector Target = W->LineTraceSingleByChannel(Hit, Start, Start + Dir * Range, ECC_WorldStatic, Params) ? Hit.ImpactPoint : Start + Dir * Range;
		if (FVector::Dist2D(Target, From) > Range)
		{
			Target = From + (Target - From).GetSafeNormal2D() * Range;
		}
		if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(W))
		{
			FNavLocation Snap;
			if (Nav->ProjectPointToNavigation(Target, Snap, FVector(1500.f, 1500.f, 3000.f)))
			{
				Target = Snap.Location; // inside a building -> nearest street
			}
		}
		// Everyone inside the area goes (allies, enemies, minions); never guardians or training targets.
		TArray<AValdoriaHeroCharacter*> Riders;
		for (TActorIterator<AValdoriaHeroCharacter> It(W); It; ++It)
		{
			if (!It->IsDead() && !It->IsGuardianUnit() && It->TeamId != 255 && FVector::Dist2D(It->GetActorLocation(), From) <= Area
				&& FMath::Abs(It->GetActorLocation().Z - From.Z) < 400.f)
			{
				Riders.Add(*It);
			}
		}
		for (int32 i = 0; i < Riders.Num(); ++i)
		{
			const FVector Offset = i == 0 ? FVector::ZeroVector : FRotator(0.f, 360.f * i / Riders.Num(), 0.f).Vector() * 160.f;
			const FVector Delta = Riders[i]->GetActorLocation() - From;
			Riders[i]->TeleportTo(Target + FVector(Delta.X * 0.3f, Delta.Y * 0.3f, 0.f) + Offset + FVector(0.f, 0.f, 120.f), Riders[i]->GetActorRotation());
		}
		// The rainbow bridge from where he stood to where they landed (a short-lived Bifrost arc).
		TArray<FVector> Arc;
		const float Peak = FMath::Clamp(FVector::Dist2D(From, Target) * 0.25f, 800.f, 4000.f);
		for (int32 i = 0; i <= 24; ++i)
		{
			const float T = i / 24.f;
			Arc.Add(FMath::Lerp(From, Target, T) + FVector(0.f, 0.f, 4.f * Peak * T * (1.f - T)));
		}
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AValdoriaLaneRail* Bridge = W->SpawnActor<AValdoriaLaneRail>(AValdoriaLaneRail::StaticClass(), From, FRotator::ZeroRotator, P))
		{
			Bridge->Setup(253, TEXT("Gjallarhorn"), Arc, FLinearColor::White); // team 253: nobody can mount it
			Bridge->SetLifeSpan(2.5f);
		}
		H->PlayAreaEffect(Target, Area, FLinearColor(1.f, 0.85f, 0.4f));
	}), Horn, false);
	VALDORIA_END();
}
