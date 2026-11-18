// Echoes of Valdoria - sons.

#include "ValdoriaAudio.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "ValdoriaSettings.h"

namespace ValdoriaAudio
{
	USoundBase* Load(const TCHAR* Category, const TCHAR* Name)
	{
		const FString Path = FString::Printf(TEXT("/Game/Valdoria/Audio/%s/%s.%s"), Category, Name, Name);
		return LoadObject<USoundBase>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	static float Pitch(float Base, float Variation)
	{
		return Base * FMath::FRandRange(1.f - Variation, 1.f + Variation);
	}

	void PlayAt(const UObject* WorldContext, USoundBase* Sound, const FVector& Location, float Volume, float PitchVariation, float BasePitch)
	{
		if (Sound && WorldContext)
		{
			UGameplayStatics::PlaySoundAtLocation(WorldContext, Sound, Location,
				Volume * FValdoriaOptions::Get().EffectsVolume, Pitch(BasePitch, PitchVariation));
		}
	}

	void Play2D(const UObject* WorldContext, USoundBase* Sound, float Volume, float PitchVariation, float BasePitch)
	{
		if (Sound && WorldContext)
		{
			// Sons de interface tocam mesmo com o jogo pausado (menu, loja).
			UGameplayStatics::PlaySound2D(WorldContext, Sound, Volume * FValdoriaOptions::Get().EffectsVolume, Pitch(BasePitch, PitchVariation),
				0.f, nullptr, nullptr, true);
		}
	}
}
