// Echoes of Valdoria - modo de jogo de treino.

#include "ValdoriaGameMode.h"
#include "EngineUtils.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Containers/Ticker.h"
#include "UnrealClient.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"
#include "ValdoriaHUD.h"
#include "ValdoriaHeroCharacter.h"
#include "ValdoriaHeroDefinition.h"
#include "ValdoriaPlayerController.h"
#include "ValdoriaTrainingDummy.h"

AValdoriaGameMode::AValdoriaGameMode()
{
	DefaultPawnClass = AValdoriaHeroCharacter::StaticClass();
	PlayerControllerClass = AValdoriaPlayerController::StaticClass();
	HUDClass = AValdoriaHUD::StaticClass();
}

AActor* AValdoriaGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// Time 1 nasce nos spawns sem etiqueta; "Time2" fica para o outro time (multiplayer).
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		if (It->PlayerStartTag.IsNone())
		{
			return *It;
		}
	}
	return Super::ChoosePlayerStart_Implementation(Player);
}

APawn* AValdoriaGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	APawn* Pawn = Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
	if (AValdoriaHeroCharacter* Hero = Cast<AValdoriaHeroCharacter>(Pawn))
	{
		if (DefaultHero)
		{
			Hero->SetHeroDefinition(DefaultHero);
		}
	}
	return Pawn;
}

void AValdoriaGameMode::StartPlay()
{
	Super::StartPlay();
	if (bSpawnTrainingDummies)
	{
		SpawnTrainingDummies();
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("ValdoriaAutoTest")))
	{
		GetWorldTimerManager().SetTimer(AutoTestTimer, this, &AValdoriaGameMode::RunAutoTestStep, 0.15f, true, 4.f);
	}
}

void AValdoriaGameMode::RunAutoTestStep()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	AValdoriaHeroCharacter* Hero = PC ? Cast<AValdoriaHeroCharacter>(PC->GetPawn()) : nullptr;
	AValdoriaTrainingDummy* Target = nullptr;
	for (TActorIterator<AValdoriaTrainingDummy> It(GetWorld()); It; ++It)
	{
		if (It->Kind == EValdoriaDummyKind::Basic)
		{
			Target = *It;
		}
	}
	if (!Hero || !Target)
	{
		UE_LOG(LogValdoria, Error, TEXT("AUTOTEST falhou: heroi=%d boneco=%d"), Hero != nullptr, Target != nullptr);
		FPlatformMisc::RequestExit(false);
		return;
	}
	++AutoTestStep;
	// Mira no peito do boneco básico.
	const FVector Aim = Target->GetActorLocation() + FVector(0, 0, 30.f);
	FVector CamStart, Dir;
	Hero->GetAimRay(CamStart, Dir);
	// Mira a partir da câmera (como o jogador faz com a mira no centro da tela).
	for (int32 i = 0; i < 2; ++i)
	{
		PC->SetControlRotation((Aim - Hero->GetFollowCamera()->GetComponentLocation()).Rotation());
		Hero->GetCameraBoom()->UpdateComponentToWorld();
		Hero->GetCameraBoom()->TickComponent(0.f, LEVELTICK_All, nullptr);
	}

	if (AutoTestStep <= 12)
	{
		Hero->TryActivateSlot(EValdoriaAbilitySlot::PrimaryFire);
	}
	else if (AutoTestStep == 13)
	{
		FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots/AutoTest_HUD.png")), true, false);
	}
	else if (AutoTestStep == 14)
	{
		UE_LOG(LogValdoria, Display, TEXT("AUTOTEST tiro: boneco vida=%.0f/%.0f municao=%d"), Target->GetHealth(), Target->GetMaxHealth(), Hero->GetAmmo());
		Hero->SetSprinting(true);
		UE_LOG(LogValdoria, Display, TEXT("AUTOTEST correr: velocidade=%.0f"), Hero->GetCharacterMovement()->MaxWalkSpeed);
		Hero->SetSprinting(false);
		// impulso testado depois do deitar (precisa estar no chão)
	}
	else if (AutoTestStep == 17)
	{
		Hero->SetProne(true);
	}
	else if (AutoTestStep == 18)
	{
		UE_LOG(LogValdoria, Display, TEXT("AUTOTEST deitar: deitado=%d agachado=%d altura=%.0f"), Hero->IsProne(), Hero->bIsCrouched,
			Hero->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
		Hero->SetProne(false);
		Hero->TryDash();
		UE_LOG(LogValdoria, Display, TEXT("AUTOTEST impulso: cargas restantes=%d"), Hero->GetDashCharges());
	}
	else if (AutoTestStep == 19)
	{
		Hero->AdminFillUltimate();
	}
	else if (AutoTestStep == 20)
	{
		Hero->TryActivateSlot(EValdoriaAbilitySlot::Ultimate);
	}
	else if (AutoTestStep == 21)
	{
		UE_LOG(LogValdoria, Display, TEXT("AUTOTEST ultimate: carga=%.0f%% boneco vida=%.0f"), Hero->GetUltimatePercent() * 100.f, Target->GetHealth());
		PC->SetControlRotation((Target->GetActorLocation() - Hero->GetActorLocation()).Rotation() + FRotator(-8.f, 0.f, 0.f));
	}
	else if (AutoTestStep == 22)
	{
		Hero->Crouch();
	}
	else if (AutoTestStep == 24)
	{
		UE_LOG(LogValdoria, Display, TEXT("AUTOTEST agachar: agachado=%d animacao=%s"), Hero->bIsCrouched,
			*GetNameSafe(Hero->CurrentMovementAnim.Get()));
		const FString CrouchPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots/AutoTest_Agachado.png"));
		FScreenshotRequest::RequestScreenshot(CrouchPath, true, false);
	}
	else if (AutoTestStep == 25)
	{
		Hero->UnCrouch();
	}
	else if (AutoTestStep == 27)
	{
		Hero->SetProne(true);
	}
	else if (AutoTestStep == 30)
	{
		UE_LOG(LogValdoria, Display, TEXT("AUTOTEST deitado: animacao=%s"), *GetNameSafe(Hero->CurrentMovementAnim.Get()));
		FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots/AutoTest_Deitado.png")), true, false);
	}
	else if (AutoTestStep == 31)
	{
		Hero->SetProne(false);
	}
	else if (AutoTestStep == 33)
	{
		// Mostra a animação de escalar (sem precisar de uma parede no mapa).
		if (const UValdoriaHeroDefinition* Def = Hero->GetHeroDefinition())
		{
			if (UAnimSequenceBase* Climb = Def->Animations.WallClimb.LoadSynchronous())
			{
				Hero->GetMesh()->GetAnimInstance()->PlaySlotAnimationAsDynamicMontage(Climb, Def->Animations.SlotName, 0.1f, 0.1f, 1.f, 5);
				UE_LOG(LogValdoria, Display, TEXT("AUTOTEST escalar: animacao=%s"), *Climb->GetName());
			}
		}
	}
	else if (AutoTestStep == 36)
	{
		FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots/AutoTest_Escalando.png")), true, false);
	}
	else if (AutoTestStep == 38)
	{
		if (AValdoriaPlayerController* VPC = Cast<AValdoriaPlayerController>(PC))
		{
			VPC->DebugSetAdminMode(true);
			VPC->ToggleAdminPanel();
			UE_LOG(LogValdoria, Display, TEXT("AUTOTEST admin: painel aberto=%d"), VPC->IsAdminPanelOpen());
		}
	}
	else if (AutoTestStep == 40)
	{
		FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots/AutoTest_Admin.png")), true, false);
	}
	else if (AutoTestStep == 41)
	{
		if (AValdoriaPlayerController* VPC = Cast<AValdoriaPlayerController>(PC))
		{
			VPC->ToggleAdminPanel();
			VPC->HelpPressed();
			UE_LOG(LogValdoria, Display, TEXT("AUTOTEST ajuda: tela aberta=%d"), VPC->IsHelpOpen());
		}
	}
	else if (AutoTestStep == 43)
	{
		FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots/AutoTest_Ajuda.png")), true, false);
	}
	else if (AutoTestStep == 44)
	{
		if (AValdoriaPlayerController* VPC = Cast<AValdoriaPlayerController>(PC))
		{
			VPC->HelpReleased();
		}
	}
	else if (AutoTestStep == 46)
	{
		const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots/AutoTest.png"));
		// Abre o menu de pausa (Esc) para conferir que ele aparece na captura.
		if (AValdoriaPlayerController* VPC = Cast<AValdoriaPlayerController>(PC))
		{
			VPC->TogglePauseMenu();
			UE_LOG(LogValdoria, Display, TEXT("AUTOTEST pausa: menu aberto=%d jogo pausado=%d"), VPC->IsPauseMenuOpen(), VPC->IsPaused());
			VPC->DebugOpenSettings();
		}
		FScreenshotRequest::RequestScreenshot(Path, true, false);
		UE_LOG(LogValdoria, Display, TEXT("AUTOTEST captura: %s"), *Path);
		// Com o jogo pausado o timer para; quem fecha o jogo é o ticker do motor.
		GetWorldTimerManager().ClearTimer(AutoTestTimer);
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
		{
			UE_LOG(LogValdoria, Display, TEXT("AUTOTEST fim"));
			FPlatformMisc::RequestExit(false);
			return false;
		}), 3.f);
	}
	else if (AutoTestStep >= 60)
	{
		UE_LOG(LogValdoria, Display, TEXT("AUTOTEST fim"));
		GetWorldTimerManager().ClearTimer(AutoTestTimer);
		FPlatformMisc::RequestExit(false);
	}
}

void AValdoriaGameMode::SpawnTrainingDummies()
{
	FTransform Origin = FTransform::Identity;
	if (AActor* Start = ChoosePlayerStart_Implementation(nullptr))
	{
		Origin = Start->GetActorTransform();
	}
	const FVector Forward = Origin.GetRotation().GetForwardVector().GetSafeNormal2D();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
	const FRotator Facing = (-Forward).Rotation();

	struct FDummySetup { float Distance; float Side; EValdoriaDummyKind Kind; };
	const FDummySetup Setups[] = {
		{ 900.f, -350.f, EValdoriaDummyKind::Basic },
		{ 900.f, 0.f, EValdoriaDummyKind::Armored },
		{ 900.f, 350.f, EValdoriaDummyKind::Shielded },
		{ 1800.f, 0.f, EValdoriaDummyKind::Strafing },
	};
	for (const FDummySetup& S : Setups)
	{
		const FVector Location = Origin.GetLocation() + Forward * S.Distance + Right * S.Side + FVector(0, 0, 20.f);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		if (AValdoriaTrainingDummy* Dummy = GetWorld()->SpawnActorDeferred<AValdoriaTrainingDummy>(
			AValdoriaTrainingDummy::StaticClass(), FTransform(Facing, Location), nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
		{
			Dummy->Kind = S.Kind;
			Dummy->FinishSpawning(FTransform(Facing, Location));
		}
	}
}

AActor* AValdoriaGameMode::SpawnDummyAt(const FVector& Location, const FRotator& Facing, uint8 Kind)
{
	AValdoriaTrainingDummy* Dummy = GetWorld()->SpawnActorDeferred<AValdoriaTrainingDummy>(
		AValdoriaTrainingDummy::StaticClass(), FTransform(Facing, Location), nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (Dummy)
	{
		Dummy->Kind = static_cast<EValdoriaDummyKind>(FMath::Clamp<int32>(Kind, 0, 3));
		Dummy->FinishSpawning(FTransform(Facing, Location));
	}
	return Dummy;
}

void AValdoriaGameMode::OnHeroKilled(AValdoriaHeroCharacter* Victim, AValdoriaHeroCharacter* Killer)
{
	if (!Victim)
	{
		return;
	}
	if (AValdoriaTrainingDummy* Dummy = Cast<AValdoriaTrainingDummy>(Victim))
	{
		Dummy->ScheduleRevive();
		return;
	}
	if (AController* Controller = Victim->GetController())
	{
		FTimerHandle Handle;
		FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &AValdoriaGameMode::RespawnPlayer, Controller);
		GetWorldTimerManager().SetTimer(Handle, Delegate, PlayerRespawnDelay, false);
	}
}

void AValdoriaGameMode::RespawnPlayer(AController* Controller)
{
	if (!Controller)
	{
		return;
	}
	UValdoriaHeroDefinition* Keep = nullptr;
	if (AValdoriaHeroCharacter* Old = Cast<AValdoriaHeroCharacter>(Controller->GetPawn()))
	{
		Keep = Old->HeroDefinition;
		Controller->UnPossess();
		Old->Destroy();
	}
	RestartPlayer(Controller);
	if (Keep)
	{
		if (AValdoriaHeroCharacter* NewHero = Cast<AValdoriaHeroCharacter>(Controller->GetPawn()))
		{
			NewHero->SetHeroDefinition(Keep);
		}
	}
}
