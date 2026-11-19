// Echoes of Valdoria - 6v6 MOBA with Guardian Spots.

#include "ValdoriaMobaMode.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Containers/Ticker.h"
#include "Misc/CommandLine.h"
#include "UnrealClient.h"
#include "ValdoriaHeroCharacter.h"
#include "ValdoriaHeroDefinition.h"
#include "ValdoriaPlayerController.h"
#include "ValdoriaTrainingDummy.h"
#include "ValdoriaLaneRail.h"

UValdoriaMobaRules::UValdoriaMobaRules()
{
	DisplayName = NSLOCTEXT("ValdoriaModes", "Moba", "Guardian Spots 6v6");
	ScoreToWin = 0; // win by killing every enemy guardian
	TimeLimit = 1200.f;
	RespawnDelay = 6.f;
	BotCount = 11;
	using B = EValdoriaSpotBuff;
	// Spot 1 (owner): -50% damage taken, full for 1 minute, fading to 0 at minute 5.
	FValdoriaSpotRule S1;
	S1.SideLanePosition = 0.62f;
	S1.MidLanePosition = 0.62f;
	S1.Buffs = { B::DamageReduction };
	S1.BuffValues = { 0.5f };
	// Spot 2 (placeholder until the owner decides): +20% damage, same fade.
	FValdoriaSpotRule S2;
	S2.SideLanePosition = 0.5f;
	S2.MidLanePosition = 0.4f;
	S2.Buffs = { B::DamageBonus };
	S2.BuffValues = { 0.2f };
	// Owner: spot 2 fades from full to 0 between minutes 5 and 10.
	S2.BuffHoldUntil = 300.f;
	S2.BuffFadeEnd = 600.f;
	// Spot 3 (owner: two guardians doing different things; buffs are placeholders): regeneration + damage bonus, no fade.
	FValdoriaSpotRule S3;
	S3.SideLanePosition = 0.38f;
	S3.MidLanePosition = 0.18f;
	S3.Buffs = { B::Regeneration, B::DamageBonus };
	S3.BuffValues = { 10.f, 0.1f };
	S3.BuffFadeEnd = 0.f;
	S3.GuardianHealth = 4000.f;
	S3.MageGuardianIndex = 0; // owner: one of the two is a support mage with a staff
	Spots = { S1, S2, S3 };
}

AValdoriaMobaMode::AValdoriaMobaMode()
{
	PrimaryActorTick.bCanEverTick = true;
	bSpawnTrainingDummies = false;
}

FVector AValdoriaMobaMode::Ground(const FVector& Point) const
{
	FHitResult Down;
	if (GetWorld()->LineTraceSingleByObjectType(Down, Point + FVector(0, 0, 3000.f), Point - FVector(0, 0, 6000.f), FCollisionObjectQueryParams(ECC_WorldStatic)))
	{
		return Down.ImpactPoint + FVector(0.f, 0.f, 120.f);
	}
	return Point;
}

FVector AValdoriaMobaMode::StreetPoint(const FVector& Point) const
{
	// Turn 45: lane points and spots were pure geometry from the bases, so some fell inside buildings (bots pushed
	// the wall forever) or on roofs. Nearest spot at street level with room for a hero, searched in rings.
	const FCollisionShape Body = FCollisionShape::MakeCapsule(55.f, 95.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ValdoriaStreetPoint), false);
	for (float Radius = 0.f; Radius <= 6000.f; Radius += 300.f)
	{
		const int32 Steps = Radius <= 0.f ? 1 : 16;
		for (int32 i = 0; i < Steps; ++i)
		{
			const float A = 2.f * PI * i / Steps;
			const FVector C(Point.X + FMath::Cos(A) * Radius, Point.Y + FMath::Sin(A) * Radius, StreetLevel + 120.f);
			if (GetWorld()->OverlapAnyTestByObjectType(C, FQuat::Identity, FCollisionObjectQueryParams(ECC_WorldStatic), Body, Params))
			{
				continue; // inside a building, wall or prop
			}
			FHitResult Down;
			if (GetWorld()->LineTraceSingleByObjectType(Down, C, C - FVector(0, 0, 400.f), FCollisionObjectQueryParams(ECC_WorldStatic), Params))
			{
				return Down.ImpactPoint + FVector(0.f, 0.f, 120.f);
			}
		}
	}
	return Ground(Point);
}

float AValdoriaMobaMode::HighestTop(const FVector& Point, float Radius) const
{
	// Highest solid surface under the point and around it (a building roof, the elevated road...).
	float Top = -100000.f;
	const FVector Offsets[] = { FVector::ZeroVector, FVector(Radius, 0, 0), FVector(-Radius, 0, 0), FVector(0, Radius, 0), FVector(0, -Radius, 0) };
	for (const FVector& O : Offsets)
	{
		FHitResult Down;
		const FVector P2(Point.X + O.X, Point.Y + O.Y, 0.f);
		if (GetWorld()->LineTraceSingleByChannel(Down, P2 + FVector(0, 0, 30000.f), P2 - FVector(0, 0, 5000.f), ECC_WorldStatic))
		{
			Top = FMath::Max(Top, Down.ImpactPoint.Z);
		}
	}
	return Top > -100000.f ? Top : Point.Z;
}

bool AValdoriaMobaMode::HasRoofAbove(const FVector& Point) const
{
	FHitResult Up;
	return GetWorld()->LineTraceSingleByChannel(Up, Point + FVector(0, 0, 50.f), Point + FVector(0, 0, 5000.f), ECC_WorldStatic);
}

void AValdoriaMobaMode::StartPlay()
{
	Rules = LoadObject<UValdoriaMobaRules>(nullptr, TEXT("/Game/Valdoria/Modes/DA_Moba.DA_Moba"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Rules)
	{
		Rules = NewObject<UValdoriaMobaRules>(this);
	}
	PlayerRespawnDelay = Rules->RespawnDelay;
	bAutoPlayer = UGameplayStatics::HasOption(OptionsString, TEXT("bots"));
	Super::StartPlay();
	MatchStart = GetWorld()->GetTimeSeconds();
	BuildLayout();
	if (FParse::Param(FCommandLine::Get(), TEXT("ValdoriaMobaTest")))
	{
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
		{
			RunMobaTestStep();
			return MobaTestStep < 100000;
		}), 0.5f);
	}
}

void AValdoriaMobaMode::BuildLayout()
{
	// Bases: untagged PlayerStart = team 0, tag "Time2" = team 1. Without a second start, mirror the first.
	Base[0] = Base[1] = FVector::ZeroVector;
	bool bHas[2] = { false, false };
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		const int32 T = It->PlayerStartTag == TEXT("Time2") ? 1 : 0;
		if (!bHas[T])
		{
			Base[T] = It->GetActorLocation();
			bHas[T] = true;
		}
	}
	if (!bHas[1])
	{
		Base[1] = Base[0] + FVector(8000.f, 0.f, 0.f);
	}
	const FVector Axis = (Base[1] - Base[0]).GetSafeNormal2D();
	const FVector SideDir = FVector::CrossProduct(FVector::UpVector, Axis);
	const FVector Centre = (Base[0] + Base[1]) * 0.5f;
	const float HalfLength = FVector::Dist2D(Base[0], Base[1]) * 0.5f;
	const float SideOffset = Rules->SideLaneRatio > 0.f ? HalfLength * Rules->SideLaneRatio : Rules->SideLaneOffset;
	AValdoriaPlayerController* PC = Cast<AValdoriaPlayerController>(GetWorld()->GetFirstPlayerController());
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Guardian Spots: 3 lanes x 3 spots x 2 teams (the last spot has two guardians).
	for (uint8 Team = 0; Team < 2; ++Team)
	{
		const FVector Toward = Team == 0 ? Axis : -Axis; // from this team's base to the centre
		for (int32 Lane = -1; Lane <= 1; ++Lane)
		{
			const bool bSide = Lane != 0;
			for (int32 SpotIdx = 0; SpotIdx < Rules->Spots.Num(); ++SpotIdx)
			{
				const FValdoriaSpotRule& Spot = Rules->Spots[SpotIdx];
				const float Along = (bSide ? Spot.SideLanePosition : Spot.MidLanePosition) * HalfLength;
				const FVector LanePoint = Base[Team] + Toward * Along + SideDir * (Lane * SideOffset);
				for (int32 g = 0; g < Spot.Buffs.Num(); ++g)
				{
					const FVector Where = StreetPoint(LanePoint + SideDir * (g - (Spot.Buffs.Num() - 1) * 0.5f) * 350.f);
					AValdoriaGuardian* Guardian = GetWorld()->SpawnActor<AValdoriaGuardian>(AValdoriaGuardian::StaticClass(), Where,
						(-Toward).Rotation(), P);
					if (!Guardian)
					{
						continue;
					}
					Guardian->Style = static_cast<EValdoriaGuardianStyle>(Guardians.Num() % 4); // mix of styles for now
					Guardian->Buff = Spot.Buffs[g];
					Guardian->BuffValue = Spot.BuffValues.IsValidIndex(g) ? Spot.BuffValues[g] : 0.f;
					Guardian->BuffHoldUntil = Spot.BuffHoldUntil;
					Guardian->BuffFadeEnd = Spot.BuffFadeEnd;
					Guardian->GuardianHealth = Spot.GuardianHealth;
					Guardian->SpotIndex = SpotIdx;
					Guardian->LaneIndex = Lane + 1;
					Guardian->bSupportMage = g == Spot.MageGuardianIndex;
					Guardian->SealSeconds = Rules->SealSeconds;
					Guardian->RevertSeconds = Rules->RevertSeconds;
					Guardian->SetupSpot(Team, Rules->SpotAreaRadius);
					Guardian->ResetCombatState();
					Guardians.Add(Guardian);
				}
			}
		}
	}

	// Final guardians: in each base, behind the spawn. The match is won by bringing the enemy one down.
	for (uint8 Team = 0; Team < 2; ++Team)
	{
		const FVector Toward = Team == 0 ? Axis : -Axis;
		if (AValdoriaGuardian* Final = GetWorld()->SpawnActor<AValdoriaGuardian>(AValdoriaGuardian::StaticClass(), StreetPoint(Base[Team] - Toward * 700.f), Toward.Rotation(), P))
		{
			Final->bFinalGuardian = true;
			Final->GuardianHealth = Rules->FinalGuardianHealth;
			Final->PerSpotHealth = Rules->PerSpotHealth;
			Final->PerSpotDamage = Rules->PerSpotDamage;
			Final->FinalBaseDamage = Rules->FinalBaseDamage;
			Final->SetupSpot(Team, Rules->FinalGuardianArea);
			Final->ResetCombatState();
			FinalGuardian[Team] = Final;
		}
	}

	for (const TWeakObjectPtr<AValdoriaGuardian>& G : Guardians)
	{
		if (G.IsValid())
		{
			G->FinalLink = FinalGuardian[G->GuardTeam];
		}
	}

	// Bifrost: one per lane per team. Vertical at the spawn, then bending into a horizontal
	// line over the lane, up to the enemy base; it reaches TrailReach of the way (the middle) for now.
	for (uint8 Team = 0; Team < 2; ++Team)
	{
		const FVector Toward = Team == 0 ? Axis : -Axis;
		for (int32 Lane = -1; Lane <= 1; ++Lane)
		{
			const FVector LaneOffset = SideDir * (Lane * SideOffset);
			// The vertical start needs open sky (the metro spawn is underground): walk toward the lane until nothing is overhead.
			FVector StartFlat = Base[Team] + SideDir * (Lane * 250.f);
			for (int32 Step = 0; Step < 20 && HasRoofAbove(Ground(StartFlat)); ++Step)
			{
				StartFlat += Toward * 300.f;
			}
			const FVector Spawn = Ground(StartFlat);
			// Horizontal part: dense samples, each above the highest roof around it (owner: it went through buildings),
			// then a moving max so it climbs before a building instead of clipping its edge.
			const int32 NumSamples = 48;
			TArray<FVector> Flats;
			TArray<float> Tops;
			for (int32 k = 1; k <= NumSamples; ++k)
			{
				const float A = k / static_cast<float>(NumSamples);
				const FVector Flat = StartFlat + (Base[1 - Team] - Base[Team]).GetSafeNormal2D() * (A * HalfLength * 2.f) + LaneOffset * FMath::Min(1.f, A * 4.f);
				Flats.Add(Flat);
				Tops.Add(HighestTop(Flat, 450.f));
			}
			TArray<FVector> Points;
			Points.Add(Spawn);
			const float FirstZ = FMath::Max(Tops[0], Tops.Num() > 1 ? Tops[1] : Tops[0]) + Rules->TrailHeight;
			Points.Add(FVector(Spawn.X, Spawn.Y, FMath::Max(Spawn.Z + Rules->TrailHeight * 0.6f, FirstZ)));
			for (int32 k = 0; k < Flats.Num(); ++k)
			{
				float Top = Tops[k];
				for (int32 j = FMath::Max(0, k - 2); j <= FMath::Min(Flats.Num() - 1, k + 2); ++j)
				{
					Top = FMath::Max(Top, Tops[j]);
				}
				Points.Add(FVector(Flats[k].X, Flats[k].Y, Top + Rules->TrailHeight));
			}
			if (AValdoriaLaneRail* Rail = GetWorld()->SpawnActor<AValdoriaLaneRail>(AValdoriaLaneRail::StaticClass(), Spawn, FRotator::ZeroRotator, P))
			{
				const int32 L = Lane + 1;
				Rail->Setup(Team, Rules->LaneNames.IsValidIndex(L) ? Rules->LaneNames[L] : FString(), Points,
					Rules->LaneColors.IsValidIndex(L) ? Rules->LaneColors[L] : FLinearColor::White);
				Rail->SetReach(Rules->TrailReach);
			}
		}
	}

	BuildLaneRoutes(Axis, SideDir, Centre, HalfLength, SideOffset);
	NextMinionWave = GetWorld()->GetTimeSeconds() + Rules->MinionFirstWave;

	// Heroes: the player + allies in team 0, the enemy team in team 1, spread over the lanes.
	// Each bot walks its lane towards the enemy base when nobody is in sight.
	const int32 Size = FMath::Clamp(Rules->TeamSize, 1, 6);
	int32 HeroIndex = 0;
	for (uint8 Team = 0; Team < 2; ++Team)
	{
		const int32 Count = Team == 0 ? Size - 1 : Size;
		for (int32 i = 0; i < Count; ++i)
		{
			const int32 Lane = (i % 3) - 1;
			const FVector Start = Ground(Base[Team] + SideDir * (i - Count * 0.5f) * 180.f);
			AValdoriaTrainingDummy* Bot = GetWorld()->SpawnActor<AValdoriaTrainingDummy>(AValdoriaTrainingDummy::StaticClass(), Start,
				(Team == 0 ? Axis : -Axis).Rotation(), P);
			if (!Bot)
			{
				continue;
			}
			Bot->TeamId = Team;
			Bot->bHunter = true;
			Bot->BotDifficulty = 0;
			Bot->bNoAutoHeal = true;
			if (PC && Rules->BotHeroes.Num() > 0)
			{
				Bot->SetHeroDefinition(PC->FindHero(Rules->BotHeroes[HeroIndex++ % Rules->BotHeroes.Num()]));
			}
			Bot->SetAutopilot(true, 0.35f + 0.2f * Rules->BotDifficulty);
			Bot->SetAutopilotRoute(LaneRoutes[Team][Lane + 1]);
		}
	}
	// The player (team 0) takes the right lane, so each lane has 2 heroes per team in a 6v6 (turn 45).
	if (AValdoriaHeroCharacter* Me = PC ? Cast<AValdoriaHeroCharacter>(PC->GetPawn()) : nullptr)
	{
		Me->SetAutopilotRoute(LaneRoutes[0][2]);
	}
}

int32 AValdoriaMobaMode::GetGuardiansAlive(uint8 Team) const
{
	int32 Alive = 0;
	for (const TWeakObjectPtr<AValdoriaGuardian>& G : Guardians)
	{
		Alive += (G.IsValid() && G->GuardTeam == Team && G->IsLinkedToFinal()) ? 1 : 0;
	}
	return Alive;
}

float AValdoriaMobaMode::GetTimeLeft() const
{
	return Rules && Rules->TimeLimit > 0.f ? FMath::Max(0.f, Rules->TimeLimit - static_cast<float>(GetWorld()->GetTimeSeconds() - MatchStart)) : -1.f;
}

void AValdoriaMobaMode::OnHeroKilled(AValdoriaHeroCharacter* Victim, AValdoriaHeroCharacter* Killer)
{
	if (const AValdoriaTrainingDummy* Minion = Cast<AValdoriaTrainingDummy>(Victim); Minion && Minion->bMinion)
	{
		Victim->SetLifeSpan(1.5f); // minions do not come back and do not count as hero kills
		return;
	}
	if (Victim && !Victim->IsA<AValdoriaGuardian>() && Victim->IsA<AValdoriaTrainingDummy>())
	{
		// MOBA bots respawn at their base after the respawn delay (not where they died).
		AValdoriaTrainingDummy* Bot = Cast<AValdoriaTrainingDummy>(Victim);
		const FVector Home = Ground(Base[FMath::Clamp<int32>(Bot->TeamId, 0, 1)]);
		FTimerHandle Timer;
		GetWorldTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(Bot, [Bot, Home]()
		{
			Bot->ResetCombatState();
			Bot->TeleportTo(Home, Bot->GetActorRotation());
			Bot->RestartAutopilotRoute();
		}), GetRespawnDelayFor(Bot), false);
		Bot->RespawnAt = GetWorld()->GetTimeSeconds() + GetRespawnDelayFor(Bot);
	}
	else
	{
		Super::OnHeroKilled(Victim, Killer);
	}
	if (const AValdoriaGuardian* Fallen = Cast<AValdoriaGuardian>(Victim))
	{
		FallTimes.Add(Fallen->GuardTeam * 100 + Fallen->LaneIndex * 10 + Fallen->SpotIndex, GetWorld()->GetTimeSeconds() - MatchStart);
	}
	if (Result == 0 && Victim && Killer && !Victim->IsA<AValdoriaGuardian>())
	{
		(Killer->TeamId == 0 ? Kills0 : Kills1)++;
	}
}

void AValdoriaMobaMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Rules && Rules->bMinions && Result == 0 && NextMinionWave > 0.f && GetWorld()->GetTimeSeconds() >= NextMinionWave)
	{
		NextMinionWave = GetWorld()->GetTimeSeconds() + Rules->MinionWaveSeconds;
		SpawnMinionWave();
	}
	if (bAutoPlayer)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (AValdoriaHeroCharacter* Hero = Cast<AValdoriaHeroCharacter>(PC->GetPawn()); Hero && !Hero->IsAutopilot())
			{
				Hero->SetAutopilot(true, 0.7f);
			}
		}
	}
	ComebackTimer += DeltaSeconds;
	if (ComebackTimer >= 0.5f)
	{
		ComebackTimer = 0.f;
		TickComeback();
	}
	if (Result != 0 || Guardians.Num() == 0)
	{
		return;
	}
	// Win: bring down the enemy final guardian. Time out: more guardians standing, then more kills.
	if (FinalGuardian[1].IsValid() && FinalGuardian[1]->IsDead())
	{
		EndMatch(0);
	}
	else if (FinalGuardian[0].IsValid() && FinalGuardian[0]->IsDead())
	{
		EndMatch(1);
	}
	else if (GetTimeLeft() == 0.f)
	{
		const int32 A = GetGuardiansAlive(0), B = GetGuardiansAlive(1);
		EndMatch(A != B ? (A > B ? 0 : 1) : (Kills0 >= Kills1 ? 0 : 1));
	}
}

void AValdoriaMobaMode::EndMatch(uint8 Winner)
{
	Result = Winner == 0 ? 1 : 2;
	FTimerHandle Restart;
	GetWorldTimerManager().SetTimer(Restart, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)), true,
			bAutoPlayer ? TEXT("game=/Script/EchoesOfValdoria.ValdoriaMobaMode?bots=1") : TEXT("game=/Script/EchoesOfValdoria.ValdoriaMobaMode"));
	}), 8.f, false);
}

void AValdoriaMobaMode::TickComeback()
{
	const float T = GetWorld()->GetTimeSeconds() - MatchStart;
	auto FellEarly = [this](int32 Key)
	{
		const float* When = FallTimes.Find(Key);
		return When && *When < Rules->EarlyWindow;
	};
	const FValdoriaSpotRule* S1 = Rules->Spots.IsValidIndex(0) ? &Rules->Spots[0] : nullptr;
	for (uint8 Team = 0; Team < 2; ++Team)
	{
		// Spot 1 fell early (any lane): that team keeps the spot-1 buff doubled and constant until EarlyWindow.
		bool bSpot1Early = false;
		for (int32 Lane = 0; Lane < 3; ++Lane)
		{
			bSpot1Early |= FellEarly(Team * 100 + Lane * 10 + 0);
		}
		if (S1 && bSpot1Early && T < Rules->EarlyWindow && S1->Buffs.Num() > 0 && S1->BuffValues.Num() > 0)
		{
			for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
			{
				if (It->TeamId == Team && !It->IsDead() && !It->IsA<AValdoriaGuardian>())
				{
					It->ApplyTeamBuff(static_cast<uint8>(S1->Buffs[0]), S1->BuffValues[0] * Rules->Spot1EarlyFallMultiplier, 1.f);
				}
			}
		}
	}
	// Last spot of a lane: x3 buffs if spots 1 and 2 of that lane fell early, until LastSpotBoostUntil.
	for (const TWeakObjectPtr<AValdoriaGuardian>& G : Guardians)
	{
		if (!G.IsValid() || G->SpotIndex != Rules->Spots.Num() - 1)
		{
			continue;
		}
		const int32 Key = G->GuardTeam * 100 + G->LaneIndex * 10;
		const bool bBoost = FellEarly(Key + 0) && FellEarly(Key + 1) && T < Rules->LastSpotBoostUntil;
		G->BuffMultiplier = bBoost ? Rules->LastSpotMultiplier : 1.f;
	}
}
void AValdoriaMobaMode::RunMobaTestStep()
{
	++MobaTestStep; // every 0.5 s
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	AValdoriaHeroCharacter* Hero = PC ? Cast<AValdoriaHeroCharacter>(PC->GetPawn()) : nullptr;
	if (!Hero)
	{
		return;
	}
	auto Shot = [](const TCHAR* Name)
	{
		FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots") / Name), true, false);
	};
	// Enemy spot 1 of the middle lane.
	AValdoriaGuardian* Target = nullptr;
	for (const TWeakObjectPtr<AValdoriaGuardian>& G : Guardians)
	{
		if (G.IsValid() && G->GuardTeam == 1 && G->LaneIndex == 1 && G->SpotIndex == 0)
		{
			Target = G.Get();
		}
	}
	switch (MobaTestStep)
	{
	case 12:
	{
		int32 Rails = 0;
		for (TActorIterator<AValdoriaLaneRail> It(GetWorld()); It; ++It) { ++Rails; }
		UE_LOG(LogValdoria, Display, TEXT("MOBATEST inicio: guardioes=%d (vivos 0:%d 1:%d) finais=%d/%d trilhas=%d"), Guardians.Num(),
			GetGuardiansAlive(0), GetGuardiansAlive(1), FinalGuardian[0].IsValid(), FinalGuardian[1].IsValid(), Rails);
		if (AValdoriaGuardian* F = FinalGuardian[1].Get())
		{
			UE_LOG(LogValdoria, Display, TEXT("MOBATEST final herda: estilos=%d mago=%d vida=%.0f"), F->InheritedStyles.Num(), F->bInheritedMage, F->GetMaxHealth());
		}
		Shot(TEXT("Moba_Inicio.png"));
		break;
	}
	case 14:
		if (Target)
		{
			Hero->bGodMode = true;
			Hero->TeleportTo(Target->GetActorLocation() - Target->GetActorForwardVector() * -300.f + FVector(0, 0, 50.f), Hero->GetActorRotation());
			Hero->DealDamageTo(Target, 99999.f);
		}
		break;
	case 16:
		if (Target)
		{
			PC->SetControlRotation((Target->GetActorLocation() - Hero->GetActorLocation()).Rotation() + FRotator(-15.f, 0.f, 0.f));
			UE_LOG(LogValdoria, Display, TEXT("MOBATEST derrotado: estado=%d imune=%d vivos inimigos=%d"), (int32)Target->State, Target->IsInvulnerable(), GetGuardiansAlive(1));
			Shot(TEXT("Moba_SemiSelado.png"));
			Hero->DealTypedDamage(Target, 50.f, EValdoriaDamageType::Physical); // should show IMMUNE
		}
		break;
	case 17:
		// Isolate the ritual: enemy heroes stop and enemy bots go home, so nobody reverses the seal during the test.
		for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
		{
			if (*It != Hero && *It != Target && It->TeamId == 1)
			{
				It->SetAutopilot(false);
				if (!Cast<AValdoriaGuardian>(*It))
				{
					It->TeleportTo(Ground(Base[1]) + FVector(0, 0, 50.f), It->GetActorRotation());
				}
			}
		}
		Hero->SetAutopilot(false); Hero->bInteractHeld = true;
		break;
	case 20: if (Target) { UE_LOG(LogValdoria, Display, TEXT("MOBATEST selando: progresso=%.2f"), Hero->GetChannelProgress()); } Shot(TEXT("Moba_Selando.png")); break;
	case 28:
		Hero->bInteractHeld = false;
		if (Target)
		{
			UE_LOG(LogValdoria, Display, TEXT("MOBATEST selado: estado=%d morto=%d vivos inimigos=%d"), (int32)Target->State, Target->IsDead(), GetGuardiansAlive(1));
		}
		Shot(TEXT("Moba_Selado.png"));
		break;
	case 30:
	{
		// Bifrost: go home and look at our middle trail, then hold Space.
		Hero->TeleportTo(Ground(Base[0]) + FVector(0, 0, 50.f), Hero->GetActorRotation());
		for (TActorIterator<AValdoriaLaneRail> It(GetWorld()); It; ++It)
		{
			if (It->Team == 0 && It->LaneName.Contains(TEXT("Paulista")))
			{
				PC->SetControlRotation((It->GetPointAt(900.f) - (Hero->GetActorLocation() + FVector(0, 0, 60.f))).Rotation());
			}
		}
		break;
	}
	case 31: Hero->LastHarmedTime = -100.f; PressKey(EKeys::SpaceBar, true); break;
	case 33: PressKey(EKeys::SpaceBar, false); UE_LOG(LogValdoria, Display, TEXT("MOBATEST bifrost: montado=%d trilha=%s"), Hero->IsRiding(), *Hero->GetRideLaneName()); break;
	case 35: if (Hero->IsRiding()) { PC->SetControlRotation(FRotator(-22.f, Hero->GetActorRotation().Yaw, 0.f)); } break; // look along the bridge for the picture
	case 36: UE_LOG(LogValdoria, Display, TEXT("MOBATEST bifrost hero at (%.0f,%.0f,%.0f)"), Hero->GetActorLocation().X, Hero->GetActorLocation().Y, Hero->GetActorLocation().Z); Shot(TEXT("Moba_Bifrost.png")); UE_LOG(LogValdoria, Display, TEXT("MOBATEST bifrost depois: montado=%d"), Hero->IsRiding()); break;
	case 40:
		UE_LOG(LogValdoria, Display, TEXT("MOBATEST fim"));
		MobaTestStep = 100000;
		FPlatformMisc::RequestExit(false);
		break;
	default: break;
	}
}

// ================================================================ lanes and minions (turn 45)

void AValdoriaMobaMode::BuildLaneRoutes(const FVector& Axis, const FVector& SideDir, const FVector& Centre, float HalfLength, float SideOffset)
{
	// Each lane: base exit -> lane centre -> every ENEMY spot of that lane -> enemy base, ordered along the
	// base-to-base axis, plus extra points every ~35 m so bots and minions stay on their own lane instead of the
	// navmesh shortcut through another lane. Every point is a free street spot.
	for (uint8 Team = 0; Team < 2; ++Team)
	{
		const FVector Toward = Team == 0 ? Axis : -Axis;
		for (int32 Lane = -1; Lane <= 1; ++Lane)
		{
			const FVector LaneOffset = SideDir * (Lane * SideOffset);
			TArray<FVector> Key;
			Key.Add(Base[Team] + Toward * 600.f + LaneOffset * 0.5f);
			for (const TWeakObjectPtr<AValdoriaGuardian>& G : Guardians)
			{
				if (G.IsValid() && G->LaneIndex == Lane + 1 && G->GuardTeam != Team) // own giants stand on their points and block the way (6v6 test)
				{
					Key.Add(G->GetActorLocation());
				}
			}
			Key.Add(Centre + LaneOffset);
			Key.Add(Base[1 - Team] + LaneOffset * 0.3f);
			const FVector Home = Base[Team];
			Key.Sort([&Home, &Toward](const FVector& A, const FVector& B) { return FVector::DotProduct(A - Home, Toward) < FVector::DotProduct(B - Home, Toward); });
			TArray<FVector>& Route = LaneRoutes[Team][Lane + 1];
			Route.Reset();
			for (int32 k = 0; k < Key.Num(); ++k)
			{
				if (k > 0)
				{
					const float Gap = FVector::Dist2D(Key[k - 1], Key[k]);
					const int32 Extra = FMath::FloorToInt(Gap / 3500.f);
					for (int32 e = 1; e <= Extra; ++e)
					{
						Route.Add(StreetPoint(FMath::Lerp(Key[k - 1], Key[k], e / float(Extra + 1))));
					}
				}
				Route.Add(StreetPoint(Key[k]));
			}
		}
	}
}

void AValdoriaMobaMode::SpawnMinionWave()
{
	int32 Alive = 0;
	for (TActorIterator<AValdoriaTrainingDummy> It(GetWorld()); It; ++It)
	{
		Alive += (It->bMinion && !It->IsDead()) ? 1 : 0;
	}
	if (!MinionDef)
	{
		// The Recruit's rifle, much weaker and small: easy to tell from heroes and from the (big) guardians.
		MinionDef = UValdoriaHeroDefinition::CreateFallbackRecruit(this);
		MinionDef->HeroId = TEXT("minion");
		MinionDef->DisplayName = NSLOCTEXT("Valdoria", "MinionName", "Minion");
		MinionDef->MaxHealth = Rules->MinionHealth;
		MinionDef->MeshScale *= Rules->MinionScale;
		MinionDef->CapsuleRadius *= Rules->MinionScale;
		MinionDef->CapsuleHalfHeight *= Rules->MinionScale;
		MinionDef->MoveSpeed = 420.f;
		MinionDef->Weapon.Damage = Rules->MinionDamage;
		MinionDef->Weapon.Range = 1800.f;
		MinionDef->Movement.ShiftMode = EValdoriaShiftMode::None;
		MinionDef->Movement.bCanGlide = false;
		MinionDef->Movement.bCanWallClimb = false;
		MinionDef->Movement.FlightMode = EValdoriaFlightMode::None;
		MinionDef->Abilities.RemoveAll([](const FValdoriaAbilityEntry& E) { return E.Slot != EValdoriaAbilitySlot::PrimaryFire; });
		MinionDef->UltimateCost = 1000000.f;
	}
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	for (int32 n = 0; n < Rules->MinionsPerWave; ++n)
	{
		for (uint8 Team = 0; Team < 2; ++Team)
		{
			for (int32 L = 0; L < 3; ++L)
			{
				if (Alive >= Rules->MaxMinions || LaneRoutes[Team][L].Num() == 0)
				{
					return;
				}
				const FVector First = LaneRoutes[Team][L][0];
				const FVector Start = First + FVector(FMath::FRandRange(-150.f, 150.f), FMath::FRandRange(-150.f, 150.f), 0.f);
				AValdoriaTrainingDummy* M = GetWorld()->SpawnActor<AValdoriaTrainingDummy>(AValdoriaTrainingDummy::StaticClass(), Start,
					(LaneRoutes[Team][L].Last() - First).Rotation(), P);
				if (!M)
				{
					continue;
				}
				M->bMinion = true;
				M->TeamId = Team;
				M->bHunter = true;
				M->BotDifficulty = 0;
				M->bNoAutoHeal = true;
				M->SetHeroDefinition(MinionDef);
				M->SetAutopilot(true, 0.3f);
				M->SetAutopilotPace(1.5f, 1.f); // lighter thinking: dozens of minions (turn 45 6v6 test: 58 FPS)
				TArray<FVector> Route = LaneRoutes[Team][L];
				Route.RemoveAt(0); // already there
				M->SetAutopilotRoute(Route);
				++Alive;
			}
		}
	}
}

float AValdoriaMobaMode::GetRespawnDelayFor(const AValdoriaHeroCharacter* Victim) const
{
	// Turn 46 (provisional, owner balances): grows with the hero's level and, after 10 minutes, with match time.
	// Deaths and farm are not counted on their own (farm already becomes level; counting deaths punishes twice).
	//   seconds = (RespawnDelay + 1.5 x (level - 1)) x (1 + 0.03 x minutes after the 10th), capped at 50 s
	const float BaseDelay = Rules ? Rules->RespawnDelay : 6.f;
	const int32 Lvl = Victim && Victim->bUsesLevels ? Victim->Level : 1;
	const float Minutes = (GetWorld()->GetTimeSeconds() - MatchStart) / 60.f;
	const float TimeFactor = 1.f + 0.03f * FMath::Max(0.f, Minutes - 10.f);
	return FMath::Min(50.f, (BaseDelay + 1.5f * (Lvl - 1)) * TimeFactor);
}
