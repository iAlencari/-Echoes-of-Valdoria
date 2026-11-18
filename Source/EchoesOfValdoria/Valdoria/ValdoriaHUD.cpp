// Echoes of Valdoria - HUD de teste.

#include "ValdoriaHUD.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerState.h"
#include "ValdoriaHeroCharacter.h"
#include "ValdoriaHeroDefinition.h"

void AValdoriaHUD::AddHitMarker(const FVector& WorldLocation, float Damage, bool bHeadshot, bool bKill)
{
	LastHitTime = GetWorld()->GetTimeSeconds();
	bLastHitHeadshot = bHeadshot;
	bLastHitKill = bKill;
	DamageNumbers.Add({ WorldLocation + FVector(FMath::FRandRange(-30.f, 30.f), FMath::FRandRange(-30.f, 30.f), 0.f), Damage, bHeadshot, LastHitTime });
}

void AValdoriaHUD::DrawBar(float X, float Y, float W, float H, float Fraction, const FLinearColor& Color)
{
	FCanvasTileItem Back(FVector2D(X, Y), FVector2D(W, H), FLinearColor(0.f, 0.f, 0.f, 0.55f));
	Back.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Back);
	FCanvasTileItem Fill(FVector2D(X, Y), FVector2D(W * FMath::Clamp(Fraction, 0.f, 1.f), H), Color);
	Fill.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Fill);
}

void AValdoriaHUD::DrawLabel(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale, bool bCenter)
{
	UFont* Font = GEngine->GetMediumFont();
	FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Text), Font, Color);
	Item.Scale = FVector2D(Scale, Scale);
	Item.bCentreX = bCenter;
	Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.8f));
	Canvas->DrawItem(Item);
}

void AValdoriaHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	const float W = Canvas->ClipX;
	const float H = Canvas->ClipY;
	const float S = H / 1080.f; // escala para qualquer resolução
	const float Now = GetWorld()->GetTimeSeconds();
	AValdoriaHeroCharacter* Hero = Cast<AValdoriaHeroCharacter>(GetOwningPawn());

	// --- Mira
	const float Cx = W * 0.5f;
	const float Cy = H * 0.5f;
	const bool bHitRecent = Now - LastHitTime < 0.18f;
	const FLinearColor CrossColor = bHitRecent ? (bLastHitHeadshot ? FLinearColor(1.f, 0.25f, 0.2f) : FLinearColor::White) : FLinearColor(1.f, 1.f, 1.f, 0.85f);
	const float Gap = 6.f * S;
	const float Len = 8.f * S;
	DrawLine(Cx - Gap - Len, Cy, Cx - Gap, Cy, CrossColor, 2.f);
	DrawLine(Cx + Gap, Cy, Cx + Gap + Len, Cy, CrossColor, 2.f);
	DrawLine(Cx, Cy - Gap - Len, Cx, Cy - Gap, CrossColor, 2.f);
	DrawLine(Cx, Cy + Gap, Cx, Cy + Gap + Len, CrossColor, 2.f);
	if (bHitRecent)
	{
		// Marcador de acerto em "X".
		const float M = 12.f * S;
		const FLinearColor HitColor = bLastHitKill ? FLinearColor(1.f, 0.15f, 0.15f) : CrossColor;
		DrawLine(Cx - M, Cy - M, Cx - M * 0.45f, Cy - M * 0.45f, HitColor, 2.f);
		DrawLine(Cx + M, Cy - M, Cx + M * 0.45f, Cy - M * 0.45f, HitColor, 2.f);
		DrawLine(Cx - M, Cy + M, Cx - M * 0.45f, Cy + M * 0.45f, HitColor, 2.f);
		DrawLine(Cx + M, Cy + M, Cx + M * 0.45f, Cy + M * 0.45f, HitColor, 2.f);
	}

	// --- Números de dano flutuantes
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

	// --- Estatísticas (FPS, ms, ping)
	if (bShowStats)
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
		DrawLabel(FString::Printf(TEXT("FPS %.0f  |  %.1f ms  |  Ping %s"), Fps, SmoothedFrameMs, *PingText), 16.f * S, 12.f * S, Col, 0.9f * S);
	}

	if (!Hero || !Hero->GetHeroDefinition())
	{
		return;
	}
	const UValdoriaHeroDefinition* Def = Hero->GetHeroDefinition();

	// --- Vida / armadura / escudo (canto inferior esquerdo)
	const float BarX = 60.f * S;
	const float BarW = 380.f * S;
	float BarY = H - 150.f * S;
	DrawLabel(Def->DisplayName.ToString().ToUpper(), BarX, BarY - 34.f * S, Def->ThemeColor, 1.2f * S);
	const float Total = FMath::Max(1.f, Hero->GetMaxHealth() + Hero->GetMaxArmor() + Hero->GetMaxShield());
	const float HpW = BarW * Hero->GetMaxHealth() / Total;
	const float ArW = BarW * Hero->GetMaxArmor() / Total;
	const float ShW = BarW * Hero->GetMaxShield() / Total;
	DrawBar(BarX, BarY, HpW, 22.f * S, Hero->GetHealth() / FMath::Max(1.f, Hero->GetMaxHealth()), FLinearColor(0.95f, 0.95f, 0.95f, 0.95f));
	if (ArW > 0.f)
	{
		DrawBar(BarX + HpW + 2.f, BarY, ArW, 22.f * S, Hero->GetArmor() / Hero->GetMaxArmor(), FLinearColor(1.f, 0.65f, 0.15f, 0.95f));
	}
	if (ShW > 0.f)
	{
		DrawBar(BarX + HpW + ArW + 4.f, BarY, ShW, 22.f * S, Hero->GetShield() / Hero->GetMaxShield(), FLinearColor(0.35f, 0.75f, 1.f, 0.95f));
	}
	DrawLabel(FString::Printf(TEXT("%.0f / %.0f"), Hero->GetHealth() + Hero->GetArmor() + Hero->GetShield(), Total), BarX, BarY + 28.f * S, FLinearColor::White, 1.1f * S);

	// --- Munição (canto inferior direito)
	const FValdoriaWeaponStats& Weapon = Hero->GetWeaponStats();
	const FString AmmoText = Hero->IsReloading() ? TEXT("RECARREGANDO...") : FString::Printf(TEXT("%d / %d"), Hero->GetAmmo(), Weapon.MagazineSize);
	DrawLabel(AmmoText, W - 300.f * S, H - 150.f * S, FLinearColor::White, 1.8f * S);
	DrawLabel(Weapon.WeaponName.ToString(), W - 300.f * S, H - 100.f * S, FLinearColor(0.8f, 0.8f, 0.85f), 1.f * S);

	// --- Habilidades (centro inferior)
	struct FSlotInfo { EValdoriaAbilitySlot Slot; const TCHAR* Key; };
	const FSlotInfo Slots[] = {
		{ EValdoriaAbilitySlot::Ability1, TEXT("F") },
		{ EValdoriaAbilitySlot::Ability2, TEXT("E") },
		{ EValdoriaAbilitySlot::SecondaryFire, TEXT("BOTÃO DIR.") },
	};
	float SlotX = W * 0.5f - 220.f * S;
	const float SlotY = H - 130.f * S;
	for (const FSlotInfo& Info : Slots)
	{
		const FValdoriaAbilityEntry* Entry = Hero->FindAbilityEntry(Info.Slot);
		if (!Entry)
		{
			continue;
		}
		const float Remaining = Hero->GetCooldownRemaining(Info.Slot);
		const FLinearColor BoxColor = Remaining > 0.f ? FLinearColor(0.1f, 0.1f, 0.12f, 0.7f) : FLinearColor(0.2f, 0.2f, 0.25f, 0.8f);
		FCanvasTileItem Box(FVector2D(SlotX, SlotY), FVector2D(130.f * S, 70.f * S), BoxColor);
		Box.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Box);
		DrawLabel(Info.Key, SlotX + 65.f * S, SlotY + 4.f * S, FLinearColor(1.f, 0.85f, 0.5f), 0.8f * S, true);
		DrawLabel(Entry->DisplayName.ToString(), SlotX + 65.f * S, SlotY + 24.f * S, FLinearColor::White, 0.8f * S, true);
		if (Remaining > 0.f)
		{
			DrawLabel(FString::Printf(TEXT("%.1f"), Remaining), SlotX + 65.f * S, SlotY + 44.f * S, FLinearColor(1.f, 0.6f, 0.4f), 0.9f * S, true);
		}
		SlotX += 145.f * S;
	}

	// --- Movimento: cargas de impulso e estado atual
	const FValdoriaMovementRules& Rules = Def->Movement;
	FString MoveText;
	if (Rules.ShiftMode == EValdoriaShiftMode::Dash || Rules.ShiftMode == EValdoriaShiftMode::SprintAndDash)
	{
		MoveText += TEXT("IMPULSO ");
		for (int32 i = 0; i < FMath::Clamp(Rules.DashCharges, 1, 5); ++i)
		{
			MoveText += i < Hero->GetDashCharges() ? TEXT("[#] ") : TEXT("[ ] ");
		}
	}
	if (Hero->IsSliding()) MoveText += TEXT(" DESLIZANDO");
	else if (Hero->IsMantling()) MoveText += TEXT(" SUBINDO");
	else if (Hero->IsWallClimbing()) MoveText += TEXT(" ESCALANDO");
	else if (Hero->IsProne()) MoveText += TEXT(" DEITADO");
	else if (Hero->bIsCrouched) MoveText += TEXT(" AGACHADO");
	else if (Hero->IsSprinting()) MoveText += TEXT(" CORRENDO");
	DrawLabel(MoveText, BarX, BarY + 56.f * S, FLinearColor(0.6f, 0.9f, 1.f), 0.9f * S);

	// --- Ultimate
	if (Hero->FindAbilityEntry(EValdoriaAbilitySlot::Ultimate))
	{
		const float Pct = Hero->GetUltimatePercent();
		const bool bReady = Pct >= 1.f;
		const FString UltText = bReady ? TEXT("ULTIMATE PRONTA [Q]") : FString::Printf(TEXT("ULTIMATE %d%%"), FMath::FloorToInt(Pct * 100.f));
		DrawLabel(UltText, W * 0.5f, SlotY - 50.f * S, bReady ? FLinearColor(1.f, 0.75f, 0.2f) : FLinearColor(0.85f, 0.85f, 0.9f), 1.1f * S, true);
	}

	if (Hero->IsDead())
	{
		DrawLabel(TEXT("VOCÊ FOI ABATIDO — renascendo..."), W * 0.5f, H * 0.4f, FLinearColor(1.f, 0.35f, 0.3f), 2.f * S, true);
	}
}
