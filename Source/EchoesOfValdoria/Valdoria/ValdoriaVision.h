// Echoes of Valdoria - team vision (fog of war) and distance-based cost cuts (turn 46).
//
// Vision: an enemy unit is REVEALED to a team when any unit of that team (hero, minion or guardian) is within its
// sight radius AND has a clear line (only static geometry blocks sight; bodies do not). The minimap only shows enemies
// revealed to the viewer's team, like MOBA minimaps. Updated 5 times a second, not every frame.
//
// Cost cuts for units the local player cannot see well (the "render radius" idea):
//  - meshes beyond RenderRadius are not drawn (engine cull distance);
//  - skeletal animation only updates while the mesh is on screen, and far meshes update at a lower rate (URO).
// The engine already skips drawing what is off screen or behind walls (frustum + occlusion culling); what costs CPU
// off screen is the animation and the AI. In multiplayer the server will also stop sending far units
// (net relevancy), and send only light position data for the minimap.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ValdoriaVision.generated.h"

class AValdoriaHeroCharacter;

UCLASS()
class ECHOESOFVALDORIA_API UValdoriaVisionSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UValdoriaVisionSubsystem, STATGROUP_Tickables); }
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override { return WorldType == EWorldType::Game || WorldType == EWorldType::PIE; }

	/** True when Unit can be seen by Team: same team, training target (255), or revealed by a unit of that team. */
	bool IsRevealedTo(const AValdoriaHeroCharacter* Unit, uint8 Team) const;

	/** Sight radius (cm) by unit kind. Provisional numbers; balance is the owner's. */
	float HeroSight = 5000.f;
	float MinionSight = 1800.f;
	float GuardianSight = 2500.f;
	/** Meshes farther than this from the camera are not drawn (cm). */
	float RenderRadius = 12000.f;

private:
	float NextUpdate = 0.f;
	/** Revealed units per team, rebuilt every update. */
	TMap<uint8, TSet<TWeakObjectPtr<const AValdoriaHeroCharacter>>> Revealed;
	/** Units already tuned: value = cheap settings applied (false = local player, full quality). */
	TMap<TWeakObjectPtr<AValdoriaHeroCharacter>, bool> Tuned;
	void UpdateVision();
	void TuneMeshes();
};
