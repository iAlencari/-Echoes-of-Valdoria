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
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"

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
	if (FParse::Param(FCommandLine::Get(), TEXT("ValdoriaOverview")))
	{
		// Print do mapa visto de cima (para conferir o layout) e fecha o jogo.
		FTimerHandle T1, T2, T3;
		GetWorldTimerManager().SetTimer(T1, FTimerDelegate::CreateLambda([this]()
		{
			APlayerController* PC = GetWorld()->GetFirstPlayerController();
			if (AValdoriaHeroCharacter* Hero = PC ? Cast<AValdoriaHeroCharacter>(PC->GetPawn()) : nullptr)
			{
				Hero->SetNoclip(true);
				Hero->SetActorLocation(FVector(-7500.f, -6500.f, 6500.f));
				PC->SetControlRotation(FRotator(-38.f, 38.f, 0.f));
			}
		}), 3.f, false);
		GetWorldTimerManager().SetTimer(T2, FTimerDelegate::CreateLambda([]()
		{
			FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots/Overview.png")), false, false);
		}), 6.f, false);
		GetWorldTimerManager().SetTimer(T3, FTimerDelegate::CreateLambda([]() { FPlatformMisc::RequestExit(false); }), 8.f, false);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("ValdoriaAutoTest")))
	{
		GetWorldTimerManager().SetTimer(AutoTestTimer, this, &AValdoriaGameMode::RunAutoTestStep, 0.15f, true, 4.f);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("ValdoriaInputTest")))
	{
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
		{
			RunInputTestStep();
			return InputTestStep < 1000;
		}), 0.25f);
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
	// Altura do chão no ponto de nascimento: os bonecos só ficam onde o chão tem a mesma altura
	// (antes um deles podia nascer em cima de um bloco do mapa, e a barra aparecia "no bloco").
	auto GroundZ = [this](const FVector& P, float& OutZ)
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ValdoriaDummyGround), false);
		if (GetWorld()->LineTraceSingleByChannel(Hit, P + FVector(0, 0, 400.f), P - FVector(0, 0, 3000.f), ECC_WorldStatic, Params))
		{
			OutZ = Hit.ImpactPoint.Z;
			return true;
		}
		return false;
	};
	float StartGround = Origin.GetLocation().Z - 90.f;
	GroundZ(Origin.GetLocation(), StartGround);
	for (const FDummySetup& S : Setups)
	{
		FVector Location = FVector::ZeroVector;
		bool bFound = false;
		for (float Dist = S.Distance; Dist >= 300.f && !bFound; Dist -= 150.f)
		{
			const FVector Candidate = Origin.GetLocation() + Forward * Dist + Right * S.Side;
			float Z = 0.f;
			if (GroundZ(Candidate, Z) && FMath::Abs(Z - StartGround) < 60.f)
			{
				Location = FVector(Candidate.X, Candidate.Y, Z + 100.f);
				bFound = true;
			}
		}
		if (!bFound)
		{
			continue;
		}
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


void AValdoriaGameMode::PressKey(const FKey& Key, bool bDown)
{
	// Passa pelo mesmo caminho do teclado de verdade (Slate -> viewport -> jogo).
	FKeyEvent Event(Key, FModifierKeysState(), 0, false, 0, 0);
	if (bDown)
	{
		FSlateApplication::Get().ProcessKeyDownEvent(Event);
	}
	else
	{
		FSlateApplication::Get().ProcessKeyUpEvent(Event);
	}
}

void AValdoriaGameMode::RunInputTestStep()
{
	++InputTestStep;
	if (InputTestStep < 16)
	{
		return; // ~4 s para o jogo assentar
	}
	AValdoriaPlayerController* PC = Cast<AValdoriaPlayerController>(GetWorld()->GetFirstPlayerController());
	AValdoriaHeroCharacter* Hero = PC ? Cast<AValdoriaHeroCharacter>(PC->GetPawn()) : nullptr;
	const int32 S = InputTestStep - 15;
	auto Shot = [](const TCHAR* Name)
	{
		FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots") / Name), true, false);
	};
	switch (S)
	{
	case 1: PressKey(EKeys::Enter, true); PressKey(EKeys::Enter, false); break;
	case 3:
		UE_LOG(LogValdoria, Display, TEXT("INPUTTEST enter: chat aberto=%d"), PC && PC->IsChatOpen());
		for (const TCHAR* C = TEXT("/adm"); *C; ++C)
		{
			FSlateApplication::Get().ProcessKeyCharEvent(FCharacterEvent(*C, FModifierKeysState(), 0, false));
		}
		break;
	case 4: Shot(TEXT("Input_Chat.png")); break;
	case 5: PressKey(EKeys::Enter, true); PressKey(EKeys::Enter, false); break;
	case 7:
		UE_LOG(LogValdoria, Display, TEXT("INPUTTEST /adm: modo admin=%d painel=%d chat aberto=%d"), PC && PC->IsAdminMode(), PC && PC->IsAdminPanelOpen(), PC && PC->IsChatOpen());
		Shot(TEXT("Input_Adm.png"));
		break;
	case 9: PressKey(EKeys::F10, true); PressKey(EKeys::F10, false); break;
	case 10: UE_LOG(LogValdoria, Display, TEXT("INPUTTEST F10: painel=%d"), PC && PC->IsAdminPanelOpen()); break;
	case 11: PressKey(EKeys::Escape, true); PressKey(EKeys::Escape, false); break;
	case 12: UE_LOG(LogValdoria, Display, TEXT("INPUTTEST esc: pausa=%d"), PC && PC->IsPauseMenuOpen()); break;
	case 13: PressKey(EKeys::Escape, true); PressKey(EKeys::Escape, false); break;
	case 14: UE_LOG(LogValdoria, Display, TEXT("INPUTTEST esc de novo: pausa=%d"), PC && PC->IsPauseMenuOpen()); break;
	case 15: PressKey(EKeys::K, true); break;
	case 16: UE_LOG(LogValdoria, Display, TEXT("INPUTTEST K segurado: ajuda=%d"), PC && PC->IsHelpOpen()); PressKey(EKeys::K, false); break;
	case 17: UE_LOG(LogValdoria, Display, TEXT("INPUTTEST K solto: ajuda=%d"), PC && PC->IsHelpOpen()); Shot(TEXT("Input_Arma.png")); break;
	case 18: PressKey(EKeys::V, true); PressKey(EKeys::V, false); break;
	case 19: UE_LOG(LogValdoria, Display, TEXT("INPUTTEST V: primeira pessoa=%d"), Hero && Hero->IsFirstPerson()); Shot(TEXT("Input_PrimeiraPessoa.png")); break;
	default: break;
	}
	// Morrer duas vezes e conferir se dá para andar depois de cada renascimento.
	if ((S == 20 || S == 60) && Hero)
	{
		Hero->bGodMode = false;
		Hero->DealDamageTo(Hero, 99999.f);
		UE_LOG(LogValdoria, Display, TEXT("INPUTTEST morte %d: morto=%d"), ++InputTestDeaths, Hero->IsDead());
	}
	if (S == 44 || S == 84)
	{
		if (Hero)
		{
			InputTestStartLoc = Hero->GetActorLocation();
			PressKey(EKeys::W, true);
		}
		UE_LOG(LogValdoria, Display, TEXT("INPUTTEST renasceu: heroi=%d morto=%d modo=%d"), Hero != nullptr, Hero && Hero->IsDead(),
			Hero ? (int32)Hero->GetCharacterMovement()->MovementMode.GetValue() : -1);
	}
	if (S == 50 || S == 90)
	{
		PressKey(EKeys::W, false);
		const float Moved = Hero ? FVector::Dist2D(Hero->GetActorLocation(), InputTestStartLoc) : -1.f;
		UE_LOG(LogValdoria, Display, TEXT("INPUTTEST andou depois de renascer: %.0f cm"), Moved);
		Shot(S == 50 ? TEXT("Input_Renasceu1.png") : TEXT("Input_Renasceu2.png"));
	}
	// Voar: segurar Espaço no ar gasta combustível e sobe.
	if (S == 92 && Hero)
	{
		InputTestStartLoc = Hero->GetActorLocation();
		PressKey(EKeys::SpaceBar, true);
	}
	if (S == 100 && Hero)
	{
		UE_LOG(LogValdoria, Display, TEXT("INPUTTEST voar: voando=%d subiu=%.0f cm combustivel=%.0f%%"), Hero->IsFlying(),
			Hero->GetActorLocation().Z - InputTestStartLoc.Z, Hero->GetFlightFuelPercent() * 100.f);
		Shot(TEXT("Input_Voando.png"));
	}
	if (S == 101)
	{
		PressKey(EKeys::SpaceBar, false);
	}
	if (S >= 112)
	{
		UE_LOG(LogValdoria, Display, TEXT("INPUTTEST fim"));
		InputTestStep = 100000;
		FPlatformMisc::RequestExit(false);
	}
}
