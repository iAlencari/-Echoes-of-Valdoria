// Echoes of Valdoria - Neblinas (fog/smoke areas that work like bushes), turn 47, owner's rule:
// inside a fog area you are completely invisible to everyone outside (except clairvoyance abilities, later);
// to see someone inside you must enter too, then both see each other. Sight inside is reduced.
// Fog does not block shots or abilities, only seeing (team vision, minimap, health bars, bot targeting, rendering).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdoriaFog.generated.h"

UCLASS()
class ECHOESOFVALDORIA_API AValdoriaFogArea : public AActor
{
	GENERATED_BODY()
public:
	AValdoriaFogArea();
	virtual void BeginPlay() override;

	/** Cylinder of fog: radius and height in cm (provisional). */
	UPROPERTY(EditAnywhere, Category = "Neblina")
	float Radius = 550.f;
	UPROPERTY(EditAnywhere, Category = "Neblina")
	float Height = 400.f;

	bool Contains(const FVector& Point) const;
	/** The fog area that contains this point, or null. */
	static AValdoriaFogArea* FindAt(const UWorld* World, const FVector& Point);

private:
	void BuildVisual();
};
