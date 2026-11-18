// Echoes of Valdoria - loja de melhorias (tecla B): catálogo e tela.

#include "ValdoriaShop.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"
#include "ValdoriaHeroCharacter.h"
#include "ValdoriaPlayerController.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ValdoriaShop"

// ------------------------------------------------------------------ catálogo

void UValdoriaShopCatalog::ApplyDefaultItems()
{
	Modify();
	Items.Reset();
	auto Add = [this](const TCHAR* Id, const FText& Name, const FText& Desc, EValdoriaShopCategory Cat, EValdoriaShopStat Stat,
		TArray<int32> Costs, TArray<float> Values)
	{
		FValdoriaShopItem& I = Items.AddDefaulted_GetRef();
		I.ItemId = Id;
		I.DisplayName = Name;
		I.Description = Desc;
		I.Category = Cat;
		I.Stat = Stat;
		I.LevelCosts = MoveTemp(Costs);
		I.LevelValues = MoveTemp(Values);
	};
	using C = EValdoriaShopCategory;
	using S = EValdoriaShopStat;
	// Defesa
	Add(TEXT("placas"), LOCTEXT("Plates", "Placas de Liga"), LOCTEXT("PlatesD", "Resistência física +10% / +20% / +30%."),
		C::Defense, S::PhysicalResistance, { 300, 400, 500 }, { 0.1f, 0.2f, 0.3f });
	Add(TEXT("amuleto"), LOCTEXT("Amulet", "Amuleto Rúnico"), LOCTEXT("AmuletD", "Resistência mágica +10% / +20% / +30%."),
		C::Defense, S::MagicResistance, { 300, 400, 500 }, { 0.1f, 0.2f, 0.3f });
	Add(TEXT("coracao"), LOCTEXT("Heart", "Coração Robusto"), LOCTEXT("HeartD", "Vida máxima +25 / +50 / +75."),
		C::Defense, S::MaxHealth, { 250, 350, 450 }, { 25.f, 50.f, 75.f });
	// Utilidade
	Add(TEXT("engrenagem"), LOCTEXT("Gear", "Engrenagem do Tempo"), LOCTEXT("GearD", "Recarga das habilidades -10% / -20% / -30%."),
		C::Utility, S::CooldownReduction, { 350, 450, 550 }, { 0.1f, 0.2f, 0.3f });
	Add(TEXT("botas"), LOCTEXT("Boots", "Botas Velozes"), LOCTEXT("BootsD", "Velocidade de movimento +5% / +10% / +15%."),
		C::Utility, S::MoveSpeed, { 250, 350, 450 }, { 0.05f, 0.1f, 0.15f });
	Add(TEXT("cartucheira"), LOCTEXT("Belt", "Cartucheira"), LOCTEXT("BeltD", "Munição no pente +20% / +40% / +60%."),
		C::Utility, S::MagazineSize, { 200, 300, 400 }, { 0.2f, 0.4f, 0.6f });
	// Cura
	Add(TEXT("seiva"), LOCTEXT("Sap", "Seiva Viva"), LOCTEXT("SapD", "Toda cura que você causa +10% / +20% / +30%."),
		C::Healing, S::HealingPower, { 300, 400, 500 }, { 0.1f, 0.2f, 0.3f });
	Add(TEXT("sangue"), LOCTEXT("Blood", "Sangue de Ferro"), LOCTEXT("BloodD", "Recupera 5% / 10% / 15% do dano que você causa."),
		C::Healing, S::LifeSteal, { 350, 450, 550 }, { 0.05f, 0.1f, 0.15f });
	// Dano
	Add(TEXT("lamina"), LOCTEXT("Blade", "Lâmina Afiada"), LOCTEXT("BladeD", "Dano físico +8% / +16% / +24%."),
		C::Damage, S::PhysicalPower, { 350, 450, 550 }, { 0.08f, 0.16f, 0.24f });
	Add(TEXT("foco"), LOCTEXT("Focus", "Foco Arcano"), LOCTEXT("FocusD", "Dano mágico +8% / +16% / +24%."),
		C::Damage, S::MagicPower, { 350, 450, 550 }, { 0.08f, 0.16f, 0.24f });
	Add(TEXT("olho"), LOCTEXT("Eye", "Olho Certeiro"), LOCTEXT("EyeD", "Dano na cabeça +10% / +20% / +30%."),
		C::Damage, S::HeadshotDamage, { 300, 400, 500 }, { 0.1f, 0.2f, 0.3f });
}

const FValdoriaShopItem* UValdoriaShopCatalog::FindItem(FName ItemId) const
{
	return Items.FindByPredicate([ItemId](const FValdoriaShopItem& I) { return I.ItemId == ItemId; });
}

FValdoriaShopBonus UValdoriaShopCatalog::ComputeBonus(const TMap<FName, int32>& Levels) const
{
	FValdoriaShopBonus B;
	for (const TPair<FName, int32>& Pair : Levels)
	{
		const FValdoriaShopItem* Item = FindItem(Pair.Key);
		if (!Item || Pair.Value <= 0 || Item->LevelValues.Num() == 0)
		{
			continue;
		}
		const float V = Item->LevelValues[FMath::Clamp(Pair.Value, 1, Item->LevelValues.Num()) - 1];
		switch (Item->Stat)
		{
		case EValdoriaShopStat::MaxHealth: B.MaxHealth += V; break;
		case EValdoriaShopStat::PhysicalResistance: B.PhysicalResistance += V; break;
		case EValdoriaShopStat::MagicResistance: B.MagicResistance += V; break;
		case EValdoriaShopStat::CooldownReduction: B.CooldownReduction += V; break;
		case EValdoriaShopStat::MoveSpeed: B.MoveSpeed += V; break;
		case EValdoriaShopStat::MagazineSize: B.MagazineSize += V; break;
		case EValdoriaShopStat::HealingPower: B.HealingPower += V; break;
		case EValdoriaShopStat::LifeSteal: B.LifeSteal += V; break;
		case EValdoriaShopStat::PhysicalPower: B.PhysicalPower += V; break;
		case EValdoriaShopStat::MagicPower: B.MagicPower += V; break;
		case EValdoriaShopStat::HeadshotDamage: B.HeadshotDamage += V; break;
		}
	}
	return B;
}

// ------------------------------------------------------------------ créditos e compras

UValdoriaShopCatalog* AValdoriaPlayerController::GetShopCatalog()
{
	if (!ShopCatalog)
	{
		// O arquivo que o dono edita; sem ele, um catálogo em memória com os mesmos itens.
		ShopCatalog = LoadObject<UValdoriaShopCatalog>(nullptr, TEXT("/Game/Valdoria/Shop/DA_Loja.DA_Loja"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!ShopCatalog)
		{
			ShopCatalog = NewObject<UValdoriaShopCatalog>(this);
			ShopCatalog->ApplyDefaultItems();
		}
	}
	if (Credits < 0.f)
	{
		Credits = ShopCatalog->StartingCredits;
	}
	return ShopCatalog;
}

void AValdoriaPlayerController::AddCredits(float Amount)
{
	GetShopCatalog();
	Credits = FMath::Max(0.f, Credits + Amount);
}

void AValdoriaPlayerController::OnHeroDamageDealt(float Damage, bool bKill)
{
	const UValdoriaShopCatalog* Catalog = GetShopCatalog();
	AddCredits(Damage * Catalog->CreditsPerDamage + (bKill ? Catalog->CreditsPerKill : 0));
}

void AValdoriaPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ApplyShopToHero();
	if (!GetWorldTimerManager().IsTimerActive(CreditTimer))
	{
		// Créditos por tempo de partida (1 vez por segundo).
		GetWorldTimerManager().SetTimer(CreditTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			AddCredits(GetShopCatalog()->CreditsPerSecond);
		}), 1.f, true);
	}
}

void AValdoriaPlayerController::ApplyShopToHero()
{
	if (AValdoriaHeroCharacter* Hero = Cast<AValdoriaHeroCharacter>(GetPawn()))
	{
		Hero->SetShopBonus(GetShopCatalog()->ComputeBonus(ShopLevels));
	}
}

bool AValdoriaPlayerController::BuyItem(FName ItemId)
{
	UValdoriaShopCatalog* Catalog = GetShopCatalog();
	const FValdoriaShopItem* Item = Catalog->FindItem(ItemId);
	if (!Item)
	{
		return false;
	}
	const int32 Level = GetItemLevel(ItemId);
	if (Level >= Item->LevelCosts.Num())
	{
		return false; // nível máximo
	}
	if (Level == 0)
	{
		int32 Owned = 0;
		for (const TPair<FName, int32>& Pair : ShopLevels)
		{
			Owned += Pair.Value > 0 ? 1 : 0;
		}
		if (Owned >= Catalog->MaxItems)
		{
			return false; // sem espaço
		}
	}
	const int32 Cost = Item->LevelCosts[Level];
	if (Credits < Cost)
	{
		return false;
	}
	Credits -= Cost;
	ShopLevels.Add(ItemId, Level + 1);
	ApplyShopToHero();
	return true;
}

// ------------------------------------------------------------------ tela

void AValdoriaPlayerController::ToggleShop()
{
	if (ShopWidget.IsValid())
	{
		CloseShop();
		return;
	}
	if (PauseWidget.IsValid() || ChatWidget.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	GetShopCatalog();
	ShopWidget = BuildShopScreen();
	GEngine->GameViewport->AddViewportWidgetContent(ShopWidget.ToSharedRef(), 110);
	RefreshInputMode();
}

void AValdoriaPlayerController::CloseShop()
{
	if (!ShopWidget.IsValid())
	{
		return;
	}
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(ShopWidget.ToSharedRef());
	}
	ShopWidget.Reset();
	RefreshInputMode();
}

TSharedRef<SWidget> AValdoriaPlayerController::BuildShopScreen()
{
	UValdoriaShopCatalog* Catalog = GetShopCatalog();
	TWeakObjectPtr<AValdoriaPlayerController> WeakThis(this);
	auto Font = [](int32 Size, bool bBold = false) { return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size); };

	struct FCat { EValdoriaShopCategory Cat; FText Name; FLinearColor Color; };
	const FCat Cats[] = {
		{ EValdoriaShopCategory::Defense, LOCTEXT("CatDef", "DEFESA"), FLinearColor(0.4f, 0.7f, 1.f) },
		{ EValdoriaShopCategory::Utility, LOCTEXT("CatUtil", "UTILIDADE"), FLinearColor(1.f, 0.85f, 0.35f) },
		{ EValdoriaShopCategory::Healing, LOCTEXT("CatHeal", "CURA"), FLinearColor(0.45f, 1.f, 0.6f) },
		{ EValdoriaShopCategory::Damage, LOCTEXT("CatDmg", "DANO"), FLinearColor(1.f, 0.45f, 0.4f) },
	};

	TSharedRef<SHorizontalBox> Columns = SNew(SHorizontalBox);
	for (const FCat& Cat : Cats)
	{
		TSharedRef<SVerticalBox> Column = SNew(SVerticalBox);
		Column->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(STextBlock).Text(Cat.Name).Font(Font(15, true)).ColorAndOpacity(Cat.Color)
		];
		for (const FValdoriaShopItem& Item : Catalog->Items)
		{
			if (Item.Category != Cat.Cat)
			{
				continue;
			}
			const FName Id = Item.ItemId;
			const int32 MaxLevel = Item.LevelCosts.Num();
			Column->AddSlot().AutoHeight().Padding(0.f, 4.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.06f, 0.07f, 0.1f, 0.9f))
				.Padding(8.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Font(Font(12, true)).ColorAndOpacity(FLinearColor::White)
						.Text_Lambda([WeakThis, Id, Name = Item.DisplayName, MaxLevel]()
						{
							const int32 L = WeakThis.IsValid() ? WeakThis->GetItemLevel(Id) : 0;
							FString Dots;
							for (int32 i = 0; i < MaxLevel; ++i)
							{
								Dots += i < L ? TEXT("●") : TEXT("○");
							}
							return FText::FromString(FString::Printf(TEXT("%s  %s"), *Name.ToString(), *Dots));
						})
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 6.f)
					[
						SNew(STextBlock).Text(Item.Description).Font(Font(10)).AutoWrapText(true).ColorAndOpacity(FLinearColor(0.72f, 0.75f, 0.8f))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SButton).HAlign(HAlign_Center)
						.OnClicked_Lambda([WeakThis, Id]()
						{
							if (WeakThis.IsValid())
							{
								WeakThis->BuyItem(Id);
							}
							return FReply::Handled();
						})
						[
							SNew(STextBlock).Font(Font(11, true))
							.Text_Lambda([WeakThis, Id, Costs = Item.LevelCosts]()
							{
								const int32 L = WeakThis.IsValid() ? WeakThis->GetItemLevel(Id) : 0;
								if (L >= Costs.Num())
								{
									return LOCTEXT("Max", "Nível máximo");
								}
								return FText::FromString(FString::Printf(TEXT("%s nível %d  •  %d"),
									L == 0 ? TEXT("Comprar") : TEXT("Melhorar"), L + 1, Costs[L]));
							})
						]
					]
				]
			];
		}
		Columns->AddSlot().FillWidth(1.f).Padding(8.f, 0.f)[Column];
	}

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.02f, 0.6f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(1100.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.03f, 0.035f, 0.05f, 0.95f))
				.Padding(18.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(8.f, 0.f, 8.f, 12.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(STextBlock).Text(LOCTEXT("Title", "LOJA DE MELHORIAS")).Font(Font(24, true)).ColorAndOpacity(FLinearColor(0.97f, 0.83f, 0.48f))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock).Font(Font(16, true)).ColorAndOpacity(FLinearColor(1.f, 0.85f, 0.4f))
							.Text_Lambda([WeakThis, Catalog]()
							{
								int32 Owned = 0;
								if (WeakThis.IsValid())
								{
									for (const TPair<FName, int32>& P : WeakThis->ShopLevels)
									{
										Owned += P.Value > 0 ? 1 : 0;
									}
								}
								return FText::FromString(FString::Printf(TEXT("◆ %d créditos    itens %d/%d"),
									WeakThis.IsValid() ? WeakThis->GetCredits() : 0, Owned, Catalog ? Catalog->MaxItems : 0));
							})
						]
					]
					+ SVerticalBox::Slot().AutoHeight()[Columns]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 14.f, 0.f, 0.f)
					[
						SNew(STextBlock).Text(LOCTEXT("Hint", "Créditos vêm do tempo de partida, do dano e dos abates. Tudo zera quando a partida recomeça.  B ou Esc fecha."))
						.Font(Font(10)).ColorAndOpacity(FLinearColor(0.6f, 0.62f, 0.68f))
					]
				]
			]
		];
}

#undef LOCTEXT_NAMESPACE
