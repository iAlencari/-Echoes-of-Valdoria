// Echoes of Valdoria - boneco de treino.

#include "ValdoriaTrainingDummy.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "ValdoriaHeroDefinition.h"

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
	if (Kind == EValdoriaDummyKind::Strafing || bForceStrafe)
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
	// Fácil / médio / difícil: dano, precisão, cadência e alcance.
	static const float Damage[] = { 0.f, 6.f, 10.f, 16.f };
	static const float Accuracy[] = { 0.f, 0.3f, 0.5f, 0.75f };
	static const float Interval[] = { 0.f, 1.0f, 0.6f, 0.35f };
	static const float Range[] = { 0.f, 2500.f, 3500.f, 5000.f };
	const int32 D = FMath::Clamp(BotDifficulty, 1, 3);

	APawn* Player = GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr;
	AValdoriaHeroCharacter* Target = Cast<AValdoriaHeroCharacter>(Player);
	if (!Target || Target->IsDead() || !IsEnemy(Target))
	{
		return;
	}
	const FVector Eye = GetActorLocation() + FVector(0.f, 0.f, 60.f);
	const FVector Aim = Target->GetActorLocation() + FVector(0.f, 0.f, 40.f);
	if (FVector::Dist(Eye, Aim) > Range[D])
	{
		return;
	}
	// Vira para o jogador.
	FRotator Face = (Aim - Eye).Rotation();
	Face.Pitch = 0.f;
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), Face, DeltaSeconds, 8.f));

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < NextBotShotTime)
	{
		return;
	}
	NextBotShotTime = Now + Interval[D] * FMath::FRandRange(0.85f, 1.15f);

	// Só atira com linha de visão.
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ValdoriaBot), false, this);
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Eye, Aim, ECC_Visibility, Params) && Hit.GetActor() != Target;
	if (bBlocked)
	{
		return;
	}
	const bool bHit = FMath::FRand() < Accuracy[D];
	const FVector Impact = bHit ? Aim : Aim + FMath::VRand() * 120.f;
	PlayShotEffects(Impact, bHit);
	if (bHit)
	{
		DealDamageTo(Target, Damage[D]);
	}
}