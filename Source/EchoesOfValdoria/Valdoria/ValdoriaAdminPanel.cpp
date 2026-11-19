// Echoes of Valdoria - painel de administrador (F1) e tela de ajuda das habilidades (K).
// Tudo em Slate/C++, sem assets. O jogo continua rodando com o painel aberto; o cursor fica solto.
// Obs.: hoje o jogo é local (o painel mexe direto no servidor local). Com multiplayer, estas
// ações passam a ser RPCs para o servidor.

#include "ValdoriaPlayerController.h"
#include "AbilitySystemComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMesh.h"
#include "Styling/CoreStyle.h"
#include "ValdoriaAttributeSet.h"
#include "ValdoriaGameMode.h"
#include "ValdoriaHUD.h"
#include "ValdoriaHeroCharacter.h"
#include "ValdoriaHeroDefinition.h"
#include "ValdoriaSettings.h"
#include "ValdoriaTrainingDummy.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdoriaAdmin"

namespace ValdoriaAdminUI
{
	static FSlateFontInfo Font(int32 Size, bool bBold = false)
	{
		return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
	}

	static TSharedRef<SWidget> Title(const FText& Text)
	{
		return SNew(SBox).Padding(FMargin(0.f, 12.f, 0.f, 4.f))
			[
				SNew(STextBlock).Text(Text).Font(Font(14, true)).ColorAndOpacity(FLinearColor(1.f, 0.62f, 0.4f))
			];
	}

	static TSharedRef<SWidget> Button(const FText& Text, TFunction<void()> OnClick)
	{
		return SNew(SBox).Padding(2.f).MinDesiredWidth(140.f)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([OnClick]() { OnClick(); return FReply::Handled(); })
				[
					SNew(STextBlock).Text(Text).Font(Font(11, true))
				]
			];
	}

	static TSharedRef<SWidget> Check(const FText& Text, TFunction<bool()> Get, TFunction<void(bool)> Set)
	{
		return SNew(SBox).Padding(2.f)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([Get]() { return Get() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([Set](ECheckBoxState State) { Set(State == ECheckBoxState::Checked); })
				[
					SNew(STextBlock).Text(Text).Font(Font(11))
				]
			];
	}

	static TSharedRef<SWidget> Number(const FText& Text, float Min, float Max, float Delta, TFunction<float()> Get, TFunction<void(float)> Set)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(2.f)
			[
				SNew(STextBlock).Text(Text).Font(Font(11))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)
			[
				SNew(SBox).WidthOverride(130.f)
				[
					SNew(SSpinBox<float>)
					.MinValue(Min).MaxValue(Max).Delta(Delta)
					.Value_Lambda([Get]() { return Get(); })
					.OnValueChanged_Lambda([Set](float V) { Set(V); }) // applies while dragging, not only on Enter
					.OnValueCommitted_Lambda([Set](float V, ETextCommit::Type) { Set(V); })
				]
			];
	}
}

// ------------------------------------------------------------------ utilidades

AValdoriaHeroCharacter* AValdoriaPlayerController::GetHero() const
{
	return Cast<AValdoriaHeroCharacter>(GetPawn());
}

FVector AValdoriaPlayerController::GetAimPoint() const
{
	FVector ViewLoc;
	FRotator ViewRot;
	GetPlayerViewPoint(ViewLoc, ViewRot);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ValdoriaAdminAim), false, GetPawn());
	FHitResult Hit;
	const FVector End = ViewLoc + ViewRot.Vector() * 20000.f;
	if (GetWorld()->LineTraceSingleByChannel(Hit, ViewLoc, End, ECC_Visibility, Params))
	{
		return Hit.ImpactPoint;
	}
	return End;
}

AValdoriaHeroCharacter* AValdoriaPlayerController::PickAimedCharacter() const
{
	FVector ViewLoc;
	FRotator ViewRot;
	GetPlayerViewPoint(ViewLoc, ViewRot);
	const FVector Dir = ViewRot.Vector();
	// 1) Raio exato pelo centro da tela.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ValdoriaAdminPick), false, GetPawn());
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, ViewLoc, ViewLoc + Dir * 20000.f, ECC_Visibility, Params))
	{
		if (AValdoriaHeroCharacter* Hero = Cast<AValdoriaHeroCharacter>(Hit.GetActor()))
		{
			return Hero;
		}
	}
	// 2) Senão, o personagem mais alinhado com a mira (cone de ~10°).
	AValdoriaHeroCharacter* Best = nullptr;
	float BestDot = FMath::Cos(FMath::DegreesToRadians(20.f)); // forgiving cone (third-person camera is offset)
	for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
	{
		if (*It == GetPawn())
		{
			continue;
		}
		const FVector To = (It->GetActorLocation() - ViewLoc).GetSafeNormal();
		const float Dot = FVector::DotProduct(To, Dir);
		if (Dot > BestDot)
		{
			BestDot = Dot;
			Best = *It;
		}
	}
	return Best;
}

void AValdoriaPlayerController::AdminTeleport(const FVector& Location)
{
	if (APawn* P = GetPawn())
	{
		P->SetActorLocation(Location + FVector(0.f, 0.f, 100.f), false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void AValdoriaPlayerController::AdminSetTargetAttribute(int32 Which, float Value)
{
	AValdoriaHeroCharacter* Target = AdminTarget.Get();
	UAbilitySystemComponent* ASC = Target ? Target->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return;
	}
	Value = FMath::Max(0.f, Value);
	switch (Which)
	{
	case 0: ASC->SetNumericAttributeBase(UValdoriaAttributeSet::GetHealthAttribute(), FMath::Min(Value, Target->GetMaxHealth())); break;
	case 1: ASC->SetNumericAttributeBase(UValdoriaAttributeSet::GetMaxHealthAttribute(), FMath::Max(1.f, Value)); break;
	case 2: ASC->SetNumericAttributeBase(UValdoriaAttributeSet::GetArmorAttribute(), FMath::Min(Value, Target->GetMaxArmor())); break;
	case 3: ASC->SetNumericAttributeBase(UValdoriaAttributeSet::GetMaxArmorAttribute(), Value); break;
	case 4: ASC->SetNumericAttributeBase(UValdoriaAttributeSet::GetShieldAttribute(), FMath::Min(Value, Target->GetMaxShield())); break;
	case 5: ASC->SetNumericAttributeBase(UValdoriaAttributeSet::GetMaxShieldAttribute(), Value); break;
	default: break;
	}
	if (Which == 1 || Which == 3 || Which == 5)
	{
		Target->AdminRestoreAll();
	}
}

// ------------------------------------------------------------------ test helpers (-ValdoriaAdminTest)

static void CollectAdminControls(const TSharedRef<SWidget>& W, TSharedPtr<SWidget> Owner, TArray<TPair<FString, TSharedPtr<SWidget>>>& Out)
{
	const FName Type = W->GetType();
	if (Type == TEXT("SButton") || Type == TEXT("SCheckBox"))
	{
		Owner = W;
	}
	else if (Type == TEXT("STextBlock") && Owner.IsValid())
	{
		Out.Emplace(StaticCastSharedRef<STextBlock>(W)->GetText().ToString(), Owner);
		return;
	}
	// A widget switcher only reports its active page; walk every tab.
	FChildren* Children = Type == TEXT("SWidgetSwitcher") ? StaticCastSharedRef<SWidgetSwitcher>(W)->GetAllChildren() : W->GetChildren();
	for (int32 i = 0; Children && i < Children->Num(); ++i)
	{
		CollectAdminControls(Children->GetChildAt(i), Owner, Out);
	}
}

TArray<FString> AValdoriaPlayerController::DebugListAdminControls() const
{
	TArray<FString> Labels;
	TSharedPtr<SWidgetSwitcher> Pages = AdminPages.Pin();
	if (AdminWidget.IsValid() && Pages.IsValid())
	{
		const int32 Was = Pages->GetActiveWidgetIndex();
		for (int32 Page = 0; Page < Pages->GetNumWidgets(); ++Page)
		{
			Pages->SetActiveWidgetIndex(Page); // the switcher only exposes the active tab
			TArray<TPair<FString, TSharedPtr<SWidget>>> Found;
			CollectAdminControls(AdminWidget.ToSharedRef(), nullptr, Found);
			for (const TPair<FString, TSharedPtr<SWidget>>& F : Found)
			{
				Labels.AddUnique(F.Key);
			}
		}
		Pages->SetActiveWidgetIndex(Was);
	}
	return Labels;
}

bool AValdoriaPlayerController::DebugClickAdmin(const FString& Label)
{
	if (!AdminWidget.IsValid())
	{
		return false;
	}
	TSharedPtr<SWidgetSwitcher> Pages = AdminPages.Pin();
	const int32 NumPages = Pages.IsValid() ? Pages->GetNumWidgets() : 1;
	const int32 Was = Pages.IsValid() ? Pages->GetActiveWidgetIndex() : 0;
	TArray<TPair<FString, TSharedPtr<SWidget>>> Found;
	for (int32 Page = 0; Page < NumPages && Found.Num() == 0; ++Page)
	{
		// Try the current tab first, then the others (like a player opening that tab).
		if (Pages.IsValid()) { Pages->SetActiveWidgetIndex(Page == 0 ? Was : (Page == Was ? 0 : Page)); }
		TArray<TPair<FString, TSharedPtr<SWidget>>> All;
		CollectAdminControls(AdminWidget.ToSharedRef(), nullptr, All);
		for (const TPair<FString, TSharedPtr<SWidget>>& F : All)
		{
			if (F.Key == Label) { Found.Add(F); break; }
		}
	}
	for (const TPair<FString, TSharedPtr<SWidget>>& F : Found)
	{
		if (F.Key == Label && F.Value.IsValid())
		{
			if (F.Value->GetType() == TEXT("SButton"))
			{
				StaticCastSharedPtr<SButton>(F.Value)->SimulateClick();
			}
			else
			{
				StaticCastSharedPtr<SCheckBox>(F.Value)->ToggleCheckedState();
			}
			return true;
		}
	}
	return false;
}

// ------------------------------------------------------------------ abrir / fechar

void AValdoriaPlayerController::ToggleAdminPanel()
{
	if (!bAdminMode && !AdminWidget.IsValid())
	{
		return; // ligue o "Modo administrador" no menu de pausa (Esc)
	}
	if (AdminWidget.IsValid())
	{
		CloseAdminPanel();
	}
	else
	{
		OpenAdminPanel();
	}
}

void AValdoriaPlayerController::OpenAdminPanel()
{
	if (AdminWidget.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	AdminTarget = PickAimedCharacter();

	// Heróis disponíveis: Data Assets UValdoriaHeroDefinition do projeto.
	AdminHeroList.Reset();
	AdminMapList.Reset();
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	TArray<FAssetData> Assets;
	Registry.GetAssetsByClass(UValdoriaHeroDefinition::StaticClass()->GetClassPathName(), Assets, true);
	for (const FAssetData& Data : Assets)
	{
		if (UValdoriaHeroDefinition* Def = Cast<UValdoriaHeroDefinition>(Data.GetAsset()))
		{
			AdminHeroList.Add(Def);
		}
	}
	// Mapas jogáveis em /Game (ignora pastas internas).
	TArray<FAssetData> Maps;
	Registry.GetAssetsByClass(UWorld::StaticClass()->GetClassPathName(), Maps, true);
	for (const FAssetData& Data : Maps)
	{
		const FString Path = Data.PackageName.ToString();
		if (Path.StartsWith(TEXT("/Game/")) && !Path.Contains(TEXT("__")))
		{
			AdminMapList.Add(Path);
		}
	}

	AdminWidget = BuildAdminPanel();
	GEngine->GameViewport->AddViewportWidgetContent(AdminWidget.ToSharedRef(), 90);
	RefreshInputMode();
}

void AValdoriaPlayerController::CloseAdminPanel()
{
	if (!AdminWidget.IsValid())
	{
		return;
	}
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(AdminWidget.ToSharedRef());
	}
	AdminWidget.Reset();
	RefreshInputMode();
}

// ------------------------------------------------------------------ painel

TSharedRef<SWidget> AValdoriaPlayerController::BuildAdminPanel()
{
	using namespace ValdoriaAdminUI;
	TWeakObjectPtr<AValdoriaPlayerController> WeakThis(this);
	auto Hero = [WeakThis]() -> AValdoriaHeroCharacter* { return WeakThis.IsValid() ? WeakThis->GetHero() : nullptr; };
	auto Target = [WeakThis]() -> AValdoriaHeroCharacter* { return WeakThis.IsValid() ? WeakThis->AdminTarget.Get() : nullptr; };

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	const TSharedRef<SVerticalBox> Header = Box;

	// Owner's preferred menu style (Valorant-like): horizontal tabs on top, the tab's options below.
	static int32 LastAdminTab = 0;
	TSharedRef<SWidgetSwitcher> Pages = SNew(SWidgetSwitcher);
	AdminPages = Pages;
	TSharedRef<SWrapBox> Tabs = SNew(SWrapBox).PreferredSize(640.f);
	auto NewPage = [Pages, Tabs](const FText& Name) -> TSharedRef<SVerticalBox>
	{
		TSharedRef<SVerticalBox> Page = SNew(SVerticalBox);
		const int32 Index = Pages->GetNumWidgets();
		Pages->AddSlot()[SNew(SScrollBox) + SScrollBox::Slot()[Page]];
		Tabs->AddSlot().Padding(2.f)
		[
			SNew(SButton)
			.OnClicked_Lambda([Pages, Index]() { Pages->SetActiveWidgetIndex(Index); LastAdminTab = Index; return FReply::Handled(); })
			[
				SNew(STextBlock).Text(Name).Font(ValdoriaAdminUI::Font(11, true))
				.ColorAndOpacity_Lambda([Pages, Index]()
				{
					return FSlateColor(Pages->GetActiveWidgetIndex() == Index ? FLinearColor(1.f, 0.62f, 0.4f) : FLinearColor(0.7f, 0.72f, 0.78f));
				})
			]
		];
		return Page;
	};

	Box->AddSlot().AutoHeight()
	[
		SNew(STextBlock).Text(LOCTEXT("Header", "PAINEL DO ADMINISTRADOR")).Font(Font(18, true)).ColorAndOpacity(FLinearColor(1.f, 0.45f, 0.35f))
	];
	Box->AddSlot().AutoHeight()
	[
		SNew(STextBlock).Text(LOCTEXT("Hint", "F10 fecha  •  segure Alt para clicar aqui  •  sem Alt você anda e mira normalmente")).Font(Font(10)).ColorAndOpacity(FLinearColor(0.7f, 0.72f, 0.78f))
	];

	// --- Herói
	Box = NewPage(LOCTEXT("HeroTitle", "HERÓI"));
	{
		TSharedRef<SWrapBox> Heroes = SNew(SWrapBox).PreferredSize(640.f);
		Heroes->AddSlot()[Button(LOCTEXT("Recruit", "Recruta (teste)"), [Hero]() { if (AValdoriaHeroCharacter* H = Hero()) { H->SetHeroDefinition(nullptr); } })];
		for (UValdoriaHeroDefinition* Def : AdminHeroList)
		{
			if (!Def)
			{
				continue;
			}
			TWeakObjectPtr<UValdoriaHeroDefinition> WeakDef = Def;
			Heroes->AddSlot()[Button(Def->DisplayName.IsEmpty() ? FText::FromName(Def->GetFName()) : Def->DisplayName,
				[Hero, WeakDef]() { if (AValdoriaHeroCharacter* H = Hero()) { H->SetHeroDefinition(WeakDef.Get()); } })];
		}
		Box->AddSlot().AutoHeight()[Heroes];
	}
	Box->AddSlot().AutoHeight()[Check(LOCTEXT("God", "Imortal"), [Hero]() { return Hero() && Hero()->bGodMode; }, [Hero](bool b) { if (Hero()) Hero()->bGodMode = b; })];
	Box->AddSlot().AutoHeight()[Check(LOCTEXT("NoCd", "Sem recarga nas habilidades"), [Hero]() { return Hero() && Hero()->bNoCooldowns; }, [Hero](bool b) { if (Hero()) Hero()->bNoCooldowns = b; })];
	Box->AddSlot().AutoHeight()[Check(LOCTEXT("Ammo", "Munição infinita"), [Hero]() { return Hero() && Hero()->bInfiniteAmmo; }, [Hero](bool b) { if (Hero()) Hero()->bInfiniteAmmo = b; })];
	Box->AddSlot().AutoHeight()[Check(LOCTEXT("Dash", "Impulso infinito"), [Hero]() { return Hero() && Hero()->bInfiniteDashes; }, [Hero](bool b) { if (Hero()) Hero()->bInfiniteDashes = b; })];
	Box->AddSlot().AutoHeight()[Number(LOCTEXT("DmgMult", "Multiplicador de dano"), 0.f, 50.f, 0.1f,
		[Hero]() { return Hero() ? Hero()->GetDamageMultiplier() : 1.f; }, [Hero](float V) { if (Hero()) Hero()->AdminSetDamageMultiplier(V); })];
	Box->AddSlot().AutoHeight()[Number(LOCTEXT("SpeedMult", "Multiplicador de velocidade"), 0.1f, 5.f, 0.1f,
		[Hero]() { return Hero() ? Hero()->AdminSpeedMultiplier : 1.f; }, [Hero](float V) { if (Hero()) Hero()->AdminSetSpeedMultiplier(V); })];
	{
		TSharedRef<SWrapBox> Row = SNew(SWrapBox).PreferredSize(640.f);
		Row->AddSlot()[Button(LOCTEXT("FillUlt", "Ultimate cheia"), [Hero]() { if (Hero()) Hero()->AdminFillUltimate(); })];
		Row->AddSlot()[Button(LOCTEXT("Restore", "Vida cheia"), [Hero]() { if (Hero()) Hero()->AdminRestoreAll(); })];
		Row->AddSlot()[Button(LOCTEXT("ResetCd", "Zerar recargas"), [Hero]() { if (Hero()) Hero()->ResetCombatState(); })];
		Row->AddSlot()[Button(LOCTEXT("HurtMe", "Tirar 50 de vida de mim"), [Hero]() { if (AValdoriaHeroCharacter* H = Hero()) { const bool bGod = H->bGodMode; H->bGodMode = false; H->DealDamageTo(H, 50.f); H->bGodMode = bGod; } })];
		Box->AddSlot().AutoHeight()[Row];
	}

	// --- Arma
	Box = NewPage(LOCTEXT("WeaponTitle", "ARMA (modelos 3D prontos)"));
	{
		TSharedRef<SWrapBox> Row = SNew(SWrapBox).PreferredSize(640.f);
		IAssetRegistry& Reg = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
		TArray<FAssetData> Weapons;
		Reg.GetAssetsByPath(FName(TEXT("/Game/Valdoria/Weapons/Merged")), Weapons, false);
		Reg.GetAssetsByPath(FName(TEXT("/Game/Valdoria/Weapons/Fantasy")), Weapons, false); // Fab: +100 Fantasy Weapons
		Weapons.RemoveAll([](const FAssetData& A) { return A.AssetClassPath.GetAssetName() != TEXT("StaticMesh"); });
		Row->AddSlot()[Button(LOCTEXT("NoWeapon", "Sem arma"), [Hero]() { if (Hero()) Hero()->SetWeaponMesh(nullptr); })];
		for (const FAssetData& W : Weapons)
		{
			const FString Name = W.AssetName.ToString().Replace(TEXT("SM_Arma_"), TEXT("")).Replace(TEXT("SM_"), TEXT("")).Replace(TEXT("_"), TEXT(" "));
			if (Name == TEXT("Extras"))
			{
				continue;
			}
			const FSoftObjectPath Path = W.GetSoftObjectPath();
			Row->AddSlot()[Button(FText::FromString(Name), [Hero, Path]()
			{
				if (Hero()) Hero()->SetWeaponMesh(Cast<UStaticMesh>(Path.TryLoad()));
			})];
		}
		Box->AddSlot().AutoHeight()[Row];
	}

	// --- Alvo mirado
	Box = NewPage(LOCTEXT("TargetTitle", "ALVO NA MIRA"));
	Box->AddSlot().AutoHeight()
	[
		SNew(STextBlock).Font(Font(12, true)).ColorAndOpacity(FLinearColor(1.f, 0.85f, 0.6f))
		.Text_Lambda([Target, Hero]()
		{
			AValdoriaHeroCharacter* T = Target();
			if (!T)
			{
				return LOCTEXT("NoTarget", "Nenhum alvo. Mire em alguém e clique em 'Pegar alvo da mira'.");
			}
			const UValdoriaHeroDefinition* D = T->GetHeroDefinition();
			const float Dist = Hero() ? FVector::Dist(Hero()->GetActorLocation(), T->GetActorLocation()) / 100.f : 0.f;
			return FText::FromString(FString::Printf(TEXT("%s  •  vida %.0f/%.0f  armadura %.0f  escudo %.0f  •  %.0f m%s%s"),
				D ? *D->DisplayName.ToString() : *T->GetName(), T->GetHealth(), T->GetMaxHealth(), T->GetArmor(), T->GetShield(), Dist,
				T->IsDead() ? TEXT("  • MORTO") : TEXT(""), T->bGodMode ? TEXT("  • IMORTAL") : TEXT("")));
		})
	];
	{
		TSharedRef<SWrapBox> Row = SNew(SWrapBox).PreferredSize(640.f);
		Row->AddSlot()[Button(LOCTEXT("Pick", "Pegar alvo da mira"), [WeakThis]() { if (WeakThis.IsValid()) WeakThis->AdminTarget = WeakThis->PickAimedCharacter(); })];
		Row->AddSlot()[Button(LOCTEXT("Kill", "Matar"), [Target]()
		{
			if (AValdoriaHeroCharacter* T = Target())
			{
				T->bGodMode = false;
				if (UAbilitySystemComponent* ASC = T->GetAbilitySystemComponent())
				{
					ASC->SetNumericAttributeBase(UValdoriaAttributeSet::GetHealthAttribute(), 0.f);
				}
				T->HandleDeath(nullptr);
			}
		})];
		Row->AddSlot()[Button(LOCTEXT("HurtTarget", "Tirar 50 de vida (aliado ou inimigo)"), [Hero, Target]() { if (Hero() && Target()) Hero()->DealDamageTo(Target(), 50.f); })];
		Row->AddSlot()[Button(LOCTEXT("Revive", "Reviver / curar"), [Target]() { if (AValdoriaHeroCharacter* T = Target()) T->ResetCombatState(); })];
		Row->AddSlot()[Button(LOCTEXT("TgtGod", "Imortal sim/não"), [Target]() { if (AValdoriaHeroCharacter* T = Target()) T->bGodMode = !T->bGodMode; })];
		Row->AddSlot()[Button(LOCTEXT("Freeze", "Congelar sim/não"), [Target]()
		{
			if (AValdoriaHeroCharacter* T = Target())
			{
				UCharacterMovementComponent* Move = T->GetCharacterMovement();
				if (Move->MovementMode == MOVE_None) { Move->SetMovementMode(MOVE_Walking); }
				else { Move->DisableMovement(); }
			}
		})];
		Row->AddSlot()[Button(LOCTEXT("GoTarget", "Ir até o alvo"), [WeakThis, Target]()
		{
			if (WeakThis.IsValid() && Target())
			{
				const FVector Back = -Target()->GetActorForwardVector() * 250.f;
				WeakThis->AdminTeleport(Target()->GetActorLocation() - Back);
			}
		})];
		Box->AddSlot().AutoHeight()[Row];
	}
	const TCHAR* AttrNames[] = { TEXT("Vida"), TEXT("Vida máxima"), TEXT("Armadura"), TEXT("Armadura máxima"), TEXT("Escudo"), TEXT("Escudo máximo") };
	for (int32 i = 0; i < 6; ++i)
	{
		Box->AddSlot().AutoHeight()[Number(FText::FromString(AttrNames[i]), 0.f, 100000.f, 5.f,
			[Target, i]() -> float
			{
				AValdoriaHeroCharacter* T = Target();
				if (!T) return 0.f;
				switch (i)
				{
				case 0: return T->GetHealth();
				case 1: return T->GetMaxHealth();
				case 2: return T->GetArmor();
				case 3: return T->GetMaxArmor();
				case 4: return T->GetShield();
				default: return T->GetMaxShield();
				}
			},
			[WeakThis, i](float V) { if (WeakThis.IsValid()) WeakThis->AdminSetTargetAttribute(i, V); })];
	}

	// --- Inimigos
	Box = NewPage(LOCTEXT("EnemiesTitle", "INIMIGOS E BONECOS"));
	{
		TSharedRef<SWrapBox> Row = SNew(SWrapBox).PreferredSize(640.f);
		Row->AddSlot()[Button(LOCTEXT("KillAll", "Matar todos"), [WeakThis]()
		{
			if (!WeakThis.IsValid()) return;
			for (TActorIterator<AValdoriaTrainingDummy> It(WeakThis->GetWorld()); It; ++It)
			{
				It->bGodMode = false;
				It->HandleDeath(nullptr);
			}
		})];
		Row->AddSlot()[Button(LOCTEXT("ReviveAll", "Reviver todos"), [WeakThis]()
		{
			if (!WeakThis.IsValid()) return;
			for (TActorIterator<AValdoriaTrainingDummy> It(WeakThis->GetWorld()); It; ++It)
			{
				It->ResetCombatState();
			}
		})];
		const TCHAR* Kinds[] = { TEXT("+ Boneco básico"), TEXT("+ Com armadura"), TEXT("+ Com escudo"), TEXT("+ Andando de lado") };
		for (uint8 K = 0; K < 4; ++K)
		{
			Row->AddSlot()[Button(FText::FromString(Kinds[K]), [WeakThis, K]()
			{
				if (!WeakThis.IsValid()) return;
				if (AValdoriaGameMode* GM = WeakThis->GetWorld()->GetAuthGameMode<AValdoriaGameMode>())
				{
					const FVector Where = WeakThis->GetAimPoint() + FVector(0.f, 0.f, 100.f);
					const FRotator Facing = WeakThis->GetPawn() ? (WeakThis->GetPawn()->GetActorLocation() - Where).GetSafeNormal2D().Rotation() : FRotator::ZeroRotator;
					if (AActor* D = GM->SpawnDummyAt(Where, Facing, K)) { D->Tags.Add(TEXT("AdminSpawned")); }
				}
			})];
		}
		Box->AddSlot().AutoHeight()[Row];
	}

	// --- Movimento e física (herói)
	Box = NewPage(LOCTEXT("MoveTitle", "MOVIMENTO E FÍSICA"));
	// Noclip removed (owner, turn 44): it fell through the floor. Normal infinite flight instead, any hero.
	Box->AddSlot().AutoHeight()[Check(LOCTEXT("AdminFly", "Voar normal e infinito (Espaço sobe, Ctrl desce)"), [Hero]() { return Hero() && Hero()->bAdminFlight; }, [Hero](bool b) { if (Hero()) Hero()->bAdminFlight = b; })];
	Box->AddSlot().AutoHeight()[Number(LOCTEXT("Gravity", "Gravidade (1 = normal)"), 0.f, 5.f, 0.05f,
		[Hero]() { return Hero() ? Hero()->GetCharacterMovement()->GravityScale : 1.f; }, [Hero](float V) { if (Hero()) Hero()->GetCharacterMovement()->GravityScale = V; })];
	Box->AddSlot().AutoHeight()[Number(LOCTEXT("Jump", "Força do pulo"), 0.f, 3000.f, 10.f,
		[Hero]() { return Hero() ? Hero()->GetCharacterMovement()->JumpZVelocity : 520.f; }, [Hero](float V) { if (Hero()) Hero()->GetCharacterMovement()->JumpZVelocity = V; })];
	Box->AddSlot().AutoHeight()[Number(LOCTEXT("AirCtrl", "Controle no ar (0 a 1)"), 0.f, 1.f, 0.05f,
		[Hero]() { return Hero() ? Hero()->GetCharacterMovement()->AirControl : 0.4f; }, [Hero](float V) { if (Hero()) { Hero()->AdminAirControl = V; Hero()->GetCharacterMovement()->AirControl = V; } })];
	{
		TSharedRef<SWrapBox> Row = SNew(SWrapBox).PreferredSize(640.f);
		for (int32 Slot = 0; Slot < 3; ++Slot)
		{
			Row->AddSlot()[Button(FText::FromString(FString::Printf(TEXT("Salvar posição %d"), Slot + 1)), [WeakThis, Slot]()
			{
				if (WeakThis.IsValid() && WeakThis->GetPawn())
				{
					WeakThis->SavedPositions[Slot] = WeakThis->GetPawn()->GetActorLocation();
					WeakThis->SavedRotations[Slot] = WeakThis->GetControlRotation();
					WeakThis->bSavedPosition[Slot] = true;
				}
			})];
			Row->AddSlot()[Button(FText::FromString(FString::Printf(TEXT("Ir para posição %d"), Slot + 1)), [WeakThis, Slot]()
			{
				if (WeakThis.IsValid() && WeakThis->GetPawn() && WeakThis->bSavedPosition[Slot])
				{
					WeakThis->GetPawn()->SetActorLocation(WeakThis->SavedPositions[Slot], false, nullptr, ETeleportType::TeleportPhysics);
					WeakThis->SetControlRotation(WeakThis->SavedRotations[Slot]);
				}
			})];
		}
		Box->AddSlot().AutoHeight()[Row];
	}

	// --- Bots e treino de mira
	Box = NewPage(LOCTEXT("BotsTitle", "BOTS E TREINO DE MIRA"));
	{
		const TArray<FText> Levels = { LOCTEXT("B0", "Parados (só alvo)"), LOCTEXT("B1", "Fácil"), LOCTEXT("B2", "Médio"), LOCTEXT("B3", "Difícil") };
		TSharedRef<SWrapBox> Row = SNew(SWrapBox).PreferredSize(640.f);
		for (int32 L = 0; L < 4; ++L)
		{
			Row->AddSlot()[Button(FText::Format(LOCTEXT("BotLevel", "Bots atiram: {0}"), Levels[L]), [WeakThis, L]()
			{
				if (!WeakThis.IsValid()) return;
				WeakThis->AdminBotDifficulty = L;
				for (TActorIterator<AValdoriaTrainingDummy> It(WeakThis->GetWorld()); It; ++It)
				{
					It->BotDifficulty = L;
				}
			})];
		}
		Box->AddSlot().AutoHeight()[Row];
	}
	Box->AddSlot().AutoHeight()
	[
		SNew(STextBlock).Font(Font(11)).ColorAndOpacity(FLinearColor(0.8f, 0.82f, 0.88f))
		.Text_Lambda([WeakThis]()
		{
			static const TCHAR* Names[] = { TEXT("parados"), TEXT("fácil (6 dano, 30% de acerto)"), TEXT("médio (10 dano, 50%)"), TEXT("difícil (16 dano, 75%)") };
			return FText::FromString(FString(TEXT("Bots agora: ")) + Names[WeakThis.IsValid() ? FMath::Clamp(WeakThis->AdminBotDifficulty, 0, 3) : 0]);
		})
	];
	{
		TSharedRef<SWrapBox> Row = SNew(SWrapBox).PreferredSize(640.f);
		Row->AddSlot()[Button(LOCTEXT("MoveAll", "Todos andando de lado"), [WeakThis]()
		{
			if (!WeakThis.IsValid()) return;
			for (TActorIterator<AValdoriaTrainingDummy> It(WeakThis->GetWorld()); It; ++It) { It->bForceStrafe = !It->bForceStrafe; }
		})];
		Row->AddSlot()[Button(LOCTEXT("NoHeal", "Bonecos sem regenerar sim/não"), [WeakThis]()
		{
			if (!WeakThis.IsValid()) return;
			for (TActorIterator<AValdoriaTrainingDummy> It(WeakThis->GetWorld()); It; ++It) { It->bNoAutoHeal = !It->bNoAutoHeal; }
		})];
		Row->AddSlot()[Button(LOCTEXT("BotsGod", "Bonecos imortais sim/não"), [WeakThis]()
		{
			if (!WeakThis.IsValid()) return;
			for (TActorIterator<AValdoriaTrainingDummy> It(WeakThis->GetWorld()); It; ++It) { It->bGodMode = !It->bGodMode; }
		})];
		Row->AddSlot()[Button(LOCTEXT("HurtAlly", "+ Aliado ferido (testar cura)"), [WeakThis, Hero]()
		{
			if (!WeakThis.IsValid() || !Hero()) return;
			AValdoriaGameMode* GM = WeakThis->GetWorld()->GetAuthGameMode<AValdoriaGameMode>();
			if (!GM) return;
			const FVector Where = WeakThis->GetAimPoint() + FVector(0.f, 0.f, 100.f);
			if (AValdoriaTrainingDummy* D = Cast<AValdoriaTrainingDummy>(GM->SpawnDummyAt(Where, (Hero()->GetActorLocation() - Where).GetSafeNormal2D().Rotation(), 0)))
			{
				D->Tags.Add(TEXT("AdminSpawned"));
				D->TeamId = Hero()->TeamId;
				D->bNoAutoHeal = true;
				if (UAbilitySystemComponent* ASC = D->GetAbilitySystemComponent())
				{
					ASC->SetNumericAttributeBase(UValdoriaAttributeSet::GetHealthAttribute(), D->GetMaxHealth() * 0.3f);
				}
			}
		})];
		Row->AddSlot()[Button(LOCTEXT("AimTrainer", "Treino de mira: 6 alvos"), [WeakThis]()
		{
			if (!WeakThis.IsValid() || !WeakThis->GetPawn()) return;
			AValdoriaGameMode* GM = WeakThis->GetWorld()->GetAuthGameMode<AValdoriaGameMode>();
			if (!GM) return;
			const APawn* P = WeakThis->GetPawn();
			const FVector Fwd = FRotator(0.f, WeakThis->GetControlRotation().Yaw, 0.f).Vector();
			const FVector Right = FVector::CrossProduct(FVector::UpVector, Fwd);
			for (int32 i = 0; i < 6; ++i)
			{
				const FVector Where = P->GetActorLocation() + Fwd * (1200.f + (i % 2) * 600.f) + Right * ((i - 2.5f) * 350.f) + FVector(0, 0, 50.f);
				if (AValdoriaTrainingDummy* D = Cast<AValdoriaTrainingDummy>(GM->SpawnDummyAt(Where, (-Fwd).Rotation(), 3)))
				{
					D->bForceStrafe = true;
					D->Tags.Add(TEXT("AdminSpawned"));
				}
			}
		})];
		Row->AddSlot()[Button(LOCTEXT("ClearDummies", "Apagar bonecos criados"), [WeakThis]()
		{
			if (!WeakThis.IsValid()) return;
			int32 Index = 0;
			for (TActorIterator<AValdoriaTrainingDummy> It(WeakThis->GetWorld()); It; ++It)
			{
				if (It->ActorHasTag(TEXT("AdminSpawned"))) { It->Destroy(); ++Index; } // only what the panel created (keeps bots, guardians, originals)
			}
		})];
		Box->AddSlot().AutoHeight()[Row];
	}
	{
		TSharedRef<SWrapBox> Row = SNew(SWrapBox).PreferredSize(640.f);
		Row->AddSlot()[Button(LOCTEXT("TeamSwap", "Alvo: trocar time (aliado/inimigo)"), [Target, Hero]()
		{
			if (AValdoriaHeroCharacter* T = Target())
			{
				const uint8 MyTeam = Hero() ? Hero()->TeamId : 0;
				T->TeamId = (T->TeamId == MyTeam) ? 255 : MyTeam;
			}
		})];
		Box->AddSlot().AutoHeight()[Row];
	}

	// --- Depuração
	Box = NewPage(LOCTEXT("DebugTitle", "DEPURAÇÃO"));
	Box->AddSlot().AutoHeight()[Check(LOCTEXT("Meter", "Medidor de dano (DPS)"),
		[WeakThis]() { const AValdoriaHUD* H = WeakThis.IsValid() ? Cast<AValdoriaHUD>(WeakThis->GetHUD()) : nullptr; return H && H->bShowDamageMeter; },
		[WeakThis](bool b) { if (AValdoriaHUD* H = WeakThis.IsValid() ? Cast<AValdoriaHUD>(WeakThis->GetHUD()) : nullptr) { H->bShowDamageMeter = b; H->ResetDamageMeter(); } })];
	{
		TSharedRef<SWrapBox> Row = SNew(SWrapBox).PreferredSize(640.f);
		Row->AddSlot()[Button(LOCTEXT("ResetMeter", "Zerar medidor"), [WeakThis]() { if (AValdoriaHUD* H = WeakThis.IsValid() ? Cast<AValdoriaHUD>(WeakThis->GetHUD()) : nullptr) H->ResetDamageMeter(); })];
		Row->AddSlot()[Button(LOCTEXT("Collision", "Mostrar colisões (hitboxes)"), [WeakThis]() { if (WeakThis.IsValid()) WeakThis->ConsoleCommand(TEXT("show collision")); })];
		Row->AddSlot()[Button(LOCTEXT("FreeCam", "Câmera livre (liga/desliga)"), [WeakThis]() { if (WeakThis.IsValid()) WeakThis->ConsoleCommand(TEXT("ToggleDebugCamera")); })];
		Row->AddSlot()[Button(LOCTEXT("StatUnit", "Tempos de CPU/GPU"), [WeakThis]() { if (WeakThis.IsValid()) WeakThis->ConsoleCommand(TEXT("stat unit")); })];
		Row->AddSlot()[Button(LOCTEXT("Shot", "Tirar print (Saved/Screenshots)"), [WeakThis]() { if (WeakThis.IsValid()) WeakThis->ConsoleCommand(TEXT("HighResShot 1")); })];
		Box->AddSlot().AutoHeight()[Row];
	}

	// --- Mapa
	Box = NewPage(LOCTEXT("MapTitle", "MAPA"));
	{
		TSharedRef<SWrapBox> Row = SNew(SWrapBox).PreferredSize(640.f);
		Row->AddSlot()[Button(LOCTEXT("TpAim", "Teleportar para a mira"), [WeakThis]() { if (WeakThis.IsValid()) WeakThis->AdminTeleport(WeakThis->GetAimPoint()); })];
		Row->AddSlot()[Button(LOCTEXT("TpStart", "Voltar ao início"), [WeakThis]()
		{
			if (!WeakThis.IsValid()) return;
			for (TActorIterator<APlayerStart> It(WeakThis->GetWorld()); It; ++It)
			{
				WeakThis->AdminTeleport(It->GetActorLocation());
				break;
			}
		})];
		for (const FString& MapPath : AdminMapList)
		{
			const FString Short = FPackageName::GetShortName(MapPath);
			Row->AddSlot()[Button(FText::FromString(TEXT("Abrir mapa: ") + Short), [WeakThis, MapPath]()
			{
				if (WeakThis.IsValid())
				{
					UGameplayStatics::OpenLevel(WeakThis.Get(), FName(*MapPath));
				}
			})];
		}
		Box->AddSlot().AutoHeight()[Row];
	}

	// --- Jogo
	Box = NewPage(LOCTEXT("GameTitle", "JOGO"));
	Box->AddSlot().AutoHeight()[Number(LOCTEXT("TimeScale", "Velocidade do jogo (1 = normal)"), 0.05f, 3.f, 0.05f,
		[WeakThis]() { return WeakThis.IsValid() ? UGameplayStatics::GetGlobalTimeDilation(WeakThis.Get()) : 1.f; },
		[WeakThis](float V) { if (WeakThis.IsValid()) UGameplayStatics::SetGlobalTimeDilation(WeakThis.Get(), FMath::Clamp(V, 0.05f, 3.f)); })];
	Box->AddSlot().AutoHeight()[Check(LOCTEXT("Stats", "Mostrar FPS e ping"),
		[WeakThis]() { const AValdoriaHUD* H = WeakThis.IsValid() ? Cast<AValdoriaHUD>(WeakThis->GetHUD()) : nullptr; return H && H->bShowStats; },
		[WeakThis](bool b) { if (AValdoriaHUD* H = WeakThis.IsValid() ? Cast<AValdoriaHUD>(WeakThis->GetHUD()) : nullptr) H->bShowStats = b; })];

	Tabs->AddSlot().Padding(2.f)[Button(LOCTEXT("Close", "Fechar (F10)"), [WeakThis]() { if (WeakThis.IsValid()) WeakThis->CloseAdminPanel(); })];
	Pages->SetActiveWidgetIndex(FMath::Clamp(LastAdminTab, 0, Pages->GetNumWidgets() - 1));

	return SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Fill)
		.Padding(FMargin(16.f, 40.f, 0.f, 40.f))
		[
			SNew(SBox).WidthOverride(680.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.02f, 0.025f, 0.04f, 0.9f))
				.Padding(14.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[Header]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f)[Tabs]
					+ SVerticalBox::Slot().FillHeight(1.f)[Pages]
				]
			]
		];
}

// ------------------------------------------------------------------ tela de ajuda (K)

void AValdoriaPlayerController::HelpPressed()
{
	if (HelpWidget.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	HelpWidget = BuildHelpScreen();
	GEngine->GameViewport->AddViewportWidgetContent(HelpWidget.ToSharedRef(), 80);
	RefreshInputMode();
}

void AValdoriaPlayerController::HelpReleased()
{
	if (!HelpWidget.IsValid())
	{
		return;
	}
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(HelpWidget.ToSharedRef());
	}
	HelpWidget.Reset();
	RefreshInputMode();
}

TSharedRef<SWidget> AValdoriaPlayerController::BuildHelpScreen()
{
	using namespace ValdoriaAdminUI;
	const AValdoriaHeroCharacter* H = GetHero();
	const UValdoriaHeroDefinition* Def = H ? H->GetHeroDefinition() : nullptr;
	const FLinearColor Theme = Def ? Def->ThemeColor : FLinearColor::White;
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

	auto Entry = [&Box, Theme](const FString& Key, const FText& Name, const FText& Desc)
	{
		Box->AddSlot().AutoHeight().Padding(0.f, 8.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.f, 0.f, 14.f, 0.f)
			[
				SNew(SBox).WidthOverride(130.f)
				[
					SNew(STextBlock).Text(FText::FromString(Key)).Font(Font(14, true)).ColorAndOpacity(Theme)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Name).Font(Font(14, true)).ColorAndOpacity(FLinearColor::White)]
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Desc.IsEmpty() ? LOCTEXT("NoDesc", "(sem descrição no arquivo do herói)") : Desc)
					.Font(Font(11)).AutoWrapText(true).ColorAndOpacity(FLinearColor(0.82f, 0.84f, 0.9f))]
			]
		];
	};

	Box->AddSlot().AutoHeight()
	[
		SNew(STextBlock).Text(Def ? Def->DisplayName : LOCTEXT("NoHero", "Sem herói")).Font(Font(26, true)).ColorAndOpacity(Theme)
	];
	if (Def)
	{
		Box->AddSlot().AutoHeight()[SNew(STextBlock).Text(Def->Description).Font(Font(11)).AutoWrapText(true).ColorAndOpacity(FLinearColor(0.75f, 0.77f, 0.82f))];
		Entry(TEXT("PASSIVA"), Def->PassiveName, Def->PassiveDescription);
		struct FSlotKey { EValdoriaAbilitySlot Slot; FString Key; };
		const FValdoriaOptions& KeyOpt = FValdoriaOptions::Get();
		const FSlotKey Keys[] = {
			{ EValdoriaAbilitySlot::PrimaryFire, TEXT("BOTÃO ESQ.") },
			{ EValdoriaAbilitySlot::SecondaryFire, TEXT("BOTÃO DIR.") },
			{ EValdoriaAbilitySlot::Ability1, KeyOpt.GetSlotKeyLabel(EValdoriaAbilitySlot::Ability1) },
			{ EValdoriaAbilitySlot::Ability2, KeyOpt.GetSlotKeyLabel(EValdoriaAbilitySlot::Ability2) },
			{ EValdoriaAbilitySlot::Ability3, KeyOpt.GetSlotKeyLabel(EValdoriaAbilitySlot::Ability3) },
			{ EValdoriaAbilitySlot::Ultimate, KeyOpt.GetSlotKeyLabel(EValdoriaAbilitySlot::Ultimate) + TEXT(" (ULT)") },
		};
		for (const FSlotKey& K : Keys)
		{
			if (const FValdoriaAbilityEntry* A = H->FindAbilityEntry(K.Slot))
			{
				FString Name = A->DisplayName.ToString();
				const EValdoriaDamageType DT = K.Slot == EValdoriaAbilitySlot::PrimaryFire ? H->GetWeaponStats().DamageType : A->DamageType;
				Name += DT == EValdoriaDamageType::Magical ? TEXT("   [mágico]") : (DT == EValdoriaDamageType::Hybrid ? TEXT("   [híbrido]") : TEXT("   [físico]"));
				if (A->Cooldown > 0.f)
				{
					Name += FString::Printf(TEXT("   (recarga %.0f s)"), A->Cooldown);
				}
				Entry(K.Key, FText::FromString(Name), A->Description);
			}
		}
	}
	Box->AddSlot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
	[
		SNew(STextBlock).Text(LOCTEXT("HelpHint", "Solte o K para voltar ao jogo")).Font(Font(10)).ColorAndOpacity(FLinearColor(0.6f, 0.62f, 0.68f))
	];

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.02f, 0.55f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(760.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.03f, 0.035f, 0.05f, 0.85f))
				.Padding(24.f)
				[
					Box
				]
			]
		];
}

#undef LOCTEXT_NAMESPACE
