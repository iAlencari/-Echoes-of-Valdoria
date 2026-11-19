// Echoes of Valdoria - controle do jogador.

#include "ValdoriaPlayerController.h"
#include "ValdoriaHUD.h"
#include "ValdoriaHeroCharacter.h"
#include "ValdoriaHeroDefinition.h"
#include "ValdoriaAudio.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/NavigationConfig.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdoriaPause"

static void SetAdminMaxLoadout(AValdoriaHeroCharacter* Hero, bool bOn)
{
	// Admin mode: any hero starts with no limits (god, no cooldowns, ammo, dashes, flight fuel, speed x3).
	// Noclip was removed on purpose (it also fell through the floor). Each toggle can be switched off in the
	// panel and stays as chosen when the hero is switched.
	Hero->bGodMode = bOn;
	Hero->bNoCooldowns = bOn;
	Hero->bInfiniteAmmo = bOn;
	Hero->bInfiniteDashes = bOn;
	Hero->bInfiniteFlight = bOn;
	Hero->AdminSetSpeedMultiplier(bOn ? 3.f : 1.f);
	if (!bOn)
	{
		Hero->bAdminFlight = false;
		if (Hero->IsNoclip()) { Hero->SetNoclip(false); }
	}
	if (bOn)
	{
		Hero->AdminRestoreAll();
		Hero->AdminFillUltimate();
	}
}

void AValdoriaPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	AValdoriaHeroCharacter* Hero = GetHero();
	const bool bWant = bAdminMode && Hero && Hero->HasAuthority();
	if (bWant && AdminMaxHero.Get() != Hero)
	{
		if (AdminMaxHero.IsValid())
		{
			SetAdminMaxLoadout(AdminMaxHero.Get(), false);
		}
		SetAdminMaxLoadout(Hero, true);
		AdminMaxHero = Hero;
	}
	else if (!bWant && AdminMaxHero.IsValid())
	{
		SetAdminMaxLoadout(AdminMaxHero.Get(), false);
		AdminMaxHero.Reset();
	}
}

void AValdoriaPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		ApplyGameInput();
		ApplyOptionsToGame();
		// Enter abre o chat (capturado antes do Slate, que engolia a tecla).
		RegisterChatKey();
		MaybeOpenLobbyAtStart();
	}
}

void AValdoriaPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CloseLobby();
	ClosePauseMenu();
	CloseAdminPanel();
	UnregisterChatKey();
	CloseShop();
	Super::EndPlay(EndPlayReason);
}

void AValdoriaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	// Teclas do sistema (funcionam mesmo com o jogo pausado).
	FInputKeyBinding& Esc = InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AValdoriaPlayerController::TogglePauseMenu);
	Esc.bExecuteWhenPaused = true;
	FInputKeyBinding& AltDown = InputComponent->BindKey(EKeys::LeftAlt, IE_Pressed, this, &AValdoriaPlayerController::FreeCursorPressed);
	AltDown.bExecuteWhenPaused = true;
	FInputKeyBinding& AltUp = InputComponent->BindKey(EKeys::LeftAlt, IE_Released, this, &AValdoriaPlayerController::FreeCursorReleased);
	AltUp.bExecuteWhenPaused = true;
	FInputKeyBinding& Admin = InputComponent->BindKey(EKeys::F10, IE_Pressed, this, &AValdoriaPlayerController::ToggleAdminPanel);
	Admin.bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AValdoriaPlayerController::OpenChat);
	InputComponent->BindKey(EKeys::MiddleMouseButton, IE_Pressed, this, &AValdoriaPlayerController::ToggleCameraMode); // owner (turn 47): mouse wheel click; F keys are taken by the official client
	InputComponent->BindKey(EKeys::V, IE_Pressed, this, &AValdoriaPlayerController::MeleePressed);
	InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AValdoriaPlayerController::ScoreboardPressed);
	InputComponent->BindKey(EKeys::Tab, IE_Released, this, &AValdoriaPlayerController::ScoreboardReleased);
	InputComponent->BindKey(EKeys::Y, IE_Pressed, this, &AValdoriaPlayerController::RecapPressed);
	InputComponent->BindKey(EKeys::Y, IE_Released, this, &AValdoriaPlayerController::RecapReleased);
	InputComponent->BindKey(EKeys::B, IE_Pressed, this, &AValdoriaPlayerController::ToggleShop);
	InputComponent->BindKey(EKeys::K, IE_Pressed, this, &AValdoriaPlayerController::HelpPressed);
	InputComponent->BindKey(EKeys::K, IE_Released, this, &AValdoriaPlayerController::HelpReleased);
}

void AValdoriaPlayerController::RefreshInputMode()
{
	// Cursor solto só com menus que pedem clique (pausa, configurações, ajuda, chat) ou com Alt.
	// O painel de admin NÃO prende o mouse: dá para mirar e andar com ele aberto; Alt para clicar nele.
	if (PauseWidget.IsValid() || LobbyWidget.IsValid() || HelpWidget.IsValid() || SettingsWidget.IsValid() || ChatWidget.IsValid() || ShopWidget.IsValid() || bAltHeld)
	{
		ApplyCursorInput();
	}
	else
	{
		ApplyGameInput();
	}
}

void AValdoriaPlayerController::ApplyGameInput()
{
	// Mouse preso no centro e escondido durante o jogo.
	FInputModeGameOnly Mode;
	Mode.SetConsumeCaptureMouseDown(false);
	SetInputMode(Mode);
	bShowMouseCursor = false;
}

void AValdoriaPlayerController::ApplyCursorInput()
{
	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	if (ChatBox.IsValid() && ChatWidget.IsValid())
	{
		Mode.SetWidgetToFocus(ChatBox);
	}
	else if (PauseWidget.IsValid())
	{
		Mode.SetWidgetToFocus(PauseWidget);
	}
	else if (LobbyWidget.IsValid())
	{
		Mode.SetWidgetToFocus(LobbyWidget);
	}
	else if (AdminWidget.IsValid())
	{
		Mode.SetWidgetToFocus(AdminWidget);
	}
	SetInputMode(Mode);
	bShowMouseCursor = true;
}

void AValdoriaPlayerController::FreeCursorPressed()
{
	// Turn 46 (owner): a quick Alt tap shows/hides the allies' cooldowns without flashing the cursor; holding Alt
	// still frees the cursor (admin panel), but only after 0.25 s.
	AltPressTime = GetWorld()->GetTimeSeconds();
	GetWorldTimerManager().SetTimer(AltHoldTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		bAltHeld = true;
		ApplyCursorInput();
	}), 0.25f, false);
}

void AValdoriaPlayerController::FreeCursorReleased()
{
	GetWorldTimerManager().ClearTimer(AltHoldTimer);
	if (!bAltHeld && GetWorld()->GetTimeSeconds() - AltPressTime < 0.25f)
	{
		if (AValdoriaHUD* H = Cast<AValdoriaHUD>(GetHUD()))
		{
			H->bShowAllyCooldowns = !H->bShowAllyCooldowns;
		}
		return;
	}
	bAltHeld = false;
	RefreshInputMode();
}

void AValdoriaPlayerController::TogglePauseMenu()
{
	if (ShopWidget.IsValid())
	{
		CloseShop();
		return;
	}
	if (ChatWidget.IsValid())
	{
		CloseChat();
		return;
	}
	if (SettingsWidget.IsValid())
	{
		CloseSettings();
		return;
	}
	if (LobbyWidget.IsValid())
	{
		return; // the lobby is the menu: Esc does nothing there
	}
	if (PauseWidget.IsValid())
	{
		ClosePauseMenu();
	}
	else
	{
		OpenPauseMenu();
	}
}

static TSharedRef<SWidget> MakeMenuButton(const FText& Label, FOnClicked OnClicked)
{
	return SNew(SBox)
		.WidthOverride(320.f)
		.HeightOverride(52.f)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.OnClicked(OnClicked)
			[
				SNew(STextBlock)
				.Text(Label)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
			]
		];
}

void AValdoriaPlayerController::OpenPauseMenu()
{
	if (PauseWidget.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	// Escolha de herói: um botão por herói pronto, na cor dele.
	TSharedRef<SHorizontalBox> HeroRow = SNew(SHorizontalBox);
	struct FHeroButton { const TCHAR* Id; FText Name; FLinearColor Color; };
	const FHeroButton HeroButtons[] = {
		{ TEXT("brasa"), LOCTEXT("PickBrasa", "Brasa"), FLinearColor(1.f, 0.45f, 0.1f) },
		{ TEXT("kael"), LOCTEXT("PickKael", "Kael"), FLinearColor(0.15f, 0.75f, 0.7f) },
		{ TEXT("iara"), LOCTEXT("PickIara", "Iara"), FLinearColor(0.2f, 0.85f, 0.8f) },
		{ TEXT("vex"), LOCTEXT("PickVex", "Vex"), FLinearColor(0.6f, 0.25f, 0.95f) },
		{ TEXT("lume"), LOCTEXT("PickLume", "Lume"), FLinearColor(1.f, 0.8f, 0.3f) },
		{ TEXT("kay"), LOCTEXT("PickKay", "Kay"), FLinearColor(0.75f, 0.75f, 0.85f) },
		{ TEXT("ariel"), LOCTEXT("PickAriel", "Ariel"), FLinearColor(0.85f, 0.5f, 0.2f) },
		{ TEXT("recruta"), LOCTEXT("PickRecruit", "Recruta"), FLinearColor(0.8f, 0.8f, 0.8f) },
	};
	for (const FHeroButton& B : HeroButtons)
	{
		const FName Id(B.Id);
		HeroRow->AddSlot().AutoWidth().Padding(4.f, 0.f)
		[
			SNew(SBox).WidthOverride(110.f).HeightOverride(44.f)
			[
				SNew(SButton).HAlign(HAlign_Center).VAlign(VAlign_Center).ButtonColorAndOpacity(B.Color * 0.6f)
				.OnClicked_Lambda([this, Id]() { SelectHero(Id); ClosePauseMenu(); return FReply::Handled(); })
				[
					SNew(STextBlock).Text(B.Name).Font(FCoreStyle::GetDefaultFontStyle("Bold", 14)).ColorAndOpacity(FLinearColor::White)
				]
			]
		];
	}

	PauseWidget =
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.02f, 0.65f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 24.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Title", "PAUSA"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 40))
				.ColorAndOpacity(FLinearColor(0.97f, 0.83f, 0.48f))
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(STextBlock).Text(LOCTEXT("PickHero", "ESCOLHER HERÓI  (ou /heroi nome no chat)"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12)).ColorAndOpacity(FLinearColor(0.75f, 0.78f, 0.85f))
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 18.f)
			[
				HeroRow
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)
			[
				MakeMenuButton(LOCTEXT("Resume", "Continuar"), FOnClicked::CreateUObject(this, &AValdoriaPlayerController::OnResumeClicked))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)
			[
				MakeMenuButton(LOCTEXT("Settings", "Configurações"), FOnClicked::CreateLambda([this]() { OpenSettings(); return FReply::Handled(); }))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)
			[
				SNew(SBox).WidthOverride(320.f).HeightOverride(52.f)
				[
					SNew(SButton).HAlign(HAlign_Center).VAlign(VAlign_Center)
					.OnClicked_Lambda([this]()
					{
						bAdminMode = !bAdminMode;
						if (bAdminMode)
						{
							ClosePauseMenu(); // F10 opens the panel; admin commands work without it
						}
						else
						{
							CloseAdminPanel();
						}
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
						.Text_Lambda([this]() { return bAdminMode ? LOCTEXT("AdminOn", "Modo administrador: LIGADO (F10)") : LOCTEXT("AdminOff", "Modo administrador: desligado"); })
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)
			[
				MakeMenuButton(LOCTEXT("Lobby", "Voltar ao lobby"), FOnClicked::CreateLambda([this]() { ClosePauseMenu(); OpenLobby(); return FReply::Handled(); }))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)
			[
				MakeMenuButton(LOCTEXT("Restart", "Reiniciar treino"), FOnClicked::CreateUObject(this, &AValdoriaPlayerController::OnRestartClicked))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)
			[
				MakeMenuButton(LOCTEXT("Quit", "Sair do jogo"), FOnClicked::CreateUObject(this, &AValdoriaPlayerController::OnQuitClicked))
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 18.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Hint", "Esc volta ao jogo  •  segure Alt para soltar o mouse"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
				.ColorAndOpacity(FLinearColor(0.7f, 0.72f, 0.78f))
			]
		];
	GEngine->GameViewport->AddViewportWidgetContent(PauseWidget.ToSharedRef(), 100);
	ValdoriaAudio::Play2D(this, ValdoriaAudio::Load(TEXT("UI"), TEXT("open_001")), 0.6f);
	SetPause(true);
	ApplyCursorInput();
}

void AValdoriaPlayerController::ClosePauseMenu()
{
	if (!PauseWidget.IsValid())
	{
		return;
	}
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(PauseWidget.ToSharedRef());
	}
	PauseWidget.Reset();
	SetPause(false);
	RefreshInputMode();
}

FReply AValdoriaPlayerController::OnResumeClicked()
{
	ClosePauseMenu();
	return FReply::Handled();
}

FReply AValdoriaPlayerController::OnRestartClicked()
{
	ClosePauseMenu();
	ConsoleCommand(TEXT("RestartLevel"));
	return FReply::Handled();
}

FReply AValdoriaPlayerController::OnQuitClicked()
{
	ClosePauseMenu();
	UKismetSystemLibrary::QuitGame(GetWorld(), this, EQuitPreference::Quit, false);
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

void AValdoriaPlayerController::ToggleCameraMode()
{
	if (AValdoriaHeroCharacter* Hero = Cast<AValdoriaHeroCharacter>(GetPawn()))
	{
		Hero->SetFirstPerson(!Hero->IsFirstPerson());
	}
}

UValdoriaHeroDefinition* AValdoriaPlayerController::FindHero(FName HeroId)
{
	const FName Key(*HeroId.ToString().ToLower());
	if (TObjectPtr<UValdoriaHeroDefinition>* Cached = HeroCache.Find(Key))
	{
		return *Cached;
	}
	UValdoriaHeroDefinition* Found = nullptr;
	// 1) Data Asset do projeto com esse HeroId (o que o dono edita no editor).
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	TArray<FAssetData> Assets;
	Registry.GetAssetsByClass(UValdoriaHeroDefinition::StaticClass()->GetClassPathName(), Assets, true);
	for (const FAssetData& Data : Assets)
	{
		UValdoriaHeroDefinition* Def = Cast<UValdoriaHeroDefinition>(Data.GetAsset());
		if (Def && Def->HeroId.ToString().ToLower() == Key.ToString())
		{
			Found = Def;
			break;
		}
	}
	// 2) Sem arquivo ainda: cria em memória com os valores iniciais.
	if (!Found)
	{
		Found = UValdoriaHeroDefinition::CreatePreset(this, Key);
	}
	if (Found)
	{
		HeroCache.Add(Key, Found);
	}
	return Found;
}

bool AValdoriaPlayerController::SelectHero(FName HeroId)
{
	AValdoriaHeroCharacter* Hero = Cast<AValdoriaHeroCharacter>(GetPawn());
	if (!Hero)
	{
		return false;
	}
	if (HeroId == TEXT("recruta"))
	{
		Hero->SetHeroDefinition(nullptr);
		return true;
	}
	UValdoriaHeroDefinition* Def = FindHero(HeroId);
	if (!Def)
	{
		return false;
	}
	Hero->SetHeroDefinition(Def);
	ApplyShopToHero();
	ValdoriaAudio::Play2D(this, ValdoriaAudio::Load(TEXT("UI"), TEXT("select_001")), 0.7f);
	return true;
}

// ------------------------------------------------------------------ V punch and Tab scoreboard (turn 46)

void AValdoriaPlayerController::MeleePressed()
{
	if (AValdoriaHeroCharacter* Hero = Cast<AValdoriaHeroCharacter>(GetPawn()))
	{
		Hero->TryMeleePunch();
	}
}

void AValdoriaPlayerController::ScoreboardPressed()
{
	if (AValdoriaHUD* H = Cast<AValdoriaHUD>(GetHUD()))
	{
		H->bShowScoreboard = true;
	}
}

void AValdoriaPlayerController::ScoreboardReleased()
{
	if (AValdoriaHUD* H = Cast<AValdoriaHUD>(GetHUD()))
	{
		H->bShowScoreboard = false;
	}
}

void AValdoriaPlayerController::RecapPressed()
{
	if (AValdoriaHUD* H = Cast<AValdoriaHUD>(GetHUD()))
	{
		H->bShowDeathRecap = true;
	}
}

void AValdoriaPlayerController::RecapReleased()
{
	if (AValdoriaHUD* H = Cast<AValdoriaHUD>(GetHUD()))
	{
		H->bShowDeathRecap = false;
	}
}
