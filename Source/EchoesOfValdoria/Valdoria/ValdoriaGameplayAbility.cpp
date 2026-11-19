// Echoes of Valdoria - habilidades.

#include "ValdoriaGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ValdoriaAttributeSet.h"
#include "ValdoriaEffects.h"
#include "ValdoriaGameplayTags.h"
#include "ValdoriaHeroCharacter.h"
#include "ValdoriaHeroDefinition.h"

// ------------------------------------------------------------------ base

UValdoriaGameplayAbility::UValdoriaGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ActivationBlockedTags.AddTag(ValdoriaTags::State_Dead);
}

AValdoriaHeroCharacter* UValdoriaGameplayAbility::GetHero() const
{
	return Cast<AValdoriaHeroCharacter>(GetAvatarActorFromActorInfo());
}

EValdoriaAbilitySlot UValdoriaGameplayAbility::GetSlot() const
{
	const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();
	return Spec ? static_cast<EValdoriaAbilitySlot>(Spec->InputID % 16) : EValdoriaAbilitySlot::None; // InputID = slot + 16 * (element set + 1)
}

float UValdoriaGameplayAbility::GetEffectiveCooldown() const
{
	if (const AValdoriaHeroCharacter* Hero = GetHero())
	{
		if (const FValdoriaAbilityEntry* Entry = Hero->FindAbilityEntry(GetSlot()))
		{
			if (Entry->Cooldown > 0.f)
			{
				return Entry->Cooldown;
			}
		}
	}
	return DefaultCooldown;
}

float UValdoriaGameplayAbility::GetMagnitude() const
{
	if (const AValdoriaHeroCharacter* Hero = GetHero())
	{
		if (const FValdoriaAbilityEntry* Entry = Hero->FindAbilityEntry(GetSlot()))
		{
			if (Entry->Magnitude != 0.f)
			{
				return Entry->Magnitude;
			}
		}
	}
	return DefaultMagnitude;
}

float UValdoriaGameplayAbility::GetRadius() const
{
	if (const AValdoriaHeroCharacter* Hero = GetHero())
	{
		if (const FValdoriaAbilityEntry* Entry = Hero->FindAbilityEntry(GetSlot()))
		{
			if (Entry->Radius > 0.f)
			{
				return Entry->Radius;
			}
		}
	}
	return DefaultRadius;
}

bool UValdoriaGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	const AValdoriaHeroCharacter* Hero = ActorInfo ? Cast<AValdoriaHeroCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Hero || Hero->IsDead())
	{
		return false;
	}
	const FGameplayAbilitySpec* Spec = ActorInfo->AbilitySystemComponent.IsValid()
		? ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle) : nullptr;
	const EValdoriaAbilitySlot Slot = Spec ? static_cast<EValdoriaAbilitySlot>(Spec->InputID % 16) : EValdoriaAbilitySlot::None;
	if (Hero->GetCooldownRemaining(Slot) > 0.f)
	{
		return false;
	}
	if (bIsUltimate && !Hero->IsUltimateReady())
	{
		return false;
	}
	return true;
}

void UValdoriaGameplayAbility::StartCooldownAndCost()
{
	if (AValdoriaHeroCharacter* Hero = GetHero())
	{
		Hero->StartCooldown(GetSlot(), GetEffectiveCooldown());
		if (bIsUltimate)
		{
			Hero->ConsumeUltimate();
		}
	}
}

void UValdoriaGameplayAbility::ApplyDamageTo(AActor* Target, float Damage, bool bHeadshot) const
{
	// Tipo de dano, poder, resistências e passivas ficam no personagem.
	if (AValdoriaHeroCharacter* Hero = GetHero())
	{
		Hero->DealTypedDamage(Target, Damage, GetDamageType(), GetMagicShare(), bHeadshot);
	}
}

const FValdoriaAbilityEntry* UValdoriaGameplayAbility::GetEntry() const
{
	const AValdoriaHeroCharacter* Hero = GetHero();
	return Hero ? Hero->FindAbilityEntry(GetSlot()) : nullptr;
}

float UValdoriaGameplayAbility::GetDuration() const
{
	const FValdoriaAbilityEntry* Entry = GetEntry();
	return Entry && Entry->Duration > 0.f ? Entry->Duration : DefaultDuration;
}

float UValdoriaGameplayAbility::GetSecondary() const
{
	const FValdoriaAbilityEntry* Entry = GetEntry();
	return Entry && Entry->SecondaryMagnitude != 0.f ? Entry->SecondaryMagnitude : DefaultSecondary;
}

EValdoriaDamageType UValdoriaGameplayAbility::GetDamageType() const
{
	// O tiro principal usa o tipo da arma; as habilidades, o da própria entrada.
	if (GetSlot() == EValdoriaAbilitySlot::PrimaryFire)
	{
		if (const AValdoriaHeroCharacter* Hero = GetHero())
		{
			return Hero->GetElementDamageType(Hero->GetWeaponStats().DamageType); // Kay: per element
		}
	}
	const FValdoriaAbilityEntry* Entry = GetEntry();
	const EValdoriaDamageType Base = Entry ? Entry->DamageType : DefaultDamageType;
	const AValdoriaHeroCharacter* Hero = GetHero();
	return Hero ? Hero->GetElementDamageType(Base) : Base; // elemental heroes (Kay): the current element decides
}

float UValdoriaGameplayAbility::GetMagicShare() const
{
	if (GetSlot() == EValdoriaAbilitySlot::PrimaryFire)
	{
		if (const AValdoriaHeroCharacter* Hero = GetHero())
		{
			return Hero->GetWeaponStats().HybridMagicShare;
		}
	}
	const FValdoriaAbilityEntry* Entry = GetEntry();
	return Entry ? Entry->HybridMagicShare : 0.5f;
}

FLinearColor UValdoriaGameplayAbility::GetThemeColor() const
{
	const AValdoriaHeroCharacter* Hero = GetHero();
	// Elemental heroes (Kay) take the colour of the current element.
	if (Hero && Hero->GetHeroDefinition() && Hero->GetHeroDefinition()->ElementCount > 0)
	{
		return Hero->GetElementColor();
	}
	return Hero && Hero->GetHeroDefinition() ? Hero->GetHeroDefinition()->ThemeColor : FLinearColor::White;
}

// ------------------------------------------------------------------ tiro hitscan

UValdoriaGA_FireHitscan::UValdoriaGA_FireHitscan()
{
	DefaultCooldown = 0.f; // a cadência vem da arma
}

bool UValdoriaGA_FireHitscan::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	const AValdoriaHeroCharacter* Hero = Cast<AValdoriaHeroCharacter>(ActorInfo->AvatarActor.Get());
	return Hero && Hero->CanFireWeapon();
}

void UValdoriaGA_FireHitscan::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AValdoriaHeroCharacter* Hero = GetHero();
	if (!Hero || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FValdoriaWeaponStats& Weapon = Hero->GetWeaponStats();
	Hero->StartCooldown(GetSlot(), 1.f / FMath::Max(0.1f, Weapon.FireRate));
	Hero->ConsumeAmmo();

	FVector Start, AimDirection;
	Hero->GetAimRay(Start, AimDirection);
	// Dispersão aleatória dentro de um cone.
	const float Spread = Weapon.SpreadDegrees * Hero->GetSpreadMultiplier();
	const FVector Dir = FMath::VRandCone(AimDirection, FMath::DegreesToRadians(Spread * 0.5f));
	const FVector End = Start + Dir * Weapon.Range;
	Hero->ApplyRecoil(Weapon.RecoilDegrees);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ValdoriaHitscan), true, Hero);
	Hero->IgnoreFriendlyBarriers(Params);
	FHitResult Hit;
	const bool bHit = Hero->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	const FVector ImpactPoint = bHit ? Hit.ImpactPoint : End;

	Hero->PlayShotEffects(ImpactPoint, bHit);

	// Inimigos e barreiras inimigas (o filtro de time fica em DealTypedDamage).
	if (bHit && Hit.GetActor() && Hero->HasAuthority())
	{
		const float Distance = FVector::Dist(Start, ImpactPoint);
		float Falloff = 1.f;
		if (Weapon.FalloffEnd > Weapon.FalloffStart && Distance > Weapon.FalloffStart)
		{
			const float Alpha = FMath::Clamp((Distance - Weapon.FalloffStart) / (Weapon.FalloffEnd - Weapon.FalloffStart), 0.f, 1.f);
			Falloff = FMath::Lerp(1.f, Weapon.FalloffMinMultiplier, Alpha);
		}
		const bool bHeadshot = AValdoriaHeroCharacter::IsHeadshot(Hit);
		const float Damage = Weapon.Damage * Falloff * (bHeadshot ? Weapon.HeadshotMultiplier : 1.f);
		ApplyDamageTo(Hit.GetActor(), Damage, bHeadshot);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

// ------------------------------------------------------------------ impulso

UValdoriaGA_Dash::UValdoriaGA_Dash()
{
	DefaultCooldown = 6.f;
	DefaultMagnitude = 1800.f;
}

void UValdoriaGA_Dash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AValdoriaHeroCharacter* Hero = GetHero();
	if (!Hero || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	StartCooldownAndCost();

	FVector Direction = Hero->GetLastMovementInputVector().GetSafeNormal2D();
	if (Direction.IsNearlyZero())
	{
		Direction = Hero->GetActorForwardVector().GetSafeNormal2D();
	}
	Hero->LaunchCharacter(Direction * GetMagnitude() + FVector(0.f, 0.f, 150.f), true, true);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

// ------------------------------------------------------------------ pulso de cura

UValdoriaGA_HealPulse::UValdoriaGA_HealPulse()
{
	DefaultCooldown = 12.f;
	DefaultMagnitude = 80.f;
	DefaultRadius = 800.f;
}

void UValdoriaGA_HealPulse::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AValdoriaHeroCharacter* Hero = GetHero();
	if (!Hero || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	StartCooldownAndCost();
	Hero->PlayAreaEffect(Hero->GetActorLocation(), GetRadius(), FLinearColor(0.3f, 1.f, 0.6f));

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (Hero->HasAuthority() && SourceASC)
	{
		TArray<FOverlapResult> Overlaps;
		FCollisionShape Sphere = FCollisionShape::MakeSphere(GetRadius());
		Hero->GetWorld()->OverlapMultiByObjectType(Overlaps, Hero->GetActorLocation(), FQuat::Identity,
			FCollisionObjectQueryParams(ECC_Pawn), Sphere);
		TSet<AActor*> Done;
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* Other = Overlap.GetActor();
			if (!Other || Done.Contains(Other) || Hero->IsEnemy(Other))
			{
				continue;
			}
			Done.Add(Other);
			if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Other))
			{
				FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(UValdoriaHealEffect::StaticClass(), 1.f, SourceASC->MakeEffectContext());
				Spec.Data->SetSetByCallerMagnitude(ValdoriaTags::Data_Heal, GetMagnitude());
				SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
			}
		}
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

// ------------------------------------------------------------------ ultimate: onda de choque

UValdoriaGA_Shockwave::UValdoriaGA_Shockwave()
{
	bIsUltimate = true;
	DefaultCooldown = 1.f;
	DefaultMagnitude = 150.f;
	DefaultRadius = 900.f;
}

void UValdoriaGA_Shockwave::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AValdoriaHeroCharacter* Hero = GetHero();
	if (!Hero || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	StartCooldownAndCost();
	const FVector Center = Hero->GetActorLocation();
	Hero->PlayAreaEffect(Center, GetRadius(), FLinearColor(1.f, 0.45f, 0.1f));

	if (Hero->HasAuthority())
	{
		TArray<FOverlapResult> Overlaps;
		Hero->GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity,
			FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(GetRadius()));
		TSet<AActor*> Done;
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* Other = Overlap.GetActor();
			if (!Other || Done.Contains(Other) || !Hero->IsEnemy(Other))
			{
				continue;
			}
			Done.Add(Other);
			ApplyDamageTo(Other, GetMagnitude());
			if (ACharacter* OtherChar = Cast<ACharacter>(Other))
			{
				const FVector Push = (Other->GetActorLocation() - Center).GetSafeNormal2D() * 1200.f + FVector(0, 0, 450.f);
				OtherChar->LaunchCharacter(Push, true, true);
			}
		}
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
