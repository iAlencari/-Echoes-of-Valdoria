// Echoes of Valdoria - personagem jogável genérico.

#include "ValdoriaHeroCharacter.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "ValdoriaAttributeSet.h"
#include "ValdoriaGameMode.h"
#include "ValdoriaGameplayAbility.h"
#include "ValdoriaGameplayTags.h"
#include "ValdoriaHUD.h"
#include "ValdoriaHeroDefinition.h"
#include "ValdoriaSettings.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ValdoriaEffects.h"
#include "ValdoriaWorldActors.h"
#include "ValdoriaPlayerController.h"
#include "ValdoriaAnimInstance.h"
#include "ValdoriaAudio.h"
#include "ValdoriaLaneRail.h"
#include "Sound/SoundBase.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY(LogValdoria);

AValdoriaHeroCharacter::AValdoriaHeroCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetNetUpdateFrequency(60.f);

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	// O raio de tiro (canal Visibility) acerta a malha, não a cápsula: assim
	// dá para saber se foi na cabeça.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -96.f), FRotator(0.f, -90.f, 0.f));

	// Estilo shooter: o corpo acompanha a mira.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = 550.f;
	GetCharacterMovement()->JumpZVelocity = 520.f;
	GetCharacterMovement()->AirControl = 0.4f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2400.f;
	GetCharacterMovement()->GroundFriction = 8.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.f;
	CameraBoom->SocketOffset = FVector(0.f, 55.f, 65.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = false;
	CameraBoom->ProbeSize = 14.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(95.f);

	WeaponComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Weapon"));
	WeaponComponent->SetupAttachment(GetMesh());
	WeaponComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponComponent->SetCastShadow(true);

	RideDisc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RideDisc"));
	RideDisc->SetupAttachment(RootComponent);
	RideDisc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RideDisc->SetCastShadow(false);
	RideDisc->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DiscMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (DiscMesh.Succeeded())
	{
		RideDisc->SetStaticMesh(DiscMesh.Object);
	}
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystem->SetIsReplicated(true);
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	Attributes = CreateDefaultSubobject<UValdoriaAttributeSet>(TEXT("Attributes"));
}

void AValdoriaHeroCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AValdoriaHeroCharacter, HeroDefinition);
	DOREPLIFETIME(AValdoriaHeroCharacter, TeamId);
	DOREPLIFETIME(AValdoriaHeroCharacter, bIsDead);
	DOREPLIFETIME_CONDITION(AValdoriaHeroCharacter, CurrentAmmo, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AValdoriaHeroCharacter, bReloading, COND_SkipOwner);
	DOREPLIFETIME(AValdoriaHeroCharacter, bGodMode);
	DOREPLIFETIME(AValdoriaHeroCharacter, bNoCooldowns);
	DOREPLIFETIME(AValdoriaHeroCharacter, bInfiniteAmmo);
	DOREPLIFETIME(AValdoriaHeroCharacter, bInfiniteDashes);
	DOREPLIFETIME(AValdoriaHeroCharacter, AdminSpeedMultiplier);
	DOREPLIFETIME_CONDITION(AValdoriaHeroCharacter, bIsSprinting, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AValdoriaHeroCharacter, bIsProne, COND_SkipOwner);
}

void AValdoriaHeroCharacter::BeginPlay()
{
	Super::BeginPlay();
	DefaultMeshRelativeTransform = GetMesh()->GetRelativeTransform();
	FollowCamera->SetFieldOfView(FValdoriaOptions::Get().FieldOfView);
	AbilitySystem->InitAbilityActorInfo(this, this);
	ApplyHeroDefinition();
}

void AValdoriaHeroCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	AbilitySystem->InitAbilityActorInfo(this, this);
	if (!ActiveDefinition)
	{
		ApplyHeroDefinition();
	}
	GrantAbilities();
}

void AValdoriaHeroCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	AbilitySystem->InitAbilityActorInfo(this, this);
}

// ------------------------------------------------------------------ definição do herói

void AValdoriaHeroCharacter::SetHeroDefinition(UValdoriaHeroDefinition* NewDefinition)
{
	if (!HasAuthority())
	{
		return;
	}
	HeroDefinition = NewDefinition;
	bAbilitiesGranted = false;
	ApplyHeroDefinition();
	GrantAbilities();
}

void AValdoriaHeroCharacter::OnRep_HeroDefinition()
{
	ApplyHeroDefinition();
}

void AValdoriaHeroCharacter::ApplyHeroDefinition()
{
	if (HeroDefinition)
	{
		ActiveDefinition = HeroDefinition;
	}
	else if (!ActiveDefinition || ActiveDefinition->GetFName() != TEXT("FallbackRecruit"))
	{
		ActiveDefinition = UValdoriaHeroDefinition::CreateFallbackRecruit(this);
	}
	const UValdoriaHeroDefinition* Def = ActiveDefinition;

	GetCapsuleComponent()->SetCapsuleSize(Def->CapsuleRadius, Def->CapsuleHalfHeight);
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = Def->MoveSpeed;
	Move->JumpZVelocity = Def->JumpVelocity;
	Move->AirControl = Def->AirControl;
	const FValdoriaMovementRules& Rules = Def->Movement;
	Move->GetNavAgentPropertiesRef().bCanCrouch = Rules.bCanCrouch || Rules.bCanProne;
	Move->SetCrouchedHalfHeight(Rules.CrouchHalfHeight);
	Move->MaxWalkSpeedCrouched = Rules.CrouchSpeed;
	JumpMaxCount = FMath::Clamp(Rules.MaxJumps, 1, 5);
	DashChargesLeft = FMath::Clamp(Rules.DashCharges, 1, 5);
	DashRechargeProgress = 0.f;
	bIsSprinting = false;
	bIsProne = false;
	UpdateMovementSpeed();
	CameraBoom->TargetArmLength = Def->CameraDistance * FMath::Max(1.f, Def->CapsuleHalfHeight / 96.f);
	CameraBoom->SocketOffset = Def->CameraOffset * FVector(1.f, 1.f, Def->CapsuleHalfHeight / 96.f) + FVector(0.f, 0.f, Def->HoverHeight);

	if (USkeletalMesh* NewMesh = Def->Mesh.LoadSynchronous())
	{
		GetMesh()->SetSkeletalMesh(NewMesh);
	}
	if (Def->Animations.bUseValdoriaAnimator)
	{
		// Animador próprio em C++ (andar, correr, ataques da cintura para cima).
		GetMesh()->SetAnimInstanceClass(UValdoriaAnimInstance::StaticClass());
	}
	else if (UClass* Anim = Def->AnimClass.LoadSynchronous())
	{
		GetMesh()->SetAnimInstanceClass(Anim);
	}
	// Malha: X = largura, Y = profundidade (o modelo olha para +Y), Z = altura.
	GetMesh()->SetRelativeScale3D(FVector(Def->BodyWidth, Def->BodyDepth, Def->BodyHeight) * Def->MeshScale);
	ComboIndex = 0;
	// Arma na mão (modelo 3D do Data Asset).
	UStaticMesh* Gun = Def->WeaponMesh.IsNull() ? nullptr : Def->WeaponMesh.LoadSynchronous();
	WeaponComponent->SetStaticMesh(Gun);
	WeaponComponent->SetVisibility(Gun != nullptr);
	if (Gun)
	{
		WeaponComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Def->WeaponSocket);
		WeaponComponent->SetRelativeTransform(Def->WeaponOffset);
	}
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -Def->CapsuleHalfHeight + Def->HoverHeight));
	// Andar sobre efeito: disco brilhante sob os pés, na altura da flutuação.
	const bool bRide = Def->Locomotion == EValdoriaLocomotion::RideEffect;
	RideDisc->SetVisibility(bRide);
	if (bRide)
	{
		RideDisc->SetRelativeLocation(FVector(0.f, 0.f, -Def->CapsuleHalfHeight + Def->HoverHeight * 0.5f));
		if (UStaticMesh* Custom = Def->RideEffectMesh.IsNull() ? nullptr : Def->RideEffectMesh.LoadSynchronous())
		{
			RideDisc->SetStaticMesh(Custom);
		}
		RideDisc->SetRelativeScale3D(Def->RideEffectScale);
		const FLinearColor RideColor = Def->RideEffectColor.Equals(FLinearColor::Black) ? Def->ThemeColor : Def->RideEffectColor;
		RideDisc->SetMaterial(0, ValdoriaMakeGlowMaterial(this, RideColor, 0.5f));
	}

	if (HasAuthority())
	{
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetMaxHealthAttribute(), Def->MaxHealth + ShopBonus.MaxHealth);
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetHealthAttribute(), Def->MaxHealth + ShopBonus.MaxHealth);
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetMaxArmorAttribute(), Def->MaxArmor);
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetArmorAttribute(), Def->MaxArmor);
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetMaxShieldAttribute(), Def->MaxShield);
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetShieldAttribute(), Def->MaxShield);
	}
	bHasWeaponOverride = false;
	bPendingSlam = false;
	BreakStealth();
	SlowFraction = BuffFraction = 0.f;
	StatusEndTimes.Reset();
	RebuildBoostedWeapon();
	CurrentAmmo = BoostedWeapon.MagazineSize;
	bReloading = false;
	CooldownEndTimes.Reset();
	// O pawn pode ser possuído antes do BeginPlay (jogador entrando antes de o
	// mundo começar): por isso as habilidades também são concedidas aqui.
	GrantAbilities();
}

void AValdoriaHeroCharacter::GrantAbilities()
{
	if (!HasAuthority() || !ActiveDefinition || bAbilitiesGranted)
	{
		return;
	}
	for (const FGameplayAbilitySpecHandle& Handle : GrantedAbilities)
	{
		AbilitySystem->ClearAbility(Handle);
	}
	GrantedAbilities.Reset();
	for (const FValdoriaAbilityEntry& Entry : ActiveDefinition->Abilities)
	{
		if (Entry.AbilityClass)
		{
			FGameplayAbilitySpec Spec(Entry.AbilityClass, 1, static_cast<int32>(Entry.Slot), this);
			GrantedAbilities.Add(AbilitySystem->GiveAbility(Spec));
		}
	}
	bAbilitiesGranted = true;
}

const FValdoriaAbilityEntry* AValdoriaHeroCharacter::FindAbilityEntry(EValdoriaAbilitySlot Slot) const
{
	if (!ActiveDefinition)
	{
		return nullptr;
	}
	return ActiveDefinition->Abilities.FindByPredicate([Slot](const FValdoriaAbilityEntry& E) { return E.Slot == Slot; });
}

const FValdoriaWeaponStats& AValdoriaHeroCharacter::GetWeaponStats() const
{
	static const FValdoriaWeaponStats Default;
	if (bHasWeaponOverride)
	{
		return WeaponOverride;
	}
	return ActiveDefinition ? BoostedWeapon : Default;
}

float AValdoriaHeroCharacter::GetArmorDamageReduction() const
{
	return ActiveDefinition ? ActiveDefinition->ArmorDamageReduction : 0.3f;
}

bool AValdoriaHeroCharacter::IsEnemy(const AActor* Other) const
{
	const AValdoriaHeroCharacter* OtherHero = Cast<AValdoriaHeroCharacter>(Other);
	return OtherHero && OtherHero != this && OtherHero->TeamId != TeamId && !OtherHero->IsDead();
}

// ------------------------------------------------------------------ entrada

void AValdoriaHeroCharacter::BuildDefaultInput()
{
	if (DefaultMapping)
	{
		return;
	}
	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = Type;
		return Action;
	};
	MoveAction = MakeAction(TEXT("IA_Valdoria_Move"), EInputActionValueType::Axis2D);
	LookAction = MakeAction(TEXT("IA_Valdoria_Look"), EInputActionValueType::Axis2D);
	JumpAction = MakeAction(TEXT("IA_Valdoria_Jump"), EInputActionValueType::Boolean);
	FireAction = MakeAction(TEXT("IA_Valdoria_Fire"), EInputActionValueType::Boolean);
	SecondaryAction = MakeAction(TEXT("IA_Valdoria_Secondary"), EInputActionValueType::Boolean);
	Ability1Action = MakeAction(TEXT("IA_Valdoria_Ability1"), EInputActionValueType::Boolean);
	Ability2Action = MakeAction(TEXT("IA_Valdoria_Ability2"), EInputActionValueType::Boolean);
	Ability3Action = MakeAction(TEXT("IA_Valdoria_Ability3"), EInputActionValueType::Boolean);
	UltimateAction = MakeAction(TEXT("IA_Valdoria_Ultimate"), EInputActionValueType::Boolean);
	ReloadAction = MakeAction(TEXT("IA_Valdoria_Reload"), EInputActionValueType::Boolean);
	SprintAction = MakeAction(TEXT("IA_Valdoria_Shift"), EInputActionValueType::Boolean);
	CrouchAction = MakeAction(TEXT("IA_Valdoria_Crouch"), EInputActionValueType::Boolean);
	ProneAction = MakeAction(TEXT("IA_Valdoria_Prone"), EInputActionValueType::Boolean);

	DefaultMapping = NewObject<UInputMappingContext>(this, TEXT("IMC_Valdoria"));
	auto AddSwizzle = [this](FEnhancedActionKeyMapping& M)
	{
		M.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(DefaultMapping));
	};
	auto AddNegate = [this](FEnhancedActionKeyMapping& M)
	{
		M.Modifiers.Add(NewObject<UInputModifierNegate>(DefaultMapping));
	};
	// WASD -> vetor 2D (X = direita, Y = frente).
	AddSwizzle(DefaultMapping->MapKey(MoveAction, EKeys::W));
	{
		FEnhancedActionKeyMapping& S = DefaultMapping->MapKey(MoveAction, EKeys::S);
		AddSwizzle(S);
		AddNegate(S);
	}
	AddNegate(DefaultMapping->MapKey(MoveAction, EKeys::A));
	DefaultMapping->MapKey(MoveAction, EKeys::D);
	DefaultMapping->MapKey(LookAction, EKeys::Mouse2D);
	DefaultMapping->MapKey(JumpAction, EKeys::SpaceBar);
	DefaultMapping->MapKey(FireAction, EKeys::LeftMouseButton);
	MapAbilityKeys();
	DefaultMapping->MapKey(SprintAction, EKeys::LeftShift);
	DefaultMapping->MapKey(CrouchAction, EKeys::LeftControl);
	DefaultMapping->MapKey(ReloadAction, EKeys::R);
}

void AValdoriaHeroCharacter::MapAbilityKeys()
{
	// Teclas das habilidades conforme o esquema escolhido em Configurações > Controles.
	const FValdoriaOptions& Opt = FValdoriaOptions::Get();
	DefaultMapping->MapKey(SecondaryAction, Opt.GetSlotKey(EValdoriaAbilitySlot::SecondaryFire));
	DefaultMapping->MapKey(Ability1Action, Opt.GetSlotKey(EValdoriaAbilitySlot::Ability1));
	DefaultMapping->MapKey(Ability2Action, Opt.GetSlotKey(EValdoriaAbilitySlot::Ability2));
	DefaultMapping->MapKey(Ability3Action, Opt.GetSlotKey(EValdoriaAbilitySlot::Ability3));
	DefaultMapping->MapKey(UltimateAction, Opt.GetSlotKey(EValdoriaAbilitySlot::Ultimate));
	if (Opt.KeyScheme != 1)
	{
		DefaultMapping->MapKey(ProneAction, EKeys::X); // no estilo Valorant o X é a ultimate
	}
}

void AValdoriaHeroCharacter::RefreshKeyScheme()
{
	if (!DefaultMapping)
	{
		return;
	}
	for (UInputAction* Action : { SecondaryAction.Get(), Ability1Action.Get(), Ability2Action.Get(), Ability3Action.Get(), UltimateAction.Get(), ProneAction.Get() })
	{
		DefaultMapping->UnmapAllKeysFromAction(Action);
	}
	MapAbilityKeys();
	if (const APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->RequestRebuildControlMappings();
		}
	}
}

void AValdoriaHeroCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	BuildDefaultInput();
	if (const APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			// Limpa contextos de personagens anteriores (após renascer, o contexto do corpo antigo
			// ainda "segurava" o WASD e o herói novo ficava parado).
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(DefaultMapping, 10);
		}
	}
}

void AValdoriaHeroCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	BuildDefaultInput();
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		UE_LOG(LogValdoria, Error, TEXT("Enhanced Input não encontrado em %s"), *GetNameSafe(this));
		return;
	}
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AValdoriaHeroCharacter::Input_Move);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AValdoriaHeroCharacter::Input_Look);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AValdoriaHeroCharacter::Input_JumpPressed);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &AValdoriaHeroCharacter::Input_JumpReleased);
	Input->BindAction(SprintAction, ETriggerEvent::Started, this, &AValdoriaHeroCharacter::Input_SprintPressed);
	Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &AValdoriaHeroCharacter::Input_SprintReleased);
	Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &AValdoriaHeroCharacter::Input_CrouchPressed);
	Input->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AValdoriaHeroCharacter::Input_CrouchReleased);
	Input->BindAction(ProneAction, ETriggerEvent::Started, this, &AValdoriaHeroCharacter::Input_Prone);
	Input->BindAction(FireAction, ETriggerEvent::Started, this, &AValdoriaHeroCharacter::Input_FirePressed);
	Input->BindAction(FireAction, ETriggerEvent::Completed, this, &AValdoriaHeroCharacter::Input_FireReleased);
	Input->BindAction(SecondaryAction, ETriggerEvent::Started, this, &AValdoriaHeroCharacter::Input_Secondary);
	Input->BindAction(Ability1Action, ETriggerEvent::Started, this, &AValdoriaHeroCharacter::Input_Ability1);
	Input->BindAction(Ability2Action, ETriggerEvent::Started, this, &AValdoriaHeroCharacter::Input_Ability2);
	Input->BindAction(Ability3Action, ETriggerEvent::Started, this, &AValdoriaHeroCharacter::Input_Ability3);
	Input->BindAction(UltimateAction, ETriggerEvent::Started, this, &AValdoriaHeroCharacter::Input_Ultimate);
	Input->BindAction(ReloadAction, ETriggerEvent::Started, this, &AValdoriaHeroCharacter::Input_Reload);
}

void AValdoriaHeroCharacter::Input_Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (IsRiding())
	{
		RideForwardInput = Axis.Y; // on the Bifrost W only turns around (when looking back)
		return;
	}
	if (!Controller || bIsDead || HasStatus(EValdoriaStatus::Rooted) || HasStatus(EValdoriaStatus::Stunned))
	{
		return;
	}
	if (bNoclip)
	{
		// Voando: anda para onde a câmera aponta (inclusive para cima e para baixo).
		const FRotator View = Controller->GetControlRotation();
		AddMovementInput(View.Vector(), Axis.Y);
		AddMovementInput(FRotationMatrix(View).GetUnitAxis(EAxis::Y), Axis.X);
		return;
	}
	const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
}

void AValdoriaHeroCharacter::Input_Look(const FInputActionValue& Value)
{
	if (IsCursorFree())
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const FValdoriaOptions& Options = FValdoriaOptions::Get();
	AddControllerYawInput(Axis.X * Options.MouseSensitivity);
	AddControllerPitchInput((Options.bInvertY ? Axis.Y : -Axis.Y) * Options.MouseSensitivity);
}

bool AValdoriaHeroCharacter::IsCursorFree() const
{
	// Cursor solto (Alt ou menu de pausa): cliques e mouse não mexem no jogo.
	const APlayerController* PC = Cast<APlayerController>(Controller);
	return PC && PC->bShowMouseCursor;
}

void AValdoriaHeroCharacter::Input_FirePressed()
{
	if (IsCursorFree())
	{
		return;
	}
	bFireHeld = true;
	if (bIsSprinting && !GetMovementRules().bCanFireWhileSprinting)
	{
		SetSprinting(false);
	}
	TryActivateSlot(EValdoriaAbilitySlot::PrimaryFire);
}

void AValdoriaHeroCharacter::Input_FireReleased() { bFireHeld = false; }
void AValdoriaHeroCharacter::Input_Secondary() { TryActivateSlot(EValdoriaAbilitySlot::SecondaryFire); }
void AValdoriaHeroCharacter::Input_Ability1() { TryActivateSlot(EValdoriaAbilitySlot::Ability1); }
void AValdoriaHeroCharacter::Input_Ability2() { TryActivateSlot(EValdoriaAbilitySlot::Ability2); }
void AValdoriaHeroCharacter::Input_Ability3() { TryActivateSlot(EValdoriaAbilitySlot::Ability3); }
void AValdoriaHeroCharacter::Input_Ultimate() { TryActivateSlot(EValdoriaAbilitySlot::Ultimate); }
void AValdoriaHeroCharacter::Input_Reload() { StartReload(); }

void AValdoriaHeroCharacter::TryActivateSlot(EValdoriaAbilitySlot Slot)
{
	if (bIsDead)
	{
		return;
	}
	if (!CanUseSlot(Slot))
	{
		return;
	}
	// Atacar ou usar outra habilidade revela quem está invisível.
	if (bStealthActive && Slot != StealthSlot)
	{
		BreakStealth();
	}
	for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
	{
		if (Spec.InputID == static_cast<int32>(Slot))
		{
			if (AbilitySystem->TryActivateAbility(Spec.Handle))
			{
				PlayActionForSlot(Slot);
				PlaySlotSound(Slot);
			}
			return;
		}
	}
}

// ------------------------------------------------------------------ arma

bool AValdoriaHeroCharacter::CanFireWeapon() const
{
	if (!GetWeaponStats().bUsesAmmo)
	{
		return !bIsDead;
	}
	return !bIsDead && !bReloading && (CurrentAmmo > 0 || bInfiniteAmmo);
}

void AValdoriaHeroCharacter::ConsumeAmmo()
{
	if (!GetWeaponStats().bUsesAmmo)
	{
		return;
	}
	if (!bInfiniteAmmo)
	{
		CurrentAmmo = FMath::Max(0, CurrentAmmo - 1);
	}
	if (CurrentAmmo == 0 && IsLocallyControlled())
	{
		StartReload();
	}
}

void AValdoriaHeroCharacter::GetAimRay(FVector& OutStart, FVector& OutDirection) const
{
	if (bAutopilot)
	{
		OutStart = GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);
		OutDirection = AutopilotAim.Vector();
		return;
	}
	// Mira pelo centro da câmera; o raio começa na altura do personagem para não
	// acertar coisas entre a câmera e o herói.
	const FVector CamLoc = FollowCamera->GetComponentLocation();
	FVector Forward = FollowCamera->GetForwardVector();
	if (Controller)
	{
		Forward = Controller->GetControlRotation().Vector();
	}
	const float ToCharacter = FVector::DotProduct(GetActorLocation() - CamLoc, Forward);
	OutStart = CamLoc + Forward * FMath::Max(0.f, ToCharacter);
	OutDirection = Forward;
}

bool AValdoriaHeroCharacter::IsHeadshot(const FHitResult& Hit)
{
	const FString Bone = Hit.BoneName.ToString().ToLower();
	if (Bone.Contains(TEXT("head")) || Bone.Contains(TEXT("neck")))
	{
		return true;
	}
	if (const AValdoriaHeroCharacter* Hero = Cast<AValdoriaHeroCharacter>(Hit.GetActor()))
	{
		const float HalfHeight = Hero->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		return Hit.ImpactPoint.Z > Hero->GetActorLocation().Z + HalfHeight * 0.72f;
	}
	return false;
}

void AValdoriaHeroCharacter::StartReload()
{
	const FValdoriaWeaponStats& Weapon = GetWeaponStats();
	if (bIsDead || bReloading || !Weapon.bUsesAmmo || CurrentAmmo >= Weapon.MagazineSize)
	{
		return;
	}
	PlayReloadAnimation();
	if (ActiveDefinition && !ActiveDefinition->ReloadSound.IsNull())
	{
		ValdoriaAudio::PlayAt(this, ActiveDefinition->ReloadSound.LoadSynchronous(), GetActorLocation(), 0.6f);
	}
	bReloading = true;
	GetWorldTimerManager().SetTimer(ReloadTimer, this, &AValdoriaHeroCharacter::FinishReload, FMath::Max(0.05f, Weapon.ReloadTime), false);
	if (!HasAuthority())
	{
		ServerReload();
	}
}

void AValdoriaHeroCharacter::ServerReload_Implementation()
{
	StartReload();
}

void AValdoriaHeroCharacter::FinishReload()
{
	bReloading = false;
	CurrentAmmo = GetWeaponStats().MagazineSize;
}

// ------------------------------------------------------------------ recargas e ultimate

float AValdoriaHeroCharacter::GetCooldownRemaining(EValdoriaAbilitySlot Slot) const
{
	if (bNoCooldowns && Slot != EValdoriaAbilitySlot::PrimaryFire)
	{
		return 0.f;
	}
	const float* End = CooldownEndTimes.Find(Slot);
	return End ? FMath::Max(0.f, *End - GetWorld()->GetTimeSeconds()) : 0.f;
}

void AValdoriaHeroCharacter::StartCooldown(EValdoriaAbilitySlot Slot, float Seconds)
{
	// Redução de recarga da loja (o ritmo de tiro do botão esquerdo não muda).
	if (Slot != EValdoriaAbilitySlot::PrimaryFire)
	{
		Seconds *= 1.f - FMath::Clamp(ShopBonus.CooldownReduction, 0.f, 0.6f);
	}
	CooldownEndTimes.Add(Slot, GetWorld()->GetTimeSeconds() + FMath::Max(0.f, Seconds));
}

bool AValdoriaHeroCharacter::IsUltimateReady() const
{
	return ActiveDefinition && Attributes->GetUltimateCharge() >= ActiveDefinition->UltimateCost;
}

float AValdoriaHeroCharacter::GetUltimatePercent() const
{
	return ActiveDefinition ? FMath::Clamp(Attributes->GetUltimateCharge() / ActiveDefinition->UltimateCost, 0.f, 1.f) : 0.f;
}

void AValdoriaHeroCharacter::ConsumeUltimate()
{
	if (HasAuthority())
	{
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetUltimateChargeAttribute(), 0.f);
	}
}

// ------------------------------------------------------------------ vida, dano e morte

float AValdoriaHeroCharacter::GetHealth() const { return Attributes->GetHealth(); }
float AValdoriaHeroCharacter::GetMaxHealth() const { return Attributes->GetMaxHealth(); }
float AValdoriaHeroCharacter::GetArmor() const { return Attributes->GetArmor(); }
float AValdoriaHeroCharacter::GetMaxArmor() const { return Attributes->GetMaxArmor(); }
float AValdoriaHeroCharacter::GetShield() const { return Attributes->GetShield(); }
float AValdoriaHeroCharacter::GetMaxShield() const { return Attributes->GetMaxShield(); }

void AValdoriaHeroCharacter::OnDamageTaken(AValdoriaHeroCharacter* DamageInstigator, float Damage, bool bHeadshot)
{
	LastDamageTakenTime = GetWorld()->GetTimeSeconds();
	if (DamageInstigator && DamageInstigator != this)
	{
		LastAttacker = DamageInstigator;
		LastAttackedTime = LastDamageTakenTime;
		if (IsEnemy(DamageInstigator))
		{
			LastHarmedTime = LastDamageTakenTime;
			StopRiding(); // hit by an enemy: fall off the trail
		}
		if (const APlayerController* PC = Cast<APlayerController>(Controller))
		{
			if (AValdoriaHUD* HUD = Cast<AValdoriaHUD>(PC->GetHUD()))
			{
				HUD->AddDamageIndicator(DamageInstigator->GetActorLocation(), Damage);
			}
		}
	}
}

// ------------------------------------------------------------------ dano por tipo, cura e efeitos

const FValdoriaPassiveRules& AValdoriaHeroCharacter::GetPassive() const
{
	static const FValdoriaPassiveRules Default;
	return ActiveDefinition ? ActiveDefinition->Passive : Default;
}

float AValdoriaHeroCharacter::GetPhysicalPower() const { return (ActiveDefinition ? ActiveDefinition->PhysicalPower : 1.f) + ShopBonus.PhysicalPower; }
float AValdoriaHeroCharacter::GetMagicPower() const { return (ActiveDefinition ? ActiveDefinition->MagicPower : 1.f) + ShopBonus.MagicPower; }
float AValdoriaHeroCharacter::GetPhysicalResistance() const { return (ActiveDefinition ? ActiveDefinition->PhysicalResistance : 0.f) + ShopBonus.PhysicalResistance; }
float AValdoriaHeroCharacter::GetMagicResistance() const { return (ActiveDefinition ? ActiveDefinition->MagicResistance : 0.f) + ShopBonus.MagicResistance; }

void AValdoriaHeroCharacter::RebuildBoostedWeapon()
{
	if (!ActiveDefinition)
	{
		return;
	}
	BoostedWeapon = ActiveDefinition->Weapon;
	BoostedWeapon.MagazineSize = FMath::Max(1, FMath::RoundToInt(BoostedWeapon.MagazineSize * (1.f + ShopBonus.MagazineSize)));
}

void AValdoriaHeroCharacter::SetShopBonus(const FValdoriaShopBonus& Bonus)
{
	const float OldExtraHealth = ShopBonus.MaxHealth;
	ShopBonus = Bonus;
	RebuildBoostedWeapon();
	UpdateMovementSpeed();
	if (HasAuthority() && ActiveDefinition)
	{
		// Vida máxima nova; a vida atual sobe junto com o que foi ganho.
		const float NewMax = ActiveDefinition->MaxHealth + ShopBonus.MaxHealth;
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetMaxHealthAttribute(), NewMax);
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetHealthAttribute(),
			FMath::Clamp(GetHealth() + (ShopBonus.MaxHealth - OldExtraHealth), 1.f, NewMax));
	}
}

float AValdoriaHeroCharacter::DealTypedDamage(AActor* Target, float BaseDamage, EValdoriaDamageType Type, float MagicShare, bool bHeadshot)
{
	if (!HasAuthority() || !Target || BaseDamage <= 0.f)
	{
		return 0.f;
	}
	const float Magic = Type == EValdoriaDamageType::Physical ? 0.f : (Type == EValdoriaDamageType::Magical ? 1.f : FMath::Clamp(MagicShare, 0.f, 1.f));
	float Physical = BaseDamage * (1.f - Magic) * GetPhysicalPower();
	float Magical = BaseDamage * Magic * GetMagicPower();

	// Barreira inimiga: absorve o dano (sem resistências).
	if (AValdoriaBarrier* Barrier = Cast<AValdoriaBarrier>(Target))
	{
		if (Barrier->TeamId != TeamId)
		{
			Barrier->AbsorbDamage((Physical + Magical) * FMath::Max(0.f, GetDamageMultiplier()));
		}
		return 0.f;
	}
	AValdoriaHeroCharacter* Victim = Cast<AValdoriaHeroCharacter>(Target);
	if (!Victim || !IsEnemy(Victim))
	{
		return 0.f;
	}

	// Passivas de quem ataca.
	const FValdoriaPassiveRules& Mine = GetPassive();
	float Scale = 1.f;
	if (bHeadshot)
	{
		Scale *= 1.f + Mine.HeadshotBonus + ShopBonus.HeadshotDamage;
	}
	if (Mine.BackstabMultiplier > 1.f)
	{
		const FVector ToMe = (GetActorLocation() - Victim->GetActorLocation()).GetSafeNormal2D();
		if (FVector::DotProduct(Victim->GetActorForwardVector().GetSafeNormal2D(), ToMe) < -0.3f)
		{
			Scale *= Mine.BackstabMultiplier;
		}
	}
	// Refletir: o dano não entra e metade volta para quem atacou (sem refletir de novo).
	if (Victim->IsReflecting())
	{
		static bool bInReflect = false;
		if (!bInReflect)
		{
			bInReflect = true;
			Victim->DealTypedDamage(this, BaseDamage * 0.5f, Type, MagicShare, false);
			bInReflect = false;
		}
		return 0.f;
	}
	// Intangível: dano físico não acerta; o mágico sim.
	if (Victim->IsIntangible())
	{
		Physical = 0.f;
	}
	// Resistências e passiva de quem apanha.
	Physical *= 1.f - FMath::Clamp(Victim->GetPhysicalResistance(), 0.f, 0.9f);
	Magical *= 1.f - FMath::Clamp(Victim->GetMagicResistance(), 0.f, 0.9f);
	// Reductions stack multiplicatively and are capped at 75% so buffs never make anyone immortal.
	const float Kept = FMath::Max(0.25f, (1.f - Victim->GetPassiveDamageReduction()) * (1.f - Victim->GetTeamDamageReduction()));
	float Total = (Physical + Magical) * Scale * Kept * (1.f + GetTeamDamageBonus());
	if (ShopBonus.AntiHeal > 0.f)
	{
		Victim->ApplyAntiHeal(ShopBonus.AntiHeal, 3.f); // shop: cuts the target's healing for 3 s
	}
	// Absorver: parte do dano some e vira carga de ultimate.
	if (Victim->IsAbsorbing() && Total > 0.f)
	{
		const float Absorbed = Total * FMath::Clamp(Victim->AbsorbFraction, 0.f, 1.f);
		Total -= Absorbed;
		Victim->AddUltimateCharge(Absorbed * Victim->AbsorbChargePerPoint);
	}
	if (Total <= 0.f)
	{
		return 0.f;
	}

	// Proteção de aliado (ex.: Brasa): um aliado perto do alvo assume parte do dano.
	for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
	{
		AValdoriaHeroCharacter* Guard = *It;
		const FValdoriaPassiveRules& GuardPassive = Guard->GetPassive();
		if (Guard == Victim || Guard->IsDead() || Guard->TeamId != Victim->TeamId || GuardPassive.AllyDamageShare <= 0.f
			|| FVector::Dist(Guard->GetActorLocation(), Victim->GetActorLocation()) > GuardPassive.Radius)
		{
			continue;
		}
		const float Shared = Total * GuardPassive.AllyDamageShare;
		Total -= Shared;
		DealDamageTo(Guard, Shared);
		break;
	}
	LastCombatTime = GetWorld()->GetTimeSeconds();
	DealDamageTo(Victim, Total, bHeadshot);
	return Total;
}

void AValdoriaHeroCharacter::HealTarget(AActor* Target, float Amount)
{
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	const AValdoriaHeroCharacter* Other = Cast<AValdoriaHeroCharacter>(Target);
	if (!HasAuthority() || !TargetASC || Amount <= 0.f || !Other || Other->IsDead() || Other->GetHealth() >= Other->GetMaxHealth())
	{
		return;
	}
	FGameplayEffectSpecHandle Spec = AbilitySystem->MakeOutgoingSpec(UValdoriaHealEffect::StaticClass(), 1.f, AbilitySystem->MakeEffectContext());
	if (Spec.IsValid())
	{
		const float Received = Other->GetHealingReceivedMultiplier();
		Spec.Data->SetSetByCallerMagnitude(ValdoriaTags::Data_Heal, Amount * (1.f + ShopBonus.HealingPower) * Received);
		if (Target != this)
		{
			LastCombatTime = GetWorld()->GetTimeSeconds(); // healing others counts as general combat
		}
		AbilitySystem->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
}

void AValdoriaHeroCharacter::ApplySlow(float Fraction, float Seconds)
{
	const float Now = GetWorld()->GetTimeSeconds();
	SlowFraction = FMath::Clamp(FMath::Max(Fraction, Now < SlowEndTime ? SlowFraction : 0.f), 0.f, 0.9f);
	SlowEndTime = FMath::Max(SlowEndTime, Now + Seconds);
	UpdateMovementSpeed();
}

void AValdoriaHeroCharacter::ApplySpeedBuff(float Fraction, float Seconds)
{
	const float Now = GetWorld()->GetTimeSeconds();
	BuffFraction = FMath::Max(Fraction, Now < BuffEndTime ? BuffFraction : 0.f);
	BuffEndTime = FMath::Max(BuffEndTime, Now + Seconds);
	UpdateMovementSpeed();
}

void AValdoriaHeroCharacter::StartStealth(float Seconds, float RevealDistance, EValdoriaAbilitySlot FromSlot)
{
	bStealthActive = true;
	bStealthRevealed = false;
	StealthEndTime = GetWorld()->GetTimeSeconds() + Seconds;
	StealthRevealDistance = RevealDistance;
	StealthSlot = FromSlot;
	UpdateStealthVisibility();
}

void AValdoriaHeroCharacter::BreakStealth()
{
	if (!bStealthActive)
	{
		return;
	}
	bStealthActive = false;
	bStealthRevealed = false;
	UpdateStealthVisibility();
}

void AValdoriaHeroCharacter::UpdateStealthVisibility()
{
	// Invisível = só o próprio jogador vê o corpo (os outros e os bots não).
	const bool bStealthHidden = IsHiddenByStealth();
	GetMesh()->SetOnlyOwnerSee(bStealthHidden);
	WeaponComponent->SetOnlyOwnerSee(bStealthHidden);
}

void AValdoriaHeroCharacter::SetWeaponOverride(const FValdoriaWeaponStats& Stats, float Seconds)
{
	if (!bHasWeaponOverride)
	{
		SavedAmmo = CurrentAmmo;
	}
	WeaponOverride = Stats;
	bHasWeaponOverride = true;
	WeaponOverrideEndTime = GetWorld()->GetTimeSeconds() + Seconds;
	bReloading = false;
	GetWorldTimerManager().ClearTimer(ReloadTimer);
	CurrentAmmo = Stats.MagazineSize;
}

void AValdoriaHeroCharacter::StartLeapSlam(float Damage, float Radius, EValdoriaDamageType Type)
{
	bPendingSlam = true;
	SlamDamage = Damage;
	SlamRadius = Radius;
	SlamType = Type;
	SlamTimeout = GetWorld()->GetTimeSeconds() + 3.f;
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	LaunchCharacter(Forward * 1100.f + FVector(0.f, 0.f, 950.f), true, true);
}

void AValdoriaHeroCharacter::DoLeapSlamImpact()
{
	bPendingSlam = false;
	const FVector Center = GetActorLocation();
	PlayAreaEffect(Center, SlamRadius, ActiveDefinition ? ActiveDefinition->ThemeColor : FLinearColor(1.f, 0.4f, 0.1f));
	if (!HasAuthority())
	{
		return;
	}
	for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
	{
		AValdoriaHeroCharacter* Other = *It;
		if (!IsEnemy(Other) || FVector::Dist(Other->GetActorLocation(), Center) > SlamRadius)
		{
			continue;
		}
		DealTypedDamage(Other, SlamDamage, SlamType);
		const FVector Push = (Other->GetActorLocation() - Center).GetSafeNormal2D() * 900.f + FVector(0.f, 0.f, 500.f);
		Other->LaunchCharacter(Push, true, true);
	}
}

void AValdoriaHeroCharacter::ApplyRecoil(float Degrees)
{
	if (Degrees <= 0.f || !IsLocallyControlled() || !Controller)
	{
		return;
	}
	FRotator Rot = Controller->GetControlRotation();
	Rot.Pitch += Degrees;
	Rot.Yaw += FMath::FRandRange(-0.3f, 0.3f) * Degrees;
	Controller->SetControlRotation(Rot);
}

float AValdoriaHeroCharacter::GetSpreadMultiplier() const
{
	const float Still = GetPassive().StillSpreadMultiplier;
	if (Still < 1.f && (bIsCrouched || GetVelocity().Size2D() < 50.f) && !GetCharacterMovement()->IsFalling())
	{
		return Still;
	}
	return 1.f;
}

void AValdoriaHeroCharacter::IgnoreFriendlyBarriers(FCollisionQueryParams& Params) const
{
	for (TActorIterator<AValdoriaBarrier> It(GetWorld()); It; ++It)
	{
		if (It->TeamId == TeamId)
		{
			Params.AddIgnoredActor(*It);
		}
	}
}

void AValdoriaHeroCharacter::IgnoreFriendlyBarriers(UPrimitiveComponent* Component) const
{
	for (TActorIterator<AValdoriaBarrier> It(GetWorld()); It; ++It)
	{
		if (It->TeamId == TeamId)
		{
			Component->IgnoreActorWhenMoving(*It, true);
		}
	}
}

void AValdoriaHeroCharacter::TickStatusEffects(float DeltaSeconds)
{
	const float Now = GetWorld()->GetTimeSeconds();
	if ((SlowFraction > 0.f && Now >= SlowEndTime) || (BuffFraction > 0.f && Now >= BuffEndTime))
	{
		if (Now >= SlowEndTime) SlowFraction = 0.f;
		if (Now >= BuffEndTime) BuffFraction = 0.f;
		UpdateMovementSpeed();
	}
	if (bHasWeaponOverride && Now >= WeaponOverrideEndTime)
	{
		bHasWeaponOverride = false;
		CurrentAmmo = FMath::Min(SavedAmmo, GetWeaponStats().MagazineSize);
	}
	if (bPendingSlam && Now >= SlamTimeout)
	{
		DoLeapSlamImpact();
	}
	if (bStealthActive)
	{
		if (Now >= StealthEndTime || bIsDead)
		{
			BreakStealth();
		}
		else
		{
			// Revela enquanto houver inimigo a menos de StealthRevealDistance.
			bool bNear = false;
			for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It && !bNear; ++It)
			{
				bNear = IsEnemy(*It) && FVector::Dist(It->GetActorLocation(), GetActorLocation()) <= StealthRevealDistance;
			}
			if (bNear != bStealthRevealed)
			{
				bStealthRevealed = bNear;
				UpdateStealthVisibility();
			}
		}
	}

	// Passivas de aliados e aura (4x por segundo, no servidor).
	PassiveTimer += DeltaSeconds;
	if (!HasAuthority() || bIsDead || PassiveTimer < 0.25f)
	{
		return;
	}
	const float Step = PassiveTimer;
	PassiveTimer = 0.f;
	if (GetTeamRegen() > 0.f)
	{
		HealTarget(this, GetTeamRegen() * Step);
	}
	const FValdoriaPassiveRules& P = GetPassive();
	if (P.DamageReductionPerAlly <= 0.f && P.AuraHealPerSecond <= 0.f && P.AllyDamageShare <= 0.f)
	{
		PassiveReduction = 0.f;
		AlliesNearby = 0;
		return;
	}
	int32 Count = 0;
	for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
	{
		AValdoriaHeroCharacter* Other = *It;
		if (Other == this || Other->IsDead() || Other->TeamId != TeamId || FVector::Dist(Other->GetActorLocation(), GetActorLocation()) > P.Radius)
		{
			continue;
		}
		++Count;
		if (P.AuraHealPerSecond > 0.f)
		{
			HealTarget(Other, P.AuraHealPerSecond * Step);
		}
	}
	if (P.AuraHealPerSecond > 0.f)
	{
		HealTarget(this, P.AuraHealPerSecond * Step);
	}
	AlliesNearby = Count;
	PassiveReduction = FMath::Min(P.MaxDamageReduction, P.DamageReductionPerAlly * Count);
}

void AValdoriaHeroCharacter::OnDamageDealt(AValdoriaHeroCharacter* Victim, float Damage, bool bHeadshot)
{
	if (ActiveDefinition && HasAuthority() && !IsUltimateReady())
	{
		const float Charge = Attributes->GetUltimateCharge() + Damage * ActiveDefinition->UltimateChargePerDamage;
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetUltimateChargeAttribute(), FMath::Min(Charge, ActiveDefinition->UltimateCost));
	}
	const bool bKill = Victim && Victim->GetHealth() <= 0.f;
	// Loja: roubo de vida e créditos por dano/abate.
	if (HasAuthority() && Victim && Victim != this)
	{
		if (ShopBonus.LifeSteal > 0.f)
		{
			HealTarget(this, Damage * ShopBonus.LifeSteal);
		}
		if (AValdoriaPlayerController* PC = Cast<AValdoriaPlayerController>(Controller))
		{
			PC->OnHeroDamageDealt(Damage, bKill);
		}
	}
	ClientNotifyHit(Victim ? Victim->GetActorLocation() + FVector(0, 0, 60.f) : GetActorLocation(), Damage, bHeadshot, bKill);
}

void AValdoriaHeroCharacter::ClientNotifyHit_Implementation(FVector_NetQuantize Location, float Damage, bool bHeadshot, bool bKill)
{
	// Confirmação sonora: clique curto (mais agudo na cabeça) e um som mais forte no abate.
	ValdoriaAudio::Play2D(this, ValdoriaAudio::Load(TEXT("UI"), bKill ? TEXT("metalLatch") : TEXT("metalClick")), bKill ? 0.7f : 0.35f, 0.04f, bHeadshot ? 1.35f : 1.f);
	if (const APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (AValdoriaHUD* HUD = Cast<AValdoriaHUD>(PC->GetHUD()))
		{
			HUD->AddHitMarker(Location, Damage, bHeadshot, bKill);
		}
	}
}

void AValdoriaHeroCharacter::HandleDeath(AValdoriaHeroCharacter* Killer)
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}
	bIsDead = true;
	bFireHeld = false;
	AbilitySystem->CancelAllAbilities();
	AbilitySystem->AddLooseGameplayTag(ValdoriaTags::State_Dead);
	OnRep_IsDead();
	if (AValdoriaGameMode* GM = GetWorld()->GetAuthGameMode<AValdoriaGameMode>())
	{
		GM->OnHeroKilled(this, Killer);
	}
}

void AValdoriaHeroCharacter::OnRep_IsDead()
{
	if (bIsDead)
	{
		EnterRagdoll();
	}
	else
	{
		LeaveRagdoll();
	}
}

void AValdoriaHeroCharacter::EnterRagdoll()
{
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetSimulatePhysics(true);
}

void AValdoriaHeroCharacter::LeaveRagdoll()
{
	GetMesh()->SetSimulatePhysics(false);
	GetMesh()->SetCollisionProfileName(TEXT("CharacterMesh"));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetMesh()->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	GetMesh()->SetRelativeTransform(DefaultMeshRelativeTransform);
	if (ActiveDefinition)
	{
		GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -ActiveDefinition->CapsuleHalfHeight + ActiveDefinition->HoverHeight));
	}
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

void AValdoriaHeroCharacter::ResetCombatState()
{
	if (HasAuthority())
	{
		bIsDead = false;
		AbilitySystem->RemoveLooseGameplayTag(ValdoriaTags::State_Dead);
		OnRep_IsDead();
		AdminRestoreAll();
	}
	CurrentAmmo = GetWeaponStats().MagazineSize;
	bReloading = false;
	CooldownEndTimes.Reset();
}

void AValdoriaHeroCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TickMovementFeatures(DeltaSeconds);
	TickStatusEffects(DeltaSeconds);
	TickHover(DeltaSeconds);
	TickFootsteps(DeltaSeconds);
	TickFlightPose(DeltaSeconds);
	TickAutopilot(DeltaSeconds);
	TickRiding(DeltaSeconds);
	if (!bIsDead)
	{
		UpdateMovementAnimation();
	}

	if (bFireHeld && IsLocallyControlled() && GetWeaponStats().bAutomatic)
	{
		TryActivateSlot(EValdoriaAbilitySlot::PrimaryFire);
	}

	if (HasAuthority() && !bIsDead && ActiveDefinition)
	{
		const float Now = GetWorld()->GetTimeSeconds();
		// Escudo regenera depois de um tempo sem levar dano.
		if (GetShield() < GetMaxShield() && Now - LastDamageTakenTime >= ActiveDefinition->ShieldRegenDelay)
		{
			AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetShieldAttribute(),
				FMath::Min(GetMaxShield(), GetShield() + ActiveDefinition->ShieldRegenRate * DeltaSeconds));
		}
		// Ultimate carrega aos poucos com o tempo.
		if (!IsUltimateReady() && ActiveDefinition->UltimateChargePerSecond > 0.f)
		{
			AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetUltimateChargeAttribute(),
				FMath::Min(ActiveDefinition->UltimateCost, Attributes->GetUltimateCharge() + ActiveDefinition->UltimateChargePerSecond * DeltaSeconds));
		}
	}
}

// ------------------------------------------------------------------ admin

void AValdoriaHeroCharacter::AdminSetDamageMultiplier(float Multiplier)
{
	if (HasAuthority())
	{
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetDamageMultiplierAttribute(), FMath::Max(0.f, Multiplier));
	}
}

void AValdoriaHeroCharacter::SetWeaponMesh(UStaticMesh* NewWeapon)
{
	WeaponComponent->SetStaticMesh(NewWeapon);
	WeaponComponent->SetVisibility(NewWeapon != nullptr && !bFirstPerson);
	if (NewWeapon && ActiveDefinition)
	{
		WeaponComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, ActiveDefinition->WeaponSocket);
		WeaponComponent->SetRelativeTransform(ActiveDefinition->WeaponOffset);
	}
}

void AValdoriaHeroCharacter::SetFirstPerson(bool bEnable)
{
	bFirstPerson = bEnable;
	if (bEnable)
	{
		// Câmera nos olhos; o próprio corpo fica invisível para você (os outros continuam vendo).
		CameraBoom->TargetArmLength = 0.f;
		CameraBoom->SocketOffset = FVector(10.f, 0.f, BaseEyeHeight);
	}
	else if (ActiveDefinition)
	{
		CameraBoom->TargetArmLength = ActiveDefinition->CameraDistance * FMath::Max(1.f, ActiveDefinition->CapsuleHalfHeight / 96.f);
		CameraBoom->SocketOffset = ActiveDefinition->CameraOffset * FVector(1.f, 1.f, ActiveDefinition->CapsuleHalfHeight / 96.f) + FVector(0.f, 0.f, ActiveDefinition->HoverHeight);
	}
	GetMesh()->SetOwnerNoSee(bEnable);
	WeaponComponent->SetOwnerNoSee(bEnable);
}

void AValdoriaHeroCharacter::SetNoclip(bool bEnable)
{
	bNoclip = bEnable;
	SetActorEnableCollision(!bEnable);
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (bEnable)
	{
		Move->SetMovementMode(MOVE_Flying);
		Move->MaxFlySpeed = 1500.f;
		Move->BrakingDecelerationFlying = 4000.f;
	}
	else
	{
		Move->SetMovementMode(MOVE_Falling);
	}
}

void AValdoriaHeroCharacter::DealDamageTo(AActor* Target, float Damage, bool bHeadshot)
{
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!HasAuthority() || !TargetASC)
	{
		return;
	}
	FGameplayEffectContextHandle Context = AbilitySystem->MakeEffectContext();
	Context.AddInstigator(this, this);
	FGameplayEffectSpecHandle Spec = AbilitySystem->MakeOutgoingSpec(UValdoriaDamageEffect::StaticClass(), 1.f, Context);
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(ValdoriaTags::Data_Damage, Damage * FMath::Max(0.f, GetDamageMultiplier()));
		if (bHeadshot)
		{
			Spec.Data->AddDynamicAssetTag(ValdoriaTags::Hit_Headshot);
		}
		AbilitySystem->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
}

void AValdoriaHeroCharacter::AdminSetSpeedMultiplier(float Multiplier)
{
	AdminSpeedMultiplier = FMath::Clamp(Multiplier, 0.1f, 5.f);
	UpdateMovementSpeed();
}

float AValdoriaHeroCharacter::GetDamageMultiplier() const
{
	return Attributes->GetDamageMultiplier();
}

void AValdoriaHeroCharacter::AdminFillUltimate()
{
	if (HasAuthority() && ActiveDefinition)
	{
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetUltimateChargeAttribute(), ActiveDefinition->UltimateCost);
	}
}

void AValdoriaHeroCharacter::AdminRestoreAll()
{
	if (!HasAuthority())
	{
		return;
	}
	AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetHealthAttribute(), GetMaxHealth());
	AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetArmorAttribute(), GetMaxArmor());
	AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetShieldAttribute(), GetMaxShield());
}

// ------------------------------------------------------------------ efeitos (versão cinza, sem assets)

void AValdoriaHeroCharacter::PlayShotEffects(const FVector& Impact, bool bHit)
{
	DrawShot(Impact, bHit);
	if (HasAuthority())
	{
		MulticastShotEffects(Impact, bHit);
	}
}

void AValdoriaHeroCharacter::MulticastShotEffects_Implementation(FVector_NetQuantize Impact, bool bHit)
{
	// O dono já desenhou na hora (predição local); o servidor também.
	if (!IsLocallyControlled() && !HasAuthority())
	{
		DrawShot(Impact, bHit);
	}
}

void AValdoriaHeroCharacter::DrawShot(const FVector& Impact, bool bHit) const
{
	// Raio brilhante da ponta da arma até o impacto, e um estouro no ponto atingido.
	FVector Muzzle = GetActorLocation() + FVector(0, 0, 50.f);
	if (WeaponComponent->GetStaticMesh() && WeaponComponent->IsVisible())
	{
		Muzzle = WeaponComponent->Bounds.Origin + GetActorForwardVector() * WeaponComponent->Bounds.BoxExtent.Size() * 0.6f;
	}
	else if (GetMesh()->DoesSocketExist(TEXT("hand_r")))
	{
		Muzzle = GetMesh()->GetSocketLocation(TEXT("hand_r"));
	}
	const FLinearColor Color = ActiveDefinition ? ActiveDefinition->ThemeColor : FLinearColor(1.f, 0.9f, 0.4f);
	AValdoriaFx::Beam(GetWorld(), Muzzle, Impact, Color, bHit ? 4.f : 2.5f);
	AValdoriaFx::Burst(GetWorld(), Impact, bHit ? 45.f : 25.f, bHit ? FLinearColor::White : Color, 0.15f);
	if (bHit)
	{
		static const TCHAR* Hits[] = { TEXT("impactGeneric_light_000"), TEXT("impactGeneric_light_001"), TEXT("impactGeneric_light_002") };
		ValdoriaAudio::PlayAt(this, ValdoriaAudio::Load(TEXT("Impacts"), Hits[FMath::RandRange(0, 2)]), Impact, 0.35f);
	}
}

void AValdoriaHeroCharacter::PlayAreaEffect(const FVector& Center, float Radius, FLinearColor Color)
{
	DrawArea(Center, Radius, Color);
	if (HasAuthority())
	{
		MulticastAreaEffect(Center, Radius, Color);
	}
}

void AValdoriaHeroCharacter::MulticastAreaEffect_Implementation(FVector_NetQuantize Center, float Radius, FLinearColor Color)
{
	if (!IsLocallyControlled() && !HasAuthority())
	{
		DrawArea(Center, Radius, Color);
	}
}

void AValdoriaHeroCharacter::DrawArea(const FVector& Center, float Radius, FLinearColor Color) const
{
	// Anel no chão que se expande + bolha fraca.
	FVector Ground = Center;
	FHitResult Down;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ValdoriaFxGround), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Down, Center, Center - FVector(0, 0, 400.f), ECC_WorldStatic, Params))
	{
		Ground = Down.ImpactPoint + FVector(0, 0, 3.f);
	}
	AValdoriaFx::Ring(GetWorld(), Ground, Radius, Color);
}

// ------------------------------------------------------------------ movimento avançado
// Regras por herói em FValdoriaMovementRules (Data Asset > Movimento).

const FValdoriaMovementRules& AValdoriaHeroCharacter::GetMovementRules() const
{
	static const FValdoriaMovementRules Default;
	return ActiveDefinition ? ActiveDefinition->Movement : Default;
}

void AValdoriaHeroCharacter::UpdateMovementSpeed()
{
	if (!ActiveDefinition)
	{
		return;
	}
	const FValdoriaMovementRules& Rules = GetMovementRules();
	UCharacterMovementComponent* Move = GetCharacterMovement();
	float Speed = ActiveDefinition->MoveSpeed * AdminSpeedMultiplier * (1.f - SlowFraction) * (1.f + BuffFraction) * (1.f + ShopBonus.MoveSpeed);
	if (bIsSprinting)
	{
		Speed *= Rules.SprintMultiplier;
	}
	Move->MaxWalkSpeed = Speed;
	if (!bSliding)
	{
		Move->MaxWalkSpeedCrouched = bIsProne ? Rules.ProneSpeed : Rules.CrouchSpeed;
	}
}

void AValdoriaHeroCharacter::SetSprinting(bool bNewSprinting)
{
	const EValdoriaShiftMode Mode = GetMovementRules().ShiftMode;
	const bool bAllowed = Mode == EValdoriaShiftMode::Sprint || Mode == EValdoriaShiftMode::SprintAndDash;
	bNewSprinting = bNewSprinting && bAllowed && !bIsDead;
	if (bNewSprinting)
	{
		if (bIsProne)
		{
			SetProne(false);
		}
		if (bIsCrouched && !bSliding)
		{
			UnCrouch();
		}
	}
	bIsSprinting = bNewSprinting;
	UpdateMovementSpeed();
	if (!HasAuthority() && IsLocallyControlled())
	{
		ServerSetSprinting(bNewSprinting);
	}
}

void AValdoriaHeroCharacter::ServerSetSprinting_Implementation(bool bNewSprinting)
{
	SetSprinting(bNewSprinting);
}

void AValdoriaHeroCharacter::Input_SprintPressed()
{
	const FValdoriaMovementRules& Rules = GetMovementRules();
	const float Now = GetWorld()->GetTimeSeconds();
	switch (Rules.ShiftMode)
	{
	case EValdoriaShiftMode::Sprint:
		SetSprinting(true);
		break;
	case EValdoriaShiftMode::Dash:
		TryDash();
		break;
	case EValdoriaShiftMode::SprintAndDash:
		if (Now - LastShiftPressTime <= Rules.DoubleTapWindow)
		{
			TryDash();
			LastShiftPressTime = -10.f;
		}
		else
		{
			LastShiftPressTime = Now;
		}
		SetSprinting(true);
		break;
	default:
		break;
	}
}

void AValdoriaHeroCharacter::Input_SprintReleased()
{
	if (bIsSprinting)
	{
		SetSprinting(false);
	}
}

void AValdoriaHeroCharacter::TryDash()
{
	if (bIsDead || bMantling || DashChargesLeft <= 0)
	{
		return;
	}
	FVector Direction = GetLastMovementInputVector().GetSafeNormal2D();
	if (Direction.IsNearlyZero())
	{
		Direction = GetActorForwardVector().GetSafeNormal2D();
	}
	PerformDash(Direction);
	if (!HasAuthority())
	{
		ServerDash(Direction);
	}
}

void AValdoriaHeroCharacter::ServerDash_Implementation(FVector_NetQuantizeNormal Direction)
{
	if (DashChargesLeft > 0)
	{
		PerformDash(FVector(Direction).GetSafeNormal2D());
	}
}

void AValdoriaHeroCharacter::PerformDash(const FVector& Direction)
{
	if (bIsProne)
	{
		SetProne(false);
	}
	if (!bInfiniteDashes)
	{
		--DashChargesLeft;
	}
	PlayDashAnimation(Direction);
	const bool bGrounded = GetCharacterMovement()->IsMovingOnGround();
	LaunchCharacter(Direction * GetMovementRules().DashImpulse + FVector(0.f, 0.f, bGrounded ? 120.f : 0.f), true, true);
	DrawArea(GetActorLocation(), 120.f, FLinearColor(0.6f, 0.9f, 1.f));
}

void AValdoriaHeroCharacter::Input_CrouchPressed()
{
	bCrouchHeld = true;
	const FValdoriaMovementRules& Rules = GetMovementRules();
	if (bIsProne)
	{
		SetProne(false); // Ctrl deitado = sobe para agachado
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	const bool bFastEnough = ActiveDefinition && GetVelocity().Size2D() > ActiveDefinition->MoveSpeed * 1.1f;
	if (Rules.bCanSlide && bIsSprinting && bFastEnough && GetCharacterMovement()->IsMovingOnGround()
		&& Now - LastSlideTime >= Rules.SlideCooldown)
	{
		StartSlide();
		if (!HasAuthority())
		{
			ServerStartSlide();
		}
		return;
	}
	if (Rules.bCanCrouch)
	{
		if (bIsSprinting)
		{
			SetSprinting(false);
		}
		Crouch();
	}
}

void AValdoriaHeroCharacter::Input_CrouchReleased()
{
	bCrouchHeld = false;
	if (!bIsProne && !bSliding)
	{
		UnCrouch();
	}
}

void AValdoriaHeroCharacter::StartSlide()
{
	const FValdoriaMovementRules& Rules = GetMovementRules();
	UCharacterMovementComponent* Move = GetCharacterMovement();
	bSliding = true;
	LastSlideTime = GetWorld()->GetTimeSeconds();
	SlideEndTime = LastSlideTime + Rules.SlideDuration;
	bIsSprinting = false;
	Crouch();
	Move->GroundFriction = 0.6f;
	Move->BrakingDecelerationWalking = 350.f;
	Move->MaxWalkSpeedCrouched = ActiveDefinition->MoveSpeed * Rules.SprintMultiplier * 1.2f;
	const FVector Dir = GetVelocity().GetSafeNormal2D();
	LaunchCharacter(Dir * Rules.SlideImpulse, false, false);
}

void AValdoriaHeroCharacter::ServerStartSlide_Implementation()
{
	if (!bSliding)
	{
		StartSlide();
	}
}

void AValdoriaHeroCharacter::EndSlide()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	bSliding = false;
	Move->GroundFriction = 8.f;
	Move->BrakingDecelerationWalking = 2400.f;
	UpdateMovementSpeed();
	if (!bCrouchHeld && !bIsProne)
	{
		UnCrouch();
	}
}

void AValdoriaHeroCharacter::Input_Prone()
{
	if (!GetMovementRules().bCanProne)
	{
		return;
	}
	SetProne(!bIsProne);
}

void AValdoriaHeroCharacter::SetProne(bool bNewProne)
{
	if (bNewProne && (bIsDead || !GetCharacterMovement()->IsMovingOnGround()))
	{
		return;
	}
	bIsProne = bNewProne;
	if (bIsProne)
	{
		bIsSprinting = false;
	}
	OnRep_IsProne();
	if (!HasAuthority() && IsLocallyControlled())
	{
		ServerSetProne(bNewProne);
	}
}

void AValdoriaHeroCharacter::ServerSetProne_Implementation(bool bNewProne)
{
	SetProne(bNewProne);
}

void AValdoriaHeroCharacter::OnRep_IsProne()
{
	// Deitar usa o sistema de agachar da Unreal com uma cápsula ainda menor.
	const FValdoriaMovementRules& Rules = GetMovementRules();
	GetCharacterMovement()->SetCrouchedHalfHeight(bIsProne ? Rules.ProneHalfHeight : Rules.CrouchHalfHeight);
	UpdateMovementSpeed();
	if (bIsCrouched)
	{
		UnCrouch();
		bPendingRecrouch = bIsProne || bCrouchHeld;
	}
	else if (bIsProne)
	{
		Crouch();
	}
}

void AValdoriaHeroCharacter::Input_JumpPressed()
{
	if (IsRiding())
	{
		StopRiding(); // tap Space again to get off
		return;
	}
	JumpPressTime = GetWorld()->GetTimeSeconds();
	bTriedMountThisPress = false;
	if (HasStatus(EValdoriaStatus::Rooted) || HasStatus(EValdoriaStatus::Stunned))
	{
		return;
	}
	bJumpHeld = true;
	if (!HasAuthority())
	{
		ServerSetJumpHeld(true);
	}
	if (bIsDead || bMantling)
	{
		return;
	}
	if (bIsProne)
	{
		SetProne(false);
		return;
	}
	FVector Target;
	if (GetMovementRules().bCanMantle && FindMantleTarget(Target))
	{
		StartMantle(Target);
		if (!HasAuthority())
		{
			ServerStartMantle(Target);
		}
		return;
	}
	if (bIsCrouched)
	{
		UnCrouch();
	}
	Jump();
}

void AValdoriaHeroCharacter::Input_JumpReleased()
{
	bJumpHeld = false;
	StopJumping();
	if (!HasAuthority())
	{
		ServerSetJumpHeld(false);
	}
}

void AValdoriaHeroCharacter::ServerSetJumpHeld_Implementation(bool bHeld)
{
	bJumpHeld = bHeld;
}

bool AValdoriaHeroCharacter::IsFacingWall(float Distance) const
{
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	const FVector Start = GetActorLocation();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ValdoriaWall), false, this);
	FHitResult Hit;
	return GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Forward * (GetCapsuleComponent()->GetScaledCapsuleRadius() + Distance),
		ECC_WorldStatic, Params) && Hit.ImpactNormal.Z < 0.3f;
}

bool AValdoriaHeroCharacter::FindMantleTarget(FVector& OutTarget) const
{
	const FValdoriaMovementRules& Rules = GetMovementRules();
	const float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, HalfHeight);
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ValdoriaMantle), false, this);

	// 1) Tem parede logo à frente?
	FHitResult WallHit;
	const FVector WallStart = Feet + FVector(0.f, 0.f, 60.f);
	if (!GetWorld()->LineTraceSingleByChannel(WallHit, WallStart, WallStart + Forward * (Radius + 70.f), ECC_WorldStatic, Params)
		|| WallHit.ImpactNormal.Z > 0.3f)
	{
		return false;
	}

	// 2) Procura o topo da beirada, de cima para baixo.
	const FVector Over = WallHit.ImpactPoint + Forward * (Radius + 15.f);
	const FVector DownStart(Over.X, Over.Y, Feet.Z + Rules.MaxMantleHeight + 25.f);
	const FVector DownEnd(Over.X, Over.Y, Feet.Z + 35.f);
	FHitResult TopHit;
	if (!GetWorld()->LineTraceSingleByChannel(TopHit, DownStart, DownEnd, ECC_WorldStatic, Params)
		|| TopHit.ImpactNormal.Z < 0.7f || TopHit.bStartPenetrating)
	{
		return false;
	}
	const float LedgeHeight = TopHit.ImpactPoint.Z - Feet.Z;
	if (LedgeHeight < 40.f || LedgeHeight > Rules.MaxMantleHeight)
	{
		return false;
	}

	// 3) Cabe em pé lá em cima?
	const FVector Target = TopHit.ImpactPoint + FVector(0.f, 0.f, HalfHeight + 2.f);
	if (GetWorld()->OverlapBlockingTestByChannel(Target, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeCapsule(Radius, HalfHeight), Params))
	{
		return false;
	}
	OutTarget = Target;
	return true;
}

void AValdoriaHeroCharacter::StartMantle(const FVector& Target)
{
	bMantling = true;
	bWallClimbing = false;
	MantleElapsed = 0.f;
	MantleStart = GetActorLocation();
	MantleTarget = Target;
	if (bIsCrouched)
	{
		UnCrouch();
	}
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
}

void AValdoriaHeroCharacter::ServerStartMantle_Implementation(FVector_NetQuantize Target)
{
	// O servidor confere se a beirada existe mesmo.
	FVector ServerTarget;
	if (!bMantling && FindMantleTarget(ServerTarget) && FVector::Dist(ServerTarget, Target) < 150.f)
	{
		StartMantle(ServerTarget);
	}
}

void AValdoriaHeroCharacter::TickMovementFeatures(float DeltaSeconds)
{
	if (IsRiding())
	{
		return; // the trail moves the hero
	}
	if (bIsDead || !ActiveDefinition || bNoclip)
	{
		return;
	}
	const FValdoriaMovementRules& Rules = GetMovementRules();
	UCharacterMovementComponent* Move = GetCharacterMovement();

	// Recupera cargas de impulso, uma por vez.
	const int32 MaxCharges = FMath::Clamp(Rules.DashCharges, 1, 5);
	if (DashChargesLeft < MaxCharges)
	{
		DashRechargeProgress += DeltaSeconds;
		if (DashRechargeProgress >= Rules.DashRechargeTime)
		{
			DashRechargeProgress = 0.f;
			++DashChargesLeft;
		}
	}

	// Volta a agachar/deitar depois de trocar a altura da cápsula.
	if (bPendingRecrouch && !bIsCrouched)
	{
		bPendingRecrouch = false;
		Crouch();
	}

	if (bSliding && (GetWorld()->GetTimeSeconds() >= SlideEndTime || !Move->IsMovingOnGround()))
	{
		EndSlide();
	}

	// Correr só vale indo para a frente (ou quase).
	if (bIsSprinting && IsLocallyControlled())
	{
		const FVector Input = GetLastMovementInputVector().GetSafeNormal2D();
		if (!Input.IsNearlyZero() && FVector::DotProduct(Input, GetActorForwardVector()) < 0.3f)
		{
			SetSprinting(false);
		}
	}

	// Subindo em beirada: sobe primeiro, depois avança.
	if (bMantling)
	{
		MantleElapsed += DeltaSeconds;
		const float Alpha = FMath::Clamp(MantleElapsed / FMath::Max(0.05f, Rules.MantleDuration), 0.f, 1.f);
		const float Up = FMath::Min(1.f, Alpha * 1.6f);
		const float Ahead = FMath::Clamp((Alpha - 0.35f) / 0.65f, 0.f, 1.f);
		const FVector P(
			FMath::Lerp(MantleStart.X, MantleTarget.X, Ahead),
			FMath::Lerp(MantleStart.Y, MantleTarget.Y, Ahead),
			FMath::Lerp(MantleStart.Z, MantleTarget.Z, Up));
		SetActorLocation(P, false, nullptr, ETeleportType::None);
		if (Alpha >= 1.f)
		{
			bMantling = false;
			Move->SetMovementMode(MOVE_Walking);
		}
		return;
	}

	// --- Voar e planar (regras do Data Asset)
	const float Now = GetWorld()->GetTimeSeconds();
	if (FlightFuel < 0.f)
	{
		FlightFuel = Rules.FlightFuelSeconds;
	}
	if (Move->IsMovingOnGround())
	{
		FlightFuel = FMath::Min(Rules.FlightFuelSeconds, FlightFuel + Rules.FlightFuelRegen * DeltaSeconds);
	}
	const bool bAbilityFlight = Rules.FlightMode == EValdoriaFlightMode::Ability && Now < AbilityFlightEnd;
	const bool bFuelFlight = Rules.FlightMode == EValdoriaFlightMode::Fuel && bJumpHeld && (Move->IsFalling() || bFlying) && FlightFuel > 0.f && !IsFacingWall(45.f);
	if (bAbilityFlight || bFuelFlight)
	{
		if (!bFlying)
		{
			bFlying = true;
			Move->SetMovementMode(MOVE_Flying);
			Move->MaxFlySpeed = Rules.FlightSpeed;
			Move->BrakingDecelerationFlying = 1500.f;
		}
		if (bFuelFlight)
		{
			FlightFuel = FMath::Max(0.f, FlightFuel - DeltaSeconds);
			AddMovementInput(FVector::UpVector, 1.f); // segurar Espaço sobe
		}
		else if (bJumpHeld)
		{
			AddMovementInput(FVector::UpVector, 1.f);
		}
		else if (bCrouchHeld)
		{
			AddMovementInput(FVector::UpVector, -1.f);
		}
		bGliding = false;
		return;
	}
	if (bFlying)
	{
		StopFlight();
	}
	// Planar: caindo com Espaço segurado, a queda fica lenta.
	bGliding = false;
	if (Rules.bCanGlide && bJumpHeld && Move->IsFalling() && Move->Velocity.Z < -Rules.GlideFallSpeed && !IsFacingWall(45.f))
	{
		bGliding = true;
		Move->Velocity.Z = -Rules.GlideFallSpeed;
		Move->AirControl = Rules.GlideAirControl;
	}
	else if (ActiveDefinition)
	{
		Move->AirControl = ActiveDefinition->AirControl;
	}

	// Escalar parede: segurar Espaço no ar, de frente para uma parede.
	bWallClimbing = false;
	if (Rules.bCanWallClimb && bJumpHeld && Move->IsFalling() && WallClimbTimeUsed < Rules.MaxWallClimbTime && IsFacingWall(45.f))
	{
		bWallClimbing = true;
		WallClimbTimeUsed += DeltaSeconds;
		Move->Velocity.Z = Rules.WallClimbSpeed;
	}
	// No ar, segurando Espaço perto de uma beirada: sobe nela.
	if (Rules.bCanMantle && bJumpHeld && Move->IsFalling() && Move->Velocity.Z < 200.f + (bWallClimbing ? Rules.WallClimbSpeed : 0.f)
		&& IsLocallyControlled())
	{
		FVector Target;
		if (FindMantleTarget(Target))
		{
			StartMantle(Target);
			if (!HasAuthority())
			{
				ServerStartMantle(Target);
			}
		}
	}
}

void AValdoriaHeroCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	WallClimbTimeUsed = 0.f;
	bWallClimbing = false;
	if (bPendingSlam)
	{
		DoLeapSlamImpact();
	}
	// Landing after a flight.
	if (ActiveDefinition && bLandAfterFlight && ActiveDefinition->Animations.FlightStyle == EValdoriaFlightStyle::Superhero)
	{
		bLandAfterFlight = false;
		if (UValdoriaAnimInstance* LandAnim = Cast<UValdoriaAnimInstance>(GetMesh()->GetAnimInstance()))
		{
			const TSoftObjectPtr<UAnimSequenceBase>& Land = ActiveDefinition->Animations.LandAction;
			LandAnim->PlayAction(Land.IsNull() ? nullptr : Land.LoadSynchronous(), 1.2f, true);
		}
	}
}

// ------------------------------------------------------------------ animações de movimento
// Tocadas por cima do Animation Blueprint (slot do FValdoriaAnimationSet). Assim o mesmo
// conjunto serve para qualquer malha no esqueleto padrão da Unreal.

UAnimSequenceBase* AValdoriaHeroCharacter::PickMovementAnimation(bool& bOutLoop) const
{
	bOutLoop = true;
	if (!ActiveDefinition)
	{
		return nullptr;
	}
	const FValdoriaAnimationSet& Set = ActiveDefinition->Animations;
	const FVector Velocity = GetVelocity();
	const float Speed = Velocity.Size2D();
	auto Load = [](const TSoftObjectPtr<UAnimSequenceBase>& Ptr) { return Ptr.IsNull() ? nullptr : Ptr.LoadSynchronous(); };

	if (bMantling)
	{
		bOutLoop = false;
		return Load(Set.Mantle);
	}
	if (bWallClimbing)
	{
		return Load(Set.WallClimb);
	}
	if (IsSkating() && GetCharacterMovement()->IsMovingOnGround() && !Set.CrouchIdle.IsNull())
	{
		return Load(Set.CrouchIdle); // knees bent, no steps (reference video: sliding on a light trail)
	}
	if (bFlying)
	{
		// Levitate: calm upright pose (owner reference video), superhero: flying loop.
		const TSoftObjectPtr<UAnimSequenceBase>& FlyClip = Set.FlightStyle == EValdoriaFlightStyle::Levitate && !Set.Hover.IsNull() ? Set.Hover : Set.Fly;
		if (!FlyClip.IsNull())
		{
			return Load(FlyClip);
		}
	}
	// Flutuar: pairando sem passos (qualquer velocidade), a não ser agachado.
	if (ActiveDefinition->Locomotion == EValdoriaLocomotion::Float && !bIsCrouched && !bSliding && !Set.Hover.IsNull())
	{
		return Load(Set.Hover);
	}
	if (bIsProne)
	{
		UAnimSequenceBase* Crawl = Load(Set.ProneCrawl);
		return (Speed > 20.f && Crawl) ? Crawl : Load(Set.ProneIdle);
	}
	if (bSliding)
	{
		UAnimSequenceBase* SlideAnim = Load(Set.Slide);
		return SlideAnim ? SlideAnim : Load(Set.CrouchIdle);
	}
	if (bIsCrouched)
	{
		if (Speed < 20.f)
		{
			return Load(Set.CrouchIdle);
		}
		const float Fwd = FVector::DotProduct(Velocity, GetActorForwardVector());
		const float Right = FVector::DotProduct(Velocity, GetActorRightVector());
		if (FMath::Abs(Fwd) >= FMath::Abs(Right))
		{
			return Load(Fwd >= 0.f ? Set.CrouchWalkForward : Set.CrouchWalkBackward);
		}
		return Load(Right >= 0.f ? Set.CrouchWalkRight : Set.CrouchWalkLeft);
	}
	return nullptr;
}

void AValdoriaHeroCharacter::UpdateMovementAnimation()
{
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance || !ActiveDefinition || AnimInstance->IsA<UValdoriaAnimInstance>())
	{
		return;
	}
	// As animações extras não devem mover o personagem (o movimento é do código).
	AnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);

	bool bLoop = true;
	UAnimSequenceBase* Desired = PickMovementAnimation(bLoop);
	if (Desired == CurrentMovementAnim.Get())
	{
		return;
	}
	const FName Slot = ActiveDefinition->Animations.SlotName;
	CurrentMovementAnim = Desired;
	if (Desired)
	{
		AnimInstance->PlaySlotAnimationAsDynamicMontage(Desired, Slot, 0.15f, 0.2f, 1.f, bLoop ? 10000 : 1);
	}
	else
	{
		AnimInstance->StopSlotAnimation(0.2f, Slot);
	}
}

void AValdoriaHeroCharacter::PlayDashAnimation(const FVector& Direction)
{
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	UValdoriaAnimInstance* Valdoria = Cast<UValdoriaAnimInstance>(AnimInstance);
	if (Valdoria && ActiveDefinition && !bIsCrouched && !bIsProne)
	{
		const FValdoriaAnimationSet& DashSet = ActiveDefinition->Animations;
		const float DFwd = FVector::DotProduct(Direction, GetActorForwardVector());
		const float DRight = FVector::DotProduct(Direction, GetActorRightVector());
		const TSoftObjectPtr<UAnimSequenceBase>& DashPtr = FMath::Abs(DFwd) >= FMath::Abs(DRight)
			? (DFwd >= 0.f ? DashSet.DashForward : DashSet.DashBackward) : (DRight >= 0.f ? DashSet.DashRight : DashSet.DashLeft);
		Valdoria->PlayAction(DashPtr.IsNull() ? nullptr : DashPtr.LoadSynchronous(), 1.2f, true);
		return;
	}
	if (!AnimInstance || !ActiveDefinition || CurrentMovementAnim.IsValid())
	{
		return; // agachado/deitado já tem animação própria
	}
	const FValdoriaAnimationSet& Set = ActiveDefinition->Animations;
	const float Fwd = FVector::DotProduct(Direction, GetActorForwardVector());
	const float Right = FVector::DotProduct(Direction, GetActorRightVector());
	const TSoftObjectPtr<UAnimSequenceBase>& Ptr = FMath::Abs(Fwd) >= FMath::Abs(Right)
		? (Fwd >= 0.f ? Set.DashForward : Set.DashBackward)
		: (Right >= 0.f ? Set.DashRight : Set.DashLeft);
	if (UAnimSequenceBase* Anim = Ptr.IsNull() ? nullptr : Ptr.LoadSynchronous())
	{
		AnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
		AnimInstance->PlaySlotAnimationAsDynamicMontage(Anim, Set.SlotName, 0.05f, 0.25f, 1.2f, 1);
	}
}

float AValdoriaHeroCharacter::GetFlightFuelPercent() const
{
	const FValdoriaMovementRules& Rules = GetMovementRules();
	if (Rules.FlightMode == EValdoriaFlightMode::Ability)
	{
		const float Left = AbilityFlightEnd - GetWorld()->GetTimeSeconds();
		return FMath::Clamp(Left / FMath::Max(0.1f, Rules.FlightAbilityDuration), 0.f, 1.f);
	}
	return FlightFuel < 0.f ? 1.f : FMath::Clamp(FlightFuel / FMath::Max(0.1f, Rules.FlightFuelSeconds), 0.f, 1.f);
}

void AValdoriaHeroCharacter::StartAbilityFlight(float Seconds)
{
	const FValdoriaMovementRules& Rules = GetMovementRules();
	AbilityFlightEnd = GetWorld()->GetTimeSeconds() + (Seconds > 0.f ? Seconds : Rules.FlightAbilityDuration);
	LaunchCharacter(FVector(0.f, 0.f, 500.f), false, true);
}

void AValdoriaHeroCharacter::StopFlight()
{
	bFlying = false;
	if (!bNoclip && GetCharacterMovement()->MovementMode == MOVE_Flying)
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	}
}
// ------------------------------------------------------------------ estados de controle

void AValdoriaHeroCharacter::ApplyStatus(EValdoriaStatus Status, float Seconds)
{
	if (Seconds <= 0.f || bGodMode)
	{
		return;
	}
	const float End = GetWorld()->GetTimeSeconds() + Seconds;
	LastHarmedTime = GetWorld()->GetTimeSeconds(); // control counts as passive combat
	float& Current = StatusEndTimes.FindOrAdd(Status);
	Current = FMath::Max(Current, End);
	if (Status == EValdoriaStatus::Stunned || Status == EValdoriaStatus::CannotAttack)
	{
		bFireHeld = false;
	}
}

bool AValdoriaHeroCharacter::HasStatus(EValdoriaStatus Status) const
{
	const float* End = StatusEndTimes.Find(Status);
	return End && *End > GetWorld()->GetTimeSeconds();
}

bool AValdoriaHeroCharacter::CanUseSlot(EValdoriaAbilitySlot Slot) const
{
	if (HasStatus(EValdoriaStatus::Stunned))
	{
		return false;
	}
	if (Slot == EValdoriaAbilitySlot::PrimaryFire)
	{
		return !HasStatus(EValdoriaStatus::CannotAttack);
	}
	return !IsRiding() && !HasStatus(EValdoriaStatus::Silenced) && !HasStatus(EValdoriaStatus::CannotUseAbilities);
}

void AValdoriaHeroCharacter::AddUltimateCharge(float Amount)
{
	if (HasAuthority() && ActiveDefinition && Amount > 0.f)
	{
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetUltimateChargeAttribute(),
			FMath::Min(ActiveDefinition->UltimateCost, Attributes->GetUltimateCharge() + Amount));
	}
}

// ------------------------------------------------------------------ animador próprio

UAnimSequenceBase* AValdoriaHeroCharacter::PickBaseAnimation(bool& bOutLoop, float& OutRate) const
{
	OutRate = 1.f;
	if (!ActiveDefinition)
	{
		bOutLoop = true;
		return nullptr;
	}
	// Movimentos especiais (agachar, deslizar, subir, escalar, voar, flutuar) primeiro.
	if (UAnimSequenceBase* Special = PickMovementAnimation(bOutLoop))
	{
		return Special;
	}
	bOutLoop = true;
	const FValdoriaAnimationSet& Set = ActiveDefinition->Animations;
	auto Load = [](const TSoftObjectPtr<UAnimSequenceBase>& Ptr) { return Ptr.IsNull() ? nullptr : Ptr.LoadSynchronous(); };
	if (GetCharacterMovement()->IsFalling())
	{
		if (UAnimSequenceBase* Fall = Load(Set.FallLoop))
		{
			return Fall;
		}
	}
	const FVector Velocity = GetVelocity();
	const float Speed = Velocity.Size2D();
	if (Speed < 25.f || GetCharacterMovement()->IsFalling())
	{
		return Load(Set.Idle);
	}
	const bool bJog = Speed >= Set.WalkRunSpeed;
	const float Fwd = FVector::DotProduct(Velocity, GetActorForwardVector());
	const float Right = FVector::DotProduct(Velocity, GetActorRightVector());
	const TSoftObjectPtr<UAnimSequenceBase>* Pick;
	if (FMath::Abs(Fwd) >= FMath::Abs(Right) * 0.8f)
	{
		Pick = Fwd >= 0.f ? (bJog ? &Set.JogForward : &Set.WalkForward) : (bJog ? &Set.JogBackward : &Set.WalkBackward);
	}
	else
	{
		Pick = Right >= 0.f ? (bJog ? &Set.JogRight : &Set.WalkRight) : (bJog ? &Set.JogLeft : &Set.WalkLeft);
	}
	// Ritmo dos passos acompanha a velocidade real.
	OutRate = FMath::Clamp(Speed / FMath::Max(50.f, bJog ? Set.JogAnimSpeed : Set.WalkAnimSpeed), 0.6f, 1.8f);
	UAnimSequenceBase* Clip = Load(*Pick);
	return Clip ? Clip : Load(Set.Idle);
}

void AValdoriaHeroCharacter::PlayActionForSlot(EValdoriaAbilitySlot Slot)
{
	UValdoriaAnimInstance* Anim = GetMesh() ? Cast<UValdoriaAnimInstance>(GetMesh()->GetAnimInstance()) : nullptr;
	if (!Anim || !ActiveDefinition)
	{
		return;
	}
	const FValdoriaAnimationSet& Set = ActiveDefinition->Animations;
	auto Load = [](const TSoftObjectPtr<UAnimSequenceBase>& Ptr) { return Ptr.IsNull() ? nullptr : Ptr.LoadSynchronous(); };
	if (Slot == EValdoriaAbilitySlot::PrimaryFire)
	{
		if (Set.FireActions.Num() > 0)
		{
			// Combo: cada golpe usa a próxima animação da lista.
			const int32 Index = ComboIndex++ % Set.FireActions.Num();
			Anim->PlayAction(Load(Set.FireActions[Index]), Set.FireActionRate);
		}
		return;
	}
	Anim->PlayAction(Load(Slot == EValdoriaAbilitySlot::Ultimate ? Set.UltimateAction : Set.AbilityAction), 1.3f);
}

void AValdoriaHeroCharacter::PlayReloadAnimation()
{
	UValdoriaAnimInstance* Anim = GetMesh() ? Cast<UValdoriaAnimInstance>(GetMesh()->GetAnimInstance()) : nullptr;
	if (!Anim || !ActiveDefinition || ActiveDefinition->Animations.ReloadAction.IsNull())
	{
		return;
	}
	UAnimSequenceBase* Clip = ActiveDefinition->Animations.ReloadAction.LoadSynchronous();
	if (Clip)
	{
		// A animação dura o mesmo que a recarga.
		Anim->PlayAction(Clip, Clip->GetPlayLength() / FMath::Max(0.2f, GetWeaponStats().ReloadTime));
	}
}

void AValdoriaHeroCharacter::TickHover(float DeltaSeconds)
{
	if (!ActiveDefinition || ActiveDefinition->Locomotion != EValdoriaLocomotion::Float || bIsDead)
	{
		return;
	}
	// Flutuar: sobe e desce devagar e inclina na direção do movimento (sem passos).
	const float Time = GetWorld()->GetTimeSeconds();
	const FVector Base(0.f, 0.f, -ActiveDefinition->CapsuleHalfHeight + ActiveDefinition->HoverHeight);
	const float Bob = FMath::Sin(Time * 2.1f) * ActiveDefinition->HoverBob;
	const FVector Velocity = GetVelocity();
	const float MaxSpeed = FMath::Max(100.f, GetCharacterMovement()->GetMaxSpeed());
	const float Fwd = FMath::Clamp(FVector::DotProduct(Velocity, GetActorForwardVector()) / MaxSpeed, -1.f, 1.f);
	const float Right = FMath::Clamp(FVector::DotProduct(Velocity, GetActorRightVector()) / MaxSpeed, -1.f, 1.f);
	const FRotator Lean(-Fwd * ActiveDefinition->HoverLean, 0.f, Right * ActiveDefinition->HoverLean * 0.7f);
	const FQuat Target = Lean.Quaternion() * FRotator(0.f, -90.f, 0.f).Quaternion();
	const FQuat Current = GetMesh()->GetRelativeRotation().Quaternion();
	GetMesh()->SetRelativeLocationAndRotation(Base + FVector(0.f, 0.f, Bob), FQuat::Slerp(Current, Target, FMath::Min(1.f, DeltaSeconds * 6.f)));
}

// ------------------------------------------------------------------ sons

void AValdoriaHeroCharacter::PlaySlotSound(EValdoriaAbilitySlot Slot)
{
	if (!ActiveDefinition)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	USoundBase* Sound = nullptr;
	float Volume = 0.7f;
	if (Slot == EValdoriaAbilitySlot::PrimaryFire)
	{
		// Armas muito rápidas (feixe) tocam no máximo ~7 vezes por segundo.
		if (ActiveDefinition->FireSounds.Num() == 0 || Now - LastFireSoundTime < 0.14f)
		{
			return;
		}
		LastFireSoundTime = Now;
		const TSoftObjectPtr<USoundBase>& Pick = ActiveDefinition->FireSounds[FMath::RandRange(0, ActiveDefinition->FireSounds.Num() - 1)];
		Sound = Pick.IsNull() ? nullptr : Pick.LoadSynchronous();
		Volume = ActiveDefinition->FireVolume;
	}
	else if (const FValdoriaAbilityEntry* Entry = FindAbilityEntry(Slot))
	{
		Sound = Entry->Sound.IsNull() ? nullptr : Entry->Sound.LoadSynchronous();
		Volume = Slot == EValdoriaAbilitySlot::Ultimate ? 1.f : 0.75f;
	}
	ValdoriaAudio::PlayAt(this, Sound, GetActorLocation(), Volume);
}

void AValdoriaHeroCharacter::TickFootsteps(float DeltaSeconds)
{
	if (!ActiveDefinition || bIsDead || ActiveDefinition->FootstepVolume <= 0.f
		|| ActiveDefinition->Locomotion == EValdoriaLocomotion::Float || !GetCharacterMovement()->IsMovingOnGround())
	{
		return;
	}
	const float Speed = GetVelocity().Size2D();
	if (Speed < 120.f)
	{
		StepDistance = 0.f;
		return;
	}
	StepDistance += Speed * DeltaSeconds;
	// Passada maior correndo; agachado é mais baixo.
	const float Stride = bIsSprinting ? 210.f : (bIsCrouched ? 120.f : 165.f);
	if (StepDistance < Stride)
	{
		return;
	}
	StepDistance = 0.f;
	// Passos em concreto (Kenney Impact Sounds, CC0): combinam com a cidade.
	static const TCHAR* Steps[] = { TEXT("footstep_concrete_000"), TEXT("footstep_concrete_001"), TEXT("footstep_concrete_002"),
		TEXT("footstep_concrete_003"), TEXT("footstep_concrete_004") };
	const float Loud = ActiveDefinition->FootstepVolume * (bIsCrouched ? 0.4f : (bIsSprinting ? 1.2f : 1.f));
	const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	// Heróis grandes soam mais graves.
	const float Pitch = FMath::Clamp(1.f / FMath::Max(0.6f, ActiveDefinition->BodyHeight * ActiveDefinition->MeshScale), 0.8f, 1.2f);
	ValdoriaAudio::PlayAt(this, ValdoriaAudio::Load(TEXT("Footsteps"), Steps[FMath::RandRange(0, 4)]), Feet, Loud, 0.1f, Pitch);
}

// ------------------------------------------------------------------ flight pose
// Reference (owner's video): crouch and push off, fly almost lying down with one arm forward,
// stand upright when hovering, land. Done procedurally: tilt the mesh around the capsule centre
// by horizontal speed and blend an "arm forward" upper-body frame.

void AValdoriaHeroCharacter::TickFlightPose(float DeltaSeconds)
{
	if (!ActiveDefinition || bIsDead)
	{
		return;
	}
	const FValdoriaAnimationSet& Set = ActiveDefinition->Animations;
	UValdoriaAnimInstance* Anim = GetMesh() ? Cast<UValdoriaAnimInstance>(GetMesh()->GetAnimInstance()) : nullptr;
	auto Load = [](const TSoftObjectPtr<UAnimSequenceBase>& Ptr) { return Ptr.IsNull() ? nullptr : Ptr.LoadSynchronous(); };

	const bool bSuperhero = Set.FlightStyle == EValdoriaFlightStyle::Superhero;
	if (bFlying && !bWasFlyingPose && Anim && bSuperhero)
	{
		Anim->PlayAction(Load(Set.TakeoffAction), 1.4f, true); // push off
	}
	if (!bFlying && bWasFlyingPose)
	{
		bLandAfterFlight = true;
		if (Anim)
		{
			Anim->SetUpperPose(nullptr, 0.f, 0.f);
		}
		// Back to the normal mesh placement (floating heroes are handled by TickHover).
		if (ActiveDefinition->Locomotion != EValdoriaLocomotion::Float)
		{
			GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -ActiveDefinition->CapsuleHalfHeight + ActiveDefinition->HoverHeight),
				FRotator(0.f, -90.f, 0.f));
		}
		FlightLeanAlpha = 0.f;
	}
	bWasFlyingPose = bFlying;
	// Skating on a trail: arms out for balance (frozen frame of the fly loop) and a forward lean.
	const bool bSkating = IsSkating() && !bFlying;
	if (bSkating || bWasSkating)
	{
		const float SkateAlpha = bSkating ? 1.f : 0.f;
		const FRotator SkateLean(-14.f * SkateAlpha, 0.f, 0.f);
		const FVector Feet(0.f, 0.f, -ActiveDefinition->CapsuleHalfHeight + ActiveDefinition->HoverHeight);
		GetMesh()->SetRelativeLocationAndRotation(SkateLean.RotateVector(Feet), SkateLean.Quaternion() * FRotator(0.f, -90.f, 0.f).Quaternion());
		if (Anim)
		{
			Anim->SetUpperPose(bSkating ? Load(Set.Fly) : nullptr, 0.3f, bSkating ? 0.8f : 0.f);
		}
		bWasSkating = bSkating;
	}
	if (!bFlying)
	{
		return;
	}

	// 0 = hovering (upright), 1 = full speed forward (almost lying down).
	const FVector Velocity = GetVelocity();
	const float MaxSpeed = FMath::Max(100.f, GetCharacterMovement()->GetMaxSpeed());
	const float Fwd = FVector::DotProduct(Velocity, GetActorForwardVector()) / MaxSpeed;
	const float Right = FMath::Clamp(FVector::DotProduct(Velocity, GetActorRightVector()) / MaxSpeed, -1.f, 1.f);
	// Levitate also leans back when moving backwards (reference video); superhero only leans forward.
	const float Target = bSuperhero ? FMath::Clamp(Fwd, 0.f, 1.f) : FMath::Clamp(Fwd, -1.f, 1.f);
	FlightLeanAlpha = FMath::FInterpTo(FlightLeanAlpha, Target, DeltaSeconds, 4.f);

	// Levitate: stays upright and only leans a little (like floating).
	const float MaxLean = bSuperhero ? Set.FlightLean : FMath::Min(Set.FlightLean, ActiveDefinition->HoverLean);
	const FRotator Lean(-FlightLeanAlpha * MaxLean, 0.f, Right * MaxLean * 0.4f);
	const FQuat Rot = Lean.Quaternion() * FRotator(0.f, -90.f, 0.f).Quaternion();
	// Rotate around the capsule centre: the mesh origin sits at the feet.
	const FVector FeetOffset(0.f, 0.f, -ActiveDefinition->CapsuleHalfHeight + ActiveDefinition->HoverHeight);
	GetMesh()->SetRelativeLocationAndRotation(Lean.RotateVector(FeetOffset), Rot);
	if (Anim)
	{
		Anim->SetUpperPose(bSuperhero ? Load(Set.FlyForwardPose) : nullptr, Set.FlyForwardPoseTime, bSuperhero ? FlightLeanAlpha : 0.f);
	}
}

// ------------------------------------------------------------------ autopilot (bot brain)
// Used by deathmatch bots and by the player's own hero when switched on (/auto, admin).
// Unlike the old training-dummy shooting, it uses the hero's real kit: weapon, abilities, ultimate.

void AValdoriaHeroCharacter::SetAutopilot(bool bEnable, float Skill)
{
	bAutopilot = bEnable;
	AutopilotSkill = FMath::Clamp(Skill, 0.f, 1.f);
	AutopilotAim = GetActorRotation();
	bFireHeld = false;
}

void AValdoriaHeroCharacter::TickAutopilot(float DeltaSeconds)
{
	if (!bAutopilot || bIsDead || !ActiveDefinition)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	const FVector Eye = GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);
	auto CanSee = [this, &Eye](const AValdoriaHeroCharacter* Other)
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ValdoriaAutopilot), false, this);
		IgnoreFriendlyBarriers(Params);
		return !GetWorld()->LineTraceSingleByChannel(Hit, Eye, Other->GetActorLocation() + FVector(0.f, 0.f, 40.f), ECC_Visibility, Params)
			|| Hit.GetActor() == Other;
	};

	// Target: nearest visible enemy, re-checked twice a second.
	if (Now >= AutopilotNextPick)
	{
		AutopilotNextPick = Now + 0.5f;
		AutopilotTarget = nullptr;
		float Best = 5000.f;
		for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
		{
			if (!IsEnemy(*It) || It->IsHiddenByStealth()
				|| (LeashRadius > 0.f && FVector::Dist2D(It->GetActorLocation(), LeashCenter) > LeashRadius))
			{
				continue;
			}
			const float Dist = FVector::Dist(GetActorLocation(), It->GetActorLocation());
			if (Dist < Best && CanSee(*It))
			{
				Best = Dist;
				AutopilotTarget = *It;
			}
		}
	}
	// Guardians: an enemy who just hit an ally inside the area takes priority.
	if (LeashRadius > 0.f)
	{
		for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
		{
			AValdoriaHeroCharacter* Attacker = It->LastAttacker.Get();
			if (*It != this && It->TeamId == TeamId && Now - It->LastAttackedTime < 3.f && Attacker && IsEnemy(Attacker)
				&& FVector::Dist2D(It->GetActorLocation(), LeashCenter) <= LeashRadius
				&& FVector::Dist2D(Attacker->GetActorLocation(), LeashCenter) <= LeashRadius)
			{
				AutopilotTarget = Attacker;
				break;
			}
		}
	}
	AValdoriaHeroCharacter* Target = AutopilotTarget.Get();
	const FVector Fwd = GetActorForwardVector().GetSafeNormal2D();
	if (LeashRadius > 0.f && (!Target || Target->IsDead()) && FVector::Dist2D(GetActorLocation(), LeashCenter) > 200.f)
	{
		// Nothing to fight: walk back to the middle of the area.
		const FVector Home = (LeashCenter - GetActorLocation()).GetSafeNormal2D();
		AutopilotAim = FRotator(0.f, Home.Rotation().Yaw, 0.f);
		SetActorRotation(AutopilotAim);
		AddMovementInput(Home, 0.8f);
		return;
	}
	if (LeashRadius > 0.f && FVector::Dist2D(GetActorLocation(), LeashCenter) > LeashRadius)
	{
		AddMovementInput((LeashCenter - GetActorLocation()).GetSafeNormal2D(), 1.f); // never leave the area
	}
	if (!Target || Target->IsDead())
	{
		// Follow the route (MOBA lane) if there is one.
		if (AutopilotRoute.IsValidIndex(AutopilotRouteIndex))
		{
			const FVector ToPoint = AutopilotRoute[AutopilotRouteIndex] - GetActorLocation();
			if (ToPoint.Size2D() < 500.f)
			{
				++AutopilotRouteIndex;
			}
			AutopilotAim = FRotator(0.f, ToPoint.Rotation().Yaw, 0.f);
			SetActorRotation(AutopilotAim);
			AddMovementInput(ToPoint.GetSafeNormal2D(), 1.f);
			if (IsFacingWall(90.f))
			{
				Jump();
			}
			if (Controller)
			{
				Controller->SetControlRotation(AutopilotAim);
			}
			return;
		}
		// Roam: walk forward, turn slowly, jump at walls.
		AutopilotAim.Yaw += 20.f * DeltaSeconds;
		AutopilotAim.Pitch = 0.f;
		SetActorRotation(FRotator(0.f, AutopilotAim.Yaw, 0.f));
		AddMovementInput(Fwd, 0.6f);
		if (IsFacingWall(90.f))
		{
			AutopilotAim.Yaw += 90.f;
			Jump();
		}
		if (Controller)
		{
			Controller->SetControlRotation(AutopilotAim);
		}
		return;
	}

	// Aim at the chest with a skill-based wobble.
	const FVector Chest = Target->GetActorLocation() + FVector(0.f, 0.f, 30.f);
	const float Wobble = (1.f - AutopilotSkill) * 4.f;
	const FRotator Wanted = (Chest - Eye).Rotation() + FRotator(FMath::FRandRange(-Wobble, Wobble), FMath::FRandRange(-Wobble, Wobble), 0.f);
	AutopilotAim = FMath::RInterpTo(AutopilotAim, Wanted, DeltaSeconds, 4.f + 8.f * AutopilotSkill);
	SetActorRotation(FRotator(0.f, AutopilotAim.Yaw, 0.f));
	if (Controller)
	{
		Controller->SetControlRotation(AutopilotAim);
	}

	// Distance: close in with melee weapons, keep range with guns; strafe and hop.
	const float Dist = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
	const FValdoriaWeaponStats& Weapon = GetWeaponStats();
	const float Desired = FMath::Clamp(Weapon.Range * 0.6f, 250.f, 1200.f);
	if (Dist > Desired + 250.f)
	{
		AddMovementInput(Fwd, 1.f);
	}
	else if (Dist < Desired * 0.5f)
	{
		AddMovementInput(-Fwd, 0.7f);
	}
	AutopilotStrafe += DeltaSeconds;
	AddMovementInput(GetActorRightVector(), FMath::Sin(AutopilotStrafe * 1.1f + TeamId) >= 0.f ? 0.6f : -0.6f);
	if (IsFacingWall(90.f) || FMath::FRand() < 0.003f)
	{
		Jump();
	}

	// Shoot when roughly on target and in range.
	const float AimError = FMath::Abs(FRotator::NormalizeAxis(Wanted.Yaw - AutopilotAim.Yaw));
	if (Dist <= Weapon.Range && AimError < 12.f && CanSee(Target))
	{
		TryActivateSlot(EValdoriaAbilitySlot::PrimaryFire);
	}
	if (Weapon.bUsesAmmo && CurrentAmmo <= 0 && !bReloading)
	{
		StartReload();
	}
	// Abilities now and then (cooldowns and ranges are checked by the abilities themselves).
	if (Now >= AutopilotNextAbility && Dist < 1800.f)
	{
		AutopilotNextAbility = Now + FMath::FRandRange(1.f, 2.5f);
		static const EValdoriaAbilitySlot Slots[] = { EValdoriaAbilitySlot::SecondaryFire, EValdoriaAbilitySlot::Ability1,
			EValdoriaAbilitySlot::Ability2, EValdoriaAbilitySlot::Ability3 };
		const EValdoriaAbilitySlot Pick = Slots[FMath::RandRange(0, 3)];
		if (FindAbilityEntry(Pick) && GetCooldownRemaining(Pick) <= 0.f)
		{
			TryActivateSlot(Pick);
		}
		if (IsUltimateReady() && Dist < 1200.f)
		{
			TryActivateSlot(EValdoriaAbilitySlot::Ultimate);
		}
	}
}

// ------------------------------------------------------------------ team buffs (MOBA spots)

void AValdoriaHeroCharacter::ApplyTeamBuff(uint8 Type, float Value, float Seconds)
{
	if (Type < 3)
	{
		// Keep the stronger buff while both are active (spot buff + mage empower, etc.).
		const float Now = GetWorld()->GetTimeSeconds();
		if (Now >= TeamBuffEnd[Type] || Value >= TeamBuffValue[Type])
		{
			TeamBuffValue[Type] = Value;
		}
		TeamBuffEnd[Type] = FMath::Max(TeamBuffEnd[Type], Now + Seconds);
	}
}

float AValdoriaHeroCharacter::GetTeamDamageReduction() const
{
	return GetWorld()->GetTimeSeconds() < TeamBuffEnd[0] ? FMath::Clamp(TeamBuffValue[0], 0.f, 0.9f) : 0.f;
}

float AValdoriaHeroCharacter::GetTeamDamageBonus() const
{
	return GetWorld()->GetTimeSeconds() < TeamBuffEnd[1] ? FMath::Max(0.f, TeamBuffValue[1]) : 0.f;
}

float AValdoriaHeroCharacter::GetTeamRegen() const
{
	return GetWorld()->GetTimeSeconds() < TeamBuffEnd[2] ? FMath::Max(0.f, TeamBuffValue[2]) : 0.f;
}
// ------------------------------------------------------------------ Bifrost (lane rails)
// Mount: look at a rail and hold Space ~0.4 s. Direction follows the camera (look back to go
// home). Basic attacks only, no abilities. Falls off when hit by an enemy. Locked for 2 s after
// "passive combat" (taking damage/control from an enemy).

bool AValdoriaHeroCharacter::IsInPassiveCombat(float Seconds) const
{
	return GetWorld()->GetTimeSeconds() - LastHarmedTime < Seconds;
}

bool AValdoriaHeroCharacter::TryMountRail()
{
	if (bIsDead || IsRiding() || IsInPassiveCombat(2.f))
	{
		return false;
	}
	FVector Start, Dir;
	GetAimRay(Start, Dir);
	AValdoriaLaneRail* Best = nullptr;
	float BestDist = 300.f; // generous hitbox: no need to aim precisely at the thin line
	float BestAlong = 0.f;
	for (TActorIterator<AValdoriaLaneRail> It(GetWorld()); It; ++It)
	{
		// Own team's rails only, unless this hero can surf any rail (future hero).
		if (It->Team != TeamId && !bCanRideAnyRail)
		{
			continue;
		}
		float Along = 0.f;
		const float Dist = It->DistanceToRay(Start, Dir, 2500.f, Along);
		if (Dist < BestDist)
		{
			BestDist = Dist;
			Best = *It;
			BestAlong = Along;
		}
	}
	if (!Best)
	{
		return false;
	}
	RideRail = Best;
	RideDistance = BestAlong;
	// Start in the direction the camera looks along the trail.
	RideSign = FVector::DotProduct(Dir.GetSafeNormal2D(), Best->GetTangentAt(BestAlong).GetSafeNormal2D()) >= 0.f ? 1.f : -1.f;
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	GetCharacterMovement()->StopMovementImmediately();
	ValdoriaAudio::PlayAt(this, ValdoriaAudio::Load(TEXT("SciFi"), TEXT("forceField_000")), GetActorLocation(), 0.6f);
	return true;
}

void AValdoriaHeroCharacter::StopRiding()
{
	if (!RideRail.IsValid())
	{
		return;
	}
	RideRail = nullptr;
	if (!bNoclip)
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	}
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -ActiveDefinition->CapsuleHalfHeight + ActiveDefinition->HoverHeight),
		FRotator(0.f, -90.f, 0.f));
}

void AValdoriaHeroCharacter::TickRiding(float DeltaSeconds)
{
	// Hold Space ~0.4 s while looking at a rail to get on.
	if (bJumpHeld && !IsRiding() && GetWorld()->GetTimeSeconds() - JumpPressTime >= 0.4f && !bTriedMountThisPress)
	{
		bTriedMountThisPress = true;
		TryMountRail();
	}
	AValdoriaLaneRail* Rail = RideRail.Get();
	if (!Rail)
	{
		return;
	}
	if (bIsDead)
	{
		StopRiding();
		return;
	}
	// The camera is free while riding. To turn around: look back AND press forward (W).
	FVector Start, Dir;
	GetAimRay(Start, Dir);
	const FVector Tangent = Rail->GetTangentAt(RideDistance);
	const bool bLookingBack = FVector::DotProduct(Dir.GetSafeNormal2D(), (Tangent * RideSign).GetSafeNormal2D()) < -0.2f;
	if (bLookingBack && RideForwardInput > 0.5f)
	{
		RideSign = -RideSign;
	}
	RideForwardInput = 0.f;
	const float Sign = RideSign;
	RideDistance += Sign * RideSpeed * DeltaSeconds;
	if (RideDistance <= 0.f || RideDistance >= Rail->GetLength())
	{
		StopRiding(); // end of the trail: drop off
		return;
	}
	SetActorLocation(Rail->GetPointAt(RideDistance) - FVector(0.f, 0.f, 60.f));
	SetActorRotation(FRotator(0.f, (Tangent * Sign).Rotation().Yaw, 0.f));
	// Lying down, facing the travel direction.
	const FRotator Lie(-80.f, 0.f, 0.f);
	const FVector FeetOffset(0.f, 0.f, -ActiveDefinition->CapsuleHalfHeight);
	GetMesh()->SetRelativeLocationAndRotation(Lie.RotateVector(FeetOffset), Lie.Quaternion() * FRotator(0.f, -90.f, 0.f).Quaternion());
}

FString AValdoriaHeroCharacter::GetRideLaneName() const
{
	return RideRail.IsValid() ? RideRail->LaneName : FString();
}
