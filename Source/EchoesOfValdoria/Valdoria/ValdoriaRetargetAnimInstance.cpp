// Echoes of Valdoria - "visual body" animator (turn 49). See the header.

#include "ValdoriaRetargetAnimInstance.h"
#include "Retargeter/IKRetargeter.h"

void FValdoriaRetargetProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
	const UValdoriaRetargetAnimInstance* Owner = Cast<UValdoriaRetargetAnimInstance>(InAnimInstance);
	if (!Owner || !Owner->Retargeter || !GetRequiredBones().IsValid())
	{
		return;
	}
	// Source = the skeletal mesh this component is attached to (the hidden Mannequin).
	Node.RetargetFrom = ERetargetSourceMode::ParentSkeletalMeshComponent;
	Node.IKRetargeterAsset = Owner->Retargeter;
	if (!bNodeInitialized)
	{
		Node.Initialize_AnyThread(FAnimationInitializeContext(this));
		bNodeInitialized = true;
	}
	// Cheap (one pass over the required bones); keeps the bone map right after LOD changes.
	Node.CacheBones_AnyThread(FAnimationCacheBonesContext(this));
	Node.PreUpdate(InAnimInstance);
	Node.Update_AnyThread(FAnimationUpdateContext(this, DeltaSeconds));
}

bool FValdoriaRetargetProxy::Evaluate(FPoseContext& Output)
{
	if (!bNodeInitialized)
	{
		Output.ResetToRefPose();
		return true;
	}
	Output.ResetToRefPose();
	Node.Evaluate_AnyThread(Output);
	return true;
}
