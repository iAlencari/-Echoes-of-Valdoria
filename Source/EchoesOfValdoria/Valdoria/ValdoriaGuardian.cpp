// Echoes of Valdoria - MOBA spots (guardian bosses).

#include "ValdoriaGuardian.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "ValdoriaAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "ValdoriaHeroDefinition.h"

AValdoriaGuardian::AValdoriaGuardian()
{
	bHunter = true;
	BotDifficulty = 0; // fights with the autopilot, not fake shots
}

void AValdoriaGuardian::BeginPlay()
{
	Super::BeginPlay();
	TeamId = GuardTeam;
	// Big and tough: Brasa's kit (hammer, slam), larger body, lots of health.
	SetHeroDefinition(UValdoriaHeroDefinition::CreatePreset(this, TEXT("brasa")));
	GetMesh()->SetRelativeScale3D(GetMesh()->GetRelativeScale3D() * 1.5f);
	if (HasAuthority())
	{
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetMaxHealthAttribute(), GuardianHealth);
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetHealthAttribute(), GuardianHealth);
	}
	bNoAutoHeal = true;
	SetAutopilot(true, 0.6f);
	SetLeash(GetActorLocation(), AreaRadius);
}

void AValdoriaGuardian::ScheduleRevive()
{
	if (RespawnSeconds > 0.f)
	{
		FTimerHandle Timer;
		GetWorldTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			Revive();
			AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetHealthAttribute(), GuardianHealth);
		}), RespawnSeconds, false);
	}
}

float AValdoriaGuardian::GetCurrentBuff() const
{
	const float T = GetWorld()->GetTimeSeconds();
	if (T <= BuffHoldUntil)
	{
		return BuffValue;
	}
	const float Fade = FMath::Clamp((T - BuffHoldUntil) / FMath::Max(1.f, BuffFadeEnd - BuffHoldUntil), 0.f, 1.f);
	return BuffValue * (1.f - Fade);
}

void AValdoriaGuardian::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	BuffTimer += DeltaSeconds;
	if (!HasAuthority() || IsDead() || BuffTimer < 0.5f)
	{
		return;
	}
	BuffTimer = 0.f;
	// The buff lasts 1 s on each ally and is refreshed while the guardian lives.
	const float Value = GetCurrentBuff();
	for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
	{
		if (*It != this && It->TeamId == GuardTeam && !It->IsDead() && !It->IsA<AValdoriaGuardian>())
		{
			It->ApplyTeamBuff(static_cast<uint8>(Buff), Value, 1.f);
		}
	}
}

void AValdoriaGuardian::SetupSpot(uint8 Team, float Radius)
{
	GuardTeam = Team;
	TeamId = Team;
	AreaRadius = Radius;
	StartLocation = GetActorLocation();
	SetLeash(GetActorLocation(), AreaRadius);
}