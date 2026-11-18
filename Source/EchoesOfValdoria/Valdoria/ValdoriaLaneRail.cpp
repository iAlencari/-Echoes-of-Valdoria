// Echoes of Valdoria - Aurora Trails.

#include "ValdoriaLaneRail.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ValdoriaWorldActors.h"

AValdoriaLaneRail::AValdoriaLaneRail()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	bReplicates = true;
}

void AValdoriaLaneRail::Setup(uint8 InTeam, const FString& InLaneName, const TArray<FVector>& InPoints, FLinearColor InColor)
{
	Team = InTeam;
	LaneName = InLaneName;
	Points = InPoints;
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
	for (UStaticMeshComponent* Seg : Segments)
	{
		if (Seg)
		{
			Seg->DestroyComponent();
		}
	}
	Segments.Reset();
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	// Simple, calm colours: a thin glowing ribbon. Enemy rails are tinted red.
	const FLinearColor Tint = Team == 0 ? Color : FMath::Lerp(Color, FLinearColor(1.f, 0.1f, 0.1f), 0.7f);
	UMaterialInstanceDynamic* MID = ValdoriaMakeGlowMaterial(this, Tint, 0.55f);
	const float Usable = GetLength();
	const float Step = 250.f;
	for (float D = 0.f; D < Usable; D += Step)
	{
		const FVector A = GetPointAt(D);
		const FVector B = GetPointAt(FMath::Min(D + Step, Usable));
		const FVector Delta = B - A;
		if (Delta.IsNearlyZero())
		{
			continue;
		}
		UStaticMeshComponent* Seg = NewObject<UStaticMeshComponent>(this);
		Seg->SetStaticMesh(Cylinder);
		Seg->SetCollisionEnabled(ECollisionEnabled::NoCollision); // intangible to bullets, players and abilities
		Seg->SetCastShadow(false);
		Seg->SetupAttachment(RootComponent);
		Seg->RegisterComponent();
		Seg->SetWorldLocationAndRotation((A + B) * 0.5f, FRotationMatrix::MakeFromZ(Delta.GetSafeNormal()).Rotator());
		Seg->SetWorldScale3D(FVector(0.12f, 0.12f, Delta.Size() / 100.f));
		if (MID)
		{
			Seg->SetMaterial(0, MID);
		}
		Segments.Add(Seg);
	}
}
