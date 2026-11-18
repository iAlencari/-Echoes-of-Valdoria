// Echoes of Valdoria - opções do jogador (gráficos extras, áudio e controles).
// Resolução, janela, V-Sync, limite de FPS e qualidade ficam no UGameUserSettings da Unreal;
// o resto é salvo em GameUserSettings.ini, seção [Valdoria].

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "ValdoriaTypes.h"

struct ECHOESOFVALDORIA_API FValdoriaOptions
{
	// Gráficos (extras)
	float FieldOfView = 95.f;
	int32 ResolutionScale = 100;      // % (50 a 100)
	int32 AntiAliasing = 4;           // 0 desligado, 1 FXAA, 2 TAA, 4 TSR
	bool bLumen = true;
	bool bHardwareRayTracing = true;
	bool bMotionBlur = false;
	bool bShowFps = true;

	// Áudio (0 a 1)
	float MasterVolume = 1.f;
	float MusicVolume = 0.7f;
	float EffectsVolume = 1.f;

	// Controles
	float MouseSensitivity = 1.f;
	bool bInvertY = false;
	float CrosshairSize = 1.f;
	int32 CrosshairColor = 0;         // 0 branco, 1 verde, 2 ciano, 3 amarelo, 4 vermelho, 5 rosa
	/** Teclas das habilidades: 0 = Valdoria (botão direito, F, E, C; Q ultimate), 1 = estilo Valorant (botão direito, Q, E, C; X ultimate). */
	int32 KeyScheme = 0;

	static FValdoriaOptions& Get();
	void Load();
	void Save() const;
	/** Aplica o que é global (console variables e volume). FOV e mira são lidos pelo jogo. */
	void Apply() const;
	FLinearColor GetCrosshairColor() const;
	/** Tecla de cada botão de habilidade no esquema atual. */
	FKey GetSlotKey(EValdoriaAbilitySlot Slot) const;
	FString GetSlotKeyLabel(EValdoriaAbilitySlot Slot) const;
};
