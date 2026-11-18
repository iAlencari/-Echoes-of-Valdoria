// Echoes of Valdoria - controle do jogador: mouse, menu de pausa (Esc), Alt para soltar o
// cursor e painel de administrador (F1).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ValdoriaPlayerController.generated.h"

class SWidget;
class AValdoriaHeroCharacter;
class UValdoriaHeroDefinition;

UCLASS()
class ECHOESOFVALDORIA_API AValdoriaPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	bool IsPauseMenuOpen() const { return PauseWidget.IsValid(); }
	bool IsAdminPanelOpen() const { return AdminWidget.IsValid(); }
	bool IsChatOpen() const { return ChatWidget.IsValid(); }
	bool IsAdminMode() const { return bAdminMode; }
	void DebugSetAdminMode(bool bOn) { bAdminMode = bOn; }
	void DebugOpenSettings() { OpenSettings(); }

	/** Esc: abre/fecha o menu de pausa. */
	void TogglePauseMenu();

	/** F1: abre/fecha o painel de administrador (o jogo continua rodando). */
	void ToggleAdminPanel();

	/** V: alterna câmera em primeira / terceira pessoa. */
	void ToggleCameraMode();

	/** Tela de ajuda das habilidades (segurar K). */
	void HelpPressed();
	void HelpReleased();
	bool IsHelpOpen() const { return HelpWidget.IsValid(); }

	/** Troca o herói do jogador ("brasa", "kael", "iara", "vex" ou "recruta"). Usa o Data Asset
	 *  de Content/Valdoria/Heroes; se ainda não existir, cria um igual em memória. */
	bool SelectHero(FName HeroId);
	UValdoriaHeroDefinition* FindHero(FName HeroId);

	// --- Loja (tecla B, ValdoriaShop.cpp)
	void ToggleShop();
	bool IsShopOpen() const { return ShopWidget.IsValid(); }
	/** Compra o próximo nível do item. Retorna falso se faltar crédito ou espaço. */
	bool BuyItem(FName ItemId);
	void AddCredits(float Amount);
	int32 GetCredits() const { return FMath::Max(0, FMath::FloorToInt(Credits)); }
	int32 GetItemLevel(FName ItemId) const { const int32* L = ShopLevels.Find(ItemId); return L ? *L : 0; }
	void OnHeroDamageDealt(float Damage, bool bKill);
	/** Reaplica os bônus comprados no herói atual (ao comprar, trocar de herói e renascer). */
	void ApplyShopToHero();
	class UValdoriaShopCatalog* GetShopCatalog();

	/** Inimigo/personagem sob a mira (o que o painel edita). */
	AValdoriaHeroCharacter* PickAimedCharacter() const;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void OpenPauseMenu();
	void ClosePauseMenu();

	/** Segurar Alt: solta o cursor (para arrastar a janela ou usar outro programa). */
	void FreeCursorPressed();
	void FreeCursorReleased();

	void ApplyGameInput();
	void ApplyCursorInput();
	void RefreshInputMode();

	FReply OnResumeClicked();
	FReply OnRestartClicked();
	FReply OnQuitClicked();

	// --- Painel de administrador (ValdoriaAdminPanel.cpp)
	void OpenAdminPanel();
	void CloseAdminPanel();
	TSharedRef<SWidget> BuildAdminPanel();
	AValdoriaHeroCharacter* GetHero() const;
	FVector GetAimPoint() const;
	void AdminTeleport(const FVector& Location);
	void AdminSetTargetAttribute(int32 Which, float Value);

	TSharedRef<SWidget> BuildHelpScreen();
	TSharedPtr<SWidget> HelpWidget;

	// --- Configurações (ValdoriaSettingsMenu.cpp)
	void OpenSettings();
	void CloseSettings();
	TSharedRef<SWidget> BuildSettingsScreen();
	void ApplyOptionsToGame();
	TSharedPtr<SWidget> SettingsWidget;

	// --- Chat (Enter) com comandos: /adm, /ajuda... (ValdoriaChat.cpp)
public:
	void OpenChat();
protected:
	void RegisterChatKey();
	void UnregisterChatKey();
	TSharedPtr<class IInputProcessor> ChatKeyProcessor;
	void CloseChat();
	void SubmitChat(const FString& Text);
	void RunChatCommand(const FString& Command);
	void ChatPrint(const FString& Line, const FLinearColor& Color = FLinearColor(0.8f, 0.9f, 1.f));
	TSharedPtr<SWidget> ChatWidget;
	TSharedPtr<class SEditableTextBox> ChatBox;

	/** Modo administrador: ligado pelo menu de pausa ou por /adm; só com ele o F1 abre o painel. */
	bool bAdminMode = false;

	TSharedPtr<SWidget> PauseWidget;
	TSharedPtr<SWidget> AdminWidget;
	TWeakObjectPtr<AValdoriaHeroCharacter> AdminTarget;
	TArray<TWeakObjectPtr<UValdoriaHeroDefinition>> AdminHeroList;
	TArray<FString> AdminMapList;
	FVector SavedPositions[3];
	FRotator SavedRotations[3];
	bool bSavedPosition[3] = { false, false, false };
	int32 AdminBotDifficulty = 0;
	bool bAltHeld = false;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UValdoriaHeroDefinition>> HeroCache;

	UPROPERTY(Transient)
	TObjectPtr<class UValdoriaShopCatalog> ShopCatalog;
	TMap<FName, int32> ShopLevels;
	float Credits = -1.f;
	TSharedPtr<SWidget> ShopWidget;
	TSharedRef<SWidget> BuildShopScreen();
	void CloseShop();
	FTimerHandle CreditTimer;
	virtual void OnPossess(APawn* InPawn) override;
};
