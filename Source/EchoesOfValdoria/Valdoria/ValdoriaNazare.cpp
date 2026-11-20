// Echoes of Valdoria - Filho de Nazaré "O Prometido" (turn 48). See ValdoriaNazare.h.

#include "ValdoriaNazare.h"
#include "ValdoriaHeroCharacter.h"
#include "ValdoriaAttributeSet.h"
#include "ValdoriaAudio.h"
#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

#define VALDORIA_BEGIN() \
	AValdoriaHeroCharacter* Hero = GetHero(); \
	if (!Hero || !CommitAbility(Handle, ActorInfo, ActivationInfo)) \
	{ \
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); \
		return; \
	}
#define VALDORIA_END() EndAbility(Handle, ActorInfo, ActivationInfo, true, false)

namespace
{
	const FLinearColor Gold(1.f, 0.88f, 0.55f);

	/** Allied hero units around a spot (optionally including the caster). */
	TArray<AValdoriaHeroCharacter*> AlliesAround(AValdoriaHeroCharacter* Hero, const FVector& Where, float Radius, bool bIncludeSelf)
	{
		TArray<AValdoriaHeroCharacter*> Out;
		for (TActorIterator<AValdoriaHeroCharacter> It(Hero->GetWorld()); It; ++It)
		{
			if (!It->IsDead() && It->TeamId == Hero->TeamId && (bIncludeSelf || *It != Hero) && !It->IsGuardianUnit()
				&& FVector::Dist(It->GetActorLocation(), Where) <= Radius)
			{
				Out.Add(*It);
			}
		}
		return Out;
	}

	/** Real (scaled) HP -> design units for HealTarget. */
	float ToDesign(float RealHP) { return RealHP / ValdoriaStatScale; }
}

// ------------------------------------------------------------------ Acalmar a Tempestade (RMB)

UValdoriaGA_CalmStorm::UValdoriaGA_CalmStorm()
{
	DefaultCooldown = 12.f;
	DefaultRadius = 700.f;
	DefaultDuration = 1.5f; // silence
}

void UValdoriaGA_CalmStorm::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	const FVector Center = Hero->GetActorLocation();
	const float Radius = GetRadius();
	for (TActorIterator<AValdoriaHeroCharacter> It(Hero->GetWorld()); It; ++It)
	{
		AValdoriaHeroCharacter* T = *It;
		if (T->IsDead() || T->IsGuardianUnit() || FVector::Dist(T->GetActorLocation(), Center) > Radius)
		{
			continue;
		}
		if (T->TeamId == Hero->TeamId)
		{
			T->Cleanse();
		}
		else if (Hero->IsEnemy(T))
		{
			T->ApplyStatus(EValdoriaStatus::Silenced, GetDuration());
		}
	}
	Hero->PlayAreaEffect(Center, Radius, FLinearColor(0.6f, 0.85f, 1.f));
	ValdoriaAudio::PlayAt(Hero, ValdoriaAudio::Load(TEXT("SciFi"), TEXT("forceField_000")), Center, 0.7f);
	VALDORIA_END();
}

// ------------------------------------------------------------------ Cálice e Carne (1)

UValdoriaGA_Chalice::UValdoriaGA_Chalice()
{
	DefaultCooldown = 10.f;
	DefaultRadius = 1200.f;   // range to start; the link breaks at 1.5x
	DefaultMagnitude = 0.01f; // fraction of his max HP every 0.25 s
	DefaultDuration = 7.f;    // Ungido de Sangue
}

void UValdoriaGA_Chalice::StopChannel(AValdoriaHeroCharacter* Hero, float Cooldown)
{
	if (!Hero)
	{
		return;
	}
	Hero->GetWorld()->GetTimerManager().ClearTimer(Hero->ChaliceTimer);
	if (Hero->ChaliceTarget.IsValid() && Cooldown > 0.f)
	{
		Hero->StartCooldown(EValdoriaAbilitySlot::PrimaryFire, Cooldown);
	}
	Hero->ChaliceTarget.Reset();
}

void UValdoriaGA_Chalice::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	// Turn 48 (owner): left mouse button, works while held. Aim at an ally to link; the link then follows that ally
	// and ignores enemies or other allies who pass in front. Releasing the button ends it.
	if (Hero->ChaliceTarget.IsValid())
	{
		VALDORIA_END(); // already linked (the automatic fire calls this every frame while held)
		return;
	}
	FVector Start, Dir;
	Hero->GetAimRay(Start, Dir);
	AValdoriaHeroCharacter* Best = nullptr;
	float BestDot = 0.96f; // must be looking at the ally (about 16 degrees)
	for (AValdoriaHeroCharacter* A : AlliesAround(Hero, Hero->GetActorLocation(), GetRadius(), false))
	{
		if (!A->IsHeroUnit())
		{
			continue;
		}
		const float Dot = FVector::DotProduct(Dir, (A->GetActorLocation() - Start).GetSafeNormal());
		if (Dot > BestDot)
		{
			BestDot = Dot;
			Best = A;
		}
	}
	if (!Best)
	{
		VALDORIA_END(); // nobody under the crosshair: nothing happens, no cost
		return;
	}
	Hero->ChaliceTarget = Best;
	const float Fraction = GetMagnitude();
	const float Buff = GetDuration();
	const float BreakRange = GetRadius() * 1.5f;
	const float Cooldown = GetEffectiveCooldown();
	TWeakObjectPtr<AValdoriaHeroCharacter> WeakHero = Hero;
	Hero->GetWorld()->GetTimerManager().SetTimer(Hero->ChaliceTimer, FTimerDelegate::CreateWeakLambda(Hero, [WeakHero, Fraction, Buff, BreakRange, Cooldown]()
	{
		AValdoriaHeroCharacter* H = WeakHero.Get();
		if (!H)
		{
			return;
		}
		AValdoriaHeroCharacter* Ally = H->ChaliceTarget.Get();
		const float Cost = H->GetMaxHealth() * Fraction;
		if (!H->IsFireHeld() || !Ally || Ally->IsDead() || H->IsDead() || H->HasStatus(EValdoriaStatus::Stunned)
			|| FVector::Dist(Ally->GetActorLocation(), H->GetActorLocation()) > BreakRange
			|| H->GetHealth() - Cost < H->GetMaxHealth() * 0.01f) // he never kills himself (stops at 1%)
		{
			StopChannel(H, Cooldown);
			return;
		}
		H->PlayShotEffects(Ally->GetActorLocation() + FVector(0.f, 0.f, 40.f), true); // the red link to the ally
		if (Ally->GetHealth() >= Ally->GetMaxHealth() - 1.f)
		{
			return; // owner: an ally at full health costs nothing (the link waits; Ungido is not renewed)
		}
		H->GetAbilitySystemComponent()->SetNumericAttributeBase(UValdoriaAttributeSet::GetHealthAttribute(), H->GetHealth() - Cost);
		H->HealTarget(Ally, ToDesign(Cost));
		Ally->UngidoUntil = H->GetWorld()->GetTimeSeconds() + Buff; // Ungido de Sangue, renewed while fed
		Ally->ApplySpeedBuff(0.05f, Buff);
		Ally->SetStack(H, TEXT("ungido"), TEXT("U"), FLinearColor(0.8f, 0.1f, 0.15f), 1, Buff);
	}), 0.25f, true);
	ValdoriaAudio::PlayAt(Hero, ValdoriaAudio::Load(TEXT("SciFi"), TEXT("forceField_000")), Hero->GetActorLocation(), 0.4f);
	VALDORIA_END();
}

// ------------------------------------------------------------------ Multiplicação dos Pães (2)

UValdoriaGA_Baskets::UValdoriaGA_Baskets()
{
	DefaultCooldown = 14.f;
	DefaultMagnitude = 60.f;  // heal per basket (design units)
	DefaultDuration = 4.f;    // heal over this many seconds
	DefaultSecondary = 3.f;   // baskets
	DefaultRadius = 250.f;    // spread
}

void UValdoriaGA_Baskets::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	const int32 Count = FMath::Clamp(FMath::RoundToInt(GetSecondary()), 1, 8);
	const FVector Fwd = FRotator(0.f, Hero->GetActorRotation().Yaw, 0.f).Vector();
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 i = 0; i < Count; ++i)
	{
		const float Yaw = (i - (Count - 1) * 0.5f) * 40.f;
		FVector Spot = Hero->GetActorLocation() + Fwd.RotateAngleAxis(Yaw, FVector::UpVector) * GetRadius();
		FHitResult Down;
		FCollisionQueryParams Q(SCENE_QUERY_STAT(ValdoriaBasket), false, Hero);
		if (Hero->GetWorld()->LineTraceSingleByChannel(Down, Spot + FVector(0, 0, 100.f), Spot - FVector(0, 0, 400.f), ECC_WorldStatic, Q))
		{
			Spot = Down.ImpactPoint;
		}
		if (AValdoriaBasket* B = Hero->GetWorld()->SpawnActor<AValdoriaBasket>(AValdoriaBasket::StaticClass(), Spot, FRotator::ZeroRotator, P))
		{
			B->Setup(Hero, GetMagnitude(), GetDuration());
		}
	}
	VALDORIA_END();
}

AValdoriaBasket::AValdoriaBasket()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cyl.Succeeded())
	{
		Mesh->SetStaticMesh(Cyl.Object);
	}
	Mesh->SetRelativeScale3D(FVector(0.45f, 0.45f, 0.25f)); // a low round basket (placeholder look)
	InitialLifeSpan = 15.f;
}

void AValdoriaBasket::Setup(AValdoriaHeroCharacter* InOwner, float InHealTotal, float InHealSeconds)
{
	Giver = InOwner;
	HealTotal = InHealTotal;
	HealSeconds = FMath::Max(0.5f, InHealSeconds);
	AddActorWorldOffset(FVector(0, 0, 12.f));
}

void AValdoriaBasket::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AValdoriaHeroCharacter* G = Giver.Get();
	if (!G || !HasAuthority())
	{
		return;
	}
	for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
	{
		AValdoriaHeroCharacter* A = *It;
		if (A->IsDead() || A->TeamId != G->TeamId || !A->IsHeroUnit() || FVector::Dist2D(A->GetActorLocation(), GetActorLocation()) > 120.f
			|| FMath::Abs(A->GetActorLocation().Z - GetActorLocation().Z) > 200.f)
		{
			continue;
		}
		// Heal over time: HealTotal split in 0.5 s ticks.
		const int32 Ticks = FMath::Max(1, FMath::RoundToInt(HealSeconds / 0.5f));
		const float PerTick = HealTotal / Ticks;
		TWeakObjectPtr<AValdoriaHeroCharacter> WeakG = G;
		TWeakObjectPtr<AValdoriaHeroCharacter> WeakA = A;
		TSharedRef<int32> Left = MakeShared<int32>(Ticks);
		TSharedRef<FTimerHandle> T = MakeShared<FTimerHandle>();
		GetWorld()->GetTimerManager().SetTimer(*T, FTimerDelegate::CreateWeakLambda(G, [WeakG, WeakA, Left, T, PerTick]()
		{
			AValdoriaHeroCharacter* GG = WeakG.Get();
			AValdoriaHeroCharacter* AA = WeakA.Get();
			if (!GG || !AA || AA->IsDead() || --(*Left) < 0)
			{
				if (GG)
				{
					GG->GetWorld()->GetTimerManager().ClearTimer(*T);
				}
				return;
			}
			GG->HealTarget(AA, PerTick);
		}), 0.5f, true, 0.01f);
		G->PlayAreaEffect(GetActorLocation(), 150.f, Gold);
		Destroy();
		return;
	}
}

// ------------------------------------------------------------------ Caminhar sobre as Águas (3)

UValdoriaGA_WalkOnWater::UValdoriaGA_WalkOnWater()
{
	DefaultCooldown = 12.f;
	DefaultRadius = 900.f;    // dash length
	DefaultDuration = 2.5f;   // slow immunity and protection
	DefaultSecondary = 0.25f; // damage reduction
}

void UValdoriaGA_WalkOnWater::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	UWorld* World = Hero->GetWorld();
	const FVector Fwd = FRotator(0.f, Hero->GetControlRotation().Yaw, 0.f).Vector();
	// Glides over gaps: a long flat launch (low gravity for the duration of the dash).
	UCharacterMovementComponent* Move = Hero->GetCharacterMovement();
	const float OldGravity = FMath::IsNearlyEqual(Move->GravityScale, 0.15f) ? 1.f : Move->GravityScale;
	Move->GravityScale = 0.15f;
	Hero->LaunchCharacter(Fwd * (GetRadius() / 0.45f) + FVector(0.f, 0.f, 150.f), true, true);
	TWeakObjectPtr<AValdoriaHeroCharacter> WeakHero = Hero;
	FTimerHandle Restore;
	World->GetTimerManager().SetTimer(Restore, FTimerDelegate::CreateWeakLambda(Hero, [WeakHero, OldGravity]()
	{
		if (AValdoriaHeroCharacter* H = WeakHero.Get())
		{
			H->GetCharacterMovement()->GravityScale = OldGravity;
		}
	}), 0.5f, false);
	Hero->Cleanse(); // shakes off slows (and control) when he starts walking
	Hero->SlowImmuneUntil = World->GetTimeSeconds() + GetDuration();
	Hero->BuffReduction = GetSecondary();
	Hero->BuffReductionUntil = World->GetTimeSeconds() + GetDuration();
	Hero->PlayAreaEffect(Hero->GetActorLocation(), 250.f, FLinearColor(0.5f, 0.8f, 1.f));
	VALDORIA_END();
}

// ------------------------------------------------------------------ Rogai por Nós (C)

UValdoriaGA_Pray::UValdoriaGA_Pray()
{
	DefaultCooldown = 30.f;
	DefaultDuration = 2.f;    // kneeling
	DefaultMagnitude = 0.25f; // fraction of his max HP recovered over the prayer
}

void UValdoriaGA_Pray::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	const float Seconds = FMath::Max(0.5f, GetDuration());
	Hero->ApplyStatus(EValdoriaStatus::Rooted, Seconds);
	Hero->ApplyStatus(EValdoriaStatus::CannotAttack, Seconds);
	const int32 Ticks = FMath::RoundToInt(Seconds / 0.25f);
	const float PerTick = ToDesign(Hero->GetMaxHealth() * GetMagnitude()) / Ticks;
	TWeakObjectPtr<AValdoriaHeroCharacter> WeakHero = Hero;
	TSharedRef<int32> Left = MakeShared<int32>(Ticks);
	TSharedRef<FTimerHandle> T = MakeShared<FTimerHandle>();
	Hero->GetWorld()->GetTimerManager().SetTimer(*T, FTimerDelegate::CreateWeakLambda(Hero, [WeakHero, Left, T, PerTick]()
	{
		AValdoriaHeroCharacter* H = WeakHero.Get();
		if (!H || H->IsDead() || --(*Left) < 0 || H->HasStatus(EValdoriaStatus::Stunned))
		{
			if (H)
			{
				H->GetWorld()->GetTimerManager().ClearTimer(*T);
			}
			return;
		}
		H->HealTarget(H, PerTick);
	}), 0.25f, true);
	Hero->PlayAreaEffect(Hero->GetActorLocation(), 200.f, Gold);
	VALDORIA_END();
}

// ------------------------------------------------------------------ Crucificado (4)

UValdoriaGA_Crucified::UValdoriaGA_Crucified()
{
	bIsUltimate = true;
	DefaultCooldown = 1.f;
	DefaultDuration = 3.5f;
	DefaultRadius = 2000.f;    // allies healed and protected
	DefaultMagnitude = 0.015f; // fraction of each ally's max HP every 0.5 s (double under 25%)
	DefaultSecondary = 0.05f;  // pulse every 2 s
}

void UValdoriaGA_Crucified::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	UWorld* World = Hero->GetWorld();
	const float Seconds = FMath::Clamp(GetDuration(), 0.5f, 3.5f);
	const float Now = World->GetTimeSeconds();
	Hero->Cleanse();
	Hero->ApplyStatus(EValdoriaStatus::Rooted, Seconds); // before the immunity, which would block it
	Hero->ApplyStatus(EValdoriaStatus::CannotAttack, Seconds);
	Hero->CCImmuneUntil = Now + Seconds;
	Hero->DamageImmuneUntil = Now + Seconds;
	// Rises into the sky, slowly (light and ascension, no gore).
	UCharacterMovementComponent* Move = Hero->GetCharacterMovement();
	const float OldFlySpeed = Move->MaxFlySpeed;
	Move->MaxFlySpeed = 160.f;
	Move->SetMovementMode(MOVE_Flying);
	Move->StopMovementImmediately();
	Move->Velocity = FVector(0.f, 0.f, 160.f);
	Hero->PlayAreaEffect(Hero->GetActorLocation(), GetRadius() * 0.5f, Gold);

	const float Radius = GetRadius();
	const float TickFrac = GetMagnitude();
	const float PulseFrac = GetSecondary();
	TWeakObjectPtr<AValdoriaHeroCharacter> WeakHero = Hero;
	TSharedRef<int32> Step = MakeShared<int32>(0);
	TSharedRef<FTimerHandle> T = MakeShared<FTimerHandle>();
	const int32 Steps = FMath::RoundToInt(Seconds / 0.5f);
	World->GetTimerManager().SetTimer(*T, FTimerDelegate::CreateWeakLambda(Hero, [WeakHero, Step, T, Steps, Radius, TickFrac, PulseFrac, OldFlySpeed]()
	{
		AValdoriaHeroCharacter* H = WeakHero.Get();
		if (!H)
		{
			return;
		}
		UWorld* W = H->GetWorld();
		++(*Step);
		H->GetCharacterMovement()->Velocity = FVector(0.f, 0.f, 160.f);
		const bool bPulse = (*Step % 4) == 0; // every 2 s
		for (AValdoriaHeroCharacter* A : AlliesAround(H, H->GetActorLocation() - FVector(0, 0, 80.f * *Step), Radius, false))
		{
			const float Frac = A->GetHealth() / FMath::Max(1.f, A->GetMaxHealth());
			float Heal = A->GetMaxHealth() * TickFrac * (Frac < 0.25f ? 2.f : 1.f);
			if (bPulse)
			{
				Heal += A->GetMaxHealth() * PulseFrac;
				A->Cleanse();
			}
			H->HealTarget(A, ToDesign(Heal));
			// Allies around only die to a hit bigger than 30% of their max health (see TrySurviveLethal).
			if (W->GetTimeSeconds() >= A->LastStandGuardUntil)
			{
				A->bLastStandUsed = false; // a new Crucificado: one saved hit again
			}
			A->LastStandGuardUntil = W->GetTimeSeconds() + 0.6f;
		}
		if (bPulse)
		{
			H->PlayAreaEffect(H->GetActorLocation(), Radius * 0.5f, Gold);
		}
		if (*Step >= Steps)
		{
			W->GetTimerManager().ClearTimer(*T);
			// The end: he gives himself up and comes back at the base 3 s later.
			H->GetCharacterMovement()->MaxFlySpeed = OldFlySpeed;
			H->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
			H->DamageImmuneUntil = 0.f;
			H->RespawnOverride = 3.f;
			if (!H->IsDead())
			{
				H->GetAbilitySystemComponent()->SetNumericAttributeBase(UValdoriaAttributeSet::GetHealthAttribute(), 0.f);
				H->HandleDeath(nullptr);
			}
		}
	}), 0.5f, true);
	ValdoriaAudio::PlayAt(Hero, ValdoriaAudio::Load(TEXT("SciFi"), TEXT("lowFrequency_explosion_000")), Hero->GetActorLocation(), 0.6f);
	VALDORIA_END();
}
