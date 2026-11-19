// Echoes of Valdoria - free-for-all deathmatch.

#include "ValdoriaDeathmatchMode.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "ValdoriaHeroCharacter.h"
#include "ValdoriaHeroDefinition.h"
#include "ValdoriaPlayerController.h"
#include "ValdoriaTrainingDummy.h"

AValdoriaDeathmatchMode::AValdoriaDeathmatchMode()
{
	PrimaryActorTick.bCanEverTick = true;
	bSpawnTrainingDummies = false;
}

void AValdoriaDeathmatchMode::StartPlay()
{
	Rules = LoadObject<UValdoriaModeRules>(nullptr, TEXT("/Game/Valdoria/Modes/DA_Deathmatch.DA_Deathmatch"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Rules)
	{
		Rules = NewObject<UValdoriaModeRules>(this); // defaults until the data asset exists
	}
	PlayerRespawnDelay = Rules->RespawnDelay;
	bSpreadRespawns = true; // the player also respawns at a spread spot
	Super::StartPlay();
	MatchStart = GetWorld()->GetTimeSeconds();
	SpawnBots();
	// Bots vs bots: ?bots=1 puts the player's hero on autopilot too (watch them play).
	if (UGameplayStatics::HasOption(OptionsString, TEXT("bots")))
	{
		bAutoPlayer = true;
	}
}

void AValdoriaDeathmatchMode::SpawnBots()
{
	// Spread the bots around the player starts / the map centre.
	TArray<AActor*> Starts;
	UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), Starts);
	const FVector Centre = Starts.Num() > 0 ? Starts[0]->GetActorLocation() : FVector::ZeroVector;
	AValdoriaPlayerController* PC = Cast<AValdoriaPlayerController>(GetWorld()->GetFirstPlayerController());
	for (int32 i = 0; i < Rules->BotCount; ++i)
	{
		const float Angle = 2.f * PI * i / FMath::Max(1, Rules->BotCount);
		// Turn 44: on Nova Pauliceia part of the 14 m ring is off the map (west of the metro): bots spawned in the
		// void and fell forever. Look for ground, moving closer to the centre until something is under the point.
		FVector Where = Centre + FVector(0.f, 0.f, 100.f);
		// Turn 44 playtest: with every bot around the player start the whole match happened inside the metro and the
		// player was killed on spawn. Bots now start at random street-level spots spread over the map.
		const bool bSpread = FindSpreadSpawnPoint(Where);
		for (float Radius = 1400.f; !bSpread && Radius >= 0.f; Radius -= 350.f)
		{
			const FVector Try = Centre + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Radius;
			FHitResult Down;
			if (GetWorld()->LineTraceSingleByObjectType(Down, Try + FVector(0, 0, 300.f), Try - FVector(0, 0, 3000.f), FCollisionObjectQueryParams(ECC_WorldStatic)))
			{
				Where = Down.ImpactPoint + FVector(0.f, 0.f, 110.f);
				break;
			}
		}
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AValdoriaTrainingDummy* Bot = GetWorld()->SpawnActor<AValdoriaTrainingDummy>(AValdoriaTrainingDummy::StaticClass(), Where,
			(Centre - Where).GetSafeNormal2D().Rotation(), P);
		if (!Bot)
		{
			continue;
		}
		Bot->TeamId = static_cast<uint8>(100 + i); // free-for-all: everyone is an enemy
		Bot->BotDifficulty = Rules->BotDifficulty;
		Bot->bHunter = true;
		Bot->BotDifficulty = 0; // no fake shots: the autopilot uses the hero's real weapon and abilities
		Bot->bNoAutoHeal = true;
		Bot->bReviveAtSpreadSpot = true;
		if (PC && Rules->BotHeroes.Num() > 0)
		{
			Bot->SetHeroDefinition(PC->FindHero(Rules->BotHeroes[i % Rules->BotHeroes.Num()]));
		}
		Bot->SetAutopilot(true, 0.35f + 0.2f * Rules->BotDifficulty);
		Bots.Add(Bot);
		BotScores.Add(Bot->TeamId, 0);
	}
}

int32 AValdoriaDeathmatchMode::GetBestBotScore() const
{
	int32 Best = 0;
	for (const TPair<uint8, int32>& Pair : BotScores)
	{
		Best = FMath::Max(Best, Pair.Value);
	}
	return Best;
}

float AValdoriaDeathmatchMode::GetTimeLeft() const
{
	if (!Rules || Rules->TimeLimit <= 0.f)
	{
		return -1.f;
	}
	return FMath::Max(0.f, Rules->TimeLimit - static_cast<float>(GetWorld()->GetTimeSeconds() - MatchStart));
}

void AValdoriaDeathmatchMode::OnHeroKilled(AValdoriaHeroCharacter* Victim, AValdoriaHeroCharacter* Killer)
{
	Super::OnHeroKilled(Victim, Killer); // respawns (players) / revives (bots)
	if (Result != 0 || !Victim || !Killer || Killer == Victim)
	{
		return;
	}
	if (Killer->IsPlayerControlled())
	{
		++PlayerScore;
	}
	else if (int32* Score = BotScores.Find(Killer->TeamId))
	{
		++(*Score);
	}
	if (PlayerScore >= Rules->ScoreToWin)
	{
		EndMatch(true);
	}
	else if (GetBestBotScore() >= Rules->ScoreToWin)
	{
		EndMatch(false);
	}
}

void AValdoriaDeathmatchMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bAutoPlayer)
	{
		// Keep the player's (re)spawned hero on autopilot.
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (AValdoriaHeroCharacter* Hero = Cast<AValdoriaHeroCharacter>(PC->GetPawn()); Hero && !Hero->IsAutopilot())
			{
				Hero->SetAutopilot(true, 0.7f);
			}
		}
	}
	if (Result == 0 && Rules && Rules->TimeLimit > 0.f && GetTimeLeft() <= 0.f)
	{
		EndMatch(PlayerScore > GetBestBotScore());
	}
}

void AValdoriaDeathmatchMode::EndMatch(bool bPlayerWon)
{
	Result = bPlayerWon ? 1 : 2;
	if (PlaytestSeconds > 0 && PlaytestStep < PlaytestSeconds)
	{
		// Playtest: the match is over, write the summary on the next tick and close (the restart reset the counters).
		UE_LOG(LogValdoria, Display, TEXT("PLAYTEST partida terminou: %s"), bPlayerWon ? TEXT("vitoria do jogador") : TEXT("vitoria de um bot"));
		PlaytestStep = PlaytestSeconds - 1;
	}
	// Show the result for a few seconds, then restart the match on the same map.
	FTimerHandle Restart;
	GetWorldTimerManager().SetTimer(Restart, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)), true,
			bAutoPlayer ? TEXT("game=/Script/EchoesOfValdoria.ValdoriaDeathmatchMode?bots=1") : TEXT("game=/Script/EchoesOfValdoria.ValdoriaDeathmatchMode"));
	}), 6.f, false);
}
