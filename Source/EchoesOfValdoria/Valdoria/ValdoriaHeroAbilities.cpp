// Echoes of Valdoria - biblioteca de habilidades reutilizáveis.

#include "ValdoriaHeroAbilities.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "ValdoriaHeroCharacter.h"
#include "ValdoriaHeroDefinition.h"
#include "ValdoriaWorldActors.h"
#include "Engine/StaticMesh.h"
#include "ValdoriaAudio.h"

// Começo padrão: confere, gasta recarga/ultimate. Sai da função se não puder usar.
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
	/** Ponto para onde o jogador está mirando (até Range). */
	FVector AimPoint(AValdoriaHeroCharacter* Hero, float Range, FHitResult* OutHit = nullptr, bool bWorldOnly = false)
	{
		FVector Start, Dir;
		Hero->GetAimRay(Start, Dir);
		const FVector End = Start + Dir * Range;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ValdoriaAim), true, Hero);
		Hero->IgnoreFriendlyBarriers(Params);
		FHitResult Hit;
		// bWorldOnly = only real walls/floors stop it (object type), never characters: the WorldStatic *channel* is
		// blocked by pawn capsules too, which cut every "passes through everyone in a line" ability at the first body
		// (turn 48, owner: none of them pierced).
		const bool bHit = bWorldOnly
			? Hero->GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, FCollisionObjectQueryParams(ECC_WorldStatic), Params)
			: Hero->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
		if (OutHit)
		{
			*OutHit = Hit;
			OutHit->bBlockingHit = bHit;
		}
		return bHit ? Hit.ImpactPoint : End;
	}

	/** De onde saem os projéteis (mão ou peito). */
	FVector Muzzle(AValdoriaHeroCharacter* Hero)
	{
		return Hero->GetActorLocation() + FVector(0.f, 0.f, 55.f) + Hero->GetActorForwardVector() * 60.f
			+ Hero->GetActorRightVector() * 20.f;
	}

	bool HasLineOfSight(AValdoriaHeroCharacter* From, AActor* To)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ValdoriaLOS), false, From);
		Params.AddIgnoredActor(To);
		From->IgnoreFriendlyBarriers(Params);
		FHitResult Hit;
		return !From->GetWorld()->LineTraceSingleByChannel(Hit, From->GetActorLocation() + FVector(0, 0, 50.f),
			To->GetActorLocation() + FVector(0, 0, 40.f), ECC_WorldStatic, Params);
	}

	/** Chama Fn para cada herói vivo no cone à frente. */
	template <typename F>
	void ForEachInCone(AValdoriaHeroCharacter* Hero, float Range, float HalfAngleDeg, F&& Fn)
	{
		FVector Start, Dir;
		Hero->GetAimRay(Start, Dir);
		Dir = Dir.GetSafeNormal2D().IsNearlyZero() ? Hero->GetActorForwardVector() : Dir.GetSafeNormal2D();
		const float CosHalf = FMath::Cos(FMath::DegreesToRadians(HalfAngleDeg));
		const FVector Origin = Hero->GetActorLocation();
		for (TActorIterator<AValdoriaHeroCharacter> It(Hero->GetWorld()); It; ++It)
		{
			AValdoriaHeroCharacter* Other = *It;
			if (Other == Hero || Other->IsDead())
			{
				continue;
			}
			const FVector To = Other->GetActorLocation() - Origin;
			const float Dist = To.Size();
			const float Reach = Range + Other->GetCapsuleComponent()->GetScaledCapsuleRadius();
			if (Dist > Reach || FMath::Abs(To.Z) > 250.f)
			{
				continue;
			}
			if (Dist > 80.f && FVector::DotProduct(To.GetSafeNormal2D(), Dir) < CosHalf)
			{
				continue;
			}
			Fn(Other);
		}
	}
}

// ================================================================ ataques principais

UValdoriaGA_FireMelee::UValdoriaGA_FireMelee() {}

void UValdoriaGA_FireMelee::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	const FValdoriaWeaponStats& Weapon = Hero->GetWeaponStats();
	Hero->StartCooldown(GetSlot(), 1.f / FMath::Max(0.1f, Weapon.FireRate));
	const float Range = FMath::Min(Weapon.Range, 1500.f);
	const FVector Fwd = Hero->GetActorForwardVector();
	Hero->PlayAreaEffect(Hero->GetActorLocation() + Fwd * Range * 0.55f, Range * 0.5f, GetThemeColor());
	if (Hero->HasAuthority())
	{
		ForEachInCone(Hero, Range, FMath::Max(10.f, Weapon.SpreadDegrees * 0.5f), [&](AValdoriaHeroCharacter* Other)
		{
			if (Hero->IsEnemy(Other))
			{
				ApplyDamageTo(Other, Weapon.Damage);
				ValdoriaAudio::PlayAt(Hero, ValdoriaAudio::Load(TEXT("Impacts"), FMath::RandBool() ? TEXT("impactPunch_medium_000") : TEXT("impactPunch_medium_001")), Other->GetActorLocation(), 0.7f);
			}
		});
	}
	VALDORIA_END();
}

UValdoriaGA_FireProjectile::UValdoriaGA_FireProjectile() {}

void UValdoriaGA_FireProjectile::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	const FValdoriaWeaponStats& Weapon = Hero->GetWeaponStats();
	Hero->StartCooldown(GetSlot(), 1.f / FMath::Max(0.1f, Weapon.FireRate));
	Hero->ConsumeAmmo();
	Hero->ApplyRecoil(Weapon.RecoilDegrees);

	const FVector Start = Muzzle(Hero);
	FVector AimStart, AimDir;
	Hero->GetAimRay(AimStart, AimDir);
	const FVector Target = AimPoint(Hero, 6000.f);
	FVector Dir = (Target - Start).GetSafeNormal();
	Dir = FMath::VRandCone(Dir, FMath::DegreesToRadians(Weapon.SpreadDegrees * Hero->GetSpreadMultiplier() * 0.5f));

	AValdoriaProjectile::FParams P;
	P.Damage = Weapon.Damage;
	P.DamageType = Hero->GetElementDamageType(Weapon.DamageType); // Kay: element of the moment
	P.MagicShare = Weapon.HybridMagicShare;
	P.Speed = Weapon.ProjectileSpeed;
	P.Gravity = Weapon.ProjectileGravity;
	P.SplashRadius = Weapon.SplashRadius;
	P.AllyHeal = Weapon.AllyHeal;
	P.Color = GetThemeColor();
	P.Size = Weapon.SplashRadius > 0.f ? 0.22f : 0.1f;
	AValdoriaProjectile::Launch(Hero, Start, Dir, P);
	VALDORIA_END();
}

UValdoriaGA_TelekineticDagger::UValdoriaGA_TelekineticDagger() {}

void UValdoriaGA_TelekineticDagger::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	const FValdoriaWeaponStats& Weapon = Hero->GetWeaponStats();
	Hero->StartCooldown(GetSlot(), 1.f / FMath::Max(0.1f, Weapon.FireRate));
	if (Hero->HasAuthority())
	{
		// Pull back every dagger already out (they hit enemies on the way back) ...
		for (TActorIterator<AValdoriaTelekinesisDagger> It(Hero->GetWorld()); It; ++It)
		{
			if (It->GetOwner() == Hero)
			{
				It->Recall();
			}
		}
		// ... and throw the next one at the same time.
		const FVector Start = Hero->GetActorLocation() + FVector(0.f, 0.f, 60.f) - Hero->GetActorForwardVector() * 20.f; // from the back
		const FVector Target = AimPoint(Hero, 6000.f);
		const FVector Dir = (Target - Start).GetSafeNormal();
		const UValdoriaHeroDefinition* Def = Hero->GetHeroDefinition();
		UStaticMesh* Mesh = Def && !Def->WeaponMesh.IsNull() ? Def->WeaponMesh.LoadSynchronous() : nullptr;
		AValdoriaTelekinesisDagger::Throw(Hero, Mesh, Start, Dir, Weapon.Damage, Weapon.Damage * 0.6f, Weapon.DamageType,
			FMath::Max(1500.f, Weapon.ProjectileSpeed), FMath::Max(800.f, Weapon.Range), GetThemeColor());
	}
	VALDORIA_END();
}

UValdoriaGA_TelekineticPush::UValdoriaGA_TelekineticPush()
{
	DefaultCooldown = 9.f;
	DefaultMagnitude = 20.f;
	DefaultRadius = 2000.f;
	DefaultSecondary = 1500.f;
}

void UValdoriaGA_TelekineticPush::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	FVector Start, Dir;
	Hero->GetAimRay(Start, Dir);
	const FVector End = Start + Dir * GetRadius();
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ValdoriaTelekinesis), false, Hero);
	Hero->IgnoreFriendlyBarriers(Params);
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	// Wide sweep: the hand grabs whoever is roughly under the crosshair.
	const bool bHit = Hero->GetWorld()->SweepSingleByObjectType(Hit, Start, End, FQuat::Identity, Objects, FCollisionShape::MakeSphere(70.f), Params);
	Hero->PlayAreaEffect(bHit ? Hit.ImpactPoint : End, 120.f, GetThemeColor());
	AValdoriaHeroCharacter* Target = bHit ? Cast<AValdoriaHeroCharacter>(Hit.GetActor()) : nullptr;
	if (Target && !Target->IsDead() && Hero->HasAuthority())
	{
		const FVector Away = (Target->GetActorLocation() - Hero->GetActorLocation()).GetSafeNormal2D();
		if (Hero->IsEnemy(Target))
		{
			ApplyDamageTo(Target, GetMagnitude());
			Target->LaunchCharacter(Away * GetSecondary() + FVector(0.f, 0.f, 350.f), true, true); // expelled
		}
		else
		{
			// Ally: pulled to you, stopping just in front of you.
			const float Dist = FVector::Dist2D(Target->GetActorLocation(), Hero->GetActorLocation());
			Target->LaunchCharacter(-Away * FMath::Min(GetSecondary(), Dist * 2.2f) + FVector(0.f, 0.f, 300.f), true, true);
		}
	}
	VALDORIA_END();
}

// ================================================================ defesa

UValdoriaGA_Barrier::UValdoriaGA_Barrier()
{
	DefaultCooldown = 10.f;
	DefaultMagnitude = 800.f;
	DefaultRadius = 450.f;
	DefaultDuration = 6.f;
}

void UValdoriaGA_Barrier::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	if (Hero->HasAuthority())
	{
		const FVector Fwd = Hero->GetActorForwardVector();
		const FVector Where = Hero->GetActorLocation() + Fwd * 190.f + FVector(0.f, 0.f, 40.f);
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AValdoriaBarrier* Barrier = Hero->GetWorld()->SpawnActor<AValdoriaBarrier>(AValdoriaBarrier::StaticClass(), Where, Fwd.Rotation(), SpawnParams))
		{
			Barrier->Setup(Hero, GetMagnitude(), GetRadius(), 260.f, GetDuration(), GetThemeColor(), true);
		}
	}
	VALDORIA_END();
}

// ================================================================ área

UValdoriaGA_ThrowZone::UValdoriaGA_ThrowZone()
{
	DefaultCooldown = 9.f;
	DefaultMagnitude = 40.f;
	DefaultRadius = 350.f;
	DefaultDuration = 4.f;
	DefaultSecondary = 20.f;
}

UValdoriaGA_ThrowHealingZone::UValdoriaGA_ThrowHealingZone()
{
	bHealingZone = true;
	ThrowSpeed = 1900.f;
	ThrowGravity = 1.f;
	DefaultCooldown = 12.f;
	DefaultSecondary = 30.f;
	DefaultDuration = 5.f;
	DefaultDamageType = EValdoriaDamageType::Magical;
}

void UValdoriaGA_ThrowZone::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	const FVector Start = Muzzle(Hero);
	const FVector Dir = (AimPoint(Hero, 4000.f) - Start).GetSafeNormal() + FVector(0.f, 0.f, ThrowGravity > 0.f ? 0.12f : 0.f);
	AValdoriaProjectile::FParams P;
	P.Damage = bHealingZone ? 0.f : GetMagnitude();
	P.AllyHeal = bHealingZone ? GetMagnitude() : 0.f;
	P.DamageType = GetDamageType();
	P.MagicShare = GetMagicShare();
	P.Speed = ThrowSpeed;
	P.Gravity = ThrowGravity;
	P.SplashRadius = GetRadius() * 0.6f;
	P.Size = 0.25f;
	P.Color = GetThemeColor();
	P.ZoneRadius = GetRadius();
	P.ZoneDuration = GetDuration();
	P.ZoneHealPerSecond = bHealingZone ? GetSecondary() : 0.f;
	P.ZoneDamagePerSecond = bHealingZone ? 0.f : GetSecondary();
	AValdoriaProjectile::Launch(Hero, Start, Dir.GetSafeNormal(), P);
	VALDORIA_END();
}

UValdoriaGA_GroundSlam::UValdoriaGA_GroundSlam()
{
	DefaultCooldown = 8.f;
	DefaultMagnitude = 50.f;
	DefaultRadius = 500.f;
	DefaultDuration = 2.f;
	DefaultSecondary = 0.4f;
}

void UValdoriaGA_GroundSlam::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	const FVector Center = Hero->GetActorLocation();
	Hero->PlayAreaEffect(Center, GetRadius(), GetThemeColor());
	if (Hero->HasAuthority())
	{
		for (TActorIterator<AValdoriaHeroCharacter> It(Hero->GetWorld()); It; ++It)
		{
			AValdoriaHeroCharacter* Other = *It;
			if (!Hero->IsEnemy(Other) || FVector::Dist(Other->GetActorLocation(), Center) > GetRadius())
			{
				continue;
			}
			ApplyDamageTo(Other, GetMagnitude());
			Other->ApplySlow(GetSecondary(), GetDuration());
			Other->LaunchCharacter(FVector(0.f, 0.f, 420.f), false, true);
		}
	}
	VALDORIA_END();
}

UValdoriaGA_LeapSlam::UValdoriaGA_LeapSlam()
{
	bIsUltimate = true;
	DefaultCooldown = 1.f;
	DefaultMagnitude = 200.f;
	DefaultRadius = 800.f;
}

void UValdoriaGA_LeapSlam::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	Hero->StartLeapSlam(GetMagnitude(), GetRadius(), GetDamageType());
	VALDORIA_END();
}

UValdoriaGA_ConeBlast::UValdoriaGA_ConeBlast()
{
	DefaultCooldown = 7.f;
	DefaultMagnitude = 60.f;
	DefaultRadius = 700.f;
}

void UValdoriaGA_ConeBlast::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	const FVector Fwd = Hero->GetActorForwardVector();
	Hero->PlayAreaEffect(Hero->GetActorLocation() + Fwd * GetRadius() * 0.5f, GetRadius() * 0.5f, GetThemeColor());
	if (Hero->HasAuthority())
	{
		const float Heal = GetSecondary();
		const float SlowTime = GetDuration();
		ForEachInCone(Hero, GetRadius(), HalfAngleDegrees, [&](AValdoriaHeroCharacter* Other)
		{
			if (Hero->IsEnemy(Other))
			{
				ApplyDamageTo(Other, GetMagnitude());
				const FVector Push = (Other->GetActorLocation() - Hero->GetActorLocation()).GetSafeNormal2D() * PushForce + FVector(0, 0, 250.f);
				Other->LaunchCharacter(Push, true, true);
				if (SlowTime > 0.f)
				{
					Other->ApplySlow(0.35f, SlowTime);
				}
			}
			else if (Heal > 0.f && Other->TeamId == Hero->TeamId)
			{
				Hero->HealTarget(Other, Heal);
			}
		});
	}
	VALDORIA_END();
}

UValdoriaGA_Whirlpool::UValdoriaGA_Whirlpool()
{
	bIsUltimate = true;
	DefaultCooldown = 1.f;
	DefaultMagnitude = 50.f;
	DefaultRadius = 900.f;
	DefaultDuration = 6.f;
	DefaultSecondary = 15.f;
	DefaultDamageType = EValdoriaDamageType::Magical;
}

void UValdoriaGA_Whirlpool::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	if (Hero->HasAuthority())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AValdoriaZone* Zone = Hero->GetWorld()->SpawnActor<AValdoriaZone>(AValdoriaZone::StaticClass(),
			Hero->GetActorLocation() - FVector(0, 0, 90.f), FRotator::ZeroRotator, SpawnParams))
		{
			Zone->Setup(Hero, GetRadius(), GetDuration(), GetMagnitude(), GetSecondary(), GetDamageType(), 0.3f, GetThemeColor(), true);
		}
	}
	VALDORIA_END();
}

// ================================================================ tiros especiais

UValdoriaGA_PiercingBolt::UValdoriaGA_PiercingBolt()
{
	DefaultCooldown = 8.f;
	DefaultMagnitude = 70.f;
	DefaultRadius = 3000.f;
	DefaultSecondary = 60.f;
	DefaultDamageType = EValdoriaDamageType::Magical;
}

void UValdoriaGA_PiercingBolt::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	FHitResult WallHit;
	const FVector End = AimPoint(Hero, GetRadius(), &WallHit, true);
	FVector Start, Dir;
	Hero->GetAimRay(Start, Dir);
	// The line starts level with the hero, not at the camera behind him (so nobody behind him is hit).
	Start += Dir * FMath::Max(0.f, FVector::DotProduct(Hero->GetActorLocation() - Start, Dir));
	Hero->PlayShotEffects(End, false);
	if (Hero->HasAuthority())
	{
		TArray<FHitResult> Hits;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ValdoriaPierce), false, Hero);
		Hero->GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn),
			FCollisionShape::MakeSphere(45.f), Params);
		TSet<AActor*> Done;
		for (const FHitResult& Hit : Hits)
		{
			AValdoriaHeroCharacter* Other = Cast<AValdoriaHeroCharacter>(Hit.GetActor());
			if (!Other || Done.Contains(Other) || Other->IsDead())
			{
				continue;
			}
			Done.Add(Other);
			if (Hero->IsEnemy(Other))
			{
				ApplyDamageTo(Other, GetMagnitude());
			}
			else if (Other->TeamId == Hero->TeamId)
			{
				Hero->HealTarget(Other, GetSecondary());
			}
		}
	}
	VALDORIA_END();
}

UValdoriaGA_PrecisionMode::UValdoriaGA_PrecisionMode()
{
	bIsUltimate = true;
	DefaultCooldown = 1.f;
	DefaultMagnitude = 110.f;
	DefaultDuration = 8.f;
	DefaultSecondary = 6.f;
}

void UValdoriaGA_PrecisionMode::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	FValdoriaWeaponStats Sniper = Hero->GetWeaponStats();
	Sniper.WeaponName = NSLOCTEXT("Valdoria", "PrecisionWeapon", "Modo de precisão");
	Sniper.Damage = GetMagnitude();
	Sniper.HeadshotMultiplier = 2.5f;
	Sniper.FireRate = 1.25f;
	Sniper.bAutomatic = false;
	Sniper.bUsesAmmo = true;
	Sniper.MagazineSize = FMath::Max(1, FMath::RoundToInt(GetSecondary()));
	Sniper.SpreadDegrees = 0.f;
	Sniper.Range = 25000.f;
	Sniper.FalloffStart = Sniper.FalloffEnd = 0.f;
	Sniper.RecoilDegrees = 3.5f;
	Hero->SetWeaponOverride(Sniper, GetDuration());
	Hero->PlayAreaEffect(Hero->GetActorLocation(), 200.f, GetThemeColor());
	VALDORIA_END();
}

UValdoriaGA_BladeDance::UValdoriaGA_BladeDance()
{
	bIsUltimate = true;
	DefaultCooldown = 1.f;
	DefaultMagnitude = 100.f;
	DefaultRadius = 1200.f;
	DefaultSecondary = 5.f;
}

void UValdoriaGA_BladeDance::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	TArray<AValdoriaHeroCharacter*> Targets;
	for (TActorIterator<AValdoriaHeroCharacter> It(Hero->GetWorld()); It; ++It)
	{
		if (Hero->IsEnemy(*It) && FVector::Dist(It->GetActorLocation(), Hero->GetActorLocation()) <= GetRadius() && HasLineOfSight(Hero, *It))
		{
			Targets.Add(*It);
		}
	}
	const FVector From = Hero->GetActorLocation();
	Targets.Sort([&From](const AValdoriaHeroCharacter& A, const AValdoriaHeroCharacter& B)
	{
		return FVector::DistSquared(A.GetActorLocation(), From) < FVector::DistSquared(B.GetActorLocation(), From);
	});
	Targets.SetNum(FMath::Min(Targets.Num(), FMath::Max(1, FMath::RoundToInt(GetSecondary()))));
	FVector Prev = From;
	for (AValdoriaHeroCharacter* Target : Targets)
	{
		Hero->PlayShotEffects(Target->GetActorLocation(), true);
		Hero->PlayAreaEffect(Target->GetActorLocation(), 150.f, GetThemeColor());
		ApplyDamageTo(Target, GetMagnitude());
		Prev = Target->GetActorLocation();
	}
	if (Targets.Num() > 0 && Hero->HasAuthority())
	{
		// Aparece atrás do último alvo, de frente para ele.
		AValdoriaHeroCharacter* Last = Targets.Last();
		const FVector Behind = Last->GetActorLocation() - Last->GetActorForwardVector() * 150.f;
		Hero->TeleportTo(Behind, (Last->GetActorLocation() - Behind).Rotation());
	}
	VALDORIA_END();
}

UValdoriaGA_TelekineticDash::UValdoriaGA_TelekineticDash()
{
	DefaultCooldown = 8.f;
	DefaultMagnitude = 40.f;
	DefaultRadius = 1800.f;
	DefaultSecondary = 250.f;
}

void UValdoriaGA_TelekineticDash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	// The hero (ally or enemy) best aligned with the crosshair, within 12 degrees, in reach and in sight.
	// (A thin sweep missed too often: the camera ray passes beside a target that is clearly "under" the crosshair.)
	FVector Start, Dir;
	Hero->GetAimRay(Start, Dir);
	AValdoriaHeroCharacter* Target = nullptr;
	float BestCos = FMath::Cos(FMath::DegreesToRadians(12.f));
	for (TActorIterator<AValdoriaHeroCharacter> It(Hero->GetWorld()); It; ++It)
	{
		AValdoriaHeroCharacter* Other = *It;
		if (Other == Hero || Other->IsDead() || (Hero->IsEnemy(Other) && Other->IsHiddenByStealth()))
		{
			continue;
		}
		const FVector To = Other->GetActorLocation() - Start;
		const float Cos = FVector::DotProduct(To.GetSafeNormal(), Dir);
		if (Cos > BestCos && FVector::Dist(Other->GetActorLocation(), Hero->GetActorLocation()) <= GetRadius() && HasLineOfSight(Hero, Other))
		{
			BestCos = Cos;
			Target = Other;
		}
	}
	if (!Target)
	{
		VALDORIA_END(); // nothing under the crosshair: no cooldown spent
		return;
	}
	StartCooldownAndCost();
	AActor* Shield = nullptr;
	if (Hero->HasAuthority() && !Hero->IsEnemy(Target))
	{
		// Ally: one hand forward drives her, the other hand creates a small round shield behind her.
		const FVector Away = (Hero->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
		const FVector Where = Hero->GetActorLocation() + Away * 90.f + FVector(0.f, 0.f, 40.f);
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AValdoriaBarrier* Barrier = Hero->GetWorld()->SpawnActor<AValdoriaBarrier>(AValdoriaBarrier::StaticClass(), Where, Away.Rotation(), SpawnParams))
		{
			Barrier->Setup(Hero, GetSecondary(), 110.f, 110.f, 2.f, GetThemeColor(), true);
			Barrier->MakeRound();
			Shield = Barrier;
		}
	}
	Hero->StartTelekineticDash(Target, 2800.f, GetMagnitude(), 1400.f, Shield);
	Hero->PlayAreaEffect(Hero->GetActorLocation(), 120.f, GetThemeColor());
	VALDORIA_END();
}

UValdoriaGA_TelekineticStorm::UValdoriaGA_TelekineticStorm()
{
	bIsUltimate = true;      // spends the ultimate charge (the first build forgot this: it fired for free)
	DefaultCooldown = 1.f;
	DefaultMagnitude = 100.f;
	DefaultRadius = 1200.f;
	DefaultSecondary = 5.f;
}

void UValdoriaGA_TelekineticStorm::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	TArray<AValdoriaHeroCharacter*> Targets;
	for (TActorIterator<AValdoriaHeroCharacter> It(Hero->GetWorld()); It; ++It)
	{
		if (Hero->IsEnemy(*It) && !It->IsDead() && FVector::Dist(It->GetActorLocation(), Hero->GetActorLocation()) <= GetRadius() && HasLineOfSight(Hero, *It))
		{
			Targets.Add(*It);
		}
	}
	const FVector From = Hero->GetActorLocation();
	Targets.Sort([&From](const AValdoriaHeroCharacter& A, const AValdoriaHeroCharacter& B)
	{
		return FVector::DistSquared(A.GetActorLocation(), From) < FVector::DistSquared(B.GetActorLocation(), From);
	});
	Targets.SetNum(FMath::Min(Targets.Num(), FMath::Max(1, FMath::RoundToInt(GetSecondary()))));
	Hero->PlayAreaEffect(From, 250.f, GetThemeColor());
	if (Hero->HasAuthority())
	{
		const UValdoriaHeroDefinition* Def = Hero->GetHeroDefinition();
		UStaticMesh* Mesh = Def && !Def->WeaponMesh.IsNull() ? Def->WeaponMesh.LoadSynchronous() : nullptr;
		int32 i = 0;
		for (AValdoriaHeroCharacter* Target : Targets)
		{
			// Each dagger rises from her back on a slightly different arc and flies straight at its target.
			const FVector Start = From + FVector(0.f, 0.f, 90.f) + Hero->GetActorRightVector() * (i - (Targets.Num() - 1) * 0.5f) * 40.f;
			const FVector Aim = Target->GetActorLocation() + FVector(0.f, 0.f, 30.f);
			AValdoriaTelekinesisDagger::Throw(Hero, Mesh, Start, (Aim - Start).GetSafeNormal(), GetMagnitude(), GetMagnitude() * 0.6f,
				EValdoriaDamageType::Physical, 6000.f, FVector::Dist(Start, Aim) + 300.f, GetThemeColor());
			++i;
		}
	}
	VALDORIA_END();
}

// ================================================================ mobilidade

UValdoriaGA_GrappleSelf::UValdoriaGA_GrappleSelf()
{
	DefaultCooldown = 8.f;
	DefaultMagnitude = 2200.f;
	DefaultRadius = 2500.f;
}

void UValdoriaGA_GrappleSelf::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AValdoriaHeroCharacter* Hero = GetHero();
	FHitResult Hit;
	const FVector Point = Hero ? AimPoint(Hero, GetRadius(), &Hit, true) : FVector::ZeroVector;
	// Sem parede no alcance: não gasta a recarga.
	if (!Hero || !Hit.bBlockingHit || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	StartCooldownAndCost();
	Hero->PlayShotEffects(Point, true);
	const FVector Dir = (Point - Hero->GetActorLocation()).GetSafeNormal();
	Hero->LaunchCharacter(Dir * GetMagnitude() + FVector(0.f, 0.f, 350.f), true, true);
	VALDORIA_END();
}

UValdoriaGA_HookPull::UValdoriaGA_HookPull()
{
	DefaultCooldown = 9.f;
	DefaultMagnitude = 30.f;
	DefaultRadius = 2000.f;
	DefaultSecondary = 1600.f;
}

void UValdoriaGA_HookPull::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	FVector Start, Dir;
	Hero->GetAimRay(Start, Dir);
	const FVector End = Start + Dir * GetRadius();
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ValdoriaHook), false, Hero);
	Hero->IgnoreFriendlyBarriers(Params);
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	const bool bHit = Hero->GetWorld()->SweepSingleByObjectType(Hit, Start, End, FQuat::Identity, Objects, FCollisionShape::MakeSphere(40.f), Params);
	Hero->PlayShotEffects(bHit ? Hit.ImpactPoint : End, bHit);
	AValdoriaHeroCharacter* Target = bHit ? Cast<AValdoriaHeroCharacter>(Hit.GetActor()) : nullptr;
	if (Target && Hero->IsEnemy(Target) && Hero->HasAuthority())
	{
		ApplyDamageTo(Target, GetMagnitude());
		const FVector Pull = (Hero->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D() * GetSecondary() + FVector(0, 0, 300.f);
		Target->LaunchCharacter(Pull, true, true);
		Target->ApplySlow(0.3f, 1.f);
	}
	VALDORIA_END();
}

UValdoriaGA_LungeSlash::UValdoriaGA_LungeSlash()
{
	DefaultCooldown = 6.f;
	DefaultMagnitude = 50.f;
	DefaultRadius = 700.f;
}

void UValdoriaGA_LungeSlash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	FVector AimStart, AimDir;
	Hero->GetAimRay(AimStart, AimDir);
	const FVector Dir = AimDir.GetSafeNormal2D().IsNearlyZero() ? Hero->GetActorForwardVector() : AimDir.GetSafeNormal2D();
	const FVector From = Hero->GetActorLocation();
	const FVector To = From + Dir * GetRadius();
	// Velocidade para cobrir a distância em ~0,25 s (o atrito do chão freia no fim).
	Hero->LaunchCharacter(Dir * GetRadius() * 3.6f + FVector(0.f, 0.f, 120.f), true, true);
	Hero->PlayShotEffects(To, false);
	if (Hero->HasAuthority())
	{
		for (TActorIterator<AValdoriaHeroCharacter> It(Hero->GetWorld()); It; ++It)
		{
			AValdoriaHeroCharacter* Other = *It;
			if (!Hero->IsEnemy(Other))
			{
				continue;
			}
			const FVector Closest = FMath::ClosestPointOnSegment(Other->GetActorLocation(), From, To);
			if (FVector::Dist(Closest, Other->GetActorLocation()) < 160.f)
			{
				ApplyDamageTo(Other, GetMagnitude());
			}
		}
	}
	VALDORIA_END();
}

UValdoriaGA_Teleport::UValdoriaGA_Teleport()
{
	DefaultCooldown = 10.f;
	DefaultRadius = 1500.f;
	DefaultDuration = 0.f;
	DefaultSecondary = 0.f;
}

void UValdoriaGA_Teleport::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	FHitResult Hit;
	FVector Point = AimPoint(Hero, GetRadius(), &Hit, true);
	if (Hit.bBlockingHit)
	{
		Point += Hit.ImpactNormal * 60.f;
	}
	Point.Z += 20.f;
	const FVector Origin = Hero->GetActorLocation();
	Hero->PlayAreaEffect(Origin, 120.f, GetThemeColor());
	if (Hero->HasAuthority() && Hero->TeleportTo(Point, Hero->GetActorRotation()))
	{
		Hero->PlayAreaEffect(Point, 120.f, GetThemeColor());
		if (GetDuration() > 0.f && GetSecondary() > 0.f)
		{
			// Volta sozinho ao ponto de origem.
			TWeakObjectPtr<AValdoriaHeroCharacter> Weak(Hero);
			FTimerHandle Timer;
			Hero->GetWorldTimerManager().SetTimer(Timer, [Weak, Origin]()
			{
				if (Weak.IsValid() && !Weak->IsDead())
				{
					Weak->TeleportTo(Origin, Weak->GetActorRotation());
				}
			}, GetDuration(), false);
		}
	}
	VALDORIA_END();
}

UValdoriaGA_AllyLink::UValdoriaGA_AllyLink()
{
	DefaultCooldown = 12.f;
	DefaultMagnitude = 0.f;
	DefaultRadius = 2000.f;
	DefaultSecondary = 0.f;
}

void UValdoriaGA_AllyLink::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AValdoriaHeroCharacter* Hero = GetHero();
	// Aliado mais perto do centro da mira.
	AValdoriaHeroCharacter* Best = nullptr;
	if (Hero)
	{
		FVector Start, Dir;
		Hero->GetAimRay(Start, Dir);
		float BestDot = 0.96f;
		for (TActorIterator<AValdoriaHeroCharacter> It(Hero->GetWorld()); It; ++It)
		{
			if (*It == Hero || It->IsDead() || It->TeamId != Hero->TeamId
				|| FVector::Dist(It->GetActorLocation(), Hero->GetActorLocation()) > GetRadius())
			{
				continue;
			}
			const float Dot = FVector::DotProduct((It->GetActorLocation() - Start).GetSafeNormal(), Dir);
			if (Dot > BestDot)
			{
				BestDot = Dot;
				Best = *It;
			}
		}
	}
	if (!Hero || !Best || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	StartCooldownAndCost();
	Hero->PlayShotEffects(Best->GetActorLocation(), true);
	const int32 Mode = FMath::RoundToInt(GetSecondary());
	const FVector Between = Best->GetActorLocation() - Hero->GetActorLocation();
	if (Mode == 1)
	{
		Best->LaunchCharacter(-Between.GetSafeNormal2D() * FMath::Min(2400.f, Between.Size() * 2.2f) + FVector(0, 0, 350.f), true, true);
	}
	else if (Mode == 2)
	{
		Hero->TeleportTo(Best->GetActorLocation() - Between.GetSafeNormal2D() * 120.f, Hero->GetActorRotation());
	}
	else
	{
		Hero->LaunchCharacter(Between.GetSafeNormal2D() * FMath::Min(2400.f, Between.Size() * 2.2f) + FVector(0, 0, 350.f), true, true);
	}
	if (GetMagnitude() > 0.f)
	{
		Hero->HealTarget(Best, GetMagnitude());
	}
	VALDORIA_END();
}

// ================================================================ reforços

UValdoriaGA_Stim::UValdoriaGA_Stim()
{
	DefaultCooldown = 12.f;
	DefaultMagnitude = 50.f;
	DefaultDuration = 4.f;
	DefaultSecondary = 0.3f;
}

void UValdoriaGA_Stim::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	Hero->HealTarget(Hero, GetMagnitude());
	Hero->RefillAmmo();
	Hero->ApplySpeedBuff(GetSecondary(), GetDuration());
	Hero->PlayAreaEffect(Hero->GetActorLocation(), 150.f, FLinearColor(0.4f, 1.f, 0.4f));
	VALDORIA_END();
}

UValdoriaGA_Stealth::UValdoriaGA_Stealth()
{
	DefaultCooldown = 12.f;
	DefaultRadius = 600.f;
	DefaultDuration = 5.f;
	DefaultSecondary = 0.2f;
}

void UValdoriaGA_Stealth::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	Hero->StartStealth(GetDuration(), GetRadius(), GetSlot());
	Hero->ApplySpeedBuff(GetSecondary(), GetDuration());
	Hero->PlayAreaEffect(Hero->GetActorLocation(), 150.f, GetThemeColor());
	VALDORIA_END();
}

// ================================================================ rastro para deslizar

UValdoriaGA_SlideTrail::UValdoriaGA_SlideTrail()
{
	DefaultCooldown = 12.f;
	DefaultRadius = 160.f;
	DefaultDuration = 4.f;
	DefaultSecondary = 0.5f;
}

void UValdoriaGA_SlideTrail::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	Hero->StartSkating(GetDuration());
	Hero->ApplySpeedBuff(GetSecondary(), GetDuration());
	if (Hero->HasAuthority())
	{
		// Um pedaço de rastro a cada 0,15 s enquanto durar.
		TWeakObjectPtr<AValdoriaHeroCharacter> Weak(Hero);
		const float Width = GetRadius();
		const float Buff = GetSecondary();
		const float EndTime = Hero->GetWorld()->GetTimeSeconds() + GetDuration();
		const FLinearColor Color = GetThemeColor();
		TSharedRef<FTimerHandle> Timer = MakeShared<FTimerHandle>();
		Hero->GetWorldTimerManager().SetTimer(*Timer, [Weak, Width, Buff, EndTime, Color, Timer]()
		{
			AValdoriaHeroCharacter* H = Weak.Get();
			if (!H || H->IsDead() || H->GetWorld()->GetTimeSeconds() > EndTime)
			{
				if (H)
				{
					H->GetWorldTimerManager().ClearTimer(*Timer);
				}
				return;
			}
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			const FVector Feet = H->GetActorLocation() - FVector(0.f, 0.f, H->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 2.f);
			if (AValdoriaZone* Piece = H->GetWorld()->SpawnActor<AValdoriaZone>(AValdoriaZone::StaticClass(), Feet, FRotator::ZeroRotator, SpawnParams))
			{
				Piece->Setup(H, Width, 3.f, 0.f, 0.f, EValdoriaDamageType::Magical, 0.f, Color);
				Piece->AllySpeedBuff = Buff;
			}
		}, 0.15f, true);
	}
	VALDORIA_END();
}

// ================================================================ feixe contínuo

UValdoriaGA_FireBeam::UValdoriaGA_FireBeam() {}

void UValdoriaGA_FireBeam::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	const FValdoriaWeaponStats& Weapon = Hero->GetWeaponStats();
	const float Pulse = 1.f / FMath::Max(0.1f, Weapon.FireRate);
	Hero->StartCooldown(GetSlot(), Pulse);
	Hero->ConsumeAmmo();

	FVector Start, Dir;
	Hero->GetAimRay(Start, Dir);
	auto Valid = [&](AValdoriaHeroCharacter* T, float MaxAngleDeg)
	{
		if (!T || T == Hero || T->IsDead() || T->IsHiddenByStealth()
			|| (!Hero->IsEnemy(T) && (T->TeamId != Hero->TeamId || Weapon.AllyHeal <= 0.f)))
		{
			return false;
		}
		const FVector To = T->GetActorLocation() - Start;
		if (To.Size() > Weapon.Range || FVector::DotProduct(To.GetSafeNormal(), Dir) < FMath::Cos(FMath::DegreesToRadians(MaxAngleDeg)))
		{
			return false;
		}
		return HasLineOfSight(Hero, T);
	};
	// Trava: mantém o alvo anterior num cone maior; senão procura o mais perto do centro da mira.
	AValdoriaHeroCharacter* Target = Cast<AValdoriaHeroCharacter>(Hero->BeamTarget.Get());
	if (!Valid(Target, FMath::Max(15.f, Weapon.SpreadDegrees * 2.f)))
	{
		Target = nullptr;
		float BestDot = -1.f;
		for (TActorIterator<AValdoriaHeroCharacter> It(Hero->GetWorld()); It; ++It)
		{
			if (Valid(*It, FMath::Max(3.f, Weapon.SpreadDegrees)))
			{
				const float Dot = FVector::DotProduct((It->GetActorLocation() - Start).GetSafeNormal(), Dir);
				if (Dot > BestDot)
				{
					BestDot = Dot;
					Target = *It;
				}
			}
		}
		Hero->BeamLockTime = 0.f;
	}
	else
	{
		Hero->BeamLockTime += Pulse;
	}
	Hero->BeamTarget = Target;
	// Continuous beam visual from the casting hand, also with no target (stops at walls), with
	// motes flowing back to the hand while it drains someone (owner: the beam showed nothing without a target).
	const FVector Hand = Hero->GetMesh()->DoesSocketExist(TEXT("hand_r")) ? Hero->GetMesh()->GetSocketLocation(TEXT("hand_r")) : Hero->GetActorLocation() + FVector(0, 0, 60.f);
	FVector End = Start + Dir * Weapon.Range;
	{
		FHitResult Wall;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ValdoriaBeam), false, Hero);
		if (Hero->GetWorld()->LineTraceSingleByChannel(Wall, Start, End, ECC_Visibility, Params))
		{
			End = Wall.ImpactPoint;
		}
	}
	const FVector BeamEnd = Target ? Target->GetActorLocation() + FVector(0, 0, 30.f) : End;
	const FLinearColor BeamColor = Hero->GetHeroDefinition() ? Hero->GetHeroDefinition()->ThemeColor : FLinearColor(1.f, 0.9f, 0.4f);
	AValdoriaFx::Beam(Hero->GetWorld(), Hand, BeamEnd, BeamColor * (Target ? 2.5f : 1.5f), Target ? 9.f : 6.f, Pulse * 1.4f);
	AValdoriaFx::Burst(Hero->GetWorld(), BeamEnd, Target ? 40.f : 28.f, BeamColor, Pulse * 1.2f);
	if (Target)
	{
		for (int32 i = 0; i < 3; ++i)
		{
			const float A = FMath::Frac(Hero->GetWorld()->GetTimeSeconds() * 1.5f + i / 3.f);
			AValdoriaFx::Burst(Hero->GetWorld(), FMath::Lerp(BeamEnd, Hand, A), 14.f, FLinearColor(1.f, 0.25f, 0.3f) * 2.f, Pulse * 1.2f);
		}
	}
	if (!Target)
	{
		Hero->PlayShotEffects(End, false);
		VALDORIA_END();
		return;
	}
	Hero->PlayShotEffects(Target->GetActorLocation() + FVector(0, 0, 30.f), true);
	const FValdoriaPassiveRules& Passive = Hero->GetPassive();
	const float Ramp = 1.f + Passive.BeamRampMax * FMath::Clamp(Hero->BeamLockTime / FMath::Max(0.1f, Passive.BeamRampTime), 0.f, 1.f);
	if (Passive.BeamRampMax > 0.f)
	{
		// Foco Crescente as a stack on the target (1-5), visible only to Lume; fades 0.4 s after the beam leaves.
		const float RampFrac = FMath::Clamp(Hero->BeamLockTime / FMath::Max(0.1f, Passive.BeamRampTime), 0.f, 1.f);
		Target->SetStack(Hero, TEXT("foco"), TEXT("F"), Hero->GetHeroDefinition() ? Hero->GetHeroDefinition()->ThemeColor : FLinearColor::Yellow,
			FMath::Max(1, FMath::CeilToInt(RampFrac * 5.f)), 0.4f);
	}
	auto Affect = [&](AValdoriaHeroCharacter* T, float Scale)
	{
		if (Hero->IsEnemy(T))
		{
			ApplyDamageTo(T, Weapon.Damage * Ramp * Scale);
		}
		else if (T->TeamId == Hero->TeamId && Weapon.AllyHeal > 0.f)
		{
			Hero->HealTarget(T, Weapon.AllyHeal * Ramp * Scale);
		}
	};
	if (Hero->HasAuthority())
	{
		Affect(Target, 1.f);
		if (Weapon.SplashRadius > 0.f)
		{
			for (TActorIterator<AValdoriaHeroCharacter> It(Hero->GetWorld()); It; ++It)
			{
				if (*It != Target && *It != Hero && !It->IsDead()
					&& FVector::Dist(It->GetActorLocation(), Target->GetActorLocation()) <= Weapon.SplashRadius)
				{
					Affect(*It, 0.6f);
				}
			}
		}
	}
	VALDORIA_END();
}

// ================================================================ defesas especiais

UValdoriaGA_Reflect::UValdoriaGA_Reflect()
{
	DefaultCooldown = 10.f;
	DefaultDuration = 2.f;
}

void UValdoriaGA_Reflect::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	Hero->StartReflect(GetDuration());
	Hero->ApplyStatus(EValdoriaStatus::CannotAttack, GetDuration());
	Hero->PlayAreaEffect(Hero->GetActorLocation(), 120.f, GetThemeColor());
	VALDORIA_END();
}

UValdoriaGA_Absorb::UValdoriaGA_Absorb()
{
	DefaultCooldown = 12.f;
	DefaultDuration = 3.f;
	DefaultMagnitude = 0.6f;
	DefaultSecondary = 3.f;
}

void UValdoriaGA_Absorb::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	Hero->StartAbsorb(GetDuration(), GetMagnitude(), GetSecondary());
	Hero->PlayAreaEffect(Hero->GetActorLocation(), 160.f, GetThemeColor());
	VALDORIA_END();
}

UValdoriaGA_Intangible::UValdoriaGA_Intangible()
{
	DefaultCooldown = 14.f;
	DefaultDuration = 2.f;
	DefaultSecondary = 0.3f;
	DefaultDamageType = EValdoriaDamageType::Magical;
}

void UValdoriaGA_Intangible::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	Hero->StartIntangible(GetDuration());
	Hero->ApplyStatus(EValdoriaStatus::CannotAttack, GetDuration());
	Hero->ApplyStatus(EValdoriaStatus::CannotUseAbilities, GetDuration());
	Hero->ApplySpeedBuff(GetSecondary(), GetDuration());
	Hero->PlayAreaEffect(Hero->GetActorLocation(), 140.f, GetThemeColor());
	VALDORIA_END();
}

UValdoriaGA_WeaponSurge::UValdoriaGA_WeaponSurge()
{
	bIsUltimate = true;
	DefaultCooldown = 1.f;
	DefaultDuration = 6.f;
	DefaultRadius = 400.f;
	DefaultSecondary = 2.f;
}

void UValdoriaGA_WeaponSurge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	FValdoriaWeaponStats Surge = Hero->GetWeaponStats();
	if (const FValdoriaAbilityEntry* Entry = GetEntry())
	{
		Surge.WeaponName = Entry->DisplayName;
	}
	Surge.Damage *= GetSecondary();
	Surge.AllyHeal *= GetSecondary();
	Surge.SplashRadius = GetRadius();
	Surge.MagazineSize = 999;
	Hero->SetWeaponOverride(Surge, GetDuration());
	Hero->PlayAreaEffect(Hero->GetActorLocation(), GetRadius(), GetThemeColor());
	VALDORIA_END();
}

// ================================================================ portals

UValdoriaGA_Portal::UValdoriaGA_Portal()
{
	DefaultCooldown = 14.f;
	DefaultRadius = 2500.f;
	DefaultDuration = 12.f;
	DefaultSecondary = 0.f;
}

void UValdoriaGA_Portal::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AValdoriaHeroCharacter* Hero = GetHero();
	if (!Hero || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	// Place on the ground under the aimed point, facing the hero.
	FHitResult Hit;
	FVector Point = AimPoint(Hero, GetRadius(), &Hit, true);
	FHitResult Down;
	if (Hero->GetWorld()->LineTraceSingleByChannel(Down, Point + FVector(0, 0, 50.f), Point - FVector(0, 0, 1000.f), ECC_WorldStatic))
	{
		Point = Down.ImpactPoint;
	}
	Point.Z += 110.f;
	const FRotator Facing(0.f, (Hero->GetActorLocation() - Point).GetSafeNormal2D().Rotation().Yaw, 0.f);
	const float Now = Hero->GetWorld()->GetTimeSeconds();
	const bool bSecond = PendingEntrance.IsValid() && Now < PendingUntil;
	if (Hero->HasAuthority())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AValdoriaPortal* Portal = Hero->GetWorld()->SpawnActor<AValdoriaPortal>(AValdoriaPortal::StaticClass(), Point,
			Facing, SpawnParams);
		if (Portal)
		{
			const EValdoriaPortalAccess Access = GetSecondary() >= 1.f ? EValdoriaPortalAccess::Everyone : EValdoriaPortalAccess::TeamOnly;
			Portal->Setup(Hero, Access, GetDuration(), GetThemeColor());
			if (bSecond)
			{
				Portal->Link(PendingEntrance.Get());
				PendingEntrance->Link(Portal);
			}
			else
			{
				PendingEntrance = Portal;
			}
		}
	}
	Hero->PlayAreaEffect(Point, 120.f, GetThemeColor());
	if (bSecond)
	{
		// Pair done: now the real cooldown.
		PendingEntrance.Reset();
		StartCooldownAndCost();
	}
	else
	{
		// Entrance placed: short delay before the exit can be placed.
		PendingUntil = Now + 10.f;
		Hero->StartCooldown(GetSlot(), 0.4f);
	}
	VALDORIA_END();
}

// ================================================================ Kay and Ariel (turn 45)

UValdoriaGA_Devastation::UValdoriaGA_Devastation()
{
	bIsUltimate = true;
	DefaultCooldown = 1.f;
	DefaultMagnitude = 30.f;  // damage per elemental hit (design units)
	DefaultRadius = 900.f;    // area of the chaos around Kay
	DefaultDuration = 4.f;    // chaos time (Kay out of control)
	DefaultSecondary = 0.5f;  // fraction of the damage allies take
	DefaultDamageType = EValdoriaDamageType::Magical;
}

void UValdoriaGA_Devastation::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	StartCooldownAndCost();
	// Owner (turn 48): after the 3 s hold Kay LOSES CONTROL: every element goes wild around them - frozen ground,
	// fire tornadoes, earthquakes that knock people off balance and winds that push in random directions. Everyone
	// close gets it, allies too (allies take a part of the damage). Afterwards Kay is incapacitated for 2 s.
	const float Chaos = FMath::Max(1.f, GetDuration());
	const float Radius = GetRadius();
	const float Damage = GetMagnitude();
	const float AllyShare = FMath::Clamp(GetSecondary(), 0.f, 1.f);
	UWorld* World = Hero->GetWorld();
	Hero->ApplyStatus(EValdoriaStatus::Rooted, Chaos);
	Hero->ApplyStatus(EValdoriaStatus::CannotAttack, Chaos);
	Hero->ApplyStatus(EValdoriaStatus::CannotUseAbilities, Chaos);
	Hero->PlayAreaEffect(Hero->GetActorLocation(), Radius, FLinearColor(0.9f, 0.9f, 1.f));
	TWeakObjectPtr<AValdoriaHeroCharacter> WeakHero = Hero;
	TSharedRef<int32> Step = MakeShared<int32>(0);
	TSharedRef<FTimerHandle> T = MakeShared<FTimerHandle>();
	const int32 Steps = FMath::RoundToInt(Chaos / 0.5f);
	World->GetTimerManager().SetTimer(*T, FTimerDelegate::CreateWeakLambda(Hero, [WeakHero, Step, T, Steps, Radius, Damage, AllyShare]()
	{
		AValdoriaHeroCharacter* H = WeakHero.Get();
		if (!H || H->IsDead())
		{
			if (H) { H->GetWorld()->GetTimerManager().ClearTimer(*T); }
			return;
		}
		UWorld* W = H->GetWorld();
		const FVector Center = H->GetActorLocation();
		static const FLinearColor Colors[4] = { FLinearColor(1.f, 0.35f, 0.08f), FLinearColor(0.55f, 0.38f, 0.18f), FLinearColor(0.15f, 0.55f, 1.f), FLinearColor(0.8f, 0.95f, 1.f) };
		static const EValdoriaDamageType Types[4] = { EValdoriaDamageType::Magical, EValdoriaDamageType::Physical, EValdoriaDamageType::Magical, EValdoriaDamageType::Hybrid };
		// Hits one unit with one element (enemies full damage, allies a part; Kay is never hit by their own chaos).
		auto Strike = [H, Damage, AllyShare](AValdoriaHeroCharacter* U, int32 Element)
		{
			if (U == H || U->IsDead() || U->IsGuardianUnit())
			{
				return;
			}
			if (H->IsEnemy(U))
			{
				H->DamageSourceLabel = TEXT("Devastação");
				H->DealTypedDamage(U, Damage, Types[Element], 0.5f, false);
				H->DamageSourceLabel.Reset();
			}
			else if (AllyShare > 0.f)
			{
				H->DealRawDamageTo(U, Damage * AllyShare * ValdoriaStatScale);
			}
			switch (Element)
			{
			case 0: U->LaunchCharacter(FVector(FMath::FRandRange(-150.f, 150.f), FMath::FRandRange(-150.f, 150.f), 650.f), true, true); break; // fire tornado lifts
			case 1: U->ApplyStatus(EValdoriaStatus::Stunned, 0.6f); break;                                                                 // earthquake: off balance
			case 2: U->ApplySlow(0.6f, 1.5f); break;                                                                                       // frozen ground
			default: U->LaunchCharacter(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f).Vector() * 900.f + FVector(0.f, 0.f, 200.f), true, true); break; // random wind
			}
		};
		// Every 0.5 s: two elemental outbursts at random spots inside the area (radius 2.5 m each) ...
		for (int32 k = 0; k < 2; ++k)
		{
			const int32 Element = FMath::RandRange(0, 3);
			const FVector Spot = Center + FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f).Vector() * FMath::FRandRange(150.f, Radius * 0.85f);
			H->PlayAreaEffect(Spot, 250.f, Colors[Element]);
			for (TActorIterator<AValdoriaHeroCharacter> It(W); It; ++It)
			{
				if (FVector::Dist2D(It->GetActorLocation(), Spot) <= 250.f && FMath::Abs(It->GetActorLocation().Z - Spot.Z) < 300.f)
				{
					Strike(*It, Element);
				}
			}
		}
		// ... and every second the whole area shakes (earthquake) or the frost spreads (alternating).
		++(*Step);
		if ((*Step % 2) == 0)
		{
			const int32 Wide = ((*Step / 2) % 2 == 0) ? 1 : 2;
			H->PlayAreaEffect(Center, Radius, Colors[Wide]);
			for (TActorIterator<AValdoriaHeroCharacter> It(W); It; ++It)
			{
				if (*It != H && FVector::Dist(It->GetActorLocation(), Center) <= Radius)
				{
					Strike(*It, Wide);
				}
			}
		}
		if (*Step >= Steps)
		{
			W->GetTimerManager().ClearTimer(*T);
			// The power is spent: Kay is incapacitated for 2 s (owner).
			H->ApplyStatus(EValdoriaStatus::Stunned, 2.f);
		}
	}), 0.5f, true, 0.05f);
	VALDORIA_END();
}

UValdoriaGA_FormSwap::UValdoriaGA_FormSwap()
{
	DefaultCooldown = 4.f;
}

void UValdoriaGA_FormSwap::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	VALDORIA_BEGIN();
	const UValdoriaHeroDefinition* Def = Hero->GetHeroDefinition();
	if (!Def || Def->AlternateFormId.IsNone())
	{
		VALDORIA_END();
		return;
	}
	StartCooldownAndCost();
	Hero->SwapToForm(Def->AlternateFormId);
	VALDORIA_END();
}
