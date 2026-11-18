// Echoes of Valdoria - animador próprio em C++ (substitui o Animation Blueprint).
// O herói diz qual animação de base tocar (parado, andar, correr, agachar, flutuar...), e o
// animador mistura suavemente com a anterior. Ataques, tiros e recargas tocam só da cintura
// para cima, para as pernas continuarem andando. Tudo vem do FValdoriaAnimationSet do herói.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "ValdoriaAnimInstance.generated.h"

class UAnimSequenceBase;

/** O que o animador está tocando agora (copiado para a thread de animação a cada quadro). */
struct FValdoriaAnimFrame
{
	UAnimSequenceBase* Base = nullptr;
	float BaseTime = 0.f;
	bool bBaseLoop = true;
	UAnimSequenceBase* Prev = nullptr;
	float PrevTime = 0.f;
	bool bPrevLoop = true;
	/** 0 = só a anterior, 1 = só a atual. */
	float BaseBlend = 1.f;
	UAnimSequenceBase* Action = nullptr;
	float ActionTime = 0.f;
	float ActionWeight = 0.f;
	bool bActionFullBody = false;
};

struct FValdoriaAnimProxy : public FAnimInstanceProxy
{
	FValdoriaAnimProxy() = default;
	explicit FValdoriaAnimProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;

	FValdoriaAnimFrame Frame;
};

UCLASS(Transient, NotBlueprintable)
class ECHOESOFVALDORIA_API UValdoriaAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** Toca uma ação (tiro, golpe, habilidade, recarga). FullBody = corpo todo (impulso, deslize). */
	void PlayAction(UAnimSequenceBase* Clip, float PlayRate = 1.f, bool bFullBody = false);
	bool IsActionPlaying() const { return Frame.Action != nullptr; }

	FValdoriaAnimFrame Frame;

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override { return new FValdoriaAnimProxy(this); }
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override { delete InProxy; }

	/** Mantém as animações carregadas enquanto tocam. */
	UPROPERTY(Transient) TObjectPtr<UAnimSequenceBase> BaseRef;
	UPROPERTY(Transient) TObjectPtr<UAnimSequenceBase> PrevRef;
	UPROPERTY(Transient) TObjectPtr<UAnimSequenceBase> ActionRef;

	float BaseRate = 1.f;
	float ActionRate = 1.f;
	float ActionLength = 0.f;
};
