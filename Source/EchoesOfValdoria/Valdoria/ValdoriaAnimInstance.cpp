// Echoes of Valdoria - animador próprio em C++.

#include "ValdoriaAnimInstance.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimSequenceBase.h"
#include "AnimationRuntime.h"
#include "BonePose.h"
#include "ValdoriaHeroCharacter.h"

namespace
{
	float AdvanceTime(const UAnimSequenceBase* Seq, float Time, float Delta, bool bLoop)
	{
		if (!Seq)
		{
			return 0.f;
		}
		const float Len = FMath::Max(0.01f, Seq->GetPlayLength());
		Time += Delta;
		return bLoop ? FMath::Fmod(FMath::Max(0.f, Time), Len) : FMath::Clamp(Time, 0.f, Len);
	}

	void ExtractPose(UAnimSequenceBase* Seq, float Time, bool bLoop, FPoseContext& Ctx)
	{
		FAnimationPoseData Data(Ctx);
		Seq->GetAnimationPose(Data, FAnimExtractContext(static_cast<double>(Time), false, FDeltaTimeRecord(), bLoop));
	}
}

// ------------------------------------------------------------------ thread do jogo

void UValdoriaAnimInstance::PlayAction(UAnimSequenceBase* Clip, float PlayRate, bool bFullBody)
{
	if (!Clip)
	{
		return;
	}
	// O mesmo golpe repetido rápido (tiro automático) não reinicia enquanto está no começo.
	if (Frame.Action == Clip && ActionLength > 0.f && Frame.ActionTime < ActionLength * 0.6f && !bFullBody)
	{
		return;
	}
	ActionRef = Clip;
	Frame.Action = Clip;
	Frame.ActionTime = 0.f;
	Frame.bActionFullBody = bFullBody;
	ActionRate = FMath::Max(0.1f, PlayRate);
	ActionLength = Clip->GetPlayLength();
	if (Frame.ActionWeight <= 0.f)
	{
		Frame.ActionWeight = 0.01f;
	}
}

void UValdoriaAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	const AValdoriaHeroCharacter* Hero = Cast<AValdoriaHeroCharacter>(GetOwningActor());
	bool bLoop = true;
	float Rate = 1.f;
	UAnimSequenceBase* Desired = Hero ? Hero->PickBaseAnimation(bLoop, Rate) : nullptr;

	if (Desired != Frame.Base)
	{
		// Troca suave. Entre animações de andar em direções diferentes, mantém o passo (fase).
		const float Phase = (Frame.Base && Frame.bBaseLoop) ? Frame.BaseTime / FMath::Max(0.01f, Frame.Base->GetPlayLength()) : 0.f;
		PrevRef = Frame.Base;
		Frame.Prev = Frame.Base;
		Frame.PrevTime = Frame.BaseTime;
		Frame.bPrevLoop = Frame.bBaseLoop;
		BaseRef = Desired;
		Frame.Base = Desired;
		Frame.bBaseLoop = bLoop;
		Frame.BaseTime = (Desired && bLoop && Frame.Prev) ? Phase * Desired->GetPlayLength() : 0.f;
		Frame.BaseBlend = Frame.Prev ? 0.f : 1.f;
	}
	BaseRate = Rate;
	Frame.BaseTime = AdvanceTime(Frame.Base, Frame.BaseTime, DeltaSeconds * BaseRate, Frame.bBaseLoop);
	if (Frame.Prev && Frame.BaseBlend < 1.f)
	{
		Frame.PrevTime = AdvanceTime(Frame.Prev, Frame.PrevTime, DeltaSeconds * BaseRate, Frame.bPrevLoop);
		Frame.BaseBlend = FMath::Min(1.f, Frame.BaseBlend + DeltaSeconds / 0.18f);
	}
	if (Frame.BaseBlend >= 1.f)
	{
		Frame.Prev = nullptr;
		PrevRef = nullptr;
	}

	// Ação por cima: entra rápido e sai suave no fim.
	if (Frame.Action)
	{
		Frame.ActionTime += DeltaSeconds * ActionRate;
		const float Left = ActionLength - Frame.ActionTime;
		if (Left <= 0.f)
		{
			Frame.Action = nullptr;
			ActionRef = nullptr;
			Frame.ActionWeight = 0.f;
		}
		else
		{
			const float FadeIn = FMath::Min(1.f, Frame.ActionWeight + DeltaSeconds / 0.07f);
			const float FadeOut = FMath::Clamp(Left / FMath::Max(0.01f, 0.15f * ActionRate), 0.f, 1.f);
			Frame.ActionWeight = FMath::Min(FadeIn, FadeOut);
		}
	}
}

// ------------------------------------------------------------------ thread de animação

void FValdoriaAnimProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
	if (const UValdoriaAnimInstance* Instance = Cast<UValdoriaAnimInstance>(InAnimInstance))
	{
		Frame = Instance->Frame;
	}
}

bool FValdoriaAnimProxy::Evaluate(FPoseContext& Output)
{
	if (!Frame.Base)
	{
		Output.ResetToRefPose();
		return true;
	}
	// 1) Base (com a troca suave da anterior).
	if (Frame.Prev && Frame.BaseBlend < 1.f)
	{
		FPoseContext Current(Output);
		FPoseContext Previous(Output);
		ExtractPose(Frame.Base, Frame.BaseTime, Frame.bBaseLoop, Current);
		ExtractPose(Frame.Prev, Frame.PrevTime, Frame.bPrevLoop, Previous);
		FAnimationPoseData A(Current);
		FAnimationPoseData B(Previous);
		FAnimationPoseData Out(Output);
		FAnimationRuntime::BlendTwoPosesTogether(A, B, Frame.BaseBlend, Out);
	}
	else
	{
		ExtractPose(Frame.Base, Frame.BaseTime, Frame.bBaseLoop, Output);
	}

	// Blends Source into Output on the upper body (spine_01 and children) or the whole body.
	auto BlendUpper = [&Output](FPoseContext& Source, float Weight, bool bFullBody)
	{
		const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
		const FReferenceSkeleton& Ref = Bones.GetReferenceSkeleton();
		const int32 Spine = Ref.FindBoneIndex(TEXT("spine_01"));
		for (FCompactPoseBoneIndex Index : Output.Pose.ForEachBoneIndex())
		{
			bool bUpper = bFullBody;
			if (!bUpper && Spine != INDEX_NONE)
			{
				for (int32 Mesh = Bones.MakeMeshPoseIndex(Index).GetInt(); Mesh != INDEX_NONE; Mesh = Ref.GetParentIndex(Mesh))
				{
					if (Mesh == Spine)
					{
						bUpper = true;
						break;
					}
				}
			}
			if (bUpper)
			{
				Output.Pose[Index].BlendWith(Source.Pose[Index], Weight);
			}
		}
		Output.Pose.NormalizeRotations();
	};

	// 2) Held pose (e.g. arm forward while flying fast).
	if (Frame.Pose && Frame.PoseWeight > 0.f && Frame.Pose->GetAdditiveAnimType() == AAT_None)
	{
		FPoseContext Pose(Output);
		ExtractPose(Frame.Pose, Frame.PoseTime, false, Pose);
		BlendUpper(Pose, Frame.PoseWeight, false);
	}

	// 3) Action: whole body or spine_01 and up.
	if (Frame.Action && Frame.ActionWeight > 0.f)
	{
		const EAdditiveAnimationType Additive = Frame.Action->GetAdditiveAnimType();
		if (Additive != AAT_None)
		{
			// Additive clips (e.g. the Epic rifle fire/reload) only hold differences: add them on top.
			// Extracting them as a normal pose collapsed the upper body (it "disappeared").
			FPoseContext Delta(Output, true);
			ExtractPose(Frame.Action, Frame.ActionTime, false, Delta);
			FAnimationPoseData OutData(Output);
			const FAnimationPoseData DeltaData(Delta);
			FAnimationRuntime::AccumulateAdditivePose(OutData, DeltaData, Frame.ActionWeight, Additive);
			Output.Pose.NormalizeRotations();
		}
		else
		{
			FPoseContext Action(Output);
			ExtractPose(Frame.Action, Frame.ActionTime, false, Action);
			BlendUpper(Action, Frame.ActionWeight, Frame.bActionFullBody);
		}
	}
	return true;
}
