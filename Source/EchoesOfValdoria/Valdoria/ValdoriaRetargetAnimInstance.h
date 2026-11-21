// Echoes of Valdoria - "visual body" animator (turn 49).
// A hero can show a different model (e.g. a VRoid character imported with the VRM4U plugin) on top of the
// hidden Mannequin that runs the game's animation. This animator copies the Mannequin's pose every frame
// through an IK Retargeter (the same node as "Retarget Pose From Mesh" in an Animation Blueprint, in C++).
// The model's own post-process animation (VRM spring bones: hair and clothes) still runs after it.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "AnimNodes/AnimNode_RetargetPoseFromMesh.h"
#include "ValdoriaRetargetAnimInstance.generated.h"

class UIKRetargeter;

struct FValdoriaRetargetProxy : public FAnimInstanceProxy
{
	FValdoriaRetargetProxy() = default;
	explicit FValdoriaRetargetProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;

	FAnimNode_RetargetPoseFromMesh Node;
	bool bNodeInitialized = false;
};

UCLASS(Transient, NotBlueprintable)
class ECHOESOFVALDORIA_API UValdoriaRetargetAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** Retargeter from the Mannequin (source = the parent mesh this component is attached to) to this model. */
	UPROPERTY(Transient)
	TObjectPtr<UIKRetargeter> Retargeter;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override { return new FValdoriaRetargetProxy(this); }
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override { delete InProxy; }
};
