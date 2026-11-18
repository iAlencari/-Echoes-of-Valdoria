// Echoes of Valdoria - boneco de treino.

#include "ValdoriaTrainingDummy.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "ValdoriaHeroDefinition.h"
#include "ValdoriaAudio.h"

AValdoriaTrainingDummy::AValdoriaTrainingDummy()
{
	TeamId = 255;
	AutoPossessAI = EAutoPossessAI::Disabled;
	bUseControllerRotationYaw = false;
	// Sem controlador: ainda assim precisa de gravidade e de andar de lado.
	GetCharacterMovement()->bRunPhysicsWithNoController = true;
}

void AValdoriaTrainingDummy::BeginPlay()
{
	// Cada tipo de boneco ganha uma definição própria (criada em código).
	UValdoriaHeroDefinition* Def = UValdoriaHeroDefinition::CreateFallbackRecruit(this);
	Def->Abilities.Reset();
	Def->UltimateChargePerSecond = 0.f;
	Def->Mesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple")));
	switch (Kind)
	{
	case EValdoriaDummyKind::Armored:
		Def->MaxHealth = 200.f;
		Def->MaxArmor = 300.f;
		break;
	case EValdoriaDummyKind::Shielded:
		Def->MaxHealth = 200.f;
		Def->MaxShield = 300.f;
		break;
	case EValdoriaDummyKind::Strafing:
		Def->MaxHealth = 250.f;
		Def->MoveSpeed = 450.f;
		break;
	default:
		Def->MaxHealth = 250.f;
		break;
	}
	HeroDefinition = Def;
	Super::BeginPlay();
	StartLocation = GetActorLocation();
}

void AValdoriaTrainingDummy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || IsDead())
	{
		return;
	}
	// Vida volta ao máximo depois de um tempo sem levar dano.
	TickBot(DeltaSeconds);
	if (!bNoAutoHeal && GetWorld()->GetTimeSeconds() - LastDamageTakenTime > ResetDelay && GetHealth() < GetMaxHealth())
	{
		AdminRestoreAll();
	}
	if ((Kind == EValdoriaDummyKind::Strafing || bForceStrafe) && !bHunter)
	{
		StrafeTime += DeltaSeconds;
		const FVector Side = GetActorRightVector();
		const float Offset = FVector::DotProduct(GetActorLocation() - StartLocation, Side);
		const float Dir = FMath::Sin(StrafeTime * 1.3f) >= 0.f ? 1.f : -1.f;
		if (FMath::Abs(Offset) < 450.f || FMath::Sign(Offset) != Dir)
		{
			AddMovementInput(Side, Dir);
		}
	}
}

void AValdoriaTrainingDummy::ScheduleRevive()
{
	GetWorldTimerManager().SetTimer(ReviveTimer, this, &AValdoriaTrainingDummy::Revive, 3.f, false);
}

void AValdoriaTrainingDummy::Revive()
{
	ResetCombatState();
	SetActorLocation(StartLocation, false, nullptr, ETeleportType::TeleportPhysics);
}

void AValdoriaTrainingDummy::TickBot(float DeltaSeconds)
{
	if (BotDifficulty <= 0)
	{
		return;
	}
	// Easy / medium / hard: damage, accuracy, fire interval and range.
	static const float Damage[] = { 0.f, 6.f, 10.f, 16.f };
	static const float Accuracy[] = { 0.f, 0.3f, 0.5f, 0.75f };
	static const float Interval[] = { 0.f, 1.0f, 0.6f, 0.35f };
	static const float Range[] = { 0.f, 2500.f, 3500.f, 5000.f };
	const int32 D = FMath::Clamp(BotDifficulty, 1, 3);
	const float Now = GetWorld()->GetTimeSeconds();
	const FVector Eye = GetActorLocation() + FVector(0.f, 0.f, 60.f);

	auto CanSee = [this, &Eye](AValdoriaHeroCharacter* Other)
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ValdoriaBot), false, this);
		return !GetWorld()->LineTraceSingleByChannel(Hit, Eye, Other->GetActorLocation() + FVector(0.f, 0.f, 40.f), ECC_Visibility, Params)
			|| Hit.GetActor() == Other;
	};

	// Pick a target twice a second. Training dummies only look at the player; hunters (deathmatch)
	// go for the nearest visible enemy, player or bot. Invisible heroes are ignored.
	if (Now >= NextTargetPick)
	{
		NextTargetPick = Now + 0.5f;
		BotTarget = nullptr;
		float Best = Range[D] * (bHunter ? 1.5f : 1.f);
		for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
		{
			AValdoriaHeroCharacter* Other = *It;
			if (!IsEnemy(Other) || Other->IsHiddenByStealth() || (!bHunter && !Other->IsPlayerControlled()))
			{
				continue;
			}
			const float Dist = FVector::Dist(GetActorLocation(), Other->GetActorLocation());
			if (Dist < Best && CanSee(Other))
			{
				Best = Dist;
				BotTarget = Other;
			}
		}
	}
	AValdoriaHeroCharacter* Target = BotTarget.Get();
	if (!Target || Target->IsDead() || Target->IsHiddenByStealth())
	{
		if (bHunter)
		{
			// Nobody in sight: roam towards the map centre / spawn area, turning at walls.
			FVector Dir = (StartLocation - GetActorLocation()).GetSafeNormal2D();
			if (FVector::Dist2D(StartLocation, GetActorLocation()) < 300.f)
			{
				Dir = FRotator(0.f, FMath::Fmod(Now * 25.f + TeamId * 40.f, 360.f), 0.f).Vector();
			}
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), Dir.Rotation(), DeltaSeconds, 4.f));
			AddMovementInput(Dir, 0.6f);
			if (IsFacingWall(90.f))
			{
				Jump();
			}
		}
		return;
	}
	const FVector Aim = Target->GetActorLocation() + FVector(0.f, 0.f, 40.f);
	const float Dist = FVector::Dist(Eye, Aim);
	FRotator Face = (Aim - Eye).Rotation();
	Face.Pitch = 0.f;
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), Face, DeltaSeconds, 8.f));

	if (bHunter)
	{
		// Keep a fighting distance, strafe side to side, hop over low walls now and then.
		const FVector Fwd = GetActorForwardVector().GetSafeNormal2D();
		const FVector Side = GetActorRightVector();
		const float Desired = D >= 3 ? 900.f : 1200.f;
		if (Dist > Desired + 300.f)
		{
			AddMovementInput(Fwd, 1.f);
		}
		else if (Dist < Desired - 500.f)
		{
			AddMovementInput(-Fwd, 0.8f);
		}
		StrafeTime += DeltaSeconds;
		AddMovementInput(Side, FMath::Sin(StrafeTime * (0.9f + 0.2f * D) + TeamId) >= 0.f ? 0.7f : -0.7f);
		if (IsFacingWall(90.f) || FMath::FRand() < 0.004f * D)
		{
			Jump();
		}
	}
	if (Dist > Range[D] || Now < NextBotShotTime || !CanSee(Target))
	{
		return;
	}
	NextBotShotTime = Now + Interval[D] * FMath::FRandRange(0.85f, 1.15f);
	const bool bHit = FMath::FRand() < Accuracy[D];
	const FVector Impact = bHit ? Aim : Aim + FMath::VRand() * 120.f;
	PlayShotEffects(Impact, bHit);
	ValdoriaAudio::PlayAt(this, ValdoriaAudio::Load(TEXT("Shots"), TEXT("pistol")), GetActorLocation(), 0.35f, 0.1f);
	if (bHit)
	{
		DealTypedDamage(Target, Damage[D], EValdoriaDamageType::Physical);
	}
}
