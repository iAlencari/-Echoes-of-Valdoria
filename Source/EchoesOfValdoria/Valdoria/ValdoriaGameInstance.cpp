// Echoes of Valdoria - tela de carregamento.

#include "ValdoriaGameInstance.h"
#include "Components/AudioComponent.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "MoviePlayer.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "UObject/UObjectGlobals.h"
#include "ValdoriaAudio.h"
#include "ValdoriaSettings.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdoriaLoading"

namespace
{
	/** Dicas mostradas no carregamento (uma por vez, sorteada). */
	const TCHAR* Tips[] = {
		TEXT("Segure K para ver o que cada habilidade do seu herói faz."),
		TEXT("Aperte B para abrir a loja de melhorias: os créditos vêm do tempo, do dano e dos abates."),
		TEXT("Esc abre o menu: lá você troca de herói e muda gráficos, áudio e teclas."),
		TEXT("Brasa fica mais resistente perto de aliados e protege quem está a até 8 m dela."),
		TEXT("Vex causa 50% a mais de dano pelas costas. Chegue invisível e ataque por trás."),
		TEXT("O feixe da Lume fica mais forte quanto mais tempo ela segura no mesmo alvo."),
		TEXT("Parado ou agachado, o rifle do Kael quase não tem dispersão."),
		TEXT("Os orbes da Iara curam aliados e ferem inimigos: mire sem medo no meio da luta."),
		TEXT("Dano mágico ignora a intangibilidade; dano físico não."),
		TEXT("No estilo de teclas Valorant, as habilidades ficam em Q, E e C, e a ultimate no X."),
	};
}

void UValdoriaGameInstance::Init()
{
	Super::Init();
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UValdoriaGameInstance::OnPreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UValdoriaGameInstance::OnPostLoadMap);
	ImageNP = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Valdoria/UI/Loading/T_Loading_NP.T_Loading_NP"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	ImageTraining = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Valdoria/UI/Loading/T_Loading_NP2.T_Loading_NP2"), nullptr, LOAD_NoWarn | LOAD_Quiet);
}

void UValdoriaGameInstance::Shutdown()
{
	HideIntroOverlay();
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	Super::Shutdown();
}

FString UValdoriaGameInstance::GetMapDisplayName(const FString& MapPath)
{
	if (MapPath.Contains(TEXT("NP_NovaPauliceia")))
	{
		return TEXT("Nova Paulicéia");
	}
	if (MapPath.Contains(TEXT("ThirdPerson")) || MapPath.Contains(TEXT("Lvl_")))
	{
		return TEXT("Campo de treino");
	}
	return FPaths::GetBaseFilename(MapPath);
}

TSharedRef<SWidget> UValdoriaGameInstance::BuildLoadingScreen(const FString& MapPath, bool bWithThrobber)
{
	UTexture2D* Image = MapPath.Contains(TEXT("NP_")) ? ImageNP : ImageTraining;
	if (!Image)
	{
		Image = ImageNP ? ImageNP : ImageTraining;
	}
	BackgroundBrush = MakeShared<FSlateBrush>();
	if (Image)
	{
		BackgroundBrush->SetResourceObject(Image);
		BackgroundBrush->ImageSize = FVector2D(Image->GetSizeX(), Image->GetSizeY());
	}
	const FString Tip = Tips[FMath::RandRange(0, UE_ARRAY_COUNT(Tips) - 1)];
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");

	return SNew(SOverlay)
		// Fundo escuro (se a arte faltar) + arte do mapa cobrindo a tela.
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(White).BorderBackgroundColor(FLinearColor(0.02f, 0.025f, 0.04f))]
		+ SOverlay::Slot()
		[
			SNew(SScaleBox).Stretch(EStretch::ScaleToFill)
			[
				SNew(SImage).Image(BackgroundBrush.Get())
			]
		]
		// Faixa escura embaixo para o texto ler bem.
		+ SOverlay::Slot().VAlign(VAlign_Bottom)
		[
			SNew(SBox).HeightOverride(240.f)
			[
				SNew(SBorder).BorderImage(White).BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.02f, 0.78f))
			]
		]
		+ SOverlay::Slot().VAlign(VAlign_Bottom).HAlign(HAlign_Fill).Padding(FMargin(64.f, 0.f, 64.f, 46.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Bottom)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(LOCTEXT("Game", "ECHOES OF VALDORIA"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16)).ColorAndOpacity(FLinearColor(0.97f, 0.83f, 0.48f))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(FText::FromString(GetMapDisplayName(MapPath).ToUpper().Replace(TEXT("é"), TEXT("É")).Replace(TEXT("ã"), TEXT("Ã"))))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 40)).ColorAndOpacity(FLinearColor::White)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
				[
					SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("DICA: %s"), *Tip)))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14)).ColorAndOpacity(FLinearColor(0.82f, 0.85f, 0.9f))
					.AutoWrapText(true)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(24.f, 0.f, 0.f, 6.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(SCircularThrobber).Radius(18.f).Visibility(bWithThrobber ? EVisibility::Visible : EVisibility::Hidden)
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 8.f, 0.f, 0.f)
				[
					SNew(STextBlock).Text(LOCTEXT("Loading", "CARREGANDO"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12)).ColorAndOpacity(FLinearColor(0.75f, 0.78f, 0.85f))
				]
			]
		];
}

void UValdoriaGameInstance::OnPreLoadMap(const FString& MapName)
{
	PendingMap = MapName;
	if (IsRunningDedicatedServer() || !GetMoviePlayer() || !GetMoviePlayer()->IsInitialized())
	{
		return;
	}
	// Tela durante o carregamento (fica pelo menos 1,5 s, para dar tempo de ler a dica).
	FLoadingScreenAttributes Attributes;
	Attributes.bAutoCompleteWhenLoadingCompletes = true;
	Attributes.MinimumLoadingScreenDisplayTime = 1.5f;
	Attributes.WidgetLoadingScreen = BuildLoadingScreen(MapName, true);
	GetMoviePlayer()->SetupLoadingScreen(Attributes);
}

void UValdoriaGameInstance::OnPostLoadMap(UWorld* World)
{
	if (!World || IsRunningDedicatedServer() || !World->IsGameWorld())
	{
		return;
	}
	// Depois de carregar (e ao abrir o jogo), a mesma tela some devagar com a música de carregamento.
	ShowIntroOverlay(World->GetMapName());
}

void UValdoriaGameInstance::ShowIntroOverlay(const FString& MapPath)
{
	HideIntroOverlay();
	if (!GEngine || !GEngine->GameViewport || FParse::Param(FCommandLine::Get(), TEXT("ValdoriaNoIntro")))
	{
		return;
	}
	IntroOverlay = BuildLoadingScreen(MapPath, false);
	GEngine->GameViewport->AddViewportWidgetContent(IntroOverlay.ToSharedRef(), 1000);
	IntroStart = FPlatformTime::Seconds();
	if (USoundBase* Music = ValdoriaAudio::Load(TEXT("Music"), TEXT("TremLoadingloopl")))
	{
		LoadingMusic = UGameplayStatics::SpawnSound2D(this, Music, 0.5f * FValdoriaOptions::Get().MusicVolume * FValdoriaOptions::Get().MasterVolume);
	}
	IntroTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
	{
		const float Elapsed = static_cast<float>(FPlatformTime::Seconds() - IntroStart);
		// 1,8 s parada, depois some em 0,7 s.
		const float Alpha = 1.f - FMath::Clamp((Elapsed - 1.8f) / 0.7f, 0.f, 1.f);
		if (IntroOverlay.IsValid())
		{
			IntroOverlay->SetRenderOpacity(Alpha);
		}
		if (Alpha <= 0.f)
		{
			HideIntroOverlay();
			return false;
		}
		return true;
	}));
}

void UValdoriaGameInstance::HideIntroOverlay()
{
	if (IntroTicker.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(IntroTicker);
		IntroTicker.Reset();
	}
	if (IntroOverlay.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(IntroOverlay.ToSharedRef());
	}
	IntroOverlay.Reset();
	if (LoadingMusic)
	{
		LoadingMusic->FadeOut(1.f, 0.f);
		LoadingMusic = nullptr;
	}
}

#undef LOCTEXT_NAMESPACE
