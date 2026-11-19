// Echoes of Valdoria - global balance multipliers per game mode (turn 48). See ValdoriaMultipliers.h.

#include "ValdoriaMultipliers.h"
#include "ValdoriaMobaMode.h"
#include "ValdoriaDeathmatchMode.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	constexpr int32 AllModes = -1;
	constexpr int32 NumMults = static_cast<int32>(EValdoriaMult::Count);

	struct FMultTable
	{
		bool bLoaded = false;
		// Mode id (or AllModes) -> values; a missing entry means 1.0.
		TMap<int32, TMap<int32, float>> Values;
	};

	FMultTable& Table()
	{
		static FMultTable T;
		return T;
	}

	int32 FindKey(const FString& Key)
	{
		for (int32 i = 0; i < NumMults; ++i)
		{
			if (Key.Equals(ValdoriaMultipliers::KeyName(static_cast<EValdoriaMult>(i)), ESearchCase::IgnoreCase))
			{
				return i;
			}
		}
		return INDEX_NONE;
	}

	void Load()
	{
		FMultTable& T = Table();
		T.Values.Reset();
		T.bLoaded = true;
		const FString Path = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("Multiplicadores.ini"));
		TArray<FString> Lines;
		if (!FFileHelper::LoadFileToStringArray(Lines, *Path))
		{
			UE_LOG(LogTemp, Log, TEXT("MULTIPLICADORES: arquivo %s nao encontrado, tudo 1.0"), *Path);
			return;
		}
		int32 Section = INDEX_NONE - 1; // nothing until the first [section]
		for (FString Line : Lines)
		{
			int32 Cut = INDEX_NONE;
			if (Line.FindChar(TEXT(';'), Cut)) { Line.LeftInline(Cut); }
			if (Line.FindChar(TEXT('#'), Cut)) { Line.LeftInline(Cut); }
			Line.TrimStartAndEndInline();
			if (Line.IsEmpty())
			{
				continue;
			}
			if (Line.StartsWith(TEXT("[")))
			{
				const FString Name = Line.Mid(1).Replace(TEXT("]"), TEXT("")).TrimStartAndEnd();
				if (Name.Equals(TEXT("Todos"), ESearchCase::IgnoreCase) || Name.Equals(TEXT("All"), ESearchCase::IgnoreCase))
				{
					Section = AllModes;
				}
				else
				{
					// "[Modo 3]", "[3]" or "[Modo 3 - MOBA]": the first number is the mode id.
					FString Digits;
					for (TCHAR C : Name)
					{
						if (FChar::IsDigit(C)) { Digits.AppendChar(C); }
						else if (!Digits.IsEmpty()) { break; }
					}
					Section = Digits.IsEmpty() ? INDEX_NONE - 1 : FCString::Atoi(*Digits);
				}
				continue;
			}
			FString Key, Value;
			if (Section < AllModes || !Line.Split(TEXT("="), &Key, &Value))
			{
				continue;
			}
			const int32 Index = FindKey(Key.TrimStartAndEnd());
			if (Index == INDEX_NONE)
			{
				UE_LOG(LogTemp, Warning, TEXT("MULTIPLICADORES: chave desconhecida '%s'"), *Key);
				continue;
			}
			T.Values.FindOrAdd(Section).Add(Index, FMath::Max(0.f, FCString::Atof(*Value.TrimStartAndEnd().Replace(TEXT(","), TEXT(".")))));
		}
		UE_LOG(LogTemp, Log, TEXT("MULTIPLICADORES: %d secoes lidas de %s"), T.Values.Num(), *Path);
	}

	float Lookup(int32 Section, int32 Index)
	{
		if (const TMap<int32, float>* S = Table().Values.Find(Section))
		{
			if (const float* V = S->Find(Index))
			{
				return *V;
			}
		}
		return 1.f;
	}
}

const TCHAR* ValdoriaMultipliers::KeyName(EValdoriaMult Which)
{
	switch (Which)
	{
	case EValdoriaMult::Damage:         return TEXT("Dano");
	case EValdoriaMult::PhysicalDamage: return TEXT("DanoFisico");
	case EValdoriaMult::MagicalDamage:  return TEXT("DanoMagico");
	case EValdoriaMult::AttackDamage:   return TEXT("DanoAtaqueBasico");
	case EValdoriaMult::AbilityDamage:  return TEXT("DanoHabilidades");
	case EValdoriaMult::Healing:        return TEXT("Cura");
	case EValdoriaMult::Vampirism:      return TEXT("Vampirismo");
	case EValdoriaMult::LifeSteal:      return TEXT("RouboDeVida");
	case EValdoriaMult::MaxHealth:      return TEXT("Vida");
	case EValdoriaMult::MaxArmor:       return TEXT("Armadura");
	case EValdoriaMult::MaxShield:      return TEXT("Escudo");
	case EValdoriaMult::GuardianHealth: return TEXT("VidaGuardioes");
	case EValdoriaMult::Cooldown:       return TEXT("Recarga");
	case EValdoriaMult::UltimateCharge: return TEXT("CargaUltimate");
	case EValdoriaMult::MoveSpeed:      return TEXT("Velocidade");
	case EValdoriaMult::Gold:           return TEXT("Ouro");
	case EValdoriaMult::Experience:     return TEXT("Experiencia");
	case EValdoriaMult::RespawnTime:    return TEXT("TempoRenascer");
	default:                            return TEXT("?");
	}
}

int32 ValdoriaMultipliers::GetModeId(const UWorld* World)
{
	const AGameModeBase* Mode = World ? World->GetAuthGameMode() : nullptr;
	if (!Mode)
	{
		return 0;
	}
	if (Mode->IsA<AValdoriaMobaMode>())
	{
		return 3;
	}
	if (Mode->IsA<AValdoriaDeathmatchMode>())
	{
		return 2;
	}
	return World->GetMapName().Contains(TEXT("NP_")) ? 1 : 0;
}

float ValdoriaMultipliers::Get(const UWorld* World, EValdoriaMult Which)
{
	if (!Table().bLoaded)
	{
		Load();
	}
	const int32 Index = static_cast<int32>(Which);
	return Lookup(AllModes, Index) * Lookup(GetModeId(World), Index);
}

void ValdoriaMultipliers::Reload()
{
	Load();
}
