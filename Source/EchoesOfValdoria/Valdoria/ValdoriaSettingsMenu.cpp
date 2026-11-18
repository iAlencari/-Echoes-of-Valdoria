// Echoes of Valdoria - tela de Configurações (abre pelo menu de pausa): Gráficos, Áudio e Controles.
// Tudo é salvo: resolução/janela/V-Sync/FPS/qualidade no GameUserSettings da Unreal e o resto
// em FValdoriaOptions (GameUserSettings.ini, seção [Valdoria]).

#include "ValdoriaPlayerController.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"
#include "ValdoriaHUD.h"
#include "ValdoriaHeroCharacter.h"
#include "ValdoriaSettings.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdoriaSettings"

namespace ValdoriaSettingsUI
{
	static FSlateFontInfo Font(int32 Size, bool bBold = false)
	{
		return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
	}

	/** Linha "nome ...... [ < valor > ]" que alterna entre opções. */
	static TSharedRef<SWidget> Choice(const FText& Label, TFunction<FText()> GetText, TFunction<void(int32)> Step)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(4.f)
			[
				SNew(STextBlock).Text(Label).Font(Font(13))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)
			[
				SNew(SButton).OnClicked_Lambda([Step]() { Step(-1); return FReply::Handled(); })[SNew(STextBlock).Text(FText::FromString(TEXT(" < "))).Font(Font(13, true))]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.f)
			[
				SNew(SBox).WidthOverride(230.f).HAlign(HAlign_Center)
				[
					SNew(STextBlock).Text_Lambda([GetText]() { return GetText(); }).Font(Font(13, true)).ColorAndOpacity(FLinearColor(1.f, 0.85f, 0.5f))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)
			[
				SNew(SButton).OnClicked_Lambda([Step]() { Step(1); return FReply::Handled(); })[SNew(STextBlock).Text(FText::FromString(TEXT(" > "))).Font(Font(13, true))]
			];
	}

	static TSharedRef<SWidget> Slider(const FText& Label, float Min, float Max, TFunction<float()> Get, TFunction<void(float)> Set, TFunction<FText()> Show)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(4.f)
			[
				SNew(STextBlock).Text(Label).Font(Font(13))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.f)
			[
				SNew(SBox).WidthOverride(230.f)
				[
					SNew(SSlider)
					.MinValue(Min).MaxValue(Max)
					.Value_Lambda([Get]() { return Get(); })
					.OnValueChanged_Lambda([Set](float V) { Set(V); })
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 2.f)
			[
				SNew(SBox).WidthOverride(60.f)[SNew(STextBlock).Text_Lambda([Show]() { return Show(); }).Font(Font(12, true))]
			];
	}

	static TSharedRef<SWidget> Toggle(const FText& Label, TFunction<bool()> Get, TFunction<void(bool)> Set)
	{
		return SNew(SBox).Padding(4.f)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([Get]() { return Get() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([Set](ECheckBoxState S) { Set(S == ECheckBoxState::Checked); })
				[
					SNew(STextBlock).Text(Label).Font(Font(13))
				]
			];
	}

	static FText Pct(float V) { return FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(V * 100.f))); }
}

void AValdoriaPlayerController::ApplyOptionsToGame()
{
	const FValdoriaOptions& O = FValdoriaOptions::Get();
	O.Apply();
	if (AValdoriaHeroCharacter* Hero = GetHero())
	{
		Hero->GetFollowCamera()->SetFieldOfView(O.FieldOfView);
	}
	if (AValdoriaHUD* HUD = Cast<AValdoriaHUD>(GetHUD()))
	{
		HUD->bShowStats = O.bShowFps;
	}
}

void AValdoriaPlayerController::OpenSettings()
{
	if (SettingsWidget.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	SettingsWidget = BuildSettingsScreen();
	GEngine->GameViewport->AddViewportWidgetContent(SettingsWidget.ToSharedRef(), 110);
	RefreshInputMode();
}

void AValdoriaPlayerController::CloseSettings()
{
	if (!SettingsWidget.IsValid())
	{
		return;
	}
	FValdoriaOptions::Get().Save();
	if (UGameUserSettings* GUS = UGameUserSettings::GetGameUserSettings())
	{
		GUS->ApplySettings(false);
		GUS->SaveSettings();
	}
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(SettingsWidget.ToSharedRef());
	}
	SettingsWidget.Reset();
	RefreshInputMode();
}

TSharedRef<SWidget> AValdoriaPlayerController::BuildSettingsScreen()
{
	using namespace ValdoriaSettingsUI;
	TWeakObjectPtr<AValdoriaPlayerController> WeakThis(this);
	UGameUserSettings* GUS = UGameUserSettings::GetGameUserSettings();
	auto Opt = []() -> FValdoriaOptions& { return FValdoriaOptions::Get(); };
	auto ApplyNow = [WeakThis]() { if (WeakThis.IsValid()) WeakThis->ApplyOptionsToGame(); };
	auto ApplyGus = [GUS]() { if (GUS) { GUS->ApplySettings(false); } };

	// Resoluções suportadas pelo monitor.
	TSharedRef<TArray<FIntPoint>> Resolutions = MakeShared<TArray<FIntPoint>>();
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(*Resolutions);
	if (Resolutions->Num() == 0)
	{
		Resolutions->Add(FIntPoint(1920, 1080));
	}

	TSharedRef<SWidgetSwitcher> Pages = SNew(SWidgetSwitcher);

	// ---------------- Gráficos
	TSharedRef<SVerticalBox> Gfx = SNew(SVerticalBox);
	const TArray<FText> Quality = { LOCTEXT("Q0", "Baixa"), LOCTEXT("Q1", "Média"), LOCTEXT("Q2", "Alta"), LOCTEXT("Q3", "Épica"), LOCTEXT("Q4", "Cinemática") };
	Gfx->AddSlot().AutoHeight()[Choice(LOCTEXT("Quality", "Qualidade geral"),
		[GUS, Quality]() { const int32 L = GUS ? GUS->GetOverallScalabilityLevel() : 3; return Quality.IsValidIndex(L) ? Quality[L] : LOCTEXT("Custom", "Personalizada"); },
		[GUS, ApplyGus](int32 D) { if (GUS) { GUS->SetOverallScalabilityLevel(FMath::Clamp(GUS->GetOverallScalabilityLevel() + D, 0, 4)); ApplyGus(); } })];
	const TArray<FText> Modes = { LOCTEXT("M0", "Tela cheia"), LOCTEXT("M1", "Tela cheia em janela"), LOCTEXT("M2", "Janela") };
	Gfx->AddSlot().AutoHeight()[Choice(LOCTEXT("Window", "Modo de tela"),
		[GUS, Modes]() { return GUS ? Modes[FMath::Clamp((int32)GUS->GetFullscreenMode(), 0, 2)] : Modes[2]; },
		[GUS, ApplyGus](int32 D) { if (GUS) { GUS->SetFullscreenMode((EWindowMode::Type)((((int32)GUS->GetFullscreenMode()) + D + 3) % 3)); ApplyGus(); } })];
	Gfx->AddSlot().AutoHeight()[Choice(LOCTEXT("Res", "Resolução"),
		[GUS]() { const FIntPoint R = GUS ? GUS->GetScreenResolution() : FIntPoint(1920, 1080); return FText::FromString(FString::Printf(TEXT("%d x %d"), R.X, R.Y)); },
		[GUS, Resolutions, ApplyGus](int32 D)
		{
			if (!GUS) return;
			int32 Index = Resolutions->IndexOfByKey(GUS->GetScreenResolution());
			Index = (Index == INDEX_NONE) ? Resolutions->Num() - 1 : FMath::Clamp(Index + D, 0, Resolutions->Num() - 1);
			GUS->SetScreenResolution((*Resolutions)[Index]);
			ApplyGus();
		})];
	Gfx->AddSlot().AutoHeight()[Toggle(LOCTEXT("VSync", "V-Sync"),
		[GUS]() { return GUS && GUS->IsVSyncEnabled(); }, [GUS, ApplyGus](bool b) { if (GUS) { GUS->SetVSyncEnabled(b); ApplyGus(); } })];
	const TArray<int32> Limits = { 0, 30, 60, 120, 144, 165, 240 };
	Gfx->AddSlot().AutoHeight()[Choice(LOCTEXT("FpsLimit", "Limite de FPS"),
		[GUS]() { const float L = GUS ? GUS->GetFrameRateLimit() : 0.f; return L <= 0.f ? LOCTEXT("NoLimit", "Sem limite") : FText::FromString(FString::Printf(TEXT("%.0f"), L)); },
		[GUS, Limits, ApplyGus](int32 D)
		{
			if (!GUS) return;
			int32 Index = Limits.IndexOfByKey(FMath::RoundToInt(GUS->GetFrameRateLimit()));
			Index = FMath::Clamp((Index == INDEX_NONE ? 0 : Index) + D, 0, Limits.Num() - 1);
			GUS->SetFrameRateLimit(Limits[Index]);
			ApplyGus();
		})];
	Gfx->AddSlot().AutoHeight()[Slider(LOCTEXT("ResScale", "Escala de resolução"), 50.f, 100.f,
		[Opt]() { return (float)Opt().ResolutionScale; }, [Opt, ApplyNow](float V) { Opt().ResolutionScale = FMath::RoundToInt(V); ApplyNow(); },
		[Opt]() { return FText::FromString(FString::Printf(TEXT("%d%%"), Opt().ResolutionScale)); })];
	const TArray<int32> AAValues = { 0, 1, 2, 4 };
	const TArray<FText> AANames = { LOCTEXT("AA0", "Desligado"), LOCTEXT("AA1", "FXAA"), LOCTEXT("AA2", "TAA"), LOCTEXT("AA4", "TSR (recomendado)") };
	Gfx->AddSlot().AutoHeight()[Choice(LOCTEXT("AA", "Antisserrilhado / upscaling"),
		[Opt, AAValues, AANames]() { const int32 I = AAValues.IndexOfByKey(Opt().AntiAliasing); return AANames.IsValidIndex(I) ? AANames[I] : AANames[3]; },
		[Opt, AAValues, ApplyNow](int32 D)
		{
			int32 I = AAValues.IndexOfByKey(Opt().AntiAliasing);
			I = FMath::Clamp((I == INDEX_NONE ? 3 : I) + D, 0, AAValues.Num() - 1);
			Opt().AntiAliasing = AAValues[I];
			ApplyNow();
		})];
	Gfx->AddSlot().AutoHeight()[Slider(LOCTEXT("Fov", "Campo de visão (FOV)"), 70.f, 120.f,
		[Opt]() { return Opt().FieldOfView; }, [Opt, ApplyNow](float V) { Opt().FieldOfView = FMath::RoundToFloat(V); ApplyNow(); },
		[Opt]() { return FText::FromString(FString::Printf(TEXT("%.0f°"), Opt().FieldOfView)); })];
	Gfx->AddSlot().AutoHeight()[Toggle(LOCTEXT("Lumen", "Lumen (iluminação global e reflexos)"), [Opt]() { return Opt().bLumen; }, [Opt, ApplyNow](bool b) { Opt().bLumen = b; ApplyNow(); })];
	Gfx->AddSlot().AutoHeight()[Toggle(LOCTEXT("RT", "Ray tracing por hardware (RTX)"), [Opt]() { return Opt().bHardwareRayTracing; }, [Opt, ApplyNow](bool b) { Opt().bHardwareRayTracing = b; ApplyNow(); })];
	Gfx->AddSlot().AutoHeight()[Toggle(LOCTEXT("Blur", "Desfoque de movimento"), [Opt]() { return Opt().bMotionBlur; }, [Opt, ApplyNow](bool b) { Opt().bMotionBlur = b; ApplyNow(); })];
	Gfx->AddSlot().AutoHeight()[Toggle(LOCTEXT("Fps", "Mostrar FPS e ping"), [Opt]() { return Opt().bShowFps; }, [Opt, ApplyNow](bool b) { Opt().bShowFps = b; ApplyNow(); })];
	Gfx->AddSlot().AutoHeight().Padding(4.f, 10.f)
	[
		SNew(STextBlock).AutoWrapText(true).Font(Font(10)).ColorAndOpacity(FLinearColor(0.6f, 0.62f, 0.68f))
		.Text(LOCTEXT("DlssNote", "DLSS: precisa do plugin gratuito da NVIDIA (download aprovado pelo dono). Até lá, o TSR da própria Unreal faz o upscaling."))
	];

	// ---------------- Áudio
	TSharedRef<SVerticalBox> Audio = SNew(SVerticalBox);
	Audio->AddSlot().AutoHeight()[Slider(LOCTEXT("Master", "Volume geral"), 0.f, 1.f,
		[Opt]() { return Opt().MasterVolume; }, [Opt, ApplyNow](float V) { Opt().MasterVolume = V; ApplyNow(); }, [Opt]() { return Pct(Opt().MasterVolume); })];
	Audio->AddSlot().AutoHeight()[Slider(LOCTEXT("Music", "Música"), 0.f, 1.f,
		[Opt]() { return Opt().MusicVolume; }, [Opt](float V) { Opt().MusicVolume = V; }, [Opt]() { return Pct(Opt().MusicVolume); })];
	Audio->AddSlot().AutoHeight()[Slider(LOCTEXT("Sfx", "Efeitos"), 0.f, 1.f,
		[Opt]() { return Opt().EffectsVolume; }, [Opt](float V) { Opt().EffectsVolume = V; }, [Opt]() { return Pct(Opt().EffectsVolume); })];
	Audio->AddSlot().AutoHeight().Padding(4.f, 10.f)
	[
		SNew(STextBlock).AutoWrapText(true).Font(Font(10)).ColorAndOpacity(FLinearColor(0.6f, 0.62f, 0.68f))
		.Text(LOCTEXT("AudioNote", "Ainda não há música nem sons no jogo; os volumes de música e efeitos já ficam salvos para quando houver."))
	];

	// ---------------- Controles
	TSharedRef<SVerticalBox> Ctrl = SNew(SVerticalBox);
	Ctrl->AddSlot().AutoHeight()[Slider(LOCTEXT("Sens", "Sensibilidade do mouse"), 0.1f, 3.f,
		[Opt]() { return Opt().MouseSensitivity; }, [Opt](float V) { Opt().MouseSensitivity = V; },
		[Opt]() { return FText::FromString(FString::Printf(TEXT("%.2f"), Opt().MouseSensitivity)); })];
	Ctrl->AddSlot().AutoHeight()[Toggle(LOCTEXT("Invert", "Inverter eixo Y"), [Opt]() { return Opt().bInvertY; }, [Opt](bool b) { Opt().bInvertY = b; })];
	Ctrl->AddSlot().AutoHeight()[Slider(LOCTEXT("XSize", "Tamanho da mira"), 0.5f, 2.5f,
		[Opt]() { return Opt().CrosshairSize; }, [Opt](float V) { Opt().CrosshairSize = V; }, [Opt]() { return Pct(Opt().CrosshairSize); })];
	const TArray<FText> Colors = { LOCTEXT("C0", "Branca"), LOCTEXT("C1", "Verde"), LOCTEXT("C2", "Ciano"), LOCTEXT("C3", "Amarela"), LOCTEXT("C4", "Vermelha"), LOCTEXT("C5", "Rosa") };
	Ctrl->AddSlot().AutoHeight()[Choice(LOCTEXT("XColor", "Cor da mira"),
		[Opt, Colors]() { return Colors[FMath::Clamp(Opt().CrosshairColor, 0, 5)]; },
		[Opt](int32 D) { Opt().CrosshairColor = (Opt().CrosshairColor + D + 6) % 6; })];
	Ctrl->AddSlot().AutoHeight().Padding(4.f, 12.f)
	[
		SNew(STextBlock).AutoWrapText(true).Font(Font(11)).ColorAndOpacity(FLinearColor(0.75f, 0.77f, 0.82f))
		.Text(LOCTEXT("Keys", "Teclas: WASD andar • Mouse mirar • Esq. atirar • Dir. secundário • R recarregar • Shift correr/impulso • Ctrl agachar (correndo, desliza) • X deitar • Espaço pular/subir/escalar • F e E habilidades • Q ultimate • K ajuda das habilidades • Esc menu • Alt soltar o mouse • F10 painel de administrador"))
	];

	Pages->AddSlot()[SNew(SScrollBox) + SScrollBox::Slot()[Gfx]];
	Pages->AddSlot()[SNew(SScrollBox) + SScrollBox::Slot()[Audio]];
	Pages->AddSlot()[SNew(SScrollBox) + SScrollBox::Slot()[Ctrl]];

	auto Tab = [Pages](const FText& Name, int32 Index)
	{
		return SNew(SBox).Padding(3.f).WidthOverride(170.f)
			[
				SNew(SButton).HAlign(HAlign_Center)
				.OnClicked_Lambda([Pages, Index]() { Pages->SetActiveWidgetIndex(Index); return FReply::Handled(); })
				[
					SNew(STextBlock).Text(Name).Font(ValdoriaSettingsUI::Font(14, true))
				]
			];
	};

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.02f, 0.6f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(780.f).HeightOverride(640.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.03f, 0.035f, 0.05f, 0.95f))
				.Padding(20.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(LOCTEXT("Title", "CONFIGURAÇÕES")).Font(Font(28, true)).ColorAndOpacity(FLinearColor(0.97f, 0.83f, 0.48f))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()[Tab(LOCTEXT("TabGfx", "Gráficos"), 0)]
						+ SHorizontalBox::Slot().AutoWidth()[Tab(LOCTEXT("TabAudio", "Áudio"), 1)]
						+ SHorizontalBox::Slot().AutoWidth()[Tab(LOCTEXT("TabCtrl", "Controles"), 2)]
					]
					+ SVerticalBox::Slot().FillHeight(1.f)[Pages]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.f, 10.f, 0.f, 0.f)
					[
						SNew(SBox).WidthOverride(200.f).HeightOverride(46.f)
						[
							SNew(SButton).HAlign(HAlign_Center).VAlign(VAlign_Center)
							.OnClicked_Lambda([WeakThis]() { if (WeakThis.IsValid()) WeakThis->CloseSettings(); return FReply::Handled(); })
							[
								SNew(STextBlock).Text(LOCTEXT("Back", "Salvar e voltar")).Font(Font(14, true))
							]
						]
					]
				]
			]
		];
}

#undef LOCTEXT_NAMESPACE
