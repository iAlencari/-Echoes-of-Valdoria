// Echoes of Valdoria - team vision and distance-based cost cuts (turn 46). See ValdoriaVision.h.

#include "ValdoriaVision.h"
#include "ValdoriaHeroCharacter.h"
#include "ValdoriaTrainingDummy.h"
#include "ValdoriaGuardian.h"
#include "ValdoriaFog.h"
#include "ValdoriaHeroDefinition.h"
#include "GameFramework/PlayerController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ShapeComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

void UValdoriaVisionSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!World || World->GetTimeSeconds() < NextUpdate)
	{
		return;
	}
	NextUpdate = World->GetTimeSeconds() + 0.2f;
	UpdateVision();
	TuneMeshes();
	ApplyFogHiding();
}

bool UValdoriaVisionSubsystem::IsRevealedTo(const AValdoriaHeroCharacter* Unit, uint8 Team) const
{
	if (!Unit)
	{
		return false;
	}
	if (Unit->TeamId == Team || Unit->TeamId == 255)
	{
		return true;
	}
	const TSet<TWeakObjectPtr<const AValdoriaHeroCharacter>>* Set = Revealed.Find(Team);
	return Set && Set->Contains(Unit);
}

bool UValdoriaVisionSubsystem::IsSensedBy(const AValdoriaHeroCharacter* Unit, uint8 Team) const
{
	const TSet<TWeakObjectPtr<const AValdoriaHeroCharacter>>* Set = Sensed.Find(Team);
	return Unit && Set && Set->Contains(Unit);
}

void UValdoriaVisionSubsystem::UpdateVision()
{
	UWorld* World = GetWorld();
	struct FUnit { AValdoriaHeroCharacter* Actor; FVector Eye; float Sight; uint8 Team; const AValdoriaFogArea* Fog; };
	TArray<FUnit> Units;
	InFog.Reset();
	TSet<uint8> Teams;
	for (TActorIterator<AValdoriaHeroCharacter> It(World); It; ++It)
	{
		AValdoriaHeroCharacter* U = *It;
		if (!U || U->IsDead())
		{
			continue;
		}
		const float Sight = U->IsGuardianUnit() ? GuardianSight : U->IsMinionUnit() ? MinionSight : U->IsHeroUnit() ? HeroSight : 0.f;
		const AValdoriaFogArea* Fog = AValdoriaFogArea::FindAt(World, U->GetActorLocation());
		if (Fog)
		{
			InFog.Add(U);
		}
		Units.Add({ U, U->GetActorLocation() + FVector(0.f, 0.f, 60.f), Fog ? FMath::Min(Sight, FogSight) : Sight, U->TeamId, Fog });
		Teams.Add(U->TeamId);
	}
	Revealed.Reset();
	Sensed.Reset();
	for (uint8 Team : Teams)
	{
		if (Team == 255)
		{
			continue;
		}
		TSet<TWeakObjectPtr<const AValdoriaHeroCharacter>>& Set = Revealed.Add(Team);
		TSet<TWeakObjectPtr<const AValdoriaHeroCharacter>>& SensedSet = Sensed.Add(Team);
		for (const FUnit& Target : Units)
		{
			if (Target.Team == Team)
			{
				continue;
			}
			const bool bDummy = Target.Team == 255; // training targets: only the special senses below matter
			if (const TMap<uint8, float>* Forced = ForcedReveals.Find(Target.Actor); Forced && Forced->Contains(Team) && (*Forced)[Team] > World->GetTimeSeconds())
			{
				Set.Add(Target.Actor);
				SensedSet.Add(Target.Actor);
				continue;
			}
			// Heimdall: hears enemies within his clairvoyance radius (walls and fog do not matter) and sees every Bifrost rider.
			bool bHeard = false;
			for (const FUnit& Eye : Units)
			{
				const UValdoriaHeroDefinition* Def = Eye.Team == Team ? Eye.Actor->GetHeroDefinition() : nullptr;
				if (Def && ((Def->ClairvoyanceRadius > 0.f && FVector::Dist(Eye.Eye, Target.Eye) <= Def->ClairvoyanceRadius)
					|| (Def->bRailGuardian && Target.Actor->IsRiding())))
				{
					bHeard = true;
					break;
				}
			}
			if (bHeard)
			{
				Set.Add(Target.Actor);
				SensedSet.Add(Target.Actor);
				continue;
			}
			if (bDummy)
			{
				continue;
			}
			// Stealth: only seen from very close.
			const float StealthCap = Target.Actor->IsHiddenByStealth() ? 500.f : 1e9f;
			for (const FUnit& Eye : Units)
			{
				if (Eye.Team != Team || Eye.Sight <= 0.f)
				{
					continue;
				}
				if (Target.Fog && Eye.Fog != Target.Fog)
				{
					continue; // inside fog: only seen by someone inside the same fog
				}
				const float DistSq = FVector::DistSquared(Eye.Eye, Target.Eye);
				const float Range = FMath::Min(Eye.Sight, StealthCap);
				if (DistSq > Range * Range)
				{
					continue;
				}
				// Only walls and buildings block sight (bodies do not).
				if (!World->LineTraceTestByObjectType(Eye.Eye, Target.Eye, FCollisionObjectQueryParams(ECC_WorldStatic)))
				{
					Set.Add(Target.Actor);
					break;
				}
			}
		}
	}
}

void UValdoriaVisionSubsystem::TuneMeshes()
{
	// Every unit except the local player's own body: far meshes are not drawn, the animation graph pauses off screen
	// (montages still play, so ability notifies fire) and update-rate optimisation lowers the animation rate with
	// distance. Re-checked every update because the local pawn can change (respawn, hero swap).
	for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
	{
		AValdoriaHeroCharacter* U = *It;
		if (!U || !U->GetMesh())
		{
			continue;
		}
		const bool bCheap = !(U->IsLocallyControlled() && U->IsPlayerControlled());
		const bool* Was = Tuned.Find(U);
		if (Was && *Was == bCheap)
		{
			continue;
		}
		Tuned.Add(U, bCheap);
		USkeletalMeshComponent* Mesh = U->GetMesh();
		Mesh->VisibilityBasedAnimTickOption = bCheap ? EVisibilityBasedAnimTickOption::OnlyTickMontagesAndRefreshBonesWhenPlayingMontages
			: EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		Mesh->bEnableUpdateRateOptimizations = bCheap;
		TArray<UPrimitiveComponent*> Prims;
		U->GetComponents<UPrimitiveComponent>(Prims);
		for (UPrimitiveComponent* P : Prims)
		{
			if (P && !P->IsA<UShapeComponent>())
			{
				P->SetCullDistance(bCheap ? RenderRadius : 0.f);
			}
		}
	}
}

bool UValdoriaVisionSubsystem::IsHiddenInFogFrom(const AValdoriaHeroCharacter* Unit, uint8 Team) const
{
	return Unit && InFog.Contains(Unit) && !IsRevealedTo(Unit, Team);
}

void UValdoriaVisionSubsystem::ApplyFogHiding()
{
	// Rendering for the local player's team: enemies hidden in fog disappear; they come back when seen or out.
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const AValdoriaHeroCharacter* Me = PC ? Cast<AValdoriaHeroCharacter>(PC->GetPawn()) : nullptr;
	const uint8 MyTeam = Me ? Me->TeamId : 254;
	for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
	{
		AValdoriaHeroCharacter* U = *It;
		const bool bHide = Me && U != Me && IsHiddenInFogFrom(U, MyTeam);
		const bool bWas = HiddenByFog.Contains(U);
		if (bHide != bWas)
		{
			U->SetActorHiddenInGame(bHide);
			if (bHide) { HiddenByFog.Add(U); } else { HiddenByFog.Remove(U); }
		}
	}
}

void UValdoriaVisionSubsystem::RevealUnit(const AValdoriaHeroCharacter* Unit, uint8 Team, float Seconds)
{
	if (Unit && Seconds > 0.f)
	{
		float& Until = ForcedReveals.FindOrAdd(Unit).FindOrAdd(Team);
		Until = FMath::Max(Until, GetWorld()->GetTimeSeconds() + Seconds);
	}
}
