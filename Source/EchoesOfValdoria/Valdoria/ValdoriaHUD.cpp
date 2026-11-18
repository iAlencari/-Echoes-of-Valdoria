// Echoes of Valdoria - HUD desenhado em código.

#include "ValdoriaHUD.h"
#include "CanvasItem.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "ValdoriaHeroCharacter.h"
#include "ValdoriaHeroDefinition.h"

// ------------------------------------------------------------------ utilidades de desenho

void AValdoriaHUD::AddHitMarker(const FVector& WorldLocation, float Damage, bool bHeadshot, bool bKill)
{
	LastHitTime = GetWorld()->GetTimeSeconds();
	bLastHitHeadshot = bHeadshot;
	bLastHitKill = bKill;
	DamageNumbers.Add({ WorldLocation + FVector(FMath::FRandRange(-30.f, 30.f), FMath::FRandRange(-30.f, 30.f), 0.f), Damage, bHeadshot, LastHitTime });
}

void AValdoriaHUD::DrawRect(float X, float Y, float W, float H, const FLinearColor& Color)
{
	FCanvasTileItem Tile(FVector2D(X, Y), FVector2D(FMath::Max(0.f, W), FMath::Max(0.f, H)), Color);
	Tile.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Tile);
}

void AValdoriaHUD::DrawFrame(float X, float Y, float W, float H, const FLinearColor& Color, float Thickness)
{
	FCanvasBoxItem Box(FVector2D(X, Y), FVector2D(W, H));
	Box.SetColor(Color);
	Box.LineThickness = Thickness;
	Canvas->DrawItem(Box);
}

void AValdoriaHUD::DrawLabel(const FString& Text, float X, float Y, const FLinearColor& Color, float TextScale, bool bCenter, bool bLarge)
{
	UFont* Font = bLarge ? GEngine->GetLargeFont() : GEngine->GetMediumFont();
	FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Text), Font, Color);
	Item.Scale = FVector2D(TextScale, TextScale);
	Item.bCentreX = bCenter;
	Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f));
	Canvas->DrawItem(Item);
}

void AValdoriaHUD::DrawSkull(float Cx, float Cy, float Size, const FLinearColor& Color)
{
	// Caveira simples: crânio, olhos e dentes.
	DrawRect(Cx - Size * 0.5f, Cy - Size * 0.5f, Size, Size * 0.75f, Color);
	DrawRect(Cx - Size * 0.32f, Cy + Size * 0.25f, Size * 0.64f, Size * 0.25f, Color);
	const FLinearColor Hole(0.f, 0.f, 0.f, 0.9f);
	DrawRect(Cx - Size * 0.32f, Cy - Size * 0.18f, Size * 0.22f, Size * 0.22f, Hole);
	DrawRect(Cx + Size * 0.10f, Cy - Size * 0.18f, Size * 0.22f, Size * 0.22f, Hole);
	DrawRect(Cx - Size * 0.04f, Cy + Size * 0.25f, Size * 0.08f, Size * 0.25f, Hole);
}

/** Barra segmentada: vida (cor do herói), armadura (laranja) e escudo (azul) em sequência. */
void AValdoriaHUD::DrawSegmentedBar(float X, float Y, float W, float H, AValdoriaHeroCharacter* Hero, const FLinearColor& HealthColor)
{
	const float MaxH = Hero->GetMaxHealth();
	const float MaxA = Hero->GetMaxArmor();
	const float MaxS = Hero->GetMaxShield();
	const float Total = FMath::Max(1.f, MaxH + MaxA + MaxS);
	const float Unit = W / Total;

	DrawRect(X - 2.f, Y - 2.f, W + 4.f, H + 4.f, FLinearColor(0.f, 0.f, 0.f, 0.6f));
	DrawRect(X, Y, W, H, FLinearColor(0.08f, 0.08f, 0.1f, 0.8f));

	float Cursor = X;
	auto Section = [&](float Value, float Max, const FLinearColor& Color)
	{
		if (Max <= 0.f)
		{
			return;
		}
		DrawRect(Cursor, Y, Unit * FMath::Clamp(Value, 0.f, Max), H, Color);
		Cursor += Unit * Max;
	};
	Section(Hero->GetHealth(), MaxH, HealthColor);
	Section(Hero->GetArmor(), MaxA, FLinearColor(1.f, 0.62f, 0.12f, 0.95f));
	Section(Hero->GetShield(), MaxS, FLinearColor(0.45f, 0.8f, 1.f, 0.95f));

	// Divisões a cada N de vida.
	const int32 Segments = FMath::FloorToInt(Total / FMath::Max(5.f, HealthPerSegment));
	if (Segments > 1 && Segments < 80)
	{
		for (int32 i = 1; i < Segments; ++i)
		{
			const float Sx = X + Unit * HealthPerSegment * i;
			DrawRect(Sx - 1.f, Y, 2.f, H, FLinearColor(0.f, 0.f, 0.f, 0.55f));
		}
	}
}

// ------------------------------------------------------------------ HUD principal

void AValdoriaHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	Scale = H / 1080.f;
	AValdoriaHeroCharacter* Hero = Cast<AValdoriaHeroCharacter>(GetOwningPawn());

	DrawWorldHealthBars(Hero);
	DrawCrosshairAndHits(W, H);
	if (bShowStats)
	{
		DrawStats();
	}
	if (!Hero || !Hero->GetHeroDefinition())
	{
		return;
	}
	DrawPlayerPanel(Hero, W, H);
	DrawAbilities(Hero, W, H);
	DrawAmmo(Hero, W, H);

	if (Hero->IsDead())
	{
		DrawLabel(TEXT("VOCÊ FOI ABATIDO — renascendo..."), W * 0.5f, H * 0.4f, EnemyColor, 1.f * Scale, true, true);
	}
}

void AValdoriaHUD::DrawCrosshairAndHits(float W, float H)
{
	const float S = Scale;
	const float Now = GetWorld()->GetTimeSeconds();
	const float Cx = W * 0.5f;
	const float Cy = H * 0.5f;
	const bool bHitRecent = Now - LastHitTime < 0.18f;
	const FLinearColor CrossColor = bHitRecent ? (bLastHitHeadshot ? EnemyColor : FLinearColor::White) : FLinearColor(1.f, 1.f, 1.f, 0.85f);
	const float Gap = 6.f * S;
	const float Len = 8.f * S;
	DrawLine(Cx - Gap - Len, Cy, Cx - Gap, Cy, CrossColor, 2.f);
	DrawLine(Cx + Gap, Cy, Cx + Gap + Len, Cy, CrossColor, 2.f);
	DrawLine(Cx, Cy - Gap - Len, Cx, Cy - Gap, CrossColor, 2.f);
	DrawLine(Cx, Cy + Gap, Cx, Cy + Gap + Len, CrossColor, 2.f);
	if (bHitRecent)
	{
		const float M = 12.f * S;
		const FLinearColor HitColor = bLastHitKill ? EnemyColor : CrossColor;
		DrawLine(Cx - M, Cy - M, Cx - M * 0.45f, Cy - M * 0.45f, HitColor, 2.f);
		DrawLine(Cx + M, Cy - M, Cx + M * 0.45f, Cy - M * 0.45f, HitColor, 2.f);
		DrawLine(Cx - M, Cy + M, Cx - M * 0.45f, Cy + M * 0.45f, HitColor, 2.f);
		DrawLine(Cx + M, Cy + M, Cx + M * 0.45f, Cy + M * 0.45f, HitColor, 2.f);
	}

	for (int32 i = DamageNumbers.Num() - 1; i >= 0; --i)
	{
		const FDamageNumber& N = DamageNumbers[i];
		const float Age = Now - N.SpawnTime;
		if (Age > 0.9f)
		{
			DamageNumbers.RemoveAt(i);
			continue;
		}
		const FVector Screen = Project(N.WorldLocation + FVector(0, 0, Age * 90.f));
		if (Screen.Z > 0.f)
		{
			const FLinearColor C = N.bHeadshot ? FLinearColor(1.f, 0.3f, 0.2f, 1.f - Age) : FLinearColor(1.f, 0.95f, 0.8f, 1.f - Age);
			DrawLabel(FString::Printf(TEXT("%d"), FMath::RoundToInt(N.Damage)), Screen.X, Screen.Y, C, (N.bHeadshot ? 1.6f : 1.2f) * S, true);
		}
	}
}

void AValdoriaHUD::DrawStats()
{
	SmoothedFrameMs = FMath::Lerp(SmoothedFrameMs, GetWorld()->GetDeltaSeconds() * 1000.f, 0.05f);
	const float Fps = 1000.f / FMath::Max(SmoothedFrameMs, 0.01f);
	float PingMs = 0.f;
	if (const APlayerController* PC = GetOwningPlayerController())
	{
		if (const APlayerState* PS = PC->PlayerState)
		{
			PingMs = PS->GetPingInMilliseconds();
		}
	}
	const FString PingText = (GetNetMode() == NM_Standalone) ? TEXT("local") : FString::Printf(TEXT("%.0f ms"), PingMs);
	const FLinearColor Col = Fps >= 60.f ? FLinearColor(0.55f, 1.f, 0.55f) : (Fps >= 30.f ? FLinearColor(1.f, 0.85f, 0.4f) : FLinearColor(1.f, 0.4f, 0.4f));
	DrawLabel(FString::Printf(TEXT("FPS %.0f  |  %.1f ms  |  Ping %s"), Fps, SmoothedFrameMs, *PingText), 16.f * Scale, 12.f * Scale, Col, 0.9f * Scale);
}

/** Barras sobre a cabeça: aliado em ciano, inimigo em vermelho com caveira. */
void AValdoriaHUD::DrawWorldHealthBars(AValdoriaHeroCharacter* Viewer)
{
	const float S = Scale;
	for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
	{
		AValdoriaHeroCharacter* Other = *It;
		if (!Other || Other == Viewer || Other->IsDead())
		{
			continue;
		}
		const float HalfHeight = Other->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		const FVector Head = Other->GetActorLocation() + FVector(0.f, 0.f, HalfHeight + 35.f);
		const FVector Screen = Project(Head);
		if (Screen.Z <= 0.f)
		{
			continue;
		}
		const float Dist = Viewer ? FVector::Dist(Viewer->GetActorLocation(), Other->GetActorLocation()) : 1000.f;
		if (Dist > 6000.f)
		{
			continue;
		}
		const bool bEnemy = !Viewer || Viewer->IsEnemy(Other);
		const FLinearColor Color = bEnemy ? EnemyColor : AllyColor;
		const float BarW = FMath::Clamp(140.f * 1200.f / FMath::Max(Dist, 1.f), 70.f, 150.f) * S;
		const float BarH = 9.f * S;
		const float X = Screen.X - BarW * 0.5f;
		const float Y = Screen.Y;
		DrawSegmentedBar(X, Y, BarW, BarH, Other, Color);
		if (bEnemy)
		{
			DrawSkull(X - 12.f * S, Y + BarH * 0.5f, 11.f * S, Color);
		}
		if (const UValdoriaHeroDefinition* Def = Other->GetHeroDefinition())
		{
			DrawLabel(FString::Printf(TEXT("%.0f"), Other->GetHealth() + Other->GetArmor() + Other->GetShield()),
				Screen.X, Y - 22.f * S, Color, 1.0f * S, true);
		}
	}
}

void AValdoriaHUD::DrawPlayerPanel(AValdoriaHeroCharacter* Hero, float W, float H)
{
	const float S = Scale;
	const UValdoriaHeroDefinition* Def = Hero->GetHeroDefinition();
	const FLinearColor Theme = Def->ThemeColor;
	const float X = 48.f * S;
	const float Y = H - 132.f * S;
	const float BarW = 420.f * S;
	const float BarH = 26.f * S;

	// Nome do herói + funções.
	FString Roles;
	for (const EValdoriaHeroRole HeroRole : Def->Roles)
	{
		const UEnum* Enum = StaticEnum<EValdoriaHeroRole>();
		Roles += (Roles.IsEmpty() ? TEXT("") : TEXT(" / ")) + Enum->GetDisplayNameTextByValue(static_cast<int64>(HeroRole)).ToString();
	}
	DrawLabel(Def->DisplayName.ToString().ToUpper(), X, Y - 78.f * S, Theme, 1.8f * S, false, true);
	DrawLabel(Roles, X, Y - 30.f * S, FLinearColor(0.8f, 0.82f, 0.88f), 1.1f * S);

	// Barra de vida no estilo do herói (moldura na cor dele).
	DrawSegmentedBar(X, Y, BarW, BarH, Hero, Theme);
	DrawFrame(X - 4.f * S, Y - 4.f * S, BarW + 8.f * S, BarH + 8.f * S, Theme, 2.f);
	const float Current = Hero->GetHealth() + Hero->GetArmor() + Hero->GetShield();
	const float Max = Hero->GetMaxHealth() + Hero->GetMaxArmor() + Hero->GetMaxShield();
	DrawLabel(FString::Printf(TEXT("%.0f"), Current), X, Y + BarH + 6.f * S, FLinearColor::White, 2.0f * S, false, true);
	DrawLabel(FString::Printf(TEXT("/ %.0f"), Max), X + 92.f * S, Y + BarH + 22.f * S, FLinearColor(0.75f, 0.77f, 0.82f), 1.3f * S);

	// Estado do movimento + cargas de impulso.
	const FValdoriaMovementRules& Rules = Def->Movement;
	FString MoveText;
	if (Rules.ShiftMode == EValdoriaShiftMode::Dash || Rules.ShiftMode == EValdoriaShiftMode::SprintAndDash)
	{
		const int32 Charges = FMath::Clamp(Rules.DashCharges, 1, 5);
		for (int32 i = 0; i < Charges; ++i)
		{
			const bool bFull = i < Hero->GetDashCharges();
			DrawRect(X + BarW - (Charges - i) * 22.f * S, Y + BarH + 14.f * S, 16.f * S, 8.f * S,
				bFull ? FLinearColor(0.6f, 0.9f, 1.f, 0.95f) : FLinearColor(0.2f, 0.25f, 0.3f, 0.7f));
		}
	}
	if (Hero->IsSliding()) MoveText = TEXT("DESLIZANDO");
	else if (Hero->IsMantling()) MoveText = TEXT("SUBINDO");
	else if (Hero->IsWallClimbing()) MoveText = TEXT("ESCALANDO");
	else if (Hero->IsProne()) MoveText = TEXT("DEITADO");
	else if (Hero->bIsCrouched) MoveText = TEXT("AGACHADO");
	else if (Hero->IsSprinting()) MoveText = TEXT("CORRENDO");
	if (!MoveText.IsEmpty())
	{
		DrawLabel(MoveText, X + BarW - 180.f * S, Y + BarH + 28.f * S, FLinearColor(0.6f, 0.9f, 1.f), 0.8f * S);
	}
}

void AValdoriaHUD::DrawAbilities(AValdoriaHeroCharacter* Hero, float W, float H)
{
	const float S = Scale;
	const UValdoriaHeroDefinition* Def = Hero->GetHeroDefinition();
	const FLinearColor Theme = Def->ThemeColor;
	const float Size = 64.f * S;
	const float Gap = 12.f * S;
	const float Y = H - 112.f * S;

	struct FSlotInfo { EValdoriaAbilitySlot Slot; const TCHAR* Key; };
	const FSlotInfo Slots[] = {
		{ EValdoriaAbilitySlot::SecondaryFire, TEXT("BD") },
		{ EValdoriaAbilitySlot::Ability1, TEXT("F") },
		{ EValdoriaAbilitySlot::Ability2, TEXT("E") },
	};
	// Passiva (losango simples) + 3 habilidades + ultimate.
	const float UltSize = 86.f * S;
	const float TotalW = Size * 0.7f + Gap + (Size + Gap) * 3 + UltSize;
	float X = W * 0.5f - TotalW * 0.5f;

	// Passiva.
	DrawRect(X, Y + Size * 0.15f, Size * 0.7f, Size * 0.7f, FLinearColor(0.08f, 0.09f, 0.12f, 0.8f));
	DrawFrame(X, Y + Size * 0.15f, Size * 0.7f, Size * 0.7f, Theme * 0.8f, 1.5f);
	DrawLabel(TEXT("P"), X + Size * 0.35f, Y + Size * 0.3f, Theme, 0.9f * S, true);
	X += Size * 0.7f + Gap;

	for (const FSlotInfo& Info : Slots)
	{
		const FValdoriaAbilityEntry* Entry = Hero->FindAbilityEntry(Info.Slot);
		const float Remaining = Entry ? Hero->GetCooldownRemaining(Info.Slot) : 0.f;
		const float Cooldown = Entry ? FMath::Max(0.01f, Entry->Cooldown) : 1.f;
		DrawRect(X, Y, Size, Size, FLinearColor(0.08f, 0.09f, 0.12f, Entry ? 0.85f : 0.4f));
		if (Entry && Remaining > 0.f)
		{
			// Faixa escura que baixa conforme a recarga termina.
			const float Frac = FMath::Clamp(Remaining / Cooldown, 0.f, 1.f);
			DrawRect(X, Y + Size * (1.f - Frac), Size, Size * Frac, FLinearColor(0.f, 0.f, 0.f, 0.6f));
			DrawLabel(FString::Printf(TEXT("%.0f"), FMath::CeilToFloat(Remaining)), X + Size * 0.5f, Y + Size * 0.22f, FLinearColor::White, 1.8f * S, true);
		}
		else if (Entry)
		{
			DrawLabel(Entry->DisplayName.ToString().Left(10), X + Size * 0.5f, Y + Size * 0.4f, FLinearColor::White, 0.8f * S, true);
		}
		DrawFrame(X, Y, Size, Size, Entry && Remaining <= 0.f ? Theme : FLinearColor(0.35f, 0.37f, 0.42f), 2.f);
		DrawRect(X + 3.f * S, Y + 3.f * S, 22.f * S, 18.f * S, FLinearColor(0.f, 0.f, 0.f, 0.6f));
		DrawLabel(Info.Key, X + 14.f * S, Y + 3.f * S, FLinearColor(1.f, 0.9f, 0.6f), 0.85f * S, true);
		X += Size + Gap;
	}

	// Ultimate com moldura dupla e porcentagem.
	if (Hero->FindAbilityEntry(EValdoriaAbilitySlot::Ultimate))
	{
		const float Pct = Hero->GetUltimatePercent();
		const bool bReady = Pct >= 1.f;
		const float UY = Y - (UltSize - Size) * 0.5f;
		DrawRect(X, UY, UltSize, UltSize, FLinearColor(0.08f, 0.09f, 0.12f, 0.85f));
		DrawRect(X, UY + UltSize * (1.f - Pct), UltSize, UltSize * Pct, Theme * FLinearColor(1.f, 1.f, 1.f, 0.35f));
		const float Pulse = bReady ? 0.6f + 0.4f * FMath::Sin(GetWorld()->GetTimeSeconds() * 6.f) : 1.f;
		DrawFrame(X, UY, UltSize, UltSize, Theme * Pulse, 3.f);
		DrawFrame(X - 5.f * S, UY - 5.f * S, UltSize + 10.f * S, UltSize + 10.f * S, Theme * 0.6f, 1.5f);
		DrawLabel(bReady ? TEXT("PRONTA") : FString::Printf(TEXT("%d%%"), FMath::FloorToInt(Pct * 100.f)),
			X + UltSize * 0.5f, UY + UltSize * 0.3f, bReady ? Theme : FLinearColor::White, (bReady ? 1.2f : 1.6f) * S, true);
		DrawRect(X + 3.f * S, UY + 3.f * S, 22.f * S, 18.f * S, FLinearColor(0.f, 0.f, 0.f, 0.6f));
		DrawLabel(TEXT("Q"), X + 14.f * S, UY + 3.f * S, FLinearColor(1.f, 0.9f, 0.6f), 0.85f * S, true);
	}
}

void AValdoriaHUD::DrawAmmo(AValdoriaHeroCharacter* Hero, float W, float H)
{
	const float S = Scale;
	const UValdoriaHeroDefinition* Def = Hero->GetHeroDefinition();
	const FLinearColor Theme = Def->ThemeColor;
	const FValdoriaWeaponStats& Weapon = Hero->GetWeaponStats();
	const float X = W - 330.f * S;
	const float Y = H - 150.f * S;

	if (Hero->IsReloading())
	{
		DrawLabel(TEXT("RECARREGANDO"), X, Y + 20.f * S, Theme, 1.8f * S, false, true);
	}
	else
	{
		const FString Mag = FString::Printf(TEXT("%02d"), Hero->GetAmmo());
		DrawLabel(Mag, X, Y - 10.f * S, Theme, 3.6f * S, false, true);
		const FString Reserve = Weapon.ReserveAmmo > 0 ? FString::Printf(TEXT("/ %d"), Weapon.ReserveAmmo) : TEXT("/ ∞");
		DrawLabel(Reserve, X + 120.f * S, Y + 30.f * S, FLinearColor(0.8f, 0.82f, 0.88f), 2.0f * S, false, true);
	}

	// Ícones de balas: um por bala no pente (até 30), apagados quando gastas.
	const int32 Shown = FMath::Min(Weapon.MagazineSize, 30);
	const float BulletW = 6.f * S;
	const float BulletH = 16.f * S;
	for (int32 i = 0; i < Shown; ++i)
	{
		const int32 Threshold = FMath::CeilToInt(float(Weapon.MagazineSize) * (i + 1) / Shown);
		const bool bFull = Hero->GetAmmo() >= Threshold;
		const float BX = X + i * (BulletW + 3.f * S);
		DrawRect(BX, Y + 86.f * S, BulletW, BulletH, bFull ? Theme : FLinearColor(0.2f, 0.22f, 0.26f, 0.7f));
		DrawRect(BX, Y + 86.f * S - 4.f * S, BulletW, 4.f * S, bFull ? Theme * 1.3f : FLinearColor(0.2f, 0.22f, 0.26f, 0.5f));
	}
	DrawLabel(Weapon.WeaponName.ToString().ToUpper(), X, Y + 110.f * S, FLinearColor(0.75f, 0.77f, 0.82f), 1.0f * S);
}
