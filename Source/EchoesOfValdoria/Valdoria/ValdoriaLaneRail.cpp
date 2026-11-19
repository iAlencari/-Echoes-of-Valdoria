// Echoes of Valdoria - Aurora Trails.

#include "ValdoriaLaneRail.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ValdoriaWorldActors.h"
#include "ValdoriaHeroCharacter.h"

AValdoriaLaneRail::AValdoriaLaneRail()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	bReplicates = true;
}

void AValdoriaLaneRail::Setup(uint8 InTeam, const FString& InLaneName, const TArray<FVector>& InPoints, FLinearColor InColor)
{
	Team = InTeam;
	LaneName = InLaneName;
	// Turn 46 (owner): no 90-degree corners. Chaikin corner cutting (4 passes, ends kept) turns the polyline into a
	// smooth curve; riding, aiming at the rail and the visual all use the smoothed points.
	Points = InPoints;
	for (int32 Pass = 0; Pass < 4 && Points.Num() > 2; ++Pass)
	{
		TArray<FVector> Smooth;
		Smooth.Add(Points[0]);
		for (int32 i = 0; i + 1 < Points.Num(); ++i)
		{
			Smooth.Add(FMath::Lerp(Points[i], Points[i + 1], 0.25f));
			Smooth.Add(FMath::Lerp(Points[i], Points[i + 1], 0.75f));
		}
		Smooth.Add(Points.Last());
		Points = MoveTemp(Smooth);
	}
	Color = InColor;
	Cumulative.Reset();
	Length = 0.f;
	for (int32 i = 0; i < Points.Num(); ++i)
	{
		Length += i > 0 ? FVector::Dist(Points[i - 1], Points[i]) : 0.f;
		Cumulative.Add(Length);
	}
	RebuildVisual();
}

void AValdoriaLaneRail::SetReach(float Fraction)
{
	Reach = FMath::Clamp(Fraction, 0.05f, 1.f);
	RebuildVisual();
}

FVector AValdoriaLaneRail::GetPointAt(float Distance) const
{
	if (Points.Num() == 0)
	{
		return GetActorLocation();
	}
	Distance = FMath::Clamp(Distance, 0.f, Length);
	for (int32 i = 1; i < Points.Num(); ++i)
	{
		if (Distance <= Cumulative[i])
		{
			const float Seg = FMath::Max(1.f, Cumulative[i] - Cumulative[i - 1]);
			return FMath::Lerp(Points[i - 1], Points[i], (Distance - Cumulative[i - 1]) / Seg);
		}
	}
	return Points.Last();
}

FVector AValdoriaLaneRail::GetTangentAt(float Distance) const
{
	return (GetPointAt(Distance + 50.f) - GetPointAt(Distance - 50.f)).GetSafeNormal();
}

float AValdoriaLaneRail::DistanceToRay(const FVector& Start, const FVector& Dir, float MaxRange, float& OutAlong) const
{
	float Best = TNumericLimits<float>::Max();
	OutAlong = 0.f;
	const float Usable = GetLength();
	for (float D = 0.f; D <= Usable; D += 100.f)
	{
		const FVector P = GetPointAt(D);
		const float T = FVector::DotProduct(P - Start, Dir);
		if (T < 0.f || T > MaxRange)
		{
			continue;
		}
		const float Dist = FVector::Dist(P, Start + Dir * T);
		if (Dist < Best)
		{
			Best = Dist;
			OutAlong = D;
		}
	}
	return Best;
}

void AValdoriaLaneRail::RebuildVisual()
{
	// Turn 46 - owner's concept image (SourceArt/Concepts/Bifrost_Conceito.png): a wide bundle of loose rainbow light
	// strands that twist slowly around the path, wide enough for a whole character to ride inside, a round glowing pad
	// where it starts (the base) and a ring portal where it ends. Riders look like they float (owner wants that).
	for (UStaticMeshComponent* Seg : Segments)
	{
		if (Seg)
		{
			Seg->DestroyComponent();
		}
	}
	Segments.Reset();
	for (UInstancedStaticMeshComponent* Ism : Strands)
	{
		if (Ism)
		{
			Ism->DestroyComponent();
		}
	}
	Strands.Reset();
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	const float Usable = GetLength();
	if (!Cylinder || Usable <= 1.f)
	{
		return;
	}
	auto MakeIsm = [this, Cylinder](const FLinearColor& Tint, float Glow)
	{
		UInstancedStaticMeshComponent* Ism = NewObject<UInstancedStaticMeshComponent>(this);
		Ism->SetStaticMesh(Cylinder);
		Ism->SetCollisionEnabled(ECollisionEnabled::NoCollision); // intangible to bullets, players and abilities
		Ism->SetCastShadow(false);
		Ism->SetupAttachment(RootComponent);
		Ism->RegisterComponent();
		Ism->SetWorldTransform(FTransform::Identity);
		if (UMaterialInstanceDynamic* MID = ValdoriaMakeGlowMaterial(Ism, Tint, Glow))
		{
			Ism->SetMaterial(0, MID);
		}
		Strands.Add(Ism);
		return Ism;
	};
	auto AddSegment = [](UInstancedStaticMeshComponent* Ism, const FVector& A, const FVector& B, float Thickness)
	{
		const FVector Delta = B - A;
		if (Delta.IsNearlyZero())
		{
			return;
		}
		Ism->AddInstance(FTransform(FRotationMatrix::MakeFromZ(Delta.GetSafeNormal()).Rotator(), (A + B) * 0.5f,
			FVector(Thickness, Thickness, Delta.Size() / 100.f)), true);
	};
	auto TeamTint = [this](FLinearColor C)
	{
		return Team != 0 ? FMath::Lerp(C, FLinearColor(1.f, 0.1f, 0.1f), 0.25f) : C;
	};

	// Strands: 9 rainbow bands spread around a channel under and beside the rider (radius ~1.9 m around the waist).
	constexpr int32 NumStrands = 9;
	const float Step = 110.f;
	const float Radius = 220.f;
	for (int32 k = 0; k < NumStrands; ++k)
	{
		const float Hue = 255.f * k / NumStrands;
		const FLinearColor Band = TeamTint(FLinearColor::MakeFromHSV8(static_cast<uint8>(Hue), 255, 255)) * 0.4f; // the glow material multiplies; keep the hue instead of white
		UInstancedStaticMeshComponent* Ism = MakeIsm(Band, 0.8f);
		const float Base = FMath::DegreesToRadians(170.f + 200.f * k / (NumStrands - 1)); // around and under the rider
		const float Thick = (k % 3 == 1) ? 0.12f : 0.08f;
		FVector Prev = FVector::ZeroVector;
		for (float D = 0.f; D <= Usable; D += Step)
		{
			const FVector P = GetPointAt(D);
			const FVector T = GetTangentAt(D);
			FVector Right = FVector::CrossProduct(FVector::UpVector, T).GetSafeNormal();
			if (Right.IsNearlyZero())
			{
				Right = FVector::RightVector; // vertical part: any side axis
			}
			const FVector Up = FVector::CrossProduct(T, Right).GetSafeNormal();
			// Each strand drifts around its place (slow twist + small wave), so the bundle looks alive, not a pipe.
			const float A = Base + 0.35f * FMath::Sin(D / 900.f + k * 1.7f);
			const float R = Radius * (0.85f + 0.2f * FMath::Sin(D / 600.f + k));
			const FVector Centre = P + Up * 130.f; // around the rider's body (feet ride ~15 cm above P)
			const FVector Cur = Centre + (Right * FMath::Cos(A) + Up * FMath::Sin(A)) * R;
			if (D > 0.f)
			{
				AddSegment(Ism, Prev, Cur, Thick);
			}
			Prev = Cur;
		}
	}

	{
		int32 Count = 0;
		for (UInstancedStaticMeshComponent* I : Strands) { Count += I ? I->GetInstanceCount() : 0; }
		const FVector P0 = GetPointAt(0.f), P1 = GetPointAt(Usable * 0.5f);
		UE_LOG(LogValdoria, Display, TEXT("BIFROST %s team %d: length %.0f, strand pieces %d, start (%.0f,%.0f,%.0f) middle (%.0f,%.0f,%.0f)"), *LaneName, Team, Usable, Count, P0.X, P0.Y, P0.Z, P1.X, P1.Y, P1.Z);
	}
	// Round glowing pad at the start (base) and a ring portal at the end.
	{
		UInstancedStaticMeshComponent* Pad = MakeIsm(TeamTint(FLinearColor(0.85f, 0.9f, 1.f)), 0.8f);
		const FVector Start = GetPointAt(0.f);
		Pad->AddInstance(FTransform(FRotator::ZeroRotator, Start - FVector(0.f, 0.f, 40.f), FVector(5.f, 5.f, 0.08f)), true);
		UInstancedStaticMeshComponent* Ring = MakeIsm(TeamTint(FLinearColor(0.75f, 0.85f, 1.f)), 0.9f);
		const FVector End = GetPointAt(Usable);
		const FVector T = GetTangentAt(Usable - 60.f);
		FVector Right = FVector::CrossProduct(FVector::UpVector, T).GetSafeNormal();
		if (Right.IsNearlyZero())
		{
			Right = FVector::RightVector;
		}
		const FVector Up = FVector::CrossProduct(T, Right).GetSafeNormal();
		const FVector C = End + Up * 95.f;
		constexpr int32 RingSegs = 20;
		for (int32 i = 0; i < RingSegs; ++i)
		{
			const float A0 = 2.f * PI * i / RingSegs, A1 = 2.f * PI * (i + 1) / RingSegs;
			AddSegment(Ring, C + (Right * FMath::Cos(A0) + Up * FMath::Sin(A0)) * 230.f, C + (Right * FMath::Cos(A1) + Up * FMath::Sin(A1)) * 230.f, 0.18f);
		}
	}
}
