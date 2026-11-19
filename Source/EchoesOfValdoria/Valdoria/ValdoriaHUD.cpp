// Echoes of Valdoria - HUD desenhado em código.

#include "ValdoriaHUD.h"
#include "ValdoriaVision.h"
#include "ValdoriaMobaMode.h"
#include "ValdoriaGuardian.h"
#include "ValdoriaTrainingDummy.h"
#include "Engine/StaticMeshActor.h"
#include "CanvasItem.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "ValdoriaHeroCharacter.h"
#include "ValdoriaPlayerController.h"
#include "ValdoriaHeroDefinition.h"
#include "Engine/Texture2D.h"

/** Maiúsculas com acentos (FString::ToUpper só troca letras sem acento). */
static FString ToUpperPt(const FString& In)
{
	FString Out = In.ToUpper();
	static const TCHAR* Lower = TEXT("áàâãéêíóôõúüç");
	static const TCHAR* Upper = TEXT("ÁÀÂÃÉÊÍÓÔÕÚÜÇ");
	for (TCHAR& C : Out)
	{
		for (int32 i = 0; Lower[i]; ++i)
		{
			if (C == Lower[i])
			{
				C = Upper[i];
				break;
			}
		}
	}
	return Out;
}

#include "ValdoriaSettings.h"
#include "ValdoriaAudio.h"
#include "ValdoriaDeathmatchMode.h"
#include "ValdoriaMobaMode.h"

// ------------------------------------------------------------------ utilidades de desenho

void AValdoriaHUD::AddHitMarker(const FVector& WorldLocation, float Damage, bool bHeadshot, bool bKill)
{
	LastHitTime = GetWorld()->GetTimeSeconds();
	if (Damage < 0.f)
	{
		// Hit an immune target (semi-sealed guardian): show IMMUNE instead of damage.
		DamageNumbers.Add({ WorldLocation, -1.f, false, LastHitTime });
		return;
	}
	bLastHitHeadshot = bHeadshot;
	bLastHitKill = bKill;
	TotalDamage += Damage;
	RecentDamage.Add(TPair<float, float>(LastHitTime, Damage));
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
	if (bShowDamageMeter)
	{
		DrawDamageMeter(W, H);
	}
	DrawChat(W, H);
	DrawModeScore(W, H);
	if (!Hero || !Hero->GetHeroDefinition())
	{
		return;
	}
	DrawPlayerPanel(Hero, W, H);
	DrawAbilities(Hero, W, H);
	DrawAmmo(Hero, W, H);
	if (bShowMinimap)
	{
		DrawMinimap(Hero, W, H);
	}
	DrawTopHeroBar(Hero, W, H);
	if (Hero && Hero->bUsesLevels && !Hero->IsDead())
	{
		DrawLevelInfo(Hero, W, H);
	}
	if (bShowScoreboard)
	{
		DrawScoreboard(Hero, W, H);
	}
	if (bShowDeathRecap)
	{
		DrawDeathRecap(Hero, W, H);
	}
	DrawStatusLine(Hero, W, H);
	DrawDamageFeedback(Hero, W, H);

	if (Hero->IsDead())
	{
		DrawLabel(Hero->RespawnAt > GetWorld()->GetTimeSeconds() ? FString::Printf(TEXT("VOCÊ FOI ABATIDO — renasce em %.0f s  (segure Y: resumo da morte)"), FMath::CeilToFloat(Hero->RespawnAt - GetWorld()->GetTimeSeconds())) : FString(TEXT("VOCÊ FOI ABATIDO — renascendo...")), W * 0.5f, H * 0.4f, EnemyColor, 1.f * Scale, true, true);
	}
}

void AValdoriaHUD::DrawCrosshairAndHits(float W, float H)
{
	const float S = Scale;
	const float Now = GetWorld()->GetTimeSeconds();
	const float Cx = W * 0.5f;
	const float Cy = H * 0.5f;
	const bool bHitRecent = Now - LastHitTime < 0.18f;
	const FValdoriaOptions& Options = FValdoriaOptions::Get();
	const FLinearColor BaseColor = Options.GetCrosshairColor();
	const FLinearColor CrossColor = bHitRecent ? (bLastHitHeadshot ? EnemyColor : FLinearColor::White) : BaseColor;
	const float Gap = 6.f * S * Options.CrosshairSize;
	const float Len = 8.f * S * Options.CrosshairSize;
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
			if (N.bHeal)
			{
				DrawLabel(FString::Printf(TEXT("+%d"), FMath::RoundToInt(N.Damage)), Screen.X, Screen.Y, FLinearColor(0.35f, 1.f, 0.45f), 1.15f * S, true);
			}
			else
			{
				DrawLabel(N.Damage < 0.f ? FString(TEXT("IMMUNE")) : FString::Printf(TEXT("%d"), FMath::RoundToInt(N.Damage)), Screen.X, Screen.Y, N.Damage < 0.f ? FLinearColor(0.7f, 0.85f, 1.f) : C, (N.bHeadshot ? 1.6f : 1.2f) * S, true);
			}
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
		if (bEnemy && Viewer)
		{
			if (const UValdoriaVisionSubsystem* Vis = GetWorld()->GetSubsystem<UValdoriaVisionSubsystem>(); Vis && Vis->IsHiddenInFogFrom(Other, Viewer->TeamId))
			{
				continue; // in fog, nobody of my team inside: no bar
			}
		}
		// Inimigo atrás de parede não mostra barra (aliado aparece sempre, estilo hero shooter).
		if (bEnemy && PlayerOwner)
		{
			FVector CamLoc;
			FRotator CamRot;
			PlayerOwner->GetPlayerViewPoint(CamLoc, CamRot);
			FCollisionQueryParams Params(SCENE_QUERY_STAT(ValdoriaHudLos), false, Viewer);
			Params.AddIgnoredActor(Other);
			FHitResult Block;
			if (GetWorld()->LineTraceSingleByChannel(Block, CamLoc, Head, ECC_Visibility, Params))
			{
				continue;
			}
		}
		FLinearColor Color = bEnemy ? EnemyColor : AllyColor;
		// Allied heroes (owner, turn 46): green above half, yellow down to 25%, red blinking below with a heart
		// (a cross for viewers who heal: "this one needs you").
		const float Frac = Other->GetHealth() / FMath::Max(1.f, Other->GetMaxHealth());
		const bool bAllyHero = !bEnemy && Other->IsHeroUnit();
		if (bAllyHero)
		{
			Color = Frac > 0.5f ? FLinearColor(0.3f, 0.95f, 0.4f) : Frac > 0.25f ? FLinearColor(1.f, 0.85f, 0.2f)
				: FLinearColor(1.f, 0.2f, 0.2f) * (0.55f + 0.45f * FMath::Abs(FMath::Sin(GetWorld()->GetTimeSeconds() * 6.f)));
		}
		const float BarW = FMath::Clamp(140.f * 1200.f / FMath::Max(Dist, 1.f), 70.f, 150.f) * S;
		const float BarH = 9.f * S;
		const float X = Screen.X - BarW * 0.5f;
		const float Y = Screen.Y;
		DrawSegmentedBar(X, Y, BarW, BarH, Other, Color);
		if (bEnemy)
		{
			DrawSkull(X - 12.f * S, Y + BarH * 0.5f, 11.f * S, Color);
			// Deus do Velho Testamento (Nazaré): an enemy hero in execution range shows a blinking golden cross.
			const float Exec = Viewer ? Viewer->GetPassive().ExecuteThreshold : 0.f;
			if (Exec > 0.f && Other->IsHeroUnit() && Frac <= Exec && Dist <= 2500.f)
			{
				const FLinearColor CrossColor = FLinearColor(1.f, 0.85f, 0.35f) * (0.5f + 0.5f * FMath::Abs(FMath::Sin(GetWorld()->GetTimeSeconds() * 8.f)));
				const float Cx = Screen.X;
				const float Cy = Y - 22.f * S;
				DrawRect(Cx - 3.f * S, Cy - 14.f * S, 6.f * S, 26.f * S, CrossColor);
				DrawRect(Cx - 10.f * S, Cy - 7.f * S, 20.f * S, 6.f * S, CrossColor);
			}
		}
		else if (bAllyHero && Frac <= 0.25f)
		{
			const bool bHealer = Viewer && Viewer->GetHeroDefinition() && Viewer->GetHeroDefinition()->Roles.Contains(EValdoriaHeroRole::Rearguard);
			const float Cx = X - 14.f * S;
			const float Cy = Y + BarH * 0.5f;
			if (bHealer)
			{
				DrawRect(Cx - 2.5f * S, Cy - 8.f * S, 5.f * S, 16.f * S, Color); // cross: "needs your healing"
				DrawRect(Cx - 8.f * S, Cy - 2.5f * S, 16.f * S, 5.f * S, Color);
			}
			else
			{
				DrawLabel(TEXT("\u2665"), Cx, Cy - 10.f * S, Color, 1.2f * S, true);
			}
		}
		if (false) // owner (turn 47): no numbers on any bar above heads; only the bar and floating damage/heal numbers
		{
			DrawLabel(FString::Printf(TEXT("%.0f"), Other->GetHealth() + Other->GetArmor() + Other->GetShield()),
				Screen.X, Y - 22.f * S, Color, 1.0f * S, true);
		}
		if (bAllyHero)
		{
			// Diamond in the middle of the bar: dark grey, green when the ally's supreme is ready (owner).
			const bool bUlt = Other->IsUltimateReady() && !Other->IsSlotLocked(EValdoriaAbilitySlot::Ultimate);
			const float Cx = X + BarW * 0.5f, Cy = Y + BarH * 0.5f, R = 8.f * S;
			const FLinearColor Edge(0.18f, 0.18f, 0.2f), Fill = bUlt ? FLinearColor(0.25f, 1.f, 0.4f) : FLinearColor(0.3f, 0.3f, 0.33f);
			for (int32 Pass = 0; Pass < 2; ++Pass)
			{
				const float RR = Pass == 0 ? R : R - 2.5f * S;
				const FLinearColor C = Pass == 0 ? Edge : Fill;
				FCanvasTriangleItem Tri(FVector2D(Cx, Cy - RR), FVector2D(Cx + RR, Cy), FVector2D(Cx, Cy + RR), GWhiteTexture);
				Tri.SetColor(C);
				Canvas->DrawItem(Tri);
				FCanvasTriangleItem Tri2(FVector2D(Cx, Cy - RR), FVector2D(Cx - RR, Cy), FVector2D(Cx, Cy + RR), GWhiteTexture);
				Tri2.SetColor(C);
				Canvas->DrawItem(Tri2);
			}
			if (bShowAllyCooldowns)
			{
				// 1/2/3/4 above the name: seconds left, or the key in green when ready; 4 shows the supreme charge.
				const EValdoriaAbilitySlot Slots[4] = { EValdoriaAbilitySlot::Ability1, EValdoriaAbilitySlot::Ability2, EValdoriaAbilitySlot::Ability3, EValdoriaAbilitySlot::Ultimate };
				const TCHAR* Keys[4] = { TEXT("1"), TEXT("2"), TEXT("3"), TEXT("4") };
				const float Box = 22.f * S, BGap = 3.f * S;
				float BX = Screen.X - (Box * 4.f + BGap * 3.f) * 0.5f;
				const float BY = Y - 50.f * S;
				for (int32 i = 0; i < 4; ++i)
				{
					const bool bLocked = Other->IsSlotLocked(Slots[i]) || !Other->FindAbilityEntry(Slots[i]);
					const float Left = Other->GetCooldownRemaining(Slots[i]);
					FString Txt = Keys[i];
					FLinearColor C(0.35f, 1.f, 0.45f);
					if (bLocked) { Txt = TEXT("-"); C = FLinearColor(0.45f, 0.45f, 0.5f); }
					else if (i == 3 && !Other->IsUltimateReady()) { Txt = FString::Printf(TEXT("%d%%"), FMath::FloorToInt(Other->GetUltimatePercent() * 100.f)); C = FLinearColor(0.9f, 0.9f, 0.95f); }
					else if (Left > 0.f) { Txt = FString::Printf(TEXT("%.0f"), FMath::CeilToFloat(Left)); C = FLinearColor(1.f, 0.6f, 0.4f); }
					DrawRect(BX, BY, Box, Box, FLinearColor(0.f, 0.f, 0.f, 0.7f));
					DrawFrame(BX, BY, Box, Box, C * 0.8f, 1.f);
					DrawLabel(Txt, BX + Box * 0.5f, BY + 3.f * S, C, 0.7f * S, true);
					BX += Box + BGap;
				}
			}
		}
		// Stacks I put on this hero (only I see them), to the right of the bar.
		float StX = X + BarW + 6.f * S;
		for (const AValdoriaHeroCharacter::FValdoriaStack& St : Other->GetStacks())
		{
			if (Viewer && St.Source.Get() == Viewer && Other->IsStackLive(St))
			{
				DrawStackIcon(StX, Y - 6.f * S, 20.f * S, St.Label, St.Count, St.Color, false);
				StX += 24.f * S;
			}
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
	DrawLabel(ToUpperPt(Def->DisplayName.ToString()), X, Y - 78.f * S, Theme, 1.8f * S, false, true);
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
	if (Rules.FlightMode != EValdoriaFlightMode::None)
	{
		const float Fuel = Hero->GetFlightFuelPercent();
		const float FX = X;
		const float FY = Y + BarH + 46.f * S;
		DrawLabel(Rules.FlightMode == EValdoriaFlightMode::Fuel ? TEXT("COMBUSTÍVEL") : TEXT("VOO"), FX, FY - 2.f * S, FLinearColor(0.6f, 0.9f, 1.f), 0.75f * S);
		DrawRect(FX + 120.f * S, FY + 4.f * S, 200.f * S, 10.f * S, FLinearColor(0.f, 0.f, 0.f, 0.6f));
		DrawRect(FX + 120.f * S, FY + 4.f * S, 200.f * S * Fuel, 10.f * S, FLinearColor(0.45f, 0.85f, 1.f, 0.95f));
	}
	const float VigorPct = Hero->GetVigorPercent();
	if (VigorPct >= 0.f)
	{
		// Vigor (turn 45, only Lume for now): Shift sprint drains it, dashes cost a chunk, it refills after a pause.
		const float VX = X;
		const float VY = Y + BarH + (Rules.FlightMode != EValdoriaFlightMode::None ? 64.f : 46.f) * S;
		const FLinearColor VigorColor = VigorPct < 0.3f ? FLinearColor(1.f, 0.55f, 0.2f, 0.95f) : FLinearColor(0.55f, 1.f, 0.45f, 0.95f);
		DrawLabel(TEXT("VIGOR"), VX, VY - 2.f * S, VigorColor, 0.75f * S);
		DrawRect(VX + 120.f * S, VY + 4.f * S, 200.f * S, 10.f * S, FLinearColor(0.f, 0.f, 0.f, 0.6f));
		DrawRect(VX + 120.f * S, VY + 4.f * S, 200.f * S * VigorPct, 10.f * S, VigorColor);
	}
	if (Hero->IsFlying()) MoveText = TEXT("VOANDO");
	else if (Hero->IsGliding()) MoveText = TEXT("PLANANDO");
	else if (Hero->IsSliding()) MoveText = TEXT("DESLIZANDO");
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

void AValdoriaHUD::DrawClock(float Cx, float Cy, float Radius, float Fraction, const FLinearColor& Color)
{
	// Clock-style cooldown: a dark pie that shrinks clockwise from 12 o'clock as the cooldown ends.
	if (!Canvas || Fraction <= 0.f)
	{
		return;
	}
	const int32 Segments = FMath::Max(3, FMath::CeilToInt(36.f * Fraction));
	const float Start = -HALF_PI;
	const float Sweep = TWO_PI * FMath::Clamp(Fraction, 0.f, 1.f);
	const FVector2D C(Cx, Cy);
	for (int32 i = 0; i < Segments; ++i)
	{
		const float A0 = Start + Sweep * i / Segments;
		const float A1 = Start + Sweep * (i + 1) / Segments;
		FCanvasTriangleItem Tri(C, C + FVector2D(FMath::Cos(A0), FMath::Sin(A0)) * Radius,
			C + FVector2D(FMath::Cos(A1), FMath::Sin(A1)) * Radius, GWhiteTexture);
		Tri.SetColor(Color);
		Tri.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Tri);
	}
}

void AValdoriaHUD::DrawStackIcon(float X, float Y, float Size, const FString& Label, int32 Count, const FLinearColor& Color, bool bEnemyMark)
{
	const float S = Scale;
	const float TextK = Size / (34.f * S);
	DrawRect(X, Y, Size, Size, FLinearColor(0.05f, 0.05f, 0.08f, 0.75f));
	DrawRect(X, Y + Size - 4.f * S, Size, 4.f * S, Color);
	DrawFrame(X, Y, Size, Size, bEnemyMark ? FLinearColor(1.f, 0.15f, 0.15f) : Color, bEnemyMark ? 2.f : 1.f);
	DrawLabel(Label, X + Size * 0.5f, Y + Size * 0.12f, Color, TextK * 0.8f * S, true);
	if (Count > 1)
	{
		DrawLabel(FString::FromInt(Count), X + Size - 4.f * S, Y + Size * 0.45f, FLinearColor::White, TextK * 0.7f * S, true);
	}
	if (bEnemyMark && Canvas)
	{
		// Small red triangle in the corner = an enemy put this on you.
		FCanvasTriangleItem Tri(FVector2D(X, Y), FVector2D(X + Size * 0.4f, Y), FVector2D(X, Y + Size * 0.4f), GWhiteTexture);
		Tri.SetColor(FLinearColor(1.f, 0.1f, 0.1f, 0.95f));
		Canvas->DrawItem(Tri);
	}
}

void AValdoriaHUD::DrawAbilities(AValdoriaHeroCharacter* Hero, float W, float H)
{
	// Standard layout (owner, turn 45): only 1, 2, 3 and 4 (supreme) on screen; C smaller on the left when the hero
	// has it; passive and the right-click ability as small see-through icons with a clock cooldown; the rest is on K.
	const float S = Scale;
	const UValdoriaHeroDefinition* Def = Hero->GetHeroDefinition();
	const FLinearColor Theme = Hero->GetElementColor();
	const float Size = 64.f * S;
	const float Gap = 12.f * S;
	const float Y = H - 112.f * S;
	const FValdoriaOptions& KeyOpt = FValdoriaOptions::Get();
	const bool bHasExtra = Hero->FindAbilityEntry(EValdoriaAbilitySlot::AbilityExtra) != nullptr;
	const float ExtraSize = 48.f * S;
	const float UltSize = 86.f * S;
	const float TotalW = (bHasExtra ? ExtraSize + Gap : 0.f) + (Size + Gap) * 3.f + UltSize;
	float X = W * 0.5f - TotalW * 0.5f;
	const float RowLeft = X;

	auto DrawSlot = [&](EValdoriaAbilitySlot Slot, float SX, float SY, float SSize)
	{
		const FValdoriaAbilityEntry* Entry = Hero->FindAbilityEntry(Slot);
		const float Remaining = Entry ? Hero->GetCooldownRemaining(Slot) : 0.f;
		const float Cooldown = Entry ? FMath::Max(0.01f, Entry->Cooldown) : 1.f;
		const float K = SSize / Size;
		DrawRect(SX, SY, SSize, SSize, FLinearColor(0.08f, 0.09f, 0.12f, Entry ? 0.85f : 0.35f));
		const bool bHasIcon = Entry && DrawIcon(Entry->Icon, SX + 2.f * S, SY + 2.f * S, SSize - 4.f * S,
			Remaining > 0.f || !Hero->CanUseSlot(Slot) ? FLinearColor(0.35f, 0.35f, 0.4f) : FLinearColor::White);
		if (Entry && Remaining > 0.f)
		{
			const float Frac = FMath::Clamp(Remaining / Cooldown, 0.f, 1.f);
			DrawRect(SX, SY + SSize * (1.f - Frac), SSize, SSize * Frac, FLinearColor(0.f, 0.f, 0.f, 0.6f));
			DrawLabel(FString::Printf(TEXT("%.0f"), FMath::CeilToFloat(Remaining)), SX + SSize * 0.5f, SY + SSize * 0.22f, FLinearColor::White, K * 1.8f * S, true);
		}
		else if (Entry && !bHasIcon)
		{
			// Whole name, wrapped by words (up to 3 short lines).
			TArray<FString> Words;
			Entry->DisplayName.ToString().ParseIntoArray(Words, TEXT(" "));
			TArray<FString> NameLines;
			FString Cur;
			for (const FString& Wd : Words)
			{
				if (!Cur.IsEmpty() && Cur.Len() + 1 + Wd.Len() > 10) { NameLines.Add(Cur); Cur = Wd; }
				else { Cur = Cur.IsEmpty() ? Wd : Cur + TEXT(" ") + Wd; }
			}
			if (!Cur.IsEmpty()) { NameLines.Add(Cur); }
			const int32 NumLines = FMath::Min(NameLines.Num(), 3);
			const float LineH = SSize * 0.2f;
			const float Y0 = SY + SSize * 0.52f - LineH * NumLines * 0.5f;
			for (int32 L = 0; L < NumLines; ++L)
			{
				DrawLabel(NameLines[L], SX + SSize * 0.5f, Y0 + LineH * L, FLinearColor::White, K * 0.72f * S, true);
			}
		}
		DrawFrame(SX, SY, SSize, SSize, Entry && Remaining <= 0.f ? Theme : FLinearColor(0.35f, 0.37f, 0.42f), 2.f);
		DrawRect(SX + 3.f * S, SY + 3.f * S, 22.f * S, 18.f * S, FLinearColor(0.f, 0.f, 0.f, 0.6f));
		DrawLabel(KeyOpt.GetSlotKeyLabel(Slot), SX + 14.f * S, SY + 3.f * S, FLinearColor(1.f, 0.9f, 0.6f), 0.85f * S, true);
	};

	if (bHasExtra)
	{
		DrawSlot(EValdoriaAbilitySlot::AbilityExtra, X, Y + (Size - ExtraSize), ExtraSize);
		X += ExtraSize + Gap;
	}
	for (EValdoriaAbilitySlot Slot : { EValdoriaAbilitySlot::Ability1, EValdoriaAbilitySlot::Ability2, EValdoriaAbilitySlot::Ability3 })
	{
		DrawSlot(Slot, X, Y, Size);
		X += Size + Gap;
	}

	// Supreme (4) with double frame and percentage; hold-to-cast clock (Kay) and the current element above it.
	const float UltX = X;
	const float UY = Y - (UltSize - Size) * 0.5f;
	if (const FValdoriaAbilityEntry* UltEntry = Hero->FindAbilityEntry(EValdoriaAbilitySlot::Ultimate))
	{
		const float Pct = Hero->GetUltimatePercent();
		const bool bReady = Pct >= 1.f;
		const float CancelLeft = Hero->GetCooldownRemaining(EValdoriaAbilitySlot::Ultimate);
		DrawRect(X, UY, UltSize, UltSize, FLinearColor(0.08f, 0.09f, 0.12f, 0.85f));
		DrawIcon(UltEntry->Icon, X + 2.f * S, UY + 2.f * S, UltSize - 4.f * S, bReady ? FLinearColor::White : FLinearColor(0.45f, 0.45f, 0.5f, 0.8f));
		DrawRect(X, UY + UltSize * (1.f - Pct), UltSize, UltSize * Pct, Theme * FLinearColor(1.f, 1.f, 1.f, 0.35f));
		const float Hold = Hero->GetUltimateHoldProgress();
		if (Hold >= 0.f)
		{
			DrawClock(X + UltSize * 0.5f, UY + UltSize * 0.5f, UltSize * 0.45f, Hold, Theme * FLinearColor(1.f, 1.f, 1.f, 0.55f));
			DrawLabel(TEXT("SEGURE"), X + UltSize * 0.5f, UY + UltSize + 6.f * S, Theme, 0.8f * S, true);
		}
		const float Pulse = bReady ? 0.6f + 0.4f * FMath::Sin(GetWorld()->GetTimeSeconds() * 6.f) : 1.f;
		DrawFrame(X, UY, UltSize, UltSize, Theme * Pulse, 3.f);
		DrawFrame(X - 5.f * S, UY - 5.f * S, UltSize + 10.f * S, UltSize + 10.f * S, Theme * 0.6f, 1.5f);
		FString UltText = bReady ? FString(TEXT("PRONTA")) : FString::Printf(TEXT("%d%%"), FMath::FloorToInt(Pct * 100.f));
		if (bReady && CancelLeft > 0.f)
		{
			UltText = FString::Printf(TEXT("%.0f"), FMath::CeilToFloat(CancelLeft)); // cancelled hold: short lock
		}
		DrawLabel(UltText, X + UltSize * 0.5f, UY + UltSize * 0.3f, bReady ? Theme : FLinearColor::White, (bReady ? 1.2f : 1.6f) * S, true);
		DrawRect(X + 3.f * S, UY + 3.f * S, 22.f * S, 18.f * S, FLinearColor(0.f, 0.f, 0.f, 0.6f));
		DrawLabel(KeyOpt.GetSlotKeyLabel(EValdoriaAbilitySlot::Ultimate), X + 14.f * S, UY + 3.f * S, FLinearColor(1.f, 0.9f, 0.6f), 0.85f * S, true);
	}
	if (Def->ElementCount > 0)
	{
		// Element tag above the supreme: name in its colour + the switch cooldown (tap 4), one dot per element.
		const float Switch = Hero->GetElementSwitchRemaining();
		const FString Tag = ToUpperPt(Hero->GetElementName().ToString()) + (Switch > 0.f ? FString::Printf(TEXT("  %.0f"), FMath::CeilToFloat(Switch)) : FString());
		DrawRect(UltX - 6.f * S, UY - 34.f * S, UltSize + 12.f * S, 24.f * S, FLinearColor(0.f, 0.f, 0.f, 0.55f));
		DrawLabel(Tag, UltX + UltSize * 0.5f, UY - 33.f * S, Theme, 1.f * S, true);
		for (int32 i = 0; i < Def->ElementCount; ++i)
		{
			const FLinearColor Dot = Def->ElementColors.IsValidIndex(i) ? Def->ElementColors[i] : FLinearColor::White;
			const float DX = UltX + UltSize * 0.5f + (i - (Def->ElementCount - 1) * 0.5f) * 14.f * S;
			DrawRect(DX - 4.f * S, UY - 44.f * S, 8.f * S, 6.f * S, i == Hero->GetCurrentElement() ? Dot : Dot * FLinearColor(0.4f, 0.4f, 0.4f, 0.7f));
		}
	}

	// Passive and right-click ability: small, see-through, clock cooldown (right of the supreme).
	{
		const float Small = 40.f * S;
		float PX = UltX + UltSize + Gap * 1.5f;
		const float PY = Y + Size - Small;
		const FLinearColor Ghost(1.f, 1.f, 1.f, 0.55f);
		DrawRect(PX, PY, Small, Small, FLinearColor(0.08f, 0.09f, 0.12f, 0.35f));
		if (!DrawIcon(Def->PassiveIcon, PX + 2.f * S, PY + 2.f * S, Small - 4.f * S, Ghost))
		{
			DrawLabel(TEXT("P"), PX + Small * 0.5f, PY + Small * 0.22f, Theme * Ghost, 0.9f * S, true);
		}
		DrawFrame(PX, PY, Small, Small, Theme * FLinearColor(1.f, 1.f, 1.f, 0.45f), 1.f);
		PX += Small + Gap * 0.6f;
		if (const FValdoriaAbilityEntry* Rmb = Hero->FindAbilityEntry(EValdoriaAbilitySlot::SecondaryFire))
		{
			const float Remaining = Hero->GetCooldownRemaining(EValdoriaAbilitySlot::SecondaryFire);
			DrawRect(PX, PY, Small, Small, FLinearColor(0.08f, 0.09f, 0.12f, 0.35f));
			if (!DrawIcon(Rmb->Icon, PX + 2.f * S, PY + 2.f * S, Small - 4.f * S, Ghost))
			{
				DrawLabel(TEXT("DIR"), PX + Small * 0.5f, PY + Small * 0.25f, FLinearColor(1.f, 1.f, 1.f, 0.6f), 0.7f * S, true);
			}
			if (Remaining > 0.f)
			{
				DrawClock(PX + Small * 0.5f, PY + Small * 0.5f, Small * 0.48f, FMath::Clamp(Remaining / FMath::Max(0.01f, Rmb->Cooldown), 0.f, 1.f), FLinearColor(0.f, 0.f, 0.f, 0.6f));
				DrawLabel(FString::Printf(TEXT("%.0f"), FMath::CeilToFloat(Remaining)), PX + Small * 0.5f, PY + Small * 0.2f, FLinearColor::White, 1.1f * S, true);
			}
			DrawFrame(PX, PY, Small, Small, Remaining <= 0.f ? Theme * FLinearColor(1.f, 1.f, 1.f, 0.6f) : FLinearColor(0.35f, 0.37f, 0.42f, 0.6f), 1.f);
		}
	}

	// Stacks above the abilities: mine (small) and the ones an enemy put on me (smaller, red mark).
	{
		float SX = RowLeft;
		const float SY = UY - (Def->ElementCount > 0 ? 82.f : 40.f) * S;
		for (const AValdoriaHeroCharacter::FValdoriaStack& St : Hero->GetStacks())
		{
			AValdoriaHeroCharacter* Src = St.Source.Get();
			if (!Hero->IsStackLive(St) || !Src)
			{
				continue;
			}
			const bool bMine = Src == Hero;
			const bool bEnemy = Hero->IsEnemy(Src);
			if (!bMine && !bEnemy)
			{
				continue;
			}
			const float StSize = (bMine ? 30.f : 24.f) * S;
			DrawStackIcon(SX, SY + (30.f * S - StSize), StSize, St.Label, St.Count, St.Color, bEnemy);
			SX += StSize + 6.f * S;
		}
	}
}

void AValdoriaHUD::DrawAmmo(AValdoriaHeroCharacter* Hero, float W, float H)
{
	const float S = Scale;
	if (const AValdoriaPlayerController* VPC = Cast<AValdoriaPlayerController>(GetOwningPlayerController()))
	{
		DrawLabel(FString::Printf(TEXT("◆ %d   (B = loja)"), VPC->GetCredits()), W - 330.f * S, H - 214.f * S, FLinearColor(1.f, 0.85f, 0.4f), 0.95f * S);
	}
	const UValdoriaHeroDefinition* Def = Hero->GetHeroDefinition();
	const FLinearColor Theme = Def->ThemeColor;
	const FValdoriaWeaponStats& Weapon = Hero->GetWeaponStats();
	const float X = W - 330.f * S;
	const float Y = H - 150.f * S;

	if (Hero->HasWeaponOverride())
	{
		DrawLabel(ToUpperPt(Weapon.WeaponName.ToString()), X, Y - 40.f * S, Theme, 1.0f * S);
	}
	if (!Weapon.bUsesAmmo)
	{
		// Arma corpo a corpo: sem munição, mostra o nome da arma.
		DrawLabel(ToUpperPt(Weapon.WeaponName.ToString()), X, Y + 30.f * S, Theme, 1.4f * S, false, true);
		DrawLabel(TEXT("CORPO A CORPO"), X, Y + 86.f * S, FLinearColor(0.8f, 0.82f, 0.88f), 0.9f * S);
		return;
	}

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
	DrawLabel(ToUpperPt(Weapon.WeaponName.ToString()), X, Y + 110.f * S, FLinearColor(0.75f, 0.77f, 0.82f), 1.0f * S);
}

void AValdoriaHUD::DrawDamageMeter(float W, float H)
{
	const float S = Scale;
	const float Now = GetWorld()->GetTimeSeconds();
	RecentDamage.RemoveAll([Now](const TPair<float, float>& E) { return Now - E.Key > 5.f; });
	float Last5 = 0.f;
	for (const TPair<float, float>& E : RecentDamage)
	{
		Last5 += E.Value;
	}
	const float Elapsed = FMath::Max(1.f, Now - MeterStartTime);
	const float X = W - 300.f * S;
	const float Y = 120.f * S;
	DrawRect(X - 10.f * S, Y - 8.f * S, 280.f * S, 100.f * S, FLinearColor(0.f, 0.f, 0.f, 0.55f));
	DrawLabel(TEXT("MEDIDOR DE DANO"), X, Y, FLinearColor(1.f, 0.62f, 0.4f), 0.9f * S);
	DrawLabel(FString::Printf(TEXT("DPS (últimos 5 s): %.0f"), Last5 / 5.f), X, Y + 24.f * S, FLinearColor::White, 1.f * S);
	DrawLabel(FString::Printf(TEXT("Total: %.0f  •  média %.0f/s"), TotalDamage, TotalDamage / Elapsed), X, Y + 50.f * S, FLinearColor(0.8f, 0.82f, 0.88f), 0.9f * S);
}
void AValdoriaHUD::AddChatLine(const FString& Line, const FLinearColor& Color)
{
	ChatLines.Add({ Line, Color, GetWorld() ? static_cast<float>(GetWorld()->GetTimeSeconds()) : 0.f });
	if (ChatLines.Num() > 8)
	{
		ChatLines.RemoveAt(0);
	}
}

void AValdoriaHUD::DrawChat(float W, float H)
{
	const float S = Scale;
	const float Now = GetWorld()->GetTimeSeconds();
	float Y = H - 330.f * S;
	for (int32 i = ChatLines.Num() - 1; i >= 0; --i)
	{
		const FChatLine& L = ChatLines[i];
		const float Age = Now - L.Time;
		if (Age > 12.f && !bChatOpen)
		{
			continue;
		}
		const float Alpha = bChatOpen ? 1.f : FMath::Clamp(12.f - Age, 0.f, 1.f);
		DrawLabel(L.Text, 48.f * S, Y, L.Color * FLinearColor(1.f, 1.f, 1.f, Alpha), 0.95f * S);
		Y -= 24.f * S;
	}
}
bool AValdoriaHUD::DrawIcon(const TSoftObjectPtr<UTexture2D>& Icon, float X, float Y, float Size, const FLinearColor& Tint)
{
	if (Icon.IsNull())
	{
		return false;
	}
	UTexture2D* Texture = Icon.LoadSynchronous();
	if (!Texture)
	{
		return false;
	}
	DrawTexture(Texture, X, Y, Size, Size, 0.f, 0.f, 1.f, 1.f, Tint);
	return true;
}

void AValdoriaHUD::DrawStatusLine(AValdoriaHeroCharacter* Hero, float W, float H)
{
	// Estados e efeitos ativos, no centro, acima das habilidades.
	const float S = Scale;
	TArray<TPair<FString, FLinearColor>> Lines;
	const FLinearColor Bad(1.f, 0.45f, 0.4f);
	if (Hero->HasStatus(EValdoriaStatus::Stunned)) Lines.Add({ TEXT("ATORDOADO"), Bad });
	if (Hero->HasStatus(EValdoriaStatus::Rooted)) Lines.Add({ TEXT("ENRAIZADO"), Bad });
	if (Hero->HasStatus(EValdoriaStatus::Silenced)) Lines.Add({ TEXT("SILENCIADO"), Bad });
	if (Hero->HasStatus(EValdoriaStatus::CannotAttack)) Lines.Add({ TEXT("SEM ATAQUE"), Bad });
	if (Hero->HasStatus(EValdoriaStatus::CannotUseAbilities)) Lines.Add({ TEXT("SEM HABILIDADES"), Bad });
	if (Hero->GetTeamDamageReduction() > 0.f) Lines.Add({ FString::Printf(TEXT("SPOT: -%d%% DAMAGE TAKEN"), FMath::RoundToInt(Hero->GetTeamDamageReduction() * 100.f)), FLinearColor(0.5f, 0.85f, 1.f) });
	if (Hero->GetTeamDamageBonus() > 0.f) Lines.Add({ FString::Printf(TEXT("SPOT: +%d%% DAMAGE"), FMath::RoundToInt(Hero->GetTeamDamageBonus() * 100.f)), FLinearColor(1.f, 0.6f, 0.4f) });
	if (Hero->GetTeamRegen() > 0.f) Lines.Add({ FString::Printf(TEXT("SPOT: +%.0f HP/S"), Hero->GetTeamRegen()), FLinearColor(0.5f, 1.f, 0.6f) });
	if (Hero->IsRiding()) Lines.Add({ FString::Printf(TEXT("BIFROST  %s  (look back + W to turn around, Space to get off)"), *Hero->GetRideLaneName()), FLinearColor(0.7f, 0.85f, 1.f) });
	if (Hero->Overheal >= 1.f) Lines.Add({ FString::Printf(TEXT("SOBRECURA +%.0f (escudo que se desfaz)"), Hero->Overheal), FLinearColor(1.f, 0.95f, 0.6f) });
	if (Hero->IsReflecting()) Lines.Add({ TEXT("REFLETINDO"), FLinearColor(1.f, 0.9f, 0.5f) });
	if (Hero->IsAbsorbing()) Lines.Add({ TEXT("ABSORVENDO DANO"), FLinearColor(1.f, 0.8f, 0.4f) });
	if (Hero->IsIntangible()) Lines.Add({ TEXT("INTANGÍVEL (imune a físico)"), FLinearColor(0.8f, 0.9f, 1.f) });
	if (Hero->IsStealthActive())
	{
		Lines.Add({ Hero->IsHiddenByStealth() ? TEXT("INVISÍVEL") : TEXT("REVELADO (inimigo perto)"), FLinearColor(0.75f, 0.5f, 1.f) });
	}
	if (Hero->GetPassiveDamageReduction() > 0.f)
	{
		Lines.Add({ FString::Printf(TEXT("PROTEÇÃO -%d%% (%d aliado%s perto)"), FMath::RoundToInt(Hero->GetPassiveDamageReduction() * 100.f),
			Hero->GetAlliesNearby(), Hero->GetAlliesNearby() == 1 ? TEXT("") : TEXT("s")), FLinearColor(1.f, 0.7f, 0.35f) });
	}
	float Y = H - 170.f * S;
	for (const TPair<FString, FLinearColor>& Line : Lines)
	{
		DrawLabel(Line.Key, W * 0.5f, Y, Line.Value, 0.95f * S, true);
		Y -= 22.f * S;
	}
	// Seal / reverse-seal: progress bar and the incantation as a subtitle.
	if (Hero->GetChannelProgress() > 0.f)
	{
		const float BW = 420.f * S;
		DrawRect(W * 0.5f - BW * 0.5f, H * 0.62f, BW, 12.f * S, FLinearColor(0.f, 0.f, 0.f, 0.6f));
		DrawRect(W * 0.5f - BW * 0.5f, H * 0.62f, BW * Hero->GetChannelProgress(), 12.f * S, FLinearColor(0.95f, 0.85f, 0.5f));
		DrawLabel(Hero->GetChannelSubtitle(), W * 0.5f, H * 0.62f + 20.f * S, FLinearColor::White, 0.9f * S, true);
	}
	else if (Hero->FindChannelTarget())
	{
		DrawLabel(TEXT("Hold G: seal / reverse-seal the guardian"), W * 0.5f, H * 0.62f, FLinearColor(0.95f, 0.85f, 0.5f), 0.9f * S, true);
	}
	// Moldura roxa nas bordas enquanto invisível.
	if (Hero->IsHiddenByStealth())
	{
		const FLinearColor Edge(0.45f, 0.15f, 0.8f, 0.18f);
		DrawRect(0.f, 0.f, W, 18.f * S, Edge);
		DrawRect(0.f, H - 18.f * S, W, 18.f * S, Edge);
		DrawRect(0.f, 0.f, 18.f * S, H, Edge);
		DrawRect(W - 18.f * S, 0.f, 18.f * S, H, Edge);
	}
}

void AValdoriaHUD::AddDamageIndicator(const FVector& FromLocation, float Damage)
{
	DamageIndicators.Add({ FromLocation, Damage, static_cast<float>(GetWorld()->GetTimeSeconds()) });
}

void AValdoriaHUD::DrawDamageFeedback(AValdoriaHeroCharacter* Hero, float W, float H)
{
	// Referência de pesquisa (UX de indicadores de dano): seta na direção do atacante, maior
	// quanto mais forte o golpe, sumindo em ~1 s; tela avermelhada com pouca vida; aviso de ultimate.
	const float S = Scale;
	const float Now = GetWorld()->GetTimeSeconds();
	const float Cx = W * 0.5f;
	const float Cy = H * 0.5f;
	const APlayerController* PC = GetOwningPlayerController();
	const float ViewYaw = PC ? PC->GetControlRotation().Yaw : Hero->GetActorRotation().Yaw;
	DamageIndicators.RemoveAll([Now](const FDamageIndicator& D) { return Now - D.Time > 1.2f; });
	for (const FDamageIndicator& D : DamageIndicators)
	{
		const FVector To = D.From - Hero->GetActorLocation();
		// Ângulo relativo: 0 = na frente (topo da tela), 90 = direita.
		const float Rel = FMath::DegreesToRadians(FRotator::NormalizeAxis(To.Rotation().Yaw - ViewYaw));
		const float Alpha = 1.f - (Now - D.Time) / 1.2f;
		const float Radius = 150.f * S;
		const float Half = FMath::DegreesToRadians(FMath::Clamp(10.f + D.Damage / ValdoriaStatScale * 0.4f, 12.f, 40.f));
		const FLinearColor Color(1.f, 0.15f, 0.1f, 0.85f * Alpha);
		const int32 Segments = 8;
		for (int32 i = 0; i < Segments; ++i)
		{
			const float A0 = Rel - Half + 2.f * Half * i / Segments;
			const float A1 = Rel - Half + 2.f * Half * (i + 1) / Segments;
			DrawLine(Cx + FMath::Sin(A0) * Radius, Cy - FMath::Cos(A0) * Radius, Cx + FMath::Sin(A1) * Radius, Cy - FMath::Cos(A1) * Radius,
				Color, 6.f * S);
		}
	}
	// Pouca vida: bordas vermelhas pulsando.
	const float MaxPool = Hero->GetMaxHealth() + Hero->GetMaxArmor() + Hero->GetMaxShield();
	const float Pool = Hero->GetHealth() + Hero->GetArmor() + Hero->GetShield();
	if (!Hero->IsDead() && MaxPool > 0.f && Pool / MaxPool < 0.3f)
	{
		const float Pulse = 0.12f + 0.08f * FMath::Sin(Now * 5.f);
		const FLinearColor Edge(0.8f, 0.f, 0.f, Pulse);
		DrawRect(0.f, 0.f, W, 30.f * S, Edge);
		DrawRect(0.f, H - 30.f * S, W, 30.f * S, Edge);
		DrawRect(0.f, 0.f, 30.f * S, H, Edge);
		DrawRect(W - 30.f * S, 0.f, 30.f * S, H, Edge);
	}
	// Ultimate ficou pronta: aviso no centro por 2 s.
	const bool bReady = Hero->IsUltimateReady();
	if (bReady && !bWasUltReady)
	{
		UltReadyFlashTime = Now;
		ValdoriaAudio::Play2D(this, ValdoriaAudio::Load(TEXT("UI"), TEXT("confirmation_001")), 0.7f, 0.f, 0.8f);
	}
	bWasUltReady = bReady;
	if (Now - UltReadyFlashTime < 2.f && Hero->GetHeroDefinition())
	{
		DrawLabel(TEXT("ULTIMATE PRONTA"), Cx, Cy - 220.f * S, Hero->GetHeroDefinition()->ThemeColor, 1.6f * S, true);
	}
}

void AValdoriaHUD::DrawModeScore(float W, float H)
{
	if (const AValdoriaMobaMode* Moba = GetWorld()->GetAuthGameMode<AValdoriaMobaMode>())
	{
		const float S2 = Scale;
		const float Left = Moba->GetTimeLeft();
		const FString Line = FString::Printf(TEXT("GUARDIAN SPOTS   YOUR GUARDIANS %d   ENEMY GUARDIANS %d   KILLS %d - %d   %d:%02d"),
			Moba->GetGuardiansAlive(0), Moba->GetGuardiansAlive(1), Moba->GetKills(0), Moba->GetKills(1),
			FMath::FloorToInt(FMath::Max(0.f, Left)) / 60, FMath::FloorToInt(FMath::Max(0.f, Left)) % 60);
		DrawRect(W * 0.5f - 360.f * S2, 12.f * S2, 720.f * S2, 34.f * S2, FLinearColor(0.f, 0.f, 0.02f, 0.6f));
		DrawLabel(Line, W * 0.5f, 18.f * S2, FLinearColor(0.97f, 0.83f, 0.48f), 1.f * S2, true);
		if (Moba->GetResult() != 0)
		{
			const bool bWon = Moba->GetResult() == 1;
			DrawRect(0.f, H * 0.4f, W, 120.f * S2, FLinearColor(0.f, 0.f, 0.f, 0.6f));
			DrawLabel(bWon ? TEXT("VICTORY") : TEXT("DEFEAT"), W * 0.5f, H * 0.4f + 20.f * S2, bWon ? FLinearColor(0.5f, 1.f, 0.6f) : FLinearColor(1.f, 0.4f, 0.35f), 3.f * S2, true, true);
		}
		return;
	}
	const AValdoriaDeathmatchMode* Mode = GetWorld()->GetAuthGameMode<AValdoriaDeathmatchMode>();
	if (!Mode || !Mode->GetRules())
	{
		return;
	}
	const float S = Scale;
	const float Left = Mode->GetTimeLeft();
	const FString Time = Left < 0.f ? FString() : FString::Printf(TEXT("   %d:%02d"), FMath::FloorToInt(Left) / 60, FMath::FloorToInt(Left) % 60);
	const FString Line = FString::Printf(TEXT("%s   YOU %d   BEST BOT %d   GOAL %d%s"), *Mode->GetRules()->DisplayName.ToString().ToUpper(),
		Mode->GetPlayerScore(), Mode->GetBestBotScore(), Mode->GetRules()->ScoreToWin, *Time);
	DrawRect(W * 0.5f - 300.f * S, 12.f * S, 600.f * S, 34.f * S, FLinearColor(0.f, 0.f, 0.02f, 0.6f));
	DrawLabel(Line, W * 0.5f, 18.f * S, FLinearColor(0.97f, 0.83f, 0.48f), 1.f * S, true);
	if (Mode->GetResult() != 0)
	{
		const bool bWon = Mode->GetResult() == 1;
		DrawRect(0.f, H * 0.4f, W, 120.f * S, FLinearColor(0.f, 0.f, 0.f, 0.6f));
		DrawLabel(bWon ? TEXT("VICTORY") : TEXT("DEFEAT"), W * 0.5f, H * 0.4f + 20.f * S,
			bWon ? FLinearColor(0.5f, 1.f, 0.6f) : FLinearColor(1.f, 0.4f, 0.35f), 3.f * S, true, true);
		DrawLabel(TEXT("New match in a few seconds"), W * 0.5f, H * 0.4f + 88.f * S, FLinearColor::White, 1.f * S, true);
	}
}
// ------------------------------------------------------------------ minimap (turn 46)

void AValdoriaHUD::DrawMinimap(AValdoriaHeroCharacter* Hero, float W, float H)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (!bMiniBuilt)
	{
		// Top-down footprint of the level, built once: every collidable static mesh that is not a huge floor.
		bMiniBuilt = true;
		for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
		{
			FVector Origin, Ext;
			It->GetActorBounds(false, Origin, Ext);
			if (FMath::Max(Ext.X, Ext.Y) > 100000.f || (Ext.X < 1.f && Ext.Y < 1.f) || !It->GetActorEnableCollision())
			{
				continue;
			}
			const FBox2D B(FVector2D(Origin.X - Ext.X, Origin.Y - Ext.Y), FVector2D(Origin.X + Ext.X, Origin.Y + Ext.Y));
			MiniBounds += B;
			if (Origin.Z + Ext.Z > 250.f) // taller than a person: a building/wall, drawn on top of the floor
			{
				MiniBlocks.Add({ B, float(Origin.Z + Ext.Z) });
			}
		}
	}
	if (!MiniBounds.bIsValid)
	{
		return;
	}
	const float S = Scale;
	const float Size = 250.f * S;
	const float X0 = 16.f * S;
	const float Y0 = 36.f * S;
	// World X goes up on the map, world Y goes right (fixed north). Keep the aspect ratio.
	const FVector2D Ext = MiniBounds.GetSize();
	const float Span = FMath::Max(Ext.X, Ext.Y);
	const float K = Size / FMath::Max(1.f, Span);
	const FVector2D C = MiniBounds.GetCenter();
	auto ToMap = [&](const FVector& P)
	{
		return FVector2D(X0 + Size * 0.5f + (P.Y - C.Y) * K, Y0 + Size * 0.5f - (P.X - C.X) * K);
	};
	DrawRect(X0 - 3.f * S, Y0 - 3.f * S, Size + 6.f * S, Size + 6.f * S, FLinearColor(0.f, 0.f, 0.f, 0.75f));
	DrawRect(X0 + Size * 0.5f - Ext.Y * K * 0.5f, Y0 + Size * 0.5f - Ext.X * K * 0.5f, Ext.Y * K, Ext.X * K, FLinearColor(0.09f, 0.1f, 0.13f, 0.9f));
	for (const FMiniBlock& B : MiniBlocks)
	{
		const FVector2D A = ToMap(FVector(B.Box.Max.X, B.Box.Min.Y, 0.f));
		const FVector2D Z = ToMap(FVector(B.Box.Min.X, B.Box.Max.Y, 0.f));
		const float Shade = FMath::Clamp(0.22f + B.Height / 6000.f, 0.22f, 0.45f);
		DrawRect(A.X, A.Y, FMath::Max(1.f, Z.X - A.X), FMath::Max(1.f, Z.Y - A.Y), FLinearColor(Shade, Shade, Shade * 1.1f, 0.95f));
	}
	const uint8 MyTeam = Hero ? Hero->TeamId : 0;
	// Lanes (MOBA): thin gold lines along the routes.
	if (const AValdoriaMobaMode* Moba = World->GetAuthGameMode<AValdoriaMobaMode>())
	{
		for (int32 Lane = 0; Lane < 3; ++Lane)
		{
			const TArray<FVector>& Route = Moba->GetLaneRoute(0, Lane);
			for (int32 i = 1; i < Route.Num(); ++i)
			{
				const FVector2D A = ToMap(Route[i - 1]);
				const FVector2D B = ToMap(Route[i]);
				DrawLine(A.X, A.Y, B.X, B.Y, FLinearColor(0.85f, 0.7f, 0.3f, 0.35f), 2.f * S);
			}
		}
	}
	const UValdoriaVisionSubsystem* Vision = World->GetSubsystem<UValdoriaVisionSubsystem>();
	const FLinearColor Ally(0.35f, 0.7f, 1.f);
	const FLinearColor Enemy(1.f, 0.3f, 0.25f);
	for (TActorIterator<AValdoriaHeroCharacter> It(World); It; ++It)
	{
		AValdoriaHeroCharacter* U = *It;
		if (!U || U == Hero || U->IsDead() || U->TeamId == 255)
		{
			continue;
		}
		const bool bAlly = Hero ? !Hero->IsEnemy(U) : true;
		if (!bAlly && Vision && !Vision->IsRevealedTo(U, MyTeam))
		{
			continue; // fog of war: nobody on your team sees it
		}
		const FVector2D P = ToMap(U->GetActorLocation());
		const FLinearColor Col = bAlly ? Ally : Enemy;
		if (const AValdoriaGuardian* G = Cast<AValdoriaGuardian>(U))
		{
			const bool bAllyG = G->GuardTeam == MyTeam;
			const FLinearColor GC = G->State == EValdoriaSpotState::SemiSealed ? FLinearColor(0.95f, 0.85f, 0.3f) : (bAllyG ? Ally : Enemy);
			const float R = 5.f * S;
			DrawRect(P.X - R, P.Y - R, R * 2.f, R * 2.f, FLinearColor::Black);
			DrawRect(P.X - R + S, P.Y - R + S, R * 2.f - 2.f * S, R * 2.f - 2.f * S, GC);
			continue;
		}
		const AValdoriaTrainingDummy* Dummy = Cast<AValdoriaTrainingDummy>(U);
		const float R = (Dummy && Dummy->bMinion) ? 1.6f * S : 4.f * S;
		if (!(Dummy && Dummy->bMinion))
		{
			DrawRect(P.X - R - S, P.Y - R - S, R * 2.f + 2.f * S, R * 2.f + 2.f * S, FLinearColor::Black);
		}
		DrawRect(P.X - R, P.Y - R, R * 2.f, R * 2.f, Col);
	}
	// You: a white arrow pointing where the camera looks.
	if (Hero)
	{
		const FVector2D P = ToMap(Hero->GetActorLocation());
		const float Yaw = FMath::DegreesToRadians(Hero->GetControlRotation().Yaw);
		const FVector2D Dir(FMath::Sin(Yaw), -FMath::Cos(Yaw)); // world X up, Y right
		const FVector2D Side(-Dir.Y, Dir.X);
		const float L = 8.f * S;
		const FVector2D Tip = P + Dir * L;
		const FVector2D BL = P - Dir * L * 0.6f + Side * L * 0.6f;
		const FVector2D BR = P - Dir * L * 0.6f - Side * L * 0.6f;
		DrawLine(Tip.X, Tip.Y, BL.X, BL.Y, FLinearColor::White, 2.f * S);
		DrawLine(Tip.X, Tip.Y, BR.X, BR.Y, FLinearColor::White, 2.f * S);
		DrawLine(BL.X, BL.Y, BR.X, BR.Y, FLinearColor::White, 2.f * S);
	}
	DrawFrame(X0 - 3.f * S, Y0 - 3.f * S, Size + 6.f * S, Size + 6.f * S, FLinearColor(0.97f, 0.83f, 0.48f, 0.6f), 1.5f * S);
}

// ------------------------------------------------------------------ levels, top bar, scoreboard, death recap (turn 46)

void AValdoriaHUD::DrawLevelInfo(AValdoriaHeroCharacter* Hero, float W, float H)
{
	// Same layout math as DrawAbilities: rank pips under 1/2/3/4, "CTRL +" when a point can go there, lock at rank 0.
	const float S = Scale;
	const float Size = 64.f * S, Gap = 12.f * S, ExtraSize = 48.f * S, UltSize = 86.f * S;
	const float Y = H - 112.f * S;
	const bool bHasExtra = Hero->FindAbilityEntry(EValdoriaAbilitySlot::AbilityExtra) != nullptr;
	const float TotalW = (bHasExtra ? ExtraSize + Gap : 0.f) + (Size + Gap) * 3.f + UltSize;
	float X = W * 0.5f - TotalW * 0.5f + (bHasExtra ? ExtraSize + Gap : 0.f);
	const FLinearColor Gold(0.97f, 0.83f, 0.48f);
	const float Blink = 0.6f + 0.4f * FMath::Sin(GetWorld()->GetTimeSeconds() * 5.f);
	auto Pips = [&](EValdoriaAbilitySlot Slot, float SX, float SY, float SSize)
	{
		const int32 Rank = Hero->GetAbilityRank(Slot);
		if (Rank <= 0)
		{
			DrawRect(SX, SY, SSize, SSize, FLinearColor(0.f, 0.f, 0.f, 0.65f));
			DrawLabel(TEXT("BLOQ."), SX + SSize * 0.5f, SY + SSize * 0.38f, FLinearColor(0.7f, 0.7f, 0.75f), 0.8f * S, true);
		}
		const float PipW = (SSize - 8.f * S) / 5.f;
		for (int32 i = 0; i < 5; ++i)
		{
			DrawRect(SX + 4.f * S + i * PipW + 1.f * S, SY + SSize + 4.f * S, PipW - 2.f * S, 4.f * S, i < Rank ? Gold : FLinearColor(0.2f, 0.2f, 0.25f, 0.9f));
		}
		if (Hero->CanUpgrade(Slot))
		{
			DrawRect(SX, SY - 20.f * S, SSize, 16.f * S, FLinearColor(0.f, 0.f, 0.f, 0.6f));
			DrawLabel(TEXT("CTRL +"), SX + SSize * 0.5f, SY - 20.f * S, Gold * Blink, 0.75f * S, true);
		}
	};
	for (EValdoriaAbilitySlot Slot : { EValdoriaAbilitySlot::Ability1, EValdoriaAbilitySlot::Ability2, EValdoriaAbilitySlot::Ability3 })
	{
		Pips(Slot, X, Y, Size);
		X += Size + Gap;
	}
	Pips(EValdoriaAbilitySlot::Ultimate, X, Y - (UltSize - Size) * 0.5f, UltSize);

	// Level badge and XP bar to the left of the row.
	const float BX = W * 0.5f - TotalW * 0.5f - 70.f * S;
	const float BY = Y + 6.f * S;
	DrawRect(BX, BY, 52.f * S, 52.f * S, FLinearColor(0.f, 0.f, 0.f, 0.7f));
	DrawFrame(BX, BY, 52.f * S, 52.f * S, Gold, 2.f);
	DrawLabel(FString::Printf(TEXT("%d"), Hero->Level), BX + 26.f * S, BY + 8.f * S, FLinearColor::White, 1.6f * S, true);
	DrawLabel(TEXT("NÍVEL"), BX + 26.f * S, BY + 36.f * S, Gold, 0.65f * S, true);
	const float XPFrac = Hero->Level >= 20 ? 1.f : FMath::Clamp(Hero->XP / FMath::Max(1.f, Hero->XPToNextLevel()), 0.f, 1.f);
	DrawRect(BX, BY + 56.f * S, 52.f * S, 5.f * S, FLinearColor(0.15f, 0.15f, 0.2f, 0.9f));
	DrawRect(BX, BY + 56.f * S, 52.f * S * XPFrac, 5.f * S, FLinearColor(0.55f, 0.8f, 1.f));
	if (Hero->AbilityPoints > 0)
	{
		DrawLabel(FString::Printf(TEXT("%d ponto(s)"), Hero->AbilityPoints), BX + 26.f * S, BY - 20.f * S, Gold * Blink, 0.75f * S, true);
	}
	if (Hero->ElevationPoints > 0)
	{
		DrawLabel(FString::Printf(TEXT("%d elevação"), Hero->ElevationPoints), BX + 26.f * S, BY - 36.f * S, FLinearColor(0.8f, 0.6f, 1.f), 0.7f * S, true);
	}
}

void AValdoriaHUD::DrawTopHeroBar(AValdoriaHeroCharacter* Hero, float W, float H)
{
	// Heroes of each team at the top (owner): allies left, enemies right, under the score line. Health only for
	// allies and for enemies your team can see; dead = grey.
	UWorld* World = GetWorld();
	if (!World || !Hero)
	{
		return;
	}
	TArray<AValdoriaHeroCharacter*> Allies, Enemies;
	for (TActorIterator<AValdoriaHeroCharacter> It(World); It; ++It)
	{
		if (It->IsHeroUnit())
		{
			(Hero->IsEnemy(*It) ? Enemies : Allies).Add(*It);
		}
	}
	if (Allies.Num() + Enemies.Num() <= 1)
	{
		return;
	}
	const float S = Scale;
	const float Box = 34.f * S, Gap = 4.f * S, Y = 52.f * S;
	const UValdoriaVisionSubsystem* Vision = World->GetSubsystem<UValdoriaVisionSubsystem>();
	auto DrawOne = [&](AValdoriaHeroCharacter* U, float X, bool bEnemy)
	{
		const UValdoriaHeroDefinition* Def = U->GetHeroDefinition();
		const FLinearColor Team = bEnemy ? FLinearColor(1.f, 0.3f, 0.25f) : FLinearColor(0.35f, 0.7f, 1.f);
		const bool bSeen = !bEnemy || !Vision || Vision->IsRevealedTo(U, Hero->TeamId);
		DrawRect(X, Y, Box, Box, FLinearColor(0.f, 0.f, 0.f, 0.7f));
		const FString Initial = Def && !Def->DisplayName.IsEmpty() ? Def->DisplayName.ToString().Left(2) : FString(TEXT("Re"));
		const FLinearColor C = U->IsDead() ? FLinearColor(0.35f, 0.35f, 0.38f) : (Def ? Def->ThemeColor : FLinearColor::White);
		DrawLabel(Initial, X + Box * 0.5f, Y + Box * 0.18f, C, 1.1f * S, true);
		DrawFrame(X, Y, Box, Box, U == Hero ? FLinearColor::White : Team, U == Hero ? 2.f : 1.f);
		if (U->bUsesLevels)
		{
			DrawLabel(FString::Printf(TEXT("%d"), U->Level), X + Box - 6.f * S, Y + Box - 12.f * S, FLinearColor(0.97f, 0.83f, 0.48f), 0.6f * S, true);
		}
		if (U->IsDead() && U->RespawnAt > World->GetTimeSeconds())
		{
			DrawLabel(FString::Printf(TEXT("%.0f"), FMath::CeilToFloat(U->RespawnAt - World->GetTimeSeconds())), X + Box * 0.5f, Y + Box * 0.55f, FLinearColor::White, 0.7f * S, true);
		}
		const float Frac = U->IsDead() ? 0.f : U->GetHealth() / FMath::Max(1.f, U->GetMaxHealth());
		DrawRect(X, Y + Box + 2.f * S, Box, 4.f * S, FLinearColor(0.1f, 0.1f, 0.12f, 0.9f));
		if (bSeen)
		{
			DrawRect(X, Y + Box + 2.f * S, Box * Frac, 4.f * S, Team);
		}
		else
		{
			DrawLabel(TEXT("?"), X + Box * 0.5f, Y + Box + 2.f * S, Team, 0.6f * S, true);
		}
	};
	float X = W * 0.5f - 20.f * S - Box;
	for (AValdoriaHeroCharacter* U : Allies)
	{
		DrawOne(U, X, false);
		X -= Box + Gap;
	}
	X = W * 0.5f + 20.f * S;
	for (AValdoriaHeroCharacter* U : Enemies)
	{
		DrawOne(U, X, true);
		X += Box + Gap;
	}
}

void AValdoriaHUD::DrawScoreboard(AValdoriaHeroCharacter* Hero, float W, float H)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	TArray<AValdoriaHeroCharacter*> Units;
	for (TActorIterator<AValdoriaHeroCharacter> It(World); It; ++It)
	{
		if (It->IsHeroUnit())
		{
			Units.Add(*It);
		}
	}
	const uint8 MyTeam = Hero ? Hero->TeamId : 0;
	Units.Sort([MyTeam](const AValdoriaHeroCharacter& A, const AValdoriaHeroCharacter& B)
	{
		const bool AMine = A.TeamId == MyTeam, BMine = B.TeamId == MyTeam;
		if (AMine != BMine) return AMine;
		if (A.TeamId != B.TeamId) return A.TeamId < B.TeamId;
		return A.Stats.Kills * 2 + A.Stats.Assists > B.Stats.Kills * 2 + B.Stats.Assists;
	});
	const float S = Scale;
	const float RowH = 30.f * S;
	const float PW = 1100.f * S;
	const float PH = (Units.Num() + 3) * RowH;
	const float X0 = W * 0.5f - PW * 0.5f, Y0 = H * 0.5f - PH * 0.5f;
	DrawRect(X0, Y0, PW, PH, FLinearColor(0.02f, 0.025f, 0.04f, 0.92f));
	DrawFrame(X0, Y0, PW, PH, FLinearColor(0.97f, 0.83f, 0.48f, 0.7f), 1.5f);
	const float Cols[] = { 20.f, 300.f, 380.f, 520.f, 640.f, 760.f, 880.f, 1000.f };
	const TCHAR* Heads[] = { TEXT("HERÓI"), TEXT("NÍVEL"), TEXT("K / D / A"), TEXT("OURO"), TEXT("DANO"), TEXT("CURA"), TEXT("TANQUE"), TEXT("OBJETIVOS") };
	for (int32 c = 0; c < 8; ++c)
	{
		DrawLabel(Heads[c], X0 + Cols[c] * S, Y0 + 10.f * S, FLinearColor(0.97f, 0.83f, 0.48f), 0.85f * S);
	}
	float Y = Y0 + RowH * 1.4f;
	int32 LastTeam = -1;
	for (AValdoriaHeroCharacter* U : Units)
	{
		if (LastTeam != -1 && LastTeam != U->TeamId)
		{
			Y += RowH * 0.5f; // gap between teams
		}
		LastTeam = U->TeamId;
		const bool bMine = U->TeamId == MyTeam;
		const FLinearColor C = U == Hero ? FLinearColor::White : (bMine ? FLinearColor(0.55f, 0.8f, 1.f) : FLinearColor(1.f, 0.5f, 0.45f));
		if (U == Hero)
		{
			DrawRect(X0 + 6.f * S, Y - 3.f * S, PW - 12.f * S, RowH - 2.f * S, FLinearColor(1.f, 1.f, 1.f, 0.06f));
		}
		const AValdoriaHeroCharacter::FMatchStats& St = U->Stats;
		const FString Cells[] = {
			U->GetUnitName() + (U->IsDead() ? FString(TEXT("  (morto)")) : FString()),
			U->bUsesLevels ? FString::FromInt(U->Level) : FString(TEXT("-")),
			FString::Printf(TEXT("%d / %d / %d"), St.Kills, St.Deaths, St.Assists),
			FString::Printf(TEXT("%.0f"), St.Gold),
			FString::Printf(TEXT("%.0f"), St.Damage),
			FString::Printf(TEXT("%.0f"), St.Healing),
			FString::Printf(TEXT("%.0f"), St.DamageTaken),
			FString::FromInt(St.Seals) };
		for (int32 c = 0; c < 8; ++c)
		{
			DrawLabel(Cells[c], X0 + Cols[c] * S, Y, C, 0.9f * S);
		}
		Y += RowH;
	}
	DrawLabel(TEXT("Tanque = dano recebido  •  Objetivos = guardiões selados de vez"), X0 + 20.f * S, Y0 + PH - 24.f * S, FLinearColor(0.6f, 0.62f, 0.68f), 0.75f * S);
}

void AValdoriaHUD::DrawDeathRecap(AValdoriaHeroCharacter* Hero, float W, float H)
{
	const float S = Scale;
	const AValdoriaHeroCharacter::FDeathRecap* R = Hero ? &Hero->LastDeathRecap : nullptr;
	const float PW = 620.f * S;
	const int32 N = R && R->bValid ? FMath::Min(R->Lines.Num(), 10) : 0;
	const float PH = (N + 4) * 28.f * S;
	const float X0 = W * 0.5f - PW * 0.5f, Y0 = H * 0.3f;
	DrawRect(X0, Y0, PW, PH, FLinearColor(0.03f, 0.02f, 0.03f, 0.92f));
	DrawFrame(X0, Y0, PW, PH, FLinearColor(1.f, 0.4f, 0.35f, 0.8f), 1.5f);
	if (!R || !R->bValid)
	{
		DrawLabel(TEXT("Você ainda não morreu nesta partida."), X0 + PW * 0.5f, Y0 + 20.f * S, FLinearColor::White, 1.f * S, true);
		return;
	}
	DrawLabel(FString::Printf(TEXT("ÚLTIMA MORTE  •  abatido por %s"), *R->Killer), X0 + 20.f * S, Y0 + 12.f * S, FLinearColor(1.f, 0.5f, 0.45f), 1.f * S);
	DrawLabel(FString::Printf(TEXT("Último golpe: %s  •  dano nos últimos 10 s: %.0f"), *R->KillerSource, R->Total), X0 + 20.f * S, Y0 + 40.f * S, FLinearColor(0.8f, 0.8f, 0.85f), 0.8f * S);
	float Y = Y0 + 80.f * S;
	for (int32 i = 0; i < N; ++i)
	{
		const AValdoriaHeroCharacter::FDamageLogEntry& L = R->Lines[i];
		const float Frac = L.Amount / FMath::Max(1.f, R->Total);
		DrawRect(X0 + 20.f * S, Y + 18.f * S, (PW - 40.f * S) * Frac, 4.f * S, FLinearColor(1.f, 0.35f, 0.3f, 0.8f));
		DrawLabel(FString::Printf(TEXT("%s  —  %s"), *L.Attacker, *L.Source), X0 + 20.f * S, Y, FLinearColor::White, 0.85f * S);
		DrawLabel(FString::Printf(TEXT("%.0f  (%.0f%%)"), L.Amount, Frac * 100.f), X0 + PW - 140.f * S, Y, FLinearColor(1.f, 0.85f, 0.6f), 0.85f * S);
		Y += 28.f * S;
	}
}

void AValdoriaHUD::AddHealNumber(const FVector& WorldLocation, float Amount)
{
	FDamageNumber N;
	N.WorldLocation = WorldLocation;
	N.Damage = Amount;
	N.SpawnTime = GetWorld()->GetTimeSeconds();
	N.bHeal = true;
	DamageNumbers.Add(N);
}
