// Echoes of Valdoria - chat do jogo (Enter) com comandos de teste.
// Hoje é local; com multiplayer as mensagens e comandos passam pelo servidor.

#include "ValdoriaPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"
#include "ValdoriaHUD.h"
#include "ValdoriaHeroCharacter.h"
#include "ValdoriaTrainingDummy.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"

#include "Framework/Application/IInputProcessor.h"

#define LOCTEXT_NAMESPACE "ValdoriaChat"

/** Pega o Enter antes de qualquer widget (o Slate usava o Enter para "confirmar" e ele nunca
 *  chegava ao jogo). Só age quando o chat está fechado e não há menu aberto. */
class FValdoriaChatKeyProcessor : public IInputProcessor
{
public:
	explicit FValdoriaChatKeyProcessor(AValdoriaPlayerController* InPC) : PC(InPC) {}
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		if (InKeyEvent.GetKey() == EKeys::Enter && PC.IsValid() && !PC->IsChatOpen() && !PC->IsPauseMenuOpen())
		{
			PC->OpenChat();
			return true;
		}
		return false;
	}
	virtual const TCHAR* GetDebugName() const override { return TEXT("ValdoriaChat"); }
	TWeakObjectPtr<AValdoriaPlayerController> PC;
};

void AValdoriaPlayerController::RegisterChatKey()
{
	if (FSlateApplication::IsInitialized() && !ChatKeyProcessor.IsValid())
	{
		ChatKeyProcessor = MakeShared<FValdoriaChatKeyProcessor>(this);
		FSlateApplication::Get().RegisterInputPreProcessor(ChatKeyProcessor);
	}
}

void AValdoriaPlayerController::UnregisterChatKey()
{
	if (FSlateApplication::IsInitialized() && ChatKeyProcessor.IsValid())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(ChatKeyProcessor);
	}
	ChatKeyProcessor.Reset();
}

void AValdoriaPlayerController::ChatPrint(const FString& Line, const FLinearColor& Color)
{
	if (AValdoriaHUD* HUD = Cast<AValdoriaHUD>(GetHUD()))
	{
		HUD->AddChatLine(Line, Color);
	}
}

void AValdoriaPlayerController::OpenChat()
{
	if (ChatWidget.IsValid() || PauseWidget.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	TWeakObjectPtr<AValdoriaPlayerController> WeakThis(this);
	ChatWidget = SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(48.f, 0.f, 0.f, 290.f))
		[
			SNew(SBox).WidthOverride(560.f)
			[
				SAssignNew(ChatBox, SEditableTextBox)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 13))
				.HintText(LOCTEXT("Hint", "Digite uma mensagem ou /ajuda  (Enter envia, Esc cancela)"))
				.ClearKeyboardFocusOnCommit(false)
				.OnTextCommitted_Lambda([WeakThis](const FText& Text, ETextCommit::Type Type)
				{
					if (!WeakThis.IsValid())
					{
						return;
					}
					// Só fecha ao apertar Enter (envia) ou Esc (cancela). Perder o foco (ex.: o jogo
					// mudando o modo de entrada) fechava o chat logo depois de abrir.
					if (Type == ETextCommit::OnEnter)
					{
						WeakThis->SubmitChat(Text.ToString());
						WeakThis->CloseChat();
					}
					else if (Type == ETextCommit::OnCleared)
					{
						WeakThis->CloseChat();
					}
				})
			]
		];
	GEngine->GameViewport->AddViewportWidgetContent(ChatWidget.ToSharedRef(), 120);
	if (AValdoriaHUD* HUD = Cast<AValdoriaHUD>(GetHUD()))
	{
		HUD->bChatOpen = true;
	}
	RefreshInputMode();
	FSlateApplication::Get().SetKeyboardFocus(ChatBox, EFocusCause::SetDirectly);
}

void AValdoriaPlayerController::CloseChat()
{
	if (!ChatWidget.IsValid())
	{
		return;
	}
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(ChatWidget.ToSharedRef());
	}
	ChatWidget.Reset();
	ChatBox.Reset();
	if (AValdoriaHUD* HUD = Cast<AValdoriaHUD>(GetHUD()))
	{
		HUD->bChatOpen = false;
	}
	RefreshInputMode();
}

void AValdoriaPlayerController::SubmitChat(const FString& Text)
{
	const FString Trimmed = Text.TrimStartAndEnd();
	if (Trimmed.IsEmpty())
	{
		return;
	}
	if (Trimmed.StartsWith(TEXT("/")))
	{
		ChatPrint(Trimmed, FLinearColor(0.7f, 0.7f, 0.75f));
		RunChatCommand(Trimmed.Mid(1));
		return;
	}
	ChatPrint(FString::Printf(TEXT("Você: %s"), *Trimmed), FLinearColor::White);
}

void AValdoriaPlayerController::RunChatCommand(const FString& Command)
{
	TArray<FString> Args;
	Command.ToLower().ParseIntoArrayWS(Args);
	if (Args.Num() == 0)
	{
		return;
	}
	const FString Cmd = Args[0];
	const FString Arg = Args.Num() > 1 ? Args[1] : FString();
	AValdoriaHeroCharacter* Hero = GetHero();
	const FLinearColor Ok(0.5f, 1.f, 0.6f);
	const FLinearColor Bad(1.f, 0.5f, 0.45f);

	if (Cmd == TEXT("adm") || Cmd == TEXT("admin"))
	{
		bAdminMode = !bAdminMode;
		if (bAdminMode)
		{
			OpenAdminPanel();
			ChatPrint(TEXT("Modo administrador LIGADO. F10 abre/fecha o painel; segure Alt para clicar nele."), Ok);
		}
		else
		{
			CloseAdminPanel();
			ChatPrint(TEXT("Modo administrador desligado."), Ok);
		}
	}
	else if (Cmd == TEXT("ajuda") || Cmd == TEXT("help"))
	{
		ChatPrint(TEXT("/adm  liga/desliga o modo administrador"), Ok);
		ChatPrint(TEXT("/heroi brasa | kael | iara | vex | recruta  troca de herói (também no Esc)"), Ok);
		ChatPrint(TEXT("/imortal  /recarga  /municao  /voar  (liga/desliga no seu herói)"), Ok);
		ChatPrint(TEXT("/bots 0-3  bonecos atiram (0 parados, 3 difícil)"), Ok);
		ChatPrint(TEXT("/dano N  tira N de vida de você  •  /vida  enche a vida  •  /ult  ultimate cheia"), Ok);
		ChatPrint(TEXT("/mapa np | treino  troca de mapa  •  /tempo N  velocidade do jogo"), Ok);
	}
	else if (Cmd == TEXT("heroi") || Cmd == TEXT("herói") || Cmd == TEXT("hero"))
	{
		if (!Arg.IsEmpty() && SelectHero(FName(*Arg)))
		{
			ChatPrint(FString::Printf(TEXT("Herói trocado: %s"), *Arg), Ok);
		}
		else
		{
			ChatPrint(TEXT("Heróis: /heroi brasa | kael | iara | vex | recruta"), Bad);
		}
	}
	else if (!bAdminMode)
	{
		ChatPrint(TEXT("Esse comando precisa do modo administrador: digite /adm"), Bad);
	}
	else if (!Hero)
	{
		ChatPrint(TEXT("Sem herói agora."), Bad);
	}
	else if (Cmd == TEXT("imortal"))
	{
		Hero->bGodMode = !Hero->bGodMode;
		ChatPrint(Hero->bGodMode ? TEXT("Imortal ligado.") : TEXT("Imortal desligado."), Ok);
	}
	else if (Cmd == TEXT("recarga"))
	{
		Hero->bNoCooldowns = !Hero->bNoCooldowns;
		ChatPrint(Hero->bNoCooldowns ? TEXT("Sem recarga ligado.") : TEXT("Recargas normais."), Ok);
	}
	else if (Cmd == TEXT("municao"))
	{
		Hero->bInfiniteAmmo = !Hero->bInfiniteAmmo;
		ChatPrint(Hero->bInfiniteAmmo ? TEXT("Munição infinita ligada.") : TEXT("Munição normal."), Ok);
	}
	else if (Cmd == TEXT("voar") || Cmd == TEXT("noclip"))
	{
		Hero->SetNoclip(!Hero->IsNoclip());
		ChatPrint(Hero->IsNoclip() ? TEXT("Voando (noclip).") : TEXT("Andando normal."), Ok);
	}
	else if (Cmd == TEXT("bots"))
	{
		const int32 Level = FMath::Clamp(FCString::Atoi(*Arg), 0, 3);
		for (TActorIterator<AValdoriaTrainingDummy> It(GetWorld()); It; ++It)
		{
			It->BotDifficulty = Level;
		}
		AdminBotDifficulty = Level;
		ChatPrint(FString::Printf(TEXT("Bots no nível %d."), Level), Ok);
	}
	else if (Cmd == TEXT("dano"))
	{
		const float Amount = Arg.IsEmpty() ? 50.f : FCString::Atof(*Arg);
		const bool bGod = Hero->bGodMode;
		Hero->bGodMode = false;
		Hero->DealDamageTo(Hero, Amount);
		Hero->bGodMode = bGod;
		ChatPrint(FString::Printf(TEXT("Você levou %.0f de dano."), Amount), Ok);
	}
	else if (Cmd == TEXT("vida"))
	{
		Hero->AdminRestoreAll();
		ChatPrint(TEXT("Vida cheia."), Ok);
	}
	else if (Cmd == TEXT("ult"))
	{
		Hero->AdminFillUltimate();
		ChatPrint(TEXT("Ultimate cheia."), Ok);
	}
	else if (Cmd == TEXT("tempo"))
	{
		const float Scale = FMath::Clamp(Arg.IsEmpty() ? 1.f : FCString::Atof(*Arg), 0.05f, 3.f);
		UGameplayStatics::SetGlobalTimeDilation(this, Scale);
		ChatPrint(FString::Printf(TEXT("Velocidade do jogo: %.2f"), Scale), Ok);
	}
	else if (Cmd == TEXT("mapa"))
	{
		if (Arg == TEXT("np") || Arg == TEXT("novapauliceia"))
		{
			UGameplayStatics::OpenLevel(this, TEXT("/Game/Maps/NP_NovaPauliceia"));
		}
		else if (Arg == TEXT("treino") || Arg.IsEmpty())
		{
			UGameplayStatics::OpenLevel(this, TEXT("/Game/ThirdPerson/Lvl_ThirdPerson"));
		}
		else
		{
			ChatPrint(TEXT("Mapas: /mapa np  ou  /mapa treino"), Bad);
		}
	}
	else
	{
		ChatPrint(FString::Printf(TEXT("Comando desconhecido: /%s  (veja /ajuda)"), *Cmd), Bad);
	}
}

#undef LOCTEXT_NAMESPACE
