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
	if (GetWorld()->GetTimeSeconds() - LastDamageTakenTime > ResetDelay && GetHealth() < GetMaxHealth())
	{
		AdminRestoreAll();
	}
	if (Kind == EValdoriaDummyKind::Strafing)
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
