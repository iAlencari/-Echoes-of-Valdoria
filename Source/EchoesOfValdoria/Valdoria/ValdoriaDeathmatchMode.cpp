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
	Super::StartPlay();
	MatchStart = GetWorld()->GetTimeSeconds();
	SpawnBots();
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
		FVector Where = Centre + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * 1400.f + FVector(0.f, 0.f, 100.f);
		FHitResult Down;
		if (GetWorld()->LineTraceSingleByChannel(Down, Where + FVector(0, 0, 2000.f), Where - FVector(0, 0, 5000.f), ECC_WorldStatic))
		{
			Where = Down.ImpactPoint + FVector(0.f, 0.f, 110.f);
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
		Bot->bForceStrafe = true;
		Bot->bNoAutoHeal = true;
		if (PC && Rules->BotHeroes.Num() > 0)
		{
			Bot->SetHeroDefinition(PC->FindHero(Rules->BotHeroes[i % Rules->BotHeroes.Num()]));
		}
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
	if (Result == 0 && Rules && Rules->TimeLimit > 0.f && GetTimeLeft() <= 0.f)
	{
		EndMatch(PlayerScore > GetBestBotScore());
	}
}

void AValdoriaDeathmatchMode::EndMatch(bool bPlayerWon)
{
	Result = bPlayerWon ? 1 : 2;
	// Show the result for a few seconds, then restart the match on the same map.
	FTimerHandle Restart;
	GetWorldTimerManager().SetTimer(Restart, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)), true,
			TEXT("game=/Script/EchoesOfValdoria.ValdoriaDeathmatchMode"));
	}), 6.f, false);
}
