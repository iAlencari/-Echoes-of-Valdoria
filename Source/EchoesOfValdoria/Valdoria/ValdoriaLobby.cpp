// Echoes of Valdoria - lobby (turn 45, owner's request).
// Tabs: JOGAR (pick a mode and play), HERÓIS (pick a hero, read the kit), LOJA (skins: none available yet),
// INVENTÁRIO (empty for now), TREINO (straight to the training field), CONFIGURAÇÕES and SAIR.
// Shown once when the game opens (never in tests: any -Valdoria... flag or -ValdoriaNoLobby skips it) and from
// the pause menu ("Voltar ao lobby"). Built in Slate like the other menus, so no editor assets are needed.

#include "ValdoriaPlayerController.h"
#include "ValdoriaHeroCharacter.h"
#include "ValdoriaHeroDefinition.h"
#include "ValdoriaAudio.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdoriaLobby"

namespace
{
	/** Hero picked in the lobby; survives the map change when a mode starts. */
	FName GLobbyHero;
	bool GLobbyShownOnce = false;

	const FLinearColor Gold(0.97f, 0.83f, 0.48f);
	const FLinearColor Soft(0.75f, 0.78f, 0.85f);
	const FLinearColor Panel(0.03f, 0.04f, 0.07f, 0.88f);

	struct FLobbyMode
	{
		FText Name;
		FText Desc;
		const TCHAR* Map;     // nullptr = current map
		const TCHAR* Options;
	};

	const TArray<FLobbyMode>& LobbyModes()
	{
		static const TArray<FLobbyMode> Modes = {
			{ LOCTEXT("ModeTraining", "Campo de Treino"), LOCTEXT("ModeTrainingDesc", "Bonecos de treino, sem pressa. Teste heróis e habilidades."),
				TEXT("/Game/ThirdPerson/Lvl_ThirdPerson"), TEXT("game=/Script/EchoesOfValdoria.ValdoriaGameMode") },
			{ LOCTEXT("ModeTrainingNP", "Treino em Nova Paulicéia"), LOCTEXT("ModeTrainingNPDesc", "O mapa da cidade, só com bonecos, para conhecer as rotas."),
				TEXT("/Game/Maps/NP_NovaPauliceia"), TEXT("game=/Script/EchoesOfValdoria.ValdoriaGameMode") },
			{ LOCTEXT("ModeDM", "Mata-mata"), LOCTEXT("ModeDMDesc", "Todos contra todos com bots em Nova Paulicéia."),
				TEXT("/Game/Maps/NP_NovaPauliceia"), TEXT("game=/Script/EchoesOfValdoria.ValdoriaDeathmatchMode") },
			{ LOCTEXT("ModeMoba", "Guardian Spots (MOBA)"), LOCTEXT("ModeMobaDesc", "Dois times, rotas, guardiões e pontos de poder. Mapa grande."),
				TEXT("/Game/Maps/NP_Moba"), TEXT("game=/Script/EchoesOfValdoria.ValdoriaMobaMode") },
		};
		return Modes;
	}

	TSharedRef<SWidget> Title(const FText& Text, int32 Size = 22, FLinearColor Color = Gold)
	{
		return SNew(STextBlock).Text(Text).Font(FCoreStyle::GetDefaultFontStyle("Bold", Size)).ColorAndOpacity(Color);
	}

	TSharedRef<SWidget> Body(const FText& Text, int32 Size = 12, FLinearColor Color = Soft)
	{
		return SNew(STextBlock).Text(Text).Font(FCoreStyle::GetDefaultFontStyle("Regular", Size)).ColorAndOpacity(Color).AutoWrapText(true);
	}

	FText RolesText(const UValdoriaHeroDefinition* Def)
	{
		FString Roles;
		for (const EValdoriaHeroRole R : Def->Roles)
		{
			Roles += (Roles.IsEmpty() ? TEXT("") : TEXT(" / ")) + StaticEnum<EValdoriaHeroRole>()->GetDisplayNameTextByValue(static_cast<int64>(R)).ToString();
		}
		return FText::FromString(Roles);
	}
}

void AValdoriaPlayerController::MaybeOpenLobbyAtStart()
{
	// A hero picked in the lobby carries over to the map the mode opened.
	if (!GLobbyHero.IsNone())
	{
		const FName Hero = GLobbyHero;
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, Hero]() { SelectHero(Hero); }), 0.3f, false);
	}
	if (GLobbyShownOnce)
	{
		return;
	}
	GLobbyShownOnce = true;
	const FString Cmd = FCommandLine::Get();
	if (Cmd.Contains(TEXT("-Valdoria")) || Cmd.Contains(TEXT("?game=")) || Cmd.Contains(TEXT("ValdoriaLoadingTest")))
	{
		return; // tests and direct launches go straight into the game
	}
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]() { OpenLobby(); }), 0.2f, false);
}

void AValdoriaPlayerController::OpenLobby()
{
	if (LobbyWidget.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	ClosePauseMenu();
	CloseShop();
	LobbyWidget = BuildLobby();
	GEngine->GameViewport->AddViewportWidgetContent(LobbyWidget.ToSharedRef(), 105);
	ValdoriaAudio::Play2D(this, ValdoriaAudio::Load(TEXT("UI"), TEXT("open_001")), 0.6f);
	SetPause(true);
	RefreshInputMode();
}

void AValdoriaPlayerController::CloseLobby()
{
	if (!LobbyWidget.IsValid())
	{
		return;
	}
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(LobbyWidget.ToSharedRef());
	}
	LobbyWidget.Reset();
	SetPause(false);
	RefreshInputMode();
}

void AValdoriaPlayerController::LobbyPlay()
{
	const TArray<FLobbyMode>& Modes = LobbyModes();
	const FLobbyMode& Mode = Modes[FMath::Clamp(LobbyMode, 0, Modes.Num() - 1)];
	const FString Current = UGameplayStatics::GetCurrentLevelName(this);
	const FString Target = FPaths::GetBaseFilename(Mode.Map);
	const bool bSameMapTraining = LobbyMode == 0 && Current == Target && GetWorld()->GetAuthGameMode()
		&& GetWorld()->GetAuthGameMode()->GetClass()->GetName() == TEXT("ValdoriaGameMode");
	CloseLobby();
	if (bSameMapTraining)
	{
		return; // already on the training field: just start playing
	}
	UGameplayStatics::OpenLevel(this, FName(Mode.Map), true, Mode.Options);
}

TSharedRef<SWidget> AValdoriaPlayerController::BuildLobby()
{
	// ---- page 0: JOGAR
	TSharedRef<SVerticalBox> ModeList = SNew(SVerticalBox);
	const TArray<FLobbyMode>& Modes = LobbyModes();
	for (int32 i = 0; i < Modes.Num(); ++i)
	{
		ModeList->AddSlot().AutoHeight().Padding(0.f, 4.f)
		[
			SNew(SBox).WidthOverride(560.f)
			[
				SNew(SButton)
				.ButtonColorAndOpacity_Lambda([this, i]() { return LobbyMode == i ? FLinearColor(0.85f, 0.6f, 0.2f) : FLinearColor(0.2f, 0.22f, 0.28f); })
				.OnClicked_Lambda([this, i]() { LobbyMode = i; return FReply::Handled(); })
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(8.f, 6.f, 8.f, 0.f)[ Title(Modes[i].Name, 16, FLinearColor::White) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(8.f, 2.f, 8.f, 8.f)[ Body(Modes[i].Desc, 11) ]
				]
			]
		];
	}
	TSharedRef<SWidget> PlayPage =
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)[ Title(LOCTEXT("PlayTitle", "ESCOLHA O MODO")) ]
		+ SVerticalBox::Slot().AutoHeight()[ ModeList ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
		[
			SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 12)).ColorAndOpacity(Soft)
			.Text_Lambda([this]()
			{
				const UValdoriaHeroDefinition* Def = nullptr;
				if (const AValdoriaHeroCharacter* Hero = Cast<AValdoriaHeroCharacter>(GetPawn())) { Def = Hero->GetHeroDefinition(); }
				return FText::Format(LOCTEXT("PlayHero", "Herói: {0}   (troque na aba HERÓIS)"), Def ? Def->DisplayName : LOCTEXT("NoHero", "—"));
			})
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0.f, 14.f, 0.f, 0.f)
		[
			SNew(SBox).WidthOverride(260.f).HeightOverride(60.f)
			[
				SNew(SButton).HAlign(HAlign_Center).VAlign(VAlign_Center).ButtonColorAndOpacity(FLinearColor(0.95f, 0.55f, 0.1f))
				.OnClicked_Lambda([this]() { LobbyPlay(); return FReply::Handled(); })
				[ Title(LOCTEXT("PlayButton", "JOGAR"), 24, FLinearColor::White) ]
			]
		];

	// ---- page 1: HERÓIS (grid + details of the hero being played)
	TSharedRef<SUniformGridPanel> HeroGrid = SNew(SUniformGridPanel).SlotPadding(FMargin(5.f));
	TArray<FName> Ids = UValdoriaHeroDefinition::GetPresetIds();
	Ids.Remove(TEXT("ariel_faisca")); // second form of Ariel: reached with C in game
	Ids.Add(TEXT("recruta"));
	for (int32 i = 0; i < Ids.Num(); ++i)
	{
		const FName Id = Ids[i];
		const UValdoriaHeroDefinition* Def = FindHero(Id);
		const FText Name = Def ? Def->DisplayName : FText::FromName(Id);
		const FLinearColor Color = Def ? Def->ThemeColor : FLinearColor::Gray;
		HeroGrid->AddSlot(i % 4, i / 4)
		[
			SNew(SBox).WidthOverride(140.f).HeightOverride(70.f)
			[
				SNew(SButton).HAlign(HAlign_Center).VAlign(VAlign_Center).ButtonColorAndOpacity(Color * 0.6f)
				.OnClicked_Lambda([this, Id]() { SelectHero(Id); GLobbyHero = Id; return FReply::Handled(); })
				[ Title(Name, 14, FLinearColor::White) ]
			]
		];
	}
	auto CurrentDef = [this]() -> const UValdoriaHeroDefinition*
	{
		const AValdoriaHeroCharacter* Hero = Cast<AValdoriaHeroCharacter>(GetPawn());
		return Hero ? Hero->GetHeroDefinition() : nullptr;
	};
	TSharedRef<SWidget> HeroesPage =
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)[ Title(LOCTEXT("HeroesTitle", "HERÓIS")) ]
			+ SVerticalBox::Slot().AutoHeight()[ HeroGrid ]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(24.f, 40.f, 0.f, 0.f)
		[
			SNew(SBox).WidthOverride(380.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
					.Text_Lambda([CurrentDef]() { const UValdoriaHeroDefinition* D = CurrentDef(); return D ? D->DisplayName : FText(); })
					.ColorAndOpacity_Lambda([CurrentDef]() { const UValdoriaHeroDefinition* D = CurrentDef(); return FSlateColor(D ? D->ThemeColor : Gold); })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 8.f)
				[
					SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 12)).ColorAndOpacity(Soft)
					.Text_Lambda([CurrentDef]() { const UValdoriaHeroDefinition* D = CurrentDef(); return D ? RolesText(D) : FText(); })
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 12)).ColorAndOpacity(FLinearColor::White).AutoWrapText(true)
					.Text_Lambda([CurrentDef]() { const UValdoriaHeroDefinition* D = CurrentDef(); return D ? D->Description : FText(); })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
				[
					SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 11)).ColorAndOpacity(Soft).AutoWrapText(true)
					.Text_Lambda([CurrentDef]()
					{
						const UValdoriaHeroDefinition* D = CurrentDef();
						if (!D) { return FText(); }
						return FText::Format(LOCTEXT("HeroStats", "Vida {0}   Armadura {1}   Escudo {2}   Velocidade {3}\nPassiva: {4}\nVeja todas as habilidades no jogo segurando K."),
							FText::AsNumber(FMath::RoundToInt(D->MaxHealth)), FText::AsNumber(FMath::RoundToInt(D->MaxArmor)),
							FText::AsNumber(FMath::RoundToInt(D->MaxShield)), FText::AsNumber(FMath::RoundToInt(D->MoveSpeed)), D->PassiveName);
					})
				]
			]
		];

	// ---- page 2: LOJA (skins coming soon)
	TSharedRef<SUniformGridPanel> SkinGrid = SNew(SUniformGridPanel).SlotPadding(FMargin(6.f));
	int32 Cell = 0;
	for (const FName& Id : UValdoriaHeroDefinition::GetPresetIds())
	{
		if (Id == TEXT("ariel_faisca")) { continue; }
		const UValdoriaHeroDefinition* Def = FindHero(Id);
		const FLinearColor Color = Def ? Def->ThemeColor : FLinearColor::Gray;
		SkinGrid->AddSlot(Cell % 4, Cell / 4)
		[
			SNew(SBox).WidthOverride(150.f).HeightOverride(110.f)
			[
				SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Color * FLinearColor(0.35f, 0.35f, 0.35f, 0.9f))
				.Padding(8.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[ Title(Def ? Def->DisplayName : FText::FromName(Id), 14, FLinearColor::White) ]
					+ SVerticalBox::Slot().AutoHeight()[ Body(LOCTEXT("SkinSoon", "Visual alternativo"), 11) ]
					+ SVerticalBox::Slot().FillHeight(1.f).VAlign(VAlign_Bottom)[ Title(LOCTEXT("SkinLocked", "EM BREVE"), 13, Gold) ]
				]
			]
		];
		++Cell;
	}
	TSharedRef<SWidget> ShopPage =
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ Title(LOCTEXT("ShopTitle", "LOJA DE VISUAIS")) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 12.f)
		[ Body(LOCTEXT("ShopDesc", "Os visuais (skins) ainda não estão disponíveis. Quando chegarem, aparecem aqui com o preço; os itens de partida continuam no B, dentro do jogo.")) ]
		+ SVerticalBox::Slot().AutoHeight()[ SkinGrid ];

	// ---- page 3: INVENTÁRIO
	TSharedRef<SVerticalBox> InvList = SNew(SVerticalBox);
	for (const FText& Category : { LOCTEXT("InvSkins", "Visuais"), LOCTEXT("InvEmotes", "Gestos"), LOCTEXT("InvIcons", "Ícones de perfil"), LOCTEXT("InvSprays", "Grafites") })
	{
		InvList->AddSlot().AutoHeight().Padding(0.f, 5.f)
		[
			SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.12f, 0.13f, 0.17f, 0.9f)).Padding(10.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f)[ Title(Category, 14, FLinearColor::White) ]
				+ SHorizontalBox::Slot().AutoWidth()[ Body(LOCTEXT("InvEmpty", "nada aqui ainda"), 12) ]
			]
		];
	}
	TSharedRef<SWidget> InventoryPage =
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)[ Title(LOCTEXT("InvTitle", "INVENTÁRIO")) ]
		+ SVerticalBox::Slot().AutoHeight()[ SNew(SBox).WidthOverride(560.f)[ InvList ] ];

	TSharedRef<SWidgetSwitcher> Pages = SNew(SWidgetSwitcher)
		+ SWidgetSwitcher::Slot()[ PlayPage ]
		+ SWidgetSwitcher::Slot()[ HeroesPage ]
		+ SWidgetSwitcher::Slot()[ ShopPage ]
		+ SWidgetSwitcher::Slot()[ InventoryPage ];
	LobbyPages = Pages;

	// ---- left menu
	auto Tab = [this](const FText& Label, int32 Page)
	{
		return SNew(SBox).WidthOverride(230.f).HeightOverride(50.f)
		[
			SNew(SButton).HAlign(HAlign_Left).VAlign(VAlign_Center)
			.ButtonColorAndOpacity_Lambda([this, Page]()
			{
				const TSharedPtr<SWidgetSwitcher> P = LobbyPages.Pin();
				return P.IsValid() && P->GetActiveWidgetIndex() == Page ? FLinearColor(0.85f, 0.6f, 0.2f) : FLinearColor(0.15f, 0.16f, 0.21f);
			})
			.OnClicked_Lambda([this, Page]()
			{
				if (const TSharedPtr<SWidgetSwitcher> P = LobbyPages.Pin()) { P->SetActiveWidgetIndex(Page); }
				ValdoriaAudio::Play2D(this, ValdoriaAudio::Load(TEXT("UI"), TEXT("click_001")), 0.4f);
				return FReply::Handled();
			})
			[ SNew(STextBlock).Text(Label).Font(FCoreStyle::GetDefaultFontStyle("Bold", 16)).ColorAndOpacity(FLinearColor::White) ]
		];
	};
	auto Action = [](const FText& Label, TFunction<void()> OnClick)
	{
		return SNew(SBox).WidthOverride(230.f).HeightOverride(44.f)
		[
			SNew(SButton).HAlign(HAlign_Left).VAlign(VAlign_Center).ButtonColorAndOpacity(FLinearColor(0.1f, 0.11f, 0.15f))
			.OnClicked_Lambda([OnClick]() { OnClick(); return FReply::Handled(); })
			[ SNew(STextBlock).Text(Label).Font(FCoreStyle::GetDefaultFontStyle("Bold", 14)).ColorAndOpacity(Soft) ]
		];
	};

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.02f, 0.55f))
		.Padding(48.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 2.f)[ Title(LOCTEXT("GameTitle", "ECHOES OF VALDORIA"), 30) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 22.f)
				[
					SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 12)).ColorAndOpacity(Soft)
					.Text_Lambda([this]() { return FText::Format(LOCTEXT("Credits", "Créditos de partida: {0}"), FText::AsNumber(GetCredits())); })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)[ Tab(LOCTEXT("TabPlay", "JOGAR"), 0) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)[ Tab(LOCTEXT("TabHeroes", "HERÓIS"), 1) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)[ Tab(LOCTEXT("TabShop", "LOJA"), 2) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)[ Tab(LOCTEXT("TabInventory", "INVENTÁRIO"), 3) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 18.f, 0.f, 4.f)
				[ Action(LOCTEXT("TabTraining", "TREINO"), [this]() { LobbyMode = 0; LobbyPlay(); }) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
				[ Action(LOCTEXT("TabSettings", "CONFIGURAÇÕES"), [this]() { OpenSettings(); }) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
				[ Action(LOCTEXT("TabQuit", "SAIR DO JOGO"), [this]() { UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false); }) ]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(36.f, 0.f, 0.f, 0.f).VAlign(VAlign_Top)
			[
				SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Panel).Padding(24.f)
				[
					SNew(SScrollBox) + SScrollBox::Slot()[ Pages ]
				]
			]
		];
}

#undef LOCTEXT_NAMESPACE
