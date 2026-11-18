// Echoes of Valdoria - opções do jogador.

#include "ValdoriaSettings.h"
#include "AudioDevice.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"

static const TCHAR* Section = TEXT("Valdoria");

FValdoriaOptions& FValdoriaOptions::Get()
{
	static FValdoriaOptions Options;
	static bool bLoaded = false;
	if (!bLoaded)
	{
		bLoaded = true;
		Options.Load();
	}
	return Options;
}

void FValdoriaOptions::Load()
{
	const FString& Ini = GGameUserSettingsIni;
	GConfig->GetFloat(Section, TEXT("FieldOfView"), FieldOfView, Ini);
	GConfig->GetInt(Section, TEXT("ResolutionScale"), ResolutionScale, Ini);
	GConfig->GetInt(Section, TEXT("AntiAliasing"), AntiAliasing, Ini);
	GConfig->GetBool(Section, TEXT("Lumen"), bLumen, Ini);
	GConfig->GetBool(Section, TEXT("HardwareRayTracing"), bHardwareRayTracing, Ini);
	GConfig->GetBool(Section, TEXT("MotionBlur"), bMotionBlur, Ini);
	GConfig->GetBool(Section, TEXT("ShowFps"), bShowFps, Ini);
	GConfig->GetFloat(Section, TEXT("MasterVolume"), MasterVolume, Ini);
	GConfig->GetFloat(Section, TEXT("MusicVolume"), MusicVolume, Ini);
	GConfig->GetFloat(Section, TEXT("EffectsVolume"), EffectsVolume, Ini);
	GConfig->GetFloat(Section, TEXT("MouseSensitivity"), MouseSensitivity, Ini);
	GConfig->GetBool(Section, TEXT("InvertY"), bInvertY, Ini);
	GConfig->GetInt(Section, TEXT("KeyScheme"), KeyScheme, Ini);
	GConfig->GetFloat(Section, TEXT("CrosshairSize"), CrosshairSize, Ini);
	GConfig->GetInt(Section, TEXT("CrosshairColor"), CrosshairColor, Ini);
}

void FValdoriaOptions::Save() const
{
	const FString& Ini = GGameUserSettingsIni;
	GConfig->SetFloat(Section, TEXT("FieldOfView"), FieldOfView, Ini);
	GConfig->SetInt(Section, TEXT("ResolutionScale"), ResolutionScale, Ini);
	GConfig->SetInt(Section, TEXT("AntiAliasing"), AntiAliasing, Ini);
	GConfig->SetBool(Section, TEXT("Lumen"), bLumen, Ini);
	GConfig->SetBool(Section, TEXT("HardwareRayTracing"), bHardwareRayTracing, Ini);
	GConfig->SetBool(Section, TEXT("MotionBlur"), bMotionBlur, Ini);
	GConfig->SetBool(Section, TEXT("ShowFps"), bShowFps, Ini);
	GConfig->SetFloat(Section, TEXT("MasterVolume"), MasterVolume, Ini);
	GConfig->SetFloat(Section, TEXT("MusicVolume"), MusicVolume, Ini);
	GConfig->SetFloat(Section, TEXT("EffectsVolume"), EffectsVolume, Ini);
	GConfig->SetFloat(Section, TEXT("MouseSensitivity"), MouseSensitivity, Ini);
	GConfig->SetBool(Section, TEXT("InvertY"), bInvertY, Ini);
	GConfig->SetInt(Section, TEXT("KeyScheme"), KeyScheme, Ini);
	GConfig->SetFloat(Section, TEXT("CrosshairSize"), CrosshairSize, Ini);
	GConfig->SetInt(Section, TEXT("CrosshairColor"), CrosshairColor, Ini);
	GConfig->Flush(false, Ini);
}

static void SetCVar(const TCHAR* Name, int32 Value)
{
	if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		Var->Set(Value, ECVF_SetByGameSetting);
	}
}

void FValdoriaOptions::Apply() const
{
	SetCVar(TEXT("r.ScreenPercentage"), FMath::Clamp(ResolutionScale, 50, 100));
	SetCVar(TEXT("r.AntiAliasingMethod"), AntiAliasing);
	// Lumen ligado = iluminação global e reflexos dinâmicos; desligado = mais leve.
	SetCVar(TEXT("r.DynamicGlobalIlluminationMethod"), bLumen ? 1 : 0);
	SetCVar(TEXT("r.ReflectionMethod"), bLumen ? 1 : 2);
	SetCVar(TEXT("r.Lumen.HardwareRayTracing"), bHardwareRayTracing ? 1 : 0);
	SetCVar(TEXT("r.MotionBlurQuality"), bMotionBlur ? 4 : 0);
	if (GEngine)
	{
		if (FAudioDeviceHandle Audio = GEngine->GetMainAudioDevice())
		{
			Audio->SetTransientPrimaryVolume(FMath::Clamp(MasterVolume, 0.f, 1.f));
		}
	}
}

FLinearColor FValdoriaOptions::GetCrosshairColor() const
{
	switch (CrosshairColor)
	{
	case 1: return FLinearColor(0.3f, 1.f, 0.35f);
	case 2: return FLinearColor(0.2f, 0.9f, 1.f);
	case 3: return FLinearColor(1.f, 0.9f, 0.2f);
	case 4: return FLinearColor(1.f, 0.25f, 0.2f);
	case 5: return FLinearColor(1.f, 0.4f, 0.9f);
	default: return FLinearColor::White;
	}
}

FKey FValdoriaOptions::GetSlotKey(EValdoriaAbilitySlot Slot) const
{
	const bool bValorant = KeyScheme == 1;
	switch (Slot)
	{
	case EValdoriaAbilitySlot::PrimaryFire: return EKeys::LeftMouseButton;
	case EValdoriaAbilitySlot::SecondaryFire: return EKeys::RightMouseButton;
	case EValdoriaAbilitySlot::Ability1: return bValorant ? EKeys::Q : EKeys::F;
	case EValdoriaAbilitySlot::Ability2: return EKeys::E;
	case EValdoriaAbilitySlot::Ability3: return EKeys::C;
	case EValdoriaAbilitySlot::Ultimate: return bValorant ? EKeys::X : EKeys::Q;
	default: return EKeys::Invalid;
	}
}

FString FValdoriaOptions::GetSlotKeyLabel(EValdoriaAbilitySlot Slot) const
{
	switch (Slot)
	{
	case EValdoriaAbilitySlot::PrimaryFire: return TEXT("ESQ.");
	case EValdoriaAbilitySlot::SecondaryFire: return TEXT("BD");
	default: return GetSlotKey(Slot).GetDisplayName().ToString();
	}
}
