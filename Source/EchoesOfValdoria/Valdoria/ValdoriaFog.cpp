// Echoes of Valdoria - Neblinas (turn 47). See ValdoriaFog.h.

#include "ValdoriaFog.h"
#include "ValdoriaWorldActors.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"

AValdoriaFogArea::AValdoriaFogArea()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AValdoriaFogArea::BeginPlay()
{
	Super::BeginPlay();
	BuildVisual();
}

bool AValdoriaFogArea::Contains(const FVector& Point) const
{
	const FVector C = GetActorLocation();
	return FVector::DistSquared2D(Point, C) <= Radius * Radius && Point.Z >= C.Z - 150.f && Point.Z <= C.Z + Height;
}

AValdoriaFogArea* AValdoriaFogArea::FindAt(const UWorld* World, const FVector& Point)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AValdoriaFogArea> It(World); It; ++It)
	{
		if (It->Contains(Point))
		{
			return *It;
		}
	}
	return nullptr;
}

void AValdoriaFogArea::BuildVisual()
{
	// Soft grey-violet mist: a few overlapping translucent puffs (spheres) plus a faint low disc, no collision.
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	auto Puff = [this](UStaticMesh* Mesh, const FVector& Offset, const FVector& Scale, float Opacity)
	{
		if (!Mesh)
		{
			return;
		}
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetStaticMesh(Mesh);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetupAttachment(RootComponent);
		C->RegisterComponent();
		C->SetRelativeLocation(Offset);
		C->SetRelativeScale3D(Scale);
		if (UMaterialInstanceDynamic* MID = ValdoriaMakeGlowMaterial(C, FLinearColor(0.16f, 0.15f, 0.2f), Opacity))
		{
			C->SetMaterial(0, MID);
		}
	};
	const float R = Radius / 50.f; // engine sphere/cylinder are 100 cm wide
	Puff(Cylinder, FVector(0.f, 0.f, 20.f), FVector(R, R, 0.4f), 0.35f);
	for (int32 i = 0; i < 7; ++i)
	{
		const float A = 2.f * PI * i / 7.f;
		const FVector Off(FMath::Cos(A) * Radius * 0.55f, FMath::Sin(A) * Radius * 0.55f, 120.f + 60.f * (i % 3));
		Puff(Sphere, Off, FVector(R * 0.55f, R * 0.55f, Height / 100.f * 0.7f), 0.22f);
	}
	Puff(Sphere, FVector(0.f, 0.f, Height * 0.45f), FVector(R * 0.8f, R * 0.8f, Height / 100.f), 0.18f);
}
