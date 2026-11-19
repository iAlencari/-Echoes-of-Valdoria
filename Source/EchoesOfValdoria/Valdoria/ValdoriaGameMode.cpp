// Echoes of Valdoria - modo de jogo de treino.

#include "ValdoriaGameMode.h"
#include "ValdoriaMultipliers.h"
#include "EngineUtils.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMeshActor.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Containers/Ticker.h"
#include "UnrealClient.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "ValdoriaHUD.h"
#include "ValdoriaHeroCharacter.h"
#include "ValdoriaWorldActors.h"
#include "ValdoriaGameplayAbility.h"
#include "ValdoriaHeroDefinition.h"
#include "ValdoriaPlayerController.h"
#include "ValdoriaTrainingDummy.h"
#include "ValdoriaGuardian.h"
#include "ValdoriaSettings.h"
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
	if (FParse::Param(FCommandLine::Get(), TEXT("ValdoriaFlightTest")))
	{
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
		{
			RunFlightTestStep();
			return FlightTestStep < 100000;
		}), 0.25f);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("ValdoriaLoadingTest")))
	{
		// Print da tela de carregamento ao abrir; depois troca para a NP (carregamento entre mapas) e tira outro.
		const bool bOnNP = GetWorld()->GetMapName().Contains(TEXT("NP_"));
		FTimerHandle T1, T2;
		GetWorldTimerManager().SetTimer(T1, FTimerDelegate::CreateWeakLambda(this, [bOnNP]()
		{
			FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / (bOnNP ? TEXT("Screenshots/Carregando_NP.png") : TEXT("Screenshots/Carregando_Treino.png"))), true, false);
		}), 0.6f, false);
		GetWorldTimerManager().SetTimer(T2, FTimerDelegate::CreateWeakLambda(this, [this, bOnNP]()
		{
			if (bOnNP)
			{
				FPlatformMisc::RequestExit(false);
			}
			else
			{
				UGameplayStatics::OpenLevel(this, TEXT("/Game/Maps/NP_NovaPauliceia"), true, TEXT("ValdoriaLoadingTest"));
			}
		}), 3.5f, false);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("ValdoriaLineup")))
	{
		// Os 5 heróis lado a lado para comparar corpo, tamanho e flutuação.
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
		{
			static int32 Step = 0;
			++Step;
			AValdoriaPlayerController* PC = Cast<AValdoriaPlayerController>(GetWorld()->GetFirstPlayerController());
			AValdoriaHeroCharacter* Hero = PC ? Cast<AValdoriaHeroCharacter>(PC->GetPawn()) : nullptr;
			if (Step == 16 && Hero)
			{
				static const TCHAR* Ids[] = { TEXT("brasa"), TEXT("kael"), TEXT("iara"), TEXT("vex"), TEXT("lume") };
				const FVector Fwd = Hero->GetActorForwardVector().GetSafeNormal2D();
				const FVector Right = FVector::CrossProduct(FVector::UpVector, Fwd);
				for (int32 i = 0; i < 5; ++i)
				{
					FActorSpawnParameters P;
					P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
					const FVector Where = Hero->GetActorLocation() + Fwd * 520.f + Right * (i - 2) * 150.f;
					if (AValdoriaTrainingDummy* D = GetWorld()->SpawnActor<AValdoriaTrainingDummy>(AValdoriaTrainingDummy::StaticClass(), Where, (-Fwd).Rotation(), P))
					{
						D->TeamId = Hero->TeamId;
						D->SetHeroDefinition(PC->FindHero(Ids[i]));
					}
				}
				PC->SetControlRotation(FRotator(-8.f, Hero->GetActorRotation().Yaw, 0.f));
			}
			if (Step == 24)
			{
				FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots/Herois_Lado_a_Lado.png")), false, false);
			}
			if (Step == 27)
			{
				FPlatformMisc::RequestExit(false);
				return false;
			}
			return true;
		}), 0.25f);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("ValdoriaPoseGallery")))
	{
		// Turn 44: candidate hover/air poses side by side (Lume's body, seen in profile), to pick by eye.
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
		{
			static int32 Step = 0;
			++Step;
			AValdoriaPlayerController* PC = Cast<AValdoriaPlayerController>(GetWorld()->GetFirstPlayerController());
			AValdoriaHeroCharacter* Hero = PC ? Cast<AValdoriaHeroCharacter>(PC->GetPawn()) : nullptr;
			static const TCHAR* Clips[] = {
				TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"),                                       // 1 current hover (reference)
				TEXT("/Game/StruggleToAscension/Animations/UE5/AS_STA_idle_Ascension.AS_STA_idle_Ascension"),            // 2
				TEXT("/Game/StruggleToAscension/Animations/UE5/AS_STA_Air_idle06.AS_STA_Air_idle06"),                    // 3
				TEXT("/Game/StruggleToAscension/Animations/UE5/AS_STA_Air_idle07.AS_STA_Air_idle07"),                    // 4
				TEXT("/Game/StruggleToAscension/Animations/UE5/AS_STA_Ground_idle05_to_idle06_AirLoop.AS_STA_Ground_idle05_to_idle06_AirLoop"), // 5
				TEXT("/Game/StruggleToAscension/Animations/UE5/AS_STA_idle_Struggle.AS_STA_idle_Struggle"),              // 6
				TEXT("/Game/FreeAnimationSet/Animations/Manny/Diving_Idle_Anim.Diving_Idle_Anim"),                       // 7
				TEXT("/Game/FreeAnimationsPack2/Animations/AS_LevitatingFireballCharge2.AS_LevitatingFireballCharge2"),  // 8
			};
			constexpr int32 Num = UE_ARRAY_COUNT(Clips);
			if (Step == 16 && Hero)
			{
				const FVector Fwd = Hero->GetActorForwardVector().GetSafeNormal2D();
				const FVector Right = FVector::CrossProduct(FVector::UpVector, Fwd);
				for (int32 i = 0; i < Num; ++i)
				{
					FActorSpawnParameters P;
					P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
					const FVector Where = Hero->GetActorLocation() + Fwd * 900.f + Right * (i - (Num - 1) * 0.5f) * 140.f;
					if (AValdoriaTrainingDummy* D = GetWorld()->SpawnActor<AValdoriaTrainingDummy>(AValdoriaTrainingDummy::StaticClass(), Where, Right.Rotation(), P))
					{
						D->TeamId = Hero->TeamId;
						D->SetHeroDefinition(PC->FindHero(TEXT("lume")));
						D->DebugBaseAnim = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Clips[i]));
						D->bGodMode = true;
						UE_LOG(LogValdoria, Display, TEXT("POSEGALLERY %d (da esquerda) = %s"), i + 1, Clips[i]);
					}
				}
				PC->SetControlRotation(FRotator(-6.f, Hero->GetActorRotation().Yaw, 0.f));
				Hero->SetActorHiddenInGame(true);
			}
			auto Shot = [](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots") / Name), false, false); };
			if (Step == 24) { Shot(TEXT("Poses_A.png")); }
			if (Step == 30) { Shot(TEXT("Poses_B.png")); }
			if (Step == 36) { Shot(TEXT("Poses_C.png")); }
			if (Step == 40)
			{
				FPlatformMisc::RequestExit(false);
				return false;
			}
			return true;
		}), 0.25f);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("ValdoriaWeaponPose")))
	{
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
		{
			RunWeaponPoseStep();
			return WeaponPoseStep < 100000;
		}), 0.3f);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("ValdoriaHeroTest")))
	{
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
		{
			RunHeroTestStep();
			return HeroTestStep < 100000;
		}), 0.25f);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("ValdoriaBeamTest")))
	{
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
		{
			RunBeamTestStep();
			return BeamTestStep < 100000;
		}), 0.25f);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("ValdoriaAdminTest")))
	{
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
		{
			RunAdminTestStep();
			return AdminTestStep < 100000;
		}), 0.4f);
	}
	if (FParse::Value(FCommandLine::Get(), TEXT("ValdoriaPlaytest="), PlaytestSeconds) && PlaytestSeconds > 0)
	{
		FParse::Value(FCommandLine::Get(), TEXT("ValdoriaHero="), PlaytestHero);
		PlaytestTag = FString::Printf(TEXT("%s_%s_%s"), *UGameplayStatics::GetCurrentLevelName(this),
			*GetClass()->GetName().Replace(TEXT("Valdoria"), TEXT("")).Replace(TEXT("Mode"), TEXT("")),
			PlaytestHero.IsEmpty() ? TEXT("recruta") : *PlaytestHero);
		UE_LOG(LogValdoria, Display, TEXT("PLAYTEST inicio: %s (%d s)"), *PlaytestTag, PlaytestSeconds);
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
		{
			RunPlaytestStep();
			return PlaytestStep < 100000;
		}), 1.f);
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
		// Start just above the spawn height (was +400): in the Nova Pauliceia metro +400 is above the station
		// ceiling, so the dummies were placed on the street on top of the metro, out of sight (turn 44 playtest).
		// Only static geometry (a channel trace also hit the player's own capsule standing on the spawn, so the
		// reference height was wrong and every dummy was rejected: none on Nova Pauliceia, 1 on the default map).
		if (GetWorld()->LineTraceSingleByObjectType(Hit, P + FVector(0, 0, 150.f), P - FVector(0, 0, 3000.f), FCollisionObjectQueryParams(ECC_WorldStatic), Params))
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
	if (PlaytestSeconds > 0)
	{
		auto Name = [](const AValdoriaHeroCharacter* H)
		{
			const FString Def = H && H->GetHeroDefinition() ? H->GetHeroDefinition()->GetName().Replace(TEXT("DA_"), TEXT("")) : TEXT("?");
			return (H && H->IsPlayerControlled() ? TEXT("EU:") : TEXT("bot:")) + Def;
		};
		PlaytestDeaths.FindOrAdd(Name(Victim))++;
		if (Killer && Killer != Victim)
		{
			PlaytestKills.FindOrAdd(Name(Killer))++;
		}
		UE_LOG(LogValdoria, Display, TEXT("PLAYTEST abate: %s matou %s"), *Name(Killer), *Name(Victim));
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
		const float Delay = Victim->RespawnOverride >= 0.f ? Victim->RespawnOverride : GetRespawnDelayFor(Victim) * ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::RespawnTime);
		Victim->RespawnOverride = -1.f;
		Victim->RespawnAt = GetWorld()->GetTimeSeconds() + Delay;
		GetWorldTimerManager().SetTimer(Handle, Delegate, Delay, false);
	}
}

void AValdoriaGameMode::RespawnPlayer(AController* Controller)
{
	if (!Controller)
	{
		return;
	}
	UValdoriaHeroDefinition* Keep = nullptr;
	AValdoriaHeroCharacter::FProgress Progress; // turn 46: level, ranks and scoreboard survive the respawn
	if (AValdoriaHeroCharacter* Old = Cast<AValdoriaHeroCharacter>(Controller->GetPawn()))
	{
		Keep = Old->HeroDefinition;
		Progress = Old->TakeProgress();
		Controller->UnPossess();
		Old->Destroy();
	}
	RestartPlayer(Controller);
	if (AValdoriaHeroCharacter* NewHero = Cast<AValdoriaHeroCharacter>(Controller->GetPawn()))
	{
		if (Keep)
		{
			NewHero->SetHeroDefinition(Keep);
		}
		if (Progress.bValid)
		{
			NewHero->ApplyProgress(Progress);
		}
		FVector Spot;
		if (bSpreadRespawns && FindSpreadSpawnPoint(Spot))
		{
			NewHero->TeleportTo(Spot, NewHero->GetActorRotation());
		}
	}
}

bool AValdoriaGameMode::FindSpreadSpawnPoint(FVector& OutLocation, float MinDistance)
{
	UWorld* World = GetWorld();
	auto TopHit = [World](float X, float Y, float TopZ, FHitResult& Hit)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ValdoriaSpread), false);
		return World->LineTraceSingleByObjectType(Hit, FVector(X, Y, TopZ), FVector(X, Y, TopZ - 20000.f), FCollisionObjectQueryParams(ECC_WorldStatic), Params);
	};
	if (!bSpreadReady)
	{
		bSpreadReady = true;
		for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
		{
			const UStaticMeshComponent* C = It->GetStaticMeshComponent();
			if (C && C->GetCollisionEnabled() != ECollisionEnabled::NoCollision && C->Bounds.GetBox().GetExtent().GetMax() < 100000.f)
			{
				SpreadBounds += C->Bounds.GetBox();
			}
		}
		if (!SpreadBounds.IsValid)
		{
			return false;
		}
		// Street level = the lowest open (seen from the sky) floor found by sampling the map.
		SpreadFloorZ = TNumericLimits<float>::Max();
		for (int32 i = 0; i < 96; ++i)
		{
			FHitResult Hit;
			const float X = FMath::FRandRange(SpreadBounds.Min.X, SpreadBounds.Max.X);
			const float Y = FMath::FRandRange(SpreadBounds.Min.Y, SpreadBounds.Max.Y);
			if (TopHit(X, Y, SpreadBounds.Max.Z + 500.f, Hit) && Hit.ImpactNormal.Z > 0.85f)
			{
				SpreadFloorZ = FMath::Min(SpreadFloorZ, Hit.ImpactPoint.Z);
			}
		}
	}
	if (!SpreadBounds.IsValid || SpreadFloorZ == TNumericLimits<float>::Max())
	{
		return false;
	}
	const FBox Inner = SpreadBounds.ExpandBy(-SpreadBounds.GetExtent() * FVector(0.1f, 0.1f, 0.f));
	for (int32 Try = 0; Try < 60; ++Try)
	{
		FHitResult Hit;
		const float X = FMath::FRandRange(Inner.Min.X, Inner.Max.X);
		const float Y = FMath::FRandRange(Inner.Min.Y, Inner.Max.Y);
		if (!TopHit(X, Y, SpreadBounds.Max.Z + 500.f, Hit) || Hit.ImpactNormal.Z < 0.85f || Hit.ImpactPoint.Z > SpreadFloorZ + 400.f)
		{
			continue; // roofs, walls and holes are not spawn spots
		}
		const FVector Spot = Hit.ImpactPoint + FVector(0.f, 0.f, 110.f);
		bool bCrowded = false;
		for (TActorIterator<AValdoriaHeroCharacter> It(World); It && !bCrowded; ++It)
		{
			bCrowded = !It->IsDead() && !It->IsA<AValdoriaGuardian>() && FVector::Dist(It->GetActorLocation(), Spot) < MinDistance;
		}
		// Room for a standing hero (nothing blocking a capsule there).
		bCrowded = bCrowded || World->OverlapAnyTestByChannel(Spot, FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeCapsule(40.f, 90.f));
		if (!bCrowded)
		{
			OutLocation = Spot;
			return true;
		}
	}
	return false;
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

void AValdoriaGameMode::RunBeamTestStep()
{
	// -ValdoriaBeamTest: Lume's draining beam with and without a target (visual, life steal, overheal),
	// then the Recruit's aim glide (falls slower while aiming in the air).
	++BeamTestStep;
	AValdoriaPlayerController* PC = Cast<AValdoriaPlayerController>(GetWorld()->GetFirstPlayerController());
	AValdoriaHeroCharacter* Hero = PC ? Cast<AValdoriaHeroCharacter>(PC->GetPawn()) : nullptr;
	if (!Hero)
	{
		return;
	}
	AValdoriaTrainingDummy* Dummy = nullptr;
	for (TActorIterator<AValdoriaTrainingDummy> It(GetWorld()); It; ++It) { Dummy = *It; break; }
	auto Shot = [](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots") / Name), true, false); };
	switch (BeamTestStep)
	{
	case 12: PC->SelectHero(TEXT("lume")); break;
	case 14: PC->SetControlRotation(FRotator(0.f, Hero->GetActorRotation().Yaw + 150.f, 0.f)); PressKey(EKeys::LeftMouseButton, true); break; // no target
	case 18:
	{
		int32 Fx = 0;
		for (TActorIterator<AValdoriaFx> It(GetWorld()); It; ++It) { ++Fx; }
		const UValdoriaHeroDefinition* D = Hero->GetHeroDefinition();
		UE_LOG(LogValdoria, Display, TEXT("BEAMTEST sem alvo: efeitos=%d primario=%s municao=%d"), Fx,
			D && D->Abilities.Num() > 0 ? *D->Abilities[0].AbilityClass.Get()->GetName() : TEXT("-"), Hero->GetAmmo());
		Shot(TEXT("Beam_SemAlvo.png"));
		break;
	}
	case 19: PressKey(EKeys::LeftMouseButton, false); break;
	case 20:
		if (Dummy)
		{
			Hero->TeleportTo(Dummy->GetActorLocation() - Dummy->GetActorForwardVector() * -700.f, Hero->GetActorRotation());
			PC->SetControlRotation((Dummy->GetActorLocation() - Hero->GetActorLocation()).Rotation());
			Dummy->bGodMode = false;
			Dummy->bNoAutoHeal = true;
		}
		break;
	case 21: if (Dummy) { PC->SetControlRotation((Dummy->GetActorLocation() - Hero->GetActorLocation()).Rotation()); } PressKey(EKeys::LeftMouseButton, true); break;
	case 25: Shot(TEXT("Beam_Alvo.png")); break;
	case 34:
		PressKey(EKeys::LeftMouseButton, false);
		UE_LOG(LogValdoria, Display, TEXT("BEAMTEST lume vida=%.0f/%.0f sobrecura=%.1f alvo vida=%.0f"), Hero->GetHealth(), Hero->GetMaxHealth(), Hero->Overheal, Dummy ? Dummy->GetHealth() : -1.f);
		break;
	case 36: PC->SelectHero(TEXT("recruta")); break;
	case 38: Hero->TeleportTo(Hero->GetActorLocation() + FVector(0, 0, 2500.f), Hero->GetActorRotation()); break;
	case 41: UE_LOG(LogValdoria, Display, TEXT("BEAMTEST queda normal vz=%.0f"), Hero->GetVelocity().Z); PressKey(EKeys::RightMouseButton, true); break;
	case 43: UE_LOG(LogValdoria, Display, TEXT("BEAMTEST mirando no ar vz=%.0f"), Hero->GetVelocity().Z); Shot(TEXT("Beam_PlanarMira.png")); break;
	case 44: PressKey(EKeys::RightMouseButton, false); break;
	// Turn 44: Lume jumps, and holding Space in the air makes her sink very slowly (GlideFallSpeed).
	case 50: PC->SelectHero(TEXT("lume")); break;
	case 53:
		UE_LOG(LogValdoria, Display, TEXT("LUMETEST no chao: z=%.0f andando=%d"), Hero->GetActorLocation().Z, Hero->GetCharacterMovement()->IsMovingOnGround());
		LumeJumpStart = Hero->GetActorLocation();
		PressKey(EKeys::SpaceBar, true);
		PressKey(EKeys::W, true); // jump forward to measure the distance
		break;
	case 54: UE_LOG(LogValdoria, Display, TEXT("LUMETEST pulou: vz=%.0f no ar=%d"), Hero->GetVelocity().Z, Hero->GetCharacterMovement()->IsFalling()); break;
	case 58: case 62: case 66: case 70:
		UE_LOG(LogValdoria, Display, TEXT("LUMETEST segurando Espaco: vz=%.0f z=%.0f planando=%d anim=%s"), Hero->GetVelocity().Z, Hero->GetActorLocation().Z,
			Hero->IsGliding(), *GetNameSafe(Hero->CurrentMovementAnim.Get()));
		if (BeamTestStep == 66) { Shot(TEXT("Lume_Planando.png")); }
		break;
	case 72:
		UE_LOG(LogValdoria, Display, TEXT("LUMETEST distancia do salto (indo para a frente, segurando Espaco): %.0f cm"), FVector::Dist2D(LumeJumpStart, Hero->GetActorLocation()));
		PressKey(EKeys::SpaceBar, false);
		PressKey(EKeys::W, false);
		UE_LOG(LogValdoria, Display, TEXT("BEAMTEST fim"));
		BeamTestStep = 100000;
		FPlatformMisc::RequestExit(false);
		break;
	default: break;
	}
}

void AValdoriaGameMode::RunPlaytestStep()
{
	++PlaytestStep;
	AValdoriaPlayerController* PC = Cast<AValdoriaPlayerController>(GetWorld()->GetFirstPlayerController());
	AValdoriaHeroCharacter* Me = PC ? Cast<AValdoriaHeroCharacter>(PC->GetPawn()) : nullptr;
	auto DefName = [](const AValdoriaHeroCharacter* H)
	{
		return H && H->GetHeroDefinition() ? H->GetHeroDefinition()->GetName().Replace(TEXT("DA_"), TEXT("")) : FString(TEXT("?"));
	};
	// Pick the hero (again after a respawn if needed) and keep it on autopilot: Claude "plays" through it.
	if (Me && PlaytestStep >= 3)
	{
		if (!PlaytestHero.IsEmpty() && PlaytestHero != TEXT("recruta") && !DefName(Me).Equals(PlaytestHero, ESearchCase::IgnoreCase))
		{
			PC->SelectHero(FName(*PlaytestHero));
		}
		if (!Me->IsAutopilot() && !Me->IsDead())
		{
			Me->SetAutopilot(true, 0.7f);
		}
	}
	// Once: is there a navmesh, and can the player reach a random spot? (autopilot pathfinding, turn 44)
	if (PlaytestStep == 6 && Me)
	{
		UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		FNavLocation Spot;
		const bool bData = Nav && Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate) != nullptr;
		const bool bRandom = bData && Nav->GetRandomReachablePointInRadius(Me->GetActorLocation(), 5000.f, Spot);
		UNavigationPath* Path = bRandom ? UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), Me->GetActorLocation(), Spot.Location, Me) : nullptr;
		UE_LOG(LogValdoria, Display, TEXT("PLAYTEST navmesh: dados=%d ponto=%d caminho=%d pontos=%d"), bData, bRandom,
			Path && Path->IsValid(), Path ? Path->PathPoints.Num() : 0);
	}
	// Every 5 s: state of every hero (guardians summarised), stuck and fallen checks.
	if (PlaytestStep % 5 == 0)
	{
		int32 GuardAlive = 0, GuardTotal = 0, Minions[2] = { 0, 0 };
		float GuardHealth = 0.f, GuardMax = 0.f, GuardLowest = 1.f;
		FString Line;
		for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
		{
			AValdoriaHeroCharacter* H = *It;
			if (H->IsA<AValdoriaGuardian>())
			{
				++GuardTotal;
				GuardAlive += H->IsDead() ? 0 : 1;
				GuardHealth += H->GetHealth();
				GuardMax += H->GetMaxHealth();
				GuardLowest = FMath::Min(GuardLowest, H->GetHealth() / FMath::Max(1.f, H->GetMaxHealth()));
				continue;
			}
			if (const AValdoriaTrainingDummy* Minion = Cast<AValdoriaTrainingDummy>(H); Minion && Minion->bMinion)
			{
				Minions[FMath::Clamp<int32>(H->TeamId, 0, 1)] += H->IsDead() ? 0 : 1; // counted, not listed (turn 45)
				continue;
			}
			const FString Who = (H->IsPlayerControlled() ? TEXT("EU:") : TEXT("bot:")) + DefName(H) + FString::Printf(TEXT("#%u"), H->GetUniqueID() % 1000);
			const FVector P = H->GetActorLocation();
			Line += FString::Printf(TEXT(" | %s t%d %s %.0f/%.0f (%.0f,%.0f,%.0f)"), *Who, H->TeamId, H->IsDead() ? TEXT("MORTO") : TEXT("vivo"),
				H->GetHealth(), H->GetMaxHealth(), P.X, P.Y, P.Z);
			if (!H->IsDead() && P.Z < -3000.f && !PlaytestReported.Contains(Who + TEXT("caiu")))
			{
				PlaytestReported.Add(Who + TEXT("caiu"));
				UE_LOG(LogValdoria, Warning, TEXT("PLAYTEST PROBLEMA caiu do mapa: %s em (%.0f,%.0f,%.0f)"), *Who, P.X, P.Y, P.Z);
			}
			if (H->IsAutopilot() && !H->IsDead())
			{
				const FVector* Last = PlaytestLastPos.Find(Who);
				int32& Still = PlaytestStill.FindOrAdd(Who);
				Still = (Last && FVector::Dist(*Last, P) < 150.f) ? Still + 1 : 0;
				if (Still == 3 && !PlaytestReported.Contains(Who + TEXT("preso")))
				{
					PlaytestReported.Add(Who + TEXT("preso"));
					UE_LOG(LogValdoria, Warning, TEXT("PLAYTEST PROBLEMA parado/preso 15 s: %s em (%.0f,%.0f,%.0f)"), *Who, P.X, P.Y, P.Z);
					FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots/Playtest") / (PlaytestTag + TEXT("_preso.png"))), false, false);
				}
			}
			PlaytestLastPos.Add(Who, P);
		}
		if (PlaytestStep == 60 || PlaytestStep == 200) // turn 46: picture with the HUD (minimap) twice per run
		{
			FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots/Playtest") / FString::Printf(TEXT("%s_t%d.png"), *PlaytestTag, PlaytestStep)), true, false);
		}
		UE_LOG(LogValdoria, Display, TEXT("PLAYTEST t=%d guardioes=%d/%d vida=%.0f%% menor=%.0f%% minions=%d/%d%s"), PlaytestStep, GuardAlive, GuardTotal,
			GuardMax > 0.f ? GuardHealth / GuardMax * 100.f : 0.f, GuardLowest * 100.f, Minions[0], Minions[1], *Line);
	}
	// Screenshots at 30%, 60% and 90% of the run.
	for (int32 n = 1; n <= 3; ++n)
	{
		if (PlaytestStep == FMath::Max(4, PlaytestSeconds * (n * 3) / 10))
		{
			FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots/Playtest") / FString::Printf(TEXT("%s_%d.png"), *PlaytestTag, n)), true, false);
		}
	}
	if (PlaytestStep >= PlaytestSeconds)
	{
		FString K, D;
		for (const TPair<FString, int32>& P : PlaytestKills) { K += FString::Printf(TEXT(" %s=%d"), *P.Key, P.Value); }
		for (const TPair<FString, int32>& P : PlaytestDeaths) { D += FString::Printf(TEXT(" %s=%d"), *P.Key, P.Value); }
		UE_LOG(LogValdoria, Display, TEXT("PLAYTEST resumo %s | abates:%s | mortes:%s | FPS=%.0f"), *PlaytestTag, *K, *D, 1.f / FMath::Max(0.001f, FApp::GetDeltaTime()));
		UE_LOG(LogValdoria, Display, TEXT("PLAYTEST fim"));
		PlaytestStep = 100000;
		FPlatformMisc::RequestExit(false);
	}
}

void AValdoriaGameMode::RunAdminTestStep()
{
	++AdminTestStep;
	AValdoriaPlayerController* PC = Cast<AValdoriaPlayerController>(GetWorld()->GetFirstPlayerController());
	AValdoriaHeroCharacter* Hero = PC ? Cast<AValdoriaHeroCharacter>(PC->GetPawn()) : nullptr;
	if (!PC || !Hero)
	{
		return;
	}
	auto Snapshot = [this, PC]() -> FString
	{
		AValdoriaHeroCharacter* H = Cast<AValdoriaHeroCharacter>(PC->GetPawn());
		int32 Total = 0, Alive = 0, Spawned = 0;
		for (TActorIterator<AValdoriaTrainingDummy> It(GetWorld()); It; ++It)
		{
			++Total;
			Alive += It->IsDead() ? 0 : 1;
			Spawned += It->ActorHasTag(TEXT("AdminSpawned")) ? 1 : 0;
		}
		const AValdoriaHeroCharacter* T = PC->DebugGetAdminTarget().Get();
		return FString::Printf(TEXT("heroi=%s vida=%.0f ult=%.2f pos=(%.0f,%.0f,%.0f) imortal=%d semCD=%d municao=%d voo=%d vel=%.1f grav=%.2f ar=%.2f | bonecos=%d vivos=%d criados=%d | alvo=%s vida=%.0f mov=%d time=%d | tempo=%.2f"),
			H && H->GetHeroDefinition() ? *H->GetHeroDefinition()->GetName() : TEXT("-"), H ? H->GetHealth() : 0.f, H ? H->GetUltimatePercent() : 0.f,
			H ? H->GetActorLocation().X : 0.f, H ? H->GetActorLocation().Y : 0.f, H ? H->GetActorLocation().Z : 0.f,
			H && H->bGodMode, H && H->bNoCooldowns, H && H->bInfiniteAmmo, H && H->bAdminFlight, H ? H->AdminSpeedMultiplier : 0.f,
			H ? H->GetCharacterMovement()->GravityScale : 0.f, H ? H->GetCharacterMovement()->AirControl : 0.f,
			Total, Alive, Spawned, T ? *T->GetName() : TEXT("-"), T ? T->GetHealth() : 0.f, T ? (int32)T->GetCharacterMovement()->MovementMode : -1, T ? (int32)T->TeamId : -1,
			UGameplayStatics::GetGlobalTimeDilation(this));
	};
	if (AdminTestStep == 10)
	{
		PC->DebugSetAdminMode(true); // like /adm: admin on, panel closed
	}
	else if (AdminTestStep == 12)
	{
		UE_LOG(LogValdoria, Display, TEXT("ADMINTEST /adm (painel fechado=%d): %s"), !PC->IsAdminPanelOpen(), *Snapshot());
		// Look at the nearest dummy so "Pegar alvo da mira" has a target.
		AValdoriaTrainingDummy* Near = nullptr;
		for (TActorIterator<AValdoriaTrainingDummy> It(GetWorld()); It; ++It)
		{
			if (!Near || FVector::Dist(It->GetActorLocation(), Hero->GetActorLocation()) < FVector::Dist(Near->GetActorLocation(), Hero->GetActorLocation()))
			{
				Near = *It;
			}
		}
		if (Near)
		{
			PC->SetControlRotation((Near->GetActorLocation() - Hero->GetActorLocation()).Rotation());
		}
	}
	else if (AdminTestStep == 13)
	{
		PC->ToggleAdminPanel();
		const TArray<FString> All = PC->DebugListAdminControls();
		UE_LOG(LogValdoria, Display, TEXT("ADMINTEST F10: painel=%d controles=%d"), PC->IsAdminPanelOpen(), All.Num());
		static const TCHAR* Skip[] = { TEXT("Abrir mapa"), TEXT("Fechar (F10)"), TEXT("print"), TEXT("Câmera livre"), TEXT("Tempos de CPU"), TEXT("Mostrar colisões") };
		// Owner (turn 44): do not click every weapon model each run, only the ones the current heroes hold.
		TSet<FString> SkipWeapons;
		{
			static const TCHAR* UsedByHeroes[] = { TEXT("Rifle"), TEXT("Martelo"), TEXT("EscudoA"), TEXT("CajadoA"), TEXT("CajadoB"), TEXT("AdagaA"), TEXT("AdagaB"), TEXT("Sem arma") };
			IAssetRegistry& Reg = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
			TArray<FAssetData> Weapons;
			Reg.GetAssetsByPath(FName(TEXT("/Game/Valdoria/Weapons/Merged")), Weapons, false);
			Reg.GetAssetsByPath(FName(TEXT("/Game/Valdoria/Weapons/Fantasy")), Weapons, false);
			for (const FAssetData& W : Weapons)
			{
				if (W.AssetClassPath.GetAssetName() != TEXT("StaticMesh")) { continue; }
				// Same label rule as the admin panel's ARMA tab.
				const FString Name = W.AssetName.ToString().Replace(TEXT("SM_Arma_"), TEXT("")).Replace(TEXT("SM_"), TEXT("")).Replace(TEXT("_"), TEXT(" "));
				bool bUsed = false;
				for (const TCHAR* U : UsedByHeroes) { bUsed |= Name == U; }
				if (!bUsed) { SkipWeapons.Add(Name); }
			}
		}
		for (const FString& L : All)
		{
			bool bSkip = SkipWeapons.Contains(L);
			for (const TCHAR* S : Skip) { bSkip |= L.Contains(S); }
			if (L == TEXT("Iara") || L == TEXT("Kael Rook") || L == TEXT("Lume") || L == TEXT("Vex") || L == TEXT("Brasa") || L == TEXT("Recruta (teste)"))
			{
				continue; // hero switches go last (they reset the admin Recruit)
			}
			if (L == TEXT("Iara") || L == TEXT("Kael Rook") || L == TEXT("Lume") || L == TEXT("Vex") || L == TEXT("Brasa") || L == TEXT("Recruta (teste)"))
			{
				continue; // hero switches go last (they reset the admin Recruit)
			}
			if (!bSkip) { AdminTestQueue.Add(L); }
		}
		// Turn 44: run the main admin toggles on EVERY hero (switch, wait a few steps, then click and log).
		static const TCHAR* Heroes[] = { TEXT("Brasa"), TEXT("Kael Rook"), TEXT("Iara"), TEXT("Vex"), TEXT("Lume"), TEXT("Recruta (teste)") };
		static const TCHAR* Core[] = { TEXT("Imortal"), TEXT("Sem recarga nas habilidades"), TEXT("Munição infinita"), TEXT("Impulso infinito"),
			TEXT("Ultimate cheia"), TEXT("Vida cheia"), TEXT("Tirar 50 de vida de mim"), TEXT("Zerar recargas"), TEXT("Voar normal e infinito (Espaço sobe, Ctrl desce)") };
		for (const TCHAR* H : Heroes)
		{
			AdminTestQueue.Add(H);
			AdminTestQueue.Add(TEXT("#espera"));
			AdminTestQueue.Add(TEXT("#espera"));
			AdminTestQueue.Add(TEXT("#estado")); // state right after the switch, nothing clicked
			for (const TCHAR* C : Core)
			{
				AdminTestQueue.Add(C);
			}
			AdminTestQueue.Add(TEXT("#estado"));
		}
		FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots/Admin_Abas.png")), true, false);
	}
	else if (AdminTestStep > 14 && AdminTestQueue.Num() > 0)
	{
		const FString L = AdminTestQueue[0];
		AdminTestQueue.RemoveAt(0);
		const bool bOk = L.StartsWith(TEXT("#")) ? false : PC->DebugClickAdmin(L);
		UE_LOG(LogValdoria, Display, TEXT("ADMINTEST [%s] clicou=%d: %s"), *L, bOk, *Snapshot());
		if (L.StartsWith(TEXT("MOVIMENTO")))
		{
			FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots/Admin_Movimento.png")), true, false);
		}
	}
	else if (AdminTestStep > 14)
	{
		UE_LOG(LogValdoria, Display, TEXT("ADMINTEST fim"));
		AdminTestStep = 100000;
		FPlatformMisc::RequestExit(false);
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

void AValdoriaGameMode::RunHeroTestStep()
{
	++HeroTestStep;
	const int32 Warmup = 16;
	if (HeroTestStep < Warmup)
	{
		return;
	}
	AValdoriaPlayerController* PC = Cast<AValdoriaPlayerController>(GetWorld()->GetFirstPlayerController());
	AValdoriaHeroCharacter* Hero = PC ? Cast<AValdoriaHeroCharacter>(PC->GetPawn()) : nullptr;
	static const TCHAR* Ids[] = { TEXT("brasa"), TEXT("kael"), TEXT("iara"), TEXT("vex"), TEXT("lume"), TEXT("kay"), TEXT("ariel"), TEXT("heimdall"), TEXT("nazare") };
	const int32 PerHero = 44;
	const int32 Index = (HeroTestStep - Warmup) / PerHero;
	const int32 S = (HeroTestStep - Warmup) % PerHero;
	if (Index >= UE_ARRAY_COUNT(Ids) || !Hero)
	{
		UE_LOG(LogValdoria, Display, TEXT("HEROTEST fim"));
		HeroTestStep = 100000;
		FPlatformMisc::RequestExit(false);
		return;
	}
	const FString Id = Ids[Index];
	AValdoriaTrainingDummy* Enemy = HeroTestEnemy.Get();
	AValdoriaTrainingDummy* Ally = HeroTestAlly.Get();
	auto Shot = [&Id](int32 N)
	{
		FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()
			/ TEXT("Screenshots") / FString::Printf(TEXT("Heroi_%s_%d.png"), *Id, N)), true, false);
	};
	auto Report = [&](const TCHAR* What)
	{
		UE_LOG(LogValdoria, Display, TEXT("HEROTEST %s %s: inimigo vida=%.0f escudo=%.0f arm=%.0f dist=%.0f | aliado vida=%.0f | eu vida=%.0f arm=%.0f prot=%.2f invisivel=%d ult=%.0f%% municao=%d"),
			*Id, What, Enemy ? Enemy->GetHealth() : -1.f, Enemy ? Enemy->GetShield() : -1.f, Enemy ? Enemy->GetArmor() : -1.f,
			Enemy ? FVector::Dist2D(Enemy->GetActorLocation(), Hero->GetActorLocation()) : -1.f,
			Ally ? Ally->GetHealth() : -1.f, Hero->GetHealth(), Hero->GetArmor(), Hero->GetPassiveDamageReduction(),
			Hero->IsHiddenByStealth(), Hero->GetUltimatePercent() * 100.f, Hero->GetAmmo());
	};
	auto AimAtEnemy = [&]()
	{
		if (Enemy)
		{
			const FVector Eye = Hero->GetFollowCamera()->GetComponentLocation();
			PC->SetControlRotation((Enemy->GetActorLocation() + FVector(0, 0, 20.f) - Eye).Rotation());
		}
	};
	auto Use = [&](EValdoriaAbilitySlot Slot, const TCHAR* Name)
	{
		AimAtEnemy();
		Hero->TryActivateSlot(Slot);
		UE_LOG(LogValdoria, Display, TEXT("HEROTEST %s usou %s (recarga=%.1f)"), *Id, Name, Hero->GetCooldownRemaining(Slot));
	};

	switch (S)
	{
	case 0:
	{
		if (PC->IsShopOpen()) { PC->ToggleShop(); }
		if (Enemy) Enemy->Destroy();
		if (Ally) Ally->Destroy();
		Hero->bGodMode = false;
		PC->SelectHero(FName(*Id));
		Hero->ResetCombatState();
		if (Index == 0)
		{
			HeroTestHome = Hero->GetActorLocation();
			HeroTestHomeRot = Hero->GetActorRotation();
		}
		else
		{
			Hero->TeleportTo(HeroTestHome, HeroTestHomeRot);
			PC->SetControlRotation(HeroTestHomeRot);
		}
		const FVector Fwd = Hero->GetActorForwardVector().GetSafeNormal2D();
		const FVector Right = FVector::CrossProduct(FVector::UpVector, Fwd);
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Enemy = GetWorld()->SpawnActor<AValdoriaTrainingDummy>(AValdoriaTrainingDummy::StaticClass(),
			Hero->GetActorLocation() + Fwd * 330.f, (-Fwd).Rotation(), P);
		Ally = GetWorld()->SpawnActor<AValdoriaTrainingDummy>(AValdoriaTrainingDummy::StaticClass(),
			Hero->GetActorLocation() + Right * 250.f + Fwd * 150.f, (-Fwd).Rotation(), P);
		if (Enemy) { Enemy->bNoAutoHeal = true; }
		if (Ally) { Ally->bNoAutoHeal = true; Ally->TeamId = Hero->TeamId; }
		HeroTestEnemy = Enemy;
		HeroTestAlly = Ally;
		UE_LOG(LogValdoria, Display, TEXT("HEROTEST %s escolhido: nome=%s vida max=%.0f armadura=%.0f escudo=%.0f habilidades=%d"), *Id,
			Hero->GetHeroDefinition() ? *Hero->GetHeroDefinition()->DisplayName.ToString() : TEXT("?"), Hero->GetMaxHealth(),
			Hero->GetMaxArmor(), Hero->GetMaxShield(), Hero->GetHeroDefinition() ? Hero->GetHeroDefinition()->Abilities.Num() : 0);
		break;
	}
	case 2:
		if (Ally) { Hero->DealDamageTo(Ally, 120.f); }
		Report(TEXT("inicio"));
		break;
	case 4:
		PressKey(EKeys::D, true); // anda de lado enquanto ataca (pernas andam, tronco ataca)
		break;
	case 3: case 5: case 7: case 9:
		Use(EValdoriaAbilitySlot::PrimaryFire, TEXT("ataque"));
		break;
	case 10:
		Shot(10);
		PressKey(EKeys::D, false);
		break;
	case 11: Report(TEXT("depois do ataque")); Shot(1); break;
	case 12:
		// Back to 3.3 m in front of the enemy dummy: the sidestep while attacking used to leave short-range
		// abilities (cones, slams) out of reach, so their checks depended on how far the hero walked (turn 45).
		if (Enemy)
		{
			const FVector Away = (Hero->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
			Hero->SetActorLocation(FVector(Enemy->GetActorLocation().X, Enemy->GetActorLocation().Y, Hero->GetActorLocation().Z) + Away * 330.f, false, nullptr, ETeleportType::TeleportPhysics);
			AimAtEnemy();
		}
		break;
	case 13: Use(EValdoriaAbilitySlot::SecondaryFire, TEXT("botao direito")); break;
	case 14: case 22: case 25:
		if (Index == 4 && Enemy)
		{
			const float Before = Hero->GetHealth() + Hero->GetShield();
			const float EnemyBefore = Enemy->GetHealth();
			const float UltBefore = Hero->GetUltimatePercent();
			Enemy->DealTypedDamage(Hero, 50.f, EValdoriaDamageType::Physical);
			UE_LOG(LogValdoria, Display, TEXT("HEROTEST lume defesa (passo %d): refletindo=%d absorvendo=%d intangivel=%d perdeu=%.1f inimigo perdeu=%.1f ult +%.1f%%"),
				S, Hero->IsReflecting(), Hero->IsAbsorbing(), Hero->IsIntangible(), Before - (Hero->GetHealth() + Hero->GetShield()),
				EnemyBefore - Enemy->GetHealth(), (Hero->GetUltimatePercent() - UltBefore) * 100.f);
		}
		break;
	case 16: Report(TEXT("depois do botao direito")); Shot(2); break;
	case 18:
		if (Index == 0)
		{
			// Standard keys (owner, turn 45): ability 1 is on the real key 1.
			AimAtEnemy();
			PressKey(EKeys::One, true);
		}
		else
		{
			Use(EValdoriaAbilitySlot::Ability1, TEXT("1"));
		}
		break;
	case 19:
		if (Index == 0)
		{
			PressKey(EKeys::One, false);
			UE_LOG(LogValdoria, Display, TEXT("HEROTEST tecla 1 usou a habilidade 1 (recarga=%.1f)"), Hero->GetCooldownRemaining(EValdoriaAbilitySlot::Ability1));
		}
		if (Index == 5)
		{
			PressKey(EKeys::Four, true); // Kay: a short tap on 4 switches the element
		}
		break;
	case 20:
		if (Index == 5)
		{
			PressKey(EKeys::Four, false);
		}
		break;
	case 21:
		Report(TEXT("depois do 1")); Shot(3);
		if (Index == 5) { UE_LOG(LogValdoria, Display, TEXT("HEROTEST kay elemento depois do toque no 4: %d (%s) troca em %.1f s"), Hero->GetCurrentElement(), *Hero->GetElementName().ToString(), Hero->GetElementSwitchRemaining()); }
		break;
	case 23: AimAtEnemy(); Use(EValdoriaAbilitySlot::Ability2, TEXT("2")); break; // aimed abilities (Vex dash) need the target under the crosshair
	case 27: Report(TEXT("depois do 2")); Shot(4); break;
	case 28: if (Index != 2) { AimAtEnemy(); } Use(EValdoriaAbilitySlot::Ability3, TEXT("3")); break; // turn 45: every hero has ability 3 (turn 48: aimed, not Iara who pulls the ally)
	case 29:
		Report(TEXT("depois do 3"));
		Hero->AdminFillUltimate();
		if (Index == 5) { PressKey(EKeys::Four, true); } // Kay: hold 4 for 3 s (12 steps)
		break;
	case 30:
		if (Index == 6) { Use(EValdoriaAbilitySlot::AbilityExtra, TEXT("C (trocar de forma)")); }
		break;
	case 31:
		if (Index == 6) { UE_LOG(LogValdoria, Display, TEXT("HEROTEST ariel forma depois do C: %s"), Hero->GetHeroDefinition() ? *Hero->GetHeroDefinition()->HeroId.ToString() : TEXT("?")); }
		break;
	case 32: if (Index != 5) { Use(EValdoriaAbilitySlot::Ultimate, TEXT("ultimate")); } break;
	case 33: Use(EValdoriaAbilitySlot::PrimaryFire, TEXT("ataque na ultimate")); Shot(5); break;
	case 35: case 36: case 37:
		if (Index == 4 && Ally)
		{
			const FVector Eye = Hero->GetFollowCamera()->GetComponentLocation();
			PC->SetControlRotation((Ally->GetActorLocation() + FVector(0, 0, 20.f) - Eye).Rotation());
			Hero->TryActivateSlot(EValdoriaAbilitySlot::PrimaryFire);
			UE_LOG(LogValdoria, Display, TEXT("HEROTEST lume feixe no aliado: aliado vida=%.0f alvo=%s"), Ally->GetHealth(),
				Hero->BeamTarget.IsValid() ? *Hero->BeamTarget->GetName() : TEXT("nenhum"));
		}
		break;
	case 38: Report(TEXT("depois da ultimate")); Shot(6); break;
	// Dano recebido: um bot atira em mim (testa a passiva de proteção da Brasa).
	case 40:
		if (Enemy)
		{
			const float Before = Hero->GetHealth() + Hero->GetArmor() + Hero->GetShield();
			Enemy->DealTypedDamage(Hero, 50.f, EValdoriaDamageType::Physical);
			const float After = Hero->GetHealth() + Hero->GetArmor() + Hero->GetShield();
			UE_LOG(LogValdoria, Display, TEXT("HEROTEST %s levou 50 fisico -> perdeu %.1f (aliados perto=%d)"), *Id, Before - After, Hero->GetAlliesNearby());
			if (Index == 1) { Shot(9); }
		}
		break;
	case 41:
		if (Index == 1)
		{
			PC->AddCredits(3000.f);
			const int32 Before = Hero->GetWeaponStats().MagazineSize;
			const bool b1 = PC->BuyItem(TEXT("cartucheira"));
			const bool b2 = PC->BuyItem(TEXT("cartucheira"));
			const bool b3 = PC->BuyItem(TEXT("engrenagem"));
			UE_LOG(LogValdoria, Display, TEXT("HEROTEST loja: compras=%d%d%d pente %d -> %d creditos=%d nivel cartucheira=%d"), b1, b2, b3,
				Before, Hero->GetWeaponStats().MagazineSize, PC->GetCredits(), PC->GetItemLevel(TEXT("cartucheira")));
			PressKey(EKeys::B, true); PressKey(EKeys::B, false);
		}
		break;
	case 42:
		if (Index == 3)
		{
			// Locomoção "flutuar": só em memória, para conferir a animação de pairar e a hitbox elevada.
			UValdoriaHeroDefinition* Def = const_cast<UValdoriaHeroDefinition*>(Hero->GetHeroDefinition());
			Def->Locomotion = EValdoriaLocomotion::Float;
			Def->HoverHeight = 35.f;
			Hero->ApplyHeroDefinition();
			PC->SetControlRotation(FRotator(-10.f, Hero->GetActorRotation().Yaw + 150.f, 0.f));
		}
		break;
	case 43:
		if (Index == 5)
		{
			UE_LOG(LogValdoria, Display, TEXT("HEROTEST kay devastacao (4 segurado 3,5 s): ult=%.0f%% inimigo vida=%.0f"), Hero->GetUltimatePercent() * 100.f, Enemy ? Enemy->GetHealth() : -1.f);
			PressKey(EKeys::Four, false);
		}
		if (Index == 1)
		{
			UE_LOG(LogValdoria, Display, TEXT("HEROTEST loja aberta com B: %d"), PC->IsShopOpen());
			Shot(8);
		}
		if (Index == 3) { Shot(7); }
		break;
	default: break;
	}
}

void AValdoriaGameMode::RunWeaponPoseStep()
{
	// -ValdoriaWeaponPose: each hero holding its weapon(s), seen from two sides, to check the grip against
	// the owner's concept art. Optional Saved/grip_variants.txt, one shot pair per line, to try grips without
	// rebuilding: "id name  P Y R  X Y Z  Scale  [oP oY oR  oX oY oZ  oScale]" (o = offhand).
	++WeaponPoseStep;
	AValdoriaPlayerController* PC = Cast<AValdoriaPlayerController>(GetWorld()->GetFirstPlayerController());
	AValdoriaHeroCharacter* Hero = PC ? Cast<AValdoriaHeroCharacter>(PC->GetPawn()) : nullptr;
	if (!Hero || WeaponPoseStep < 12)
	{
		return;
	}
	static TArray<FString> Lines;
	if (WeaponPoseStep == 12)
	{
		Lines.Reset();
		FString Text;
		if (FFileHelper::LoadFileToString(Text, *(FPaths::ProjectSavedDir() / TEXT("grip_variants.txt"))))
		{
			Text.ParseIntoArrayLines(Lines);
			Lines.RemoveAll([](const FString& L) { return L.TrimStartAndEnd().IsEmpty() || L.StartsWith(TEXT("#")); });
		}
		if (Lines.Num() == 0)
		{
			Lines = { TEXT("brasa atual"), TEXT("kael atual"), TEXT("iara atual"), TEXT("vex atual"), TEXT("lume atual") };
		}
	}
	const int32 S = WeaponPoseStep - 12;
	const int32 Index = S / 5;
	if (Index >= Lines.Num())
	{
		UE_LOG(LogValdoria, Display, TEXT("WEAPONPOSE fim"));
		WeaponPoseStep = 100000;
		FPlatformMisc::RequestExit(false);
		return;
	}
	TArray<FString> Tok;
	Lines[Index].ParseIntoArrayWS(Tok);
	const FString Id = Tok.IsValidIndex(0) ? Tok[0] : FString();
	const FString Name = Id + TEXT("_") + (Tok.IsValidIndex(1) ? Tok[1] : FString(TEXT("atual")));
	auto Num = [&Tok](int32 i, float Def) { return Tok.IsValidIndex(i) ? FCString::Atof(*Tok[i]) : Def; };
	const FString Shot = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots") / TEXT("Grip_") + Name);
	switch (S % 5)
	{
	case 0:
		PC->SelectHero(FName(*Id));
		Hero->bUseControllerRotationYaw = false;
		Hero->SetActorRotation(FRotator::ZeroRotator);
		if (Tok.Num() >= 9)
		{
			Hero->WeaponComponent->SetRelativeTransform(FTransform(FRotator(Num(2, 0), Num(3, 0), Num(4, 0)), FVector(Num(5, 0), Num(6, 0), Num(7, 0)), FVector(Num(8, 1))));
		}
		if (Tok.Num() >= 16)
		{
			Hero->OffhandComponent->SetRelativeTransform(FTransform(FRotator(Num(9, 0), Num(10, 0), Num(11, 0)), FVector(Num(12, 0), Num(13, 0), Num(14, 0)), FVector(Num(15, 1))));
		}
		PC->SetControlRotation(FRotator(-8.f, 145.f, 0.f));
		break;
	case 2: FScreenshotRequest::RequestScreenshot(Shot + TEXT("_a.png"), false, false); break;
	case 3: PC->SetControlRotation(FRotator(-8.f, -90.f, 0.f)); break;
	case 4: FScreenshotRequest::RequestScreenshot(Shot + TEXT("_b.png"), false, false); break;
	default: break;
	}
}
void AValdoriaGameMode::RunFlightTestStep()
{
	++FlightTestStep;
	const int32 S = FlightTestStep - 16;
	AValdoriaPlayerController* PC = Cast<AValdoriaPlayerController>(GetWorld()->GetFirstPlayerController());
	AValdoriaHeroCharacter* Hero = PC ? Cast<AValdoriaHeroCharacter>(PC->GetPawn()) : nullptr;
	if (S < 0 || !Hero)
	{
		return;
	}
	auto SideShot = [&](const TCHAR* Name)
	{
		// Camera from the side for the screenshot (the hero keeps flying forward).
		USpringArmComponent* Boom = Hero->GetCameraBoom();
		Boom->bUsePawnControlRotation = false;
		Boom->SetWorldRotation(FRotator(-5.f, Hero->GetActorRotation().Yaw - 90.f, 0.f));
		Boom->TargetArmLength = 420.f;
		FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots") / Name), false, false);
	};
	auto RestoreCamera = [&]()
	{
		Hero->GetCameraBoom()->bUsePawnControlRotation = true;
		Hero->GetCameraBoom()->TargetArmLength = Hero->GetHeroDefinition() ? Hero->GetHeroDefinition()->CameraDistance : 300.f;
	};
	UValdoriaHeroDefinition* Def = const_cast<UValdoriaHeroDefinition*>(Hero->GetHeroDefinition());
	switch (S)
	{
	case 0:
		if (Def) { Def->Movement.FlightFuelSeconds = 60.f; Def->Movement.FlightSpeed = 1200.f; }
		Hero->FlightFuel = 60.f;
		PC->SetControlRotation(FRotator(0.f, Hero->GetActorRotation().Yaw, 0.f));
		PressKey(EKeys::SpaceBar, true);
		break;
	case 6: PressKey(EKeys::W, true); break;
	case 14: SideShot(TEXT("Voo_SuperHeroi.png")); break;
	case 15: RestoreCamera(); PressKey(EKeys::W, false); break;
	case 21: SideShot(TEXT("Voo_Parado.png")); break;
	case 22: RestoreCamera(); PressKey(EKeys::SpaceBar, false); break;
	case 27: SideShot(TEXT("Voo_Pouso.png")); break;
	case 28:
		RestoreCamera();
		if (Def)
		{
			Def->Animations.FlightStyle = EValdoriaFlightStyle::Levitate;
		}
		Hero->FlightFuel = 60.f;
		PressKey(EKeys::SpaceBar, true);
		break;
	case 34: PressKey(EKeys::W, true); break;
	case 42: SideShot(TEXT("Voo_Levitar.png")); break;
	case 43:
		RestoreCamera();
		PressKey(EKeys::W, false);
		PressKey(EKeys::SpaceBar, false);
		UE_LOG(LogValdoria, Display, TEXT("FLIGHTTEST fim"));
		FlightTestStep = 100000;
		FPlatformMisc::RequestExit(false);
		break;
	default: break;
	}
	if (S % 3 == 0)
	{
		UE_LOG(LogValdoria, Display, TEXT("FLIGHTTEST dbg %d: heroi=%s pulo=%d modo=%d combustivel=%.2f z=%.0f vz=%.0f planando=%d"), S,
			Def ? *Def->HeroId.ToString() : TEXT("-"), Hero->bJumpHeld, (int32)Hero->GetCharacterMovement()->MovementMode.GetValue(), Hero->FlightFuel,
			Hero->GetActorLocation().Z, Hero->GetVelocity().Z, Hero->IsGliding());
	}
	if (S == 14 || S == 21 || S == 42)
	{
		UE_LOG(LogValdoria, Display, TEXT("FLIGHTTEST passo %d: voando=%d velocidade=%.0f inclinacao=%.1f"), S, Hero->IsFlying(),
			Hero->GetVelocity().Size2D(), Hero->GetMesh()->GetRelativeRotation().Pitch);
	}
}
