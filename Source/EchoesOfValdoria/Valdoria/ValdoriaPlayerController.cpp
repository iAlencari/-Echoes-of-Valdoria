// Echoes of Valdoria - controle do jogador.

#include "ValdoriaPlayerController.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdoriaPause"

void AValdoriaPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		ApplyGameInput();
		ApplyOptionsToGame();
	}
}

void AValdoriaPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClosePauseMenu();
	CloseAdminPanel();
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
	FInputKeyBinding& Admin = InputComponent->BindKey(EKeys::F1, IE_Pressed, this, &AValdoriaPlayerController::ToggleAdminPanel);
	Admin.bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AValdoriaPlayerController::OpenChat);
	InputComponent->BindKey(EKeys::K, IE_Pressed, this, &AValdoriaPlayerController::HelpPressed);
	InputComponent->BindKey(EKeys::K, IE_Released, this, &AValdoriaPlayerController::HelpReleased);
}

void AValdoriaPlayerController::RefreshInputMode()
{
	// Cursor solto só com menus que pedem clique (pausa, configurações, ajuda, chat) ou com Alt.
	// O painel de admin NÃO prende o mouse: dá para mirar e andar com ele aberto; Alt para clicar nele.
	if (PauseWidget.IsValid() || HelpWidget.IsValid() || SettingsWidget.IsValid() || ChatWidget.IsValid() || bAltHeld)
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
	else if (AdminWidget.IsValid())
	{
		Mode.SetWidgetToFocus(AdminWidget);
	}
	SetInputMode(Mode);
	bShowMouseCursor = true;
}

void AValdoriaPlayerController::FreeCursorPressed()
{
	bAltHeld = true;
	ApplyCursorInput();
}

void AValdoriaPlayerController::FreeCursorReleased()
{
	bAltHeld = false;
	RefreshInputMode();
}

void AValdoriaPlayerController::TogglePauseMenu()
{
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
							ClosePauseMenu();
							OpenAdminPanel();
						}
						else
						{
							CloseAdminPanel();
						}
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
						.Text_Lambda([this]() { return bAdminMode ? LOCTEXT("AdminOn", "Modo administrador: LIGADO (F1)") : LOCTEXT("AdminOff", "Modo administrador: desligado"); })
					]
				]
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
