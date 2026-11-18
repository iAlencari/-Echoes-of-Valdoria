// Echoes of Valdoria - "Bifrost": light rails from each base along the lanes (MOBA).
// Owner's design: starts vertical at the spawn, bends into a horizontal line over the lane,
// reaches the middle without minions (Reach, changed later by minion pushes). Heroes mount it by
// looking at it and holding Space; they ride lying down, can use basic attacks but no abilities,
// fall off when hit by an enemy. The line itself is intangible to everything.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdoriaLaneRail.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class ECHOESOFVALDORIA_API AValdoriaLaneRail : public AActor
{
	GENERATED_BODY()
public:
	AValdoriaLaneRail();

	/** Builds the rail through Points (base first). Color: lane colour; enemy rails get a red accent. */
	void Setup(uint8 InTeam, const FString& InLaneName, const TArray<FVector>& InPoints, FLinearColor InColor);

	/** How far along the full path the rail reaches (0..1). Middle of the map = 0.5 of the full lane. */
	void SetReach(float Fraction);
	float GetLength() const { return Length * Reach; }

	FVector GetPointAt(float Distance) const;
	FVector GetTangentAt(float Distance) const;
	/** Closest distance from a ray to the rail; OutAlong = rail distance of that point. */
	float DistanceToRay(const FVector& Start, const FVector& Dir, float MaxRange, float& OutAlong) const;

	uint8 Team = 0;
	FString LaneName;

protected:
	void RebuildVisual();

	TArray<FVector> Points;
	TArray<float> Cumulative;
	float Length = 0.f;
	float Reach = 1.f;
	FLinearColor Color = FLinearColor::White;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Segments;
};
