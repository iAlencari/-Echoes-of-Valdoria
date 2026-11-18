// Echoes of Valdoria - 6v6 MOBA with Guardian Spots.

#include "ValdoriaMobaMode.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "ValdoriaHeroCharacter.h"
#include "ValdoriaHeroDefinition.h"
#include "ValdoriaPlayerController.h"
#include "ValdoriaTrainingDummy.h"

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
	// Spot 3 (owner: two guardians doing different things; buffs are placeholders): regeneration + damage bonus, no fade.
	FValdoriaSpotRule S3;
	S3.SideLanePosition = 0.38f;
	S3.MidLanePosition = 0.18f;
	S3.Buffs = { B::Regeneration, B::DamageBonus };
	S3.BuffValues = { 10.f, 0.1f };
	S3.BuffFadeEnd = 0.f;
	S3.GuardianHealth = 4000.f;
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
	if (GetWorld()->LineTraceSingleByChannel(Down, Point + FVector(0, 0, 3000.f), Point - FVector(0, 0, 6000.f), ECC_WorldStatic))
	{
		return Down.ImpactPoint + FVector(0.f, 0.f, 120.f);
	}
	return Point;
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
			for (const FValdoriaSpotRule& Spot : Rules->Spots)
			{
				const float Along = (bSide ? Spot.SideLanePosition : Spot.MidLanePosition) * HalfLength;
				const FVector LanePoint = Base[Team] + Toward * Along + SideDir * (Lane * Rules->SideLaneOffset);
				for (int32 g = 0; g < Spot.Buffs.Num(); ++g)
				{
					const FVector Where = Ground(LanePoint + SideDir * (g - (Spot.Buffs.Num() - 1) * 0.5f) * 350.f);
					AValdoriaGuardian* Guardian = GetWorld()->SpawnActor<AValdoriaGuardian>(AValdoriaGuardian::StaticClass(), Where,
						(-Toward).Rotation(), P);
					if (!Guardian)
					{
						continue;
					}
					Guardian->Buff = Spot.Buffs[g];
					Guardian->BuffValue = Spot.BuffValues.IsValidIndex(g) ? Spot.BuffValues[g] : 0.f;
					Guardian->BuffHoldUntil = Spot.BuffHoldUntil;
					Guardian->BuffFadeEnd = Spot.BuffFadeEnd;
					Guardian->GuardianHealth = Spot.GuardianHealth;
					Guardian->SetupSpot(Team, Rules->SpotAreaRadius);
					Guardian->ResetCombatState();
					Guardians.Add(Guardian);
				}
			}
		}
	}

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
			const FVector LaneOffset = SideDir * (Lane * Rules->SideLaneOffset);
			Bot->SetAutopilotRoute({ Base[Team] + LaneOffset + (Team == 0 ? Axis : -Axis) * HalfLength * 0.3f,
				Centre + LaneOffset, Base[1 - Team] + LaneOffset * 0.3f });
		}
	}
}

int32 AValdoriaMobaMode::GetGuardiansAlive(uint8 Team) const
{
	int32 Alive = 0;
	for (const TWeakObjectPtr<AValdoriaGuardian>& G : Guardians)
	{
		Alive += (G.IsValid() && G->GuardTeam == Team && !G->IsDead()) ? 1 : 0;
	}
	return Alive;
}

float AValdoriaMobaMode::GetTimeLeft() const
{
	return Rules && Rules->TimeLimit > 0.f ? FMath::Max(0.f, Rules->TimeLimit - static_cast<float>(GetWorld()->GetTimeSeconds() - MatchStart)) : -1.f;
}

void AValdoriaMobaMode::OnHeroKilled(AValdoriaHeroCharacter* Victim, AValdoriaHeroCharacter* Killer)
{
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
		}), Rules->RespawnDelay, false);
	}
	else
	{
		Super::OnHeroKilled(Victim, Killer);
	}
	if (Result == 0 && Victim && Killer && !Victim->IsA<AValdoriaGuardian>())
	{
		(Killer->TeamId == 0 ? Kills0 : Kills1)++;
	}
}

void AValdoriaMobaMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
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
	if (Result != 0 || Guardians.Num() == 0)
	{
		return;
	}
	// Win: every enemy guardian down. Time out: more guardians standing, then more kills.
	if (GetGuardiansAlive(1) == 0)
	{
		EndMatch(0);
	}
	else if (GetGuardiansAlive(0) == 0)
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
