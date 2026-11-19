// Echoes of Valdoria - team vision and distance-based cost cuts (turn 46). See ValdoriaVision.h.

#include "ValdoriaVision.h"
#include "ValdoriaHeroCharacter.h"
#include "ValdoriaTrainingDummy.h"
#include "ValdoriaGuardian.h"
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

void UValdoriaVisionSubsystem::UpdateVision()
{
	UWorld* World = GetWorld();
	struct FUnit { AValdoriaHeroCharacter* Actor; FVector Eye; float Sight; uint8 Team; };
	TArray<FUnit> Units;
	TSet<uint8> Teams;
	for (TActorIterator<AValdoriaHeroCharacter> It(World); It; ++It)
	{
		AValdoriaHeroCharacter* U = *It;
		if (!U || U->IsDead())
		{
			continue;
		}
		const float Sight = U->IsGuardianUnit() ? GuardianSight : U->IsMinionUnit() ? MinionSight : U->IsHeroUnit() ? HeroSight : 0.f;
		Units.Add({ U, U->GetActorLocation() + FVector(0.f, 0.f, 60.f), Sight, U->TeamId });
		Teams.Add(U->TeamId);
	}
	Revealed.Reset();
	for (uint8 Team : Teams)
	{
		if (Team == 255)
		{
			continue;
		}
		TSet<TWeakObjectPtr<const AValdoriaHeroCharacter>>& Set = Revealed.Add(Team);
		for (const FUnit& Target : Units)
		{
			if (Target.Team == Team || Target.Team == 255)
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
