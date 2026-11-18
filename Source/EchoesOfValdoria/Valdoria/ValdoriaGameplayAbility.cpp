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
	return Spec ? static_cast<EValdoriaAbilitySlot>(Spec->InputID) : EValdoriaAbilitySlot::None;
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
	const EValdoriaAbilitySlot Slot = Spec ? static_cast<EValdoriaAbilitySlot>(Spec->InputID) : EValdoriaAbilitySlot::None;
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
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!SourceASC || !TargetASC || !SourceASC->GetOwner()->HasAuthority())
	{
		return;
	}
	const float Multiplier = SourceASC->GetNumericAttribute(UValdoriaAttributeSet::GetDamageMultiplierAttribute());
	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo());
	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(UValdoriaDamageEffect::StaticClass(), 1.f, Context);
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(ValdoriaTags::Data_Damage, Damage * FMath::Max(0.f, Multiplier));
		if (bHeadshot)
		{
			Spec.Data->AddDynamicAssetTag(ValdoriaTags::Hit_Headshot);
		}
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
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
	const FVector Dir = FMath::VRandCone(AimDirection, FMath::DegreesToRadians(Weapon.SpreadDegrees * 0.5f));
	const FVector End = Start + Dir * Weapon.Range;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ValdoriaHitscan), true, Hero);
	FHitResult Hit;
	const bool bHit = Hero->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	const FVector ImpactPoint = bHit ? Hit.ImpactPoint : End;

	Hero->PlayShotEffects(ImpactPoint, bHit);

	if (bHit && Hit.GetActor() && Hero->HasAuthority() && Hero->IsEnemy(Hit.GetActor()))
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
