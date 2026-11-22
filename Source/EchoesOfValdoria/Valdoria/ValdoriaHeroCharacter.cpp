// Echoes of Valdoria - personagem jogável genérico.

#include "ValdoriaHeroCharacter.h"
#include "ValdoriaMultipliers.h"
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
#include "GameFramework/InputSettings.h"
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
#include "ValdoriaRetargetAnimInstance.h"
#include "Retargeter/IKRetargeter.h"
#include "ValdoriaAudio.h"
#include "ValdoriaLaneRail.h"
#include "ValdoriaGuardian.h"
#include "ValdoriaMobaMode.h"
#include "ValdoriaShop.h"
#include "ValdoriaVision.h"
#include "ValdoriaLaneRail.h"
#include "Sound/SoundBase.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/StaticMeshActor.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
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

	VisualMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(GetMesh());
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetVisibility(false);
	VisualMesh->SetComponentTickEnabled(false);

	WeaponComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Weapon"));
	WeaponComponent->SetupAttachment(GetMesh());
	WeaponComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponComponent->SetCastShadow(true);
	OffhandComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Offhand"));
	OffhandComponent->SetupAttachment(GetMesh());
	OffhandComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OffhandComponent->SetCastShadow(true);

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
	for (TObjectPtr<UStaticMeshComponent>* Foot : { &FootDiscL, &FootDiscR })
	{
		*Foot = CreateDefaultSubobject<UStaticMeshComponent>(Foot == &FootDiscL ? TEXT("FootDiscL") : TEXT("FootDiscR"));
		(*Foot)->SetupAttachment(RootComponent);
		(*Foot)->SetUsingAbsoluteLocation(true);
		(*Foot)->SetUsingAbsoluteRotation(true);
		(*Foot)->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		(*Foot)->SetCastShadow(false);
		(*Foot)->SetVisibility(false);
		if (DiscMesh.Succeeded())
		{
			(*Foot)->SetStaticMesh(DiscMesh.Object);
		}
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
	// Turn 49: optional visible body (VRoid etc.) driven by the hidden Mannequin through an IK Retargeter.
	{
		USkeletalMesh* Body = Def->VisualMesh.IsNull() ? nullptr : Def->VisualMesh.LoadSynchronous();
		UIKRetargeter* Rtg = Def->VisualRetargeter.IsNull() ? nullptr : Def->VisualRetargeter.LoadSynchronous();
		const bool bVisual = Body && Rtg;
		VisualMesh->SetComponentTickEnabled(bVisual);
		VisualMesh->SetSkeletalMesh(bVisual ? Body : nullptr);
		VisualMesh->SetVisibility(bVisual);
		// The Mannequin stays (hitboxes, sockets for the weapons, ragdoll) but is not drawn; its bones must
		// keep updating while hidden so the visible body and the weapons follow it.
		GetMesh()->SetVisibility(!bVisual, false);
		GetMesh()->VisibilityBasedAnimTickOption = bVisual ? EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones : EVisibilityBasedAnimTickOption::AlwaysTickPose;
		if (bVisual)
		{
			VisualMesh->SetRelativeTransform(Def->VisualOffset);
			VisualMesh->AddTickPrerequisiteComponent(GetMesh());
			VisualMesh->SetAnimInstanceClass(UValdoriaRetargetAnimInstance::StaticClass());
			if (UValdoriaRetargetAnimInstance* RA = Cast<UValdoriaRetargetAnimInstance>(VisualMesh->GetAnimInstance()))
			{
				RA->Retargeter = Rtg;
			}
		}
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
	UStaticMesh* Offhand = Def->OffhandMesh.IsNull() ? nullptr : Def->OffhandMesh.LoadSynchronous();
	OffhandComponent->SetStaticMesh(Offhand);
	OffhandComponent->SetVisibility(Offhand != nullptr);
	if (Offhand)
	{
		OffhandComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, Def->OffhandSocket);
		OffhandComponent->SetRelativeTransform(Def->OffhandOffset);
	}
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -Def->CapsuleHalfHeight + Def->HoverHeight));
	// Andar sobre efeito: disco brilhante sob os pés, na altura da flutuação.
	const bool bRide = Def->Locomotion == EValdoriaLocomotion::RideEffect;
	const bool bPerFoot = bRide && Def->bRideEffectPerFoot;
	RideDisc->SetVisibility(bRide && !bPerFoot);
	FootDiscAlpha[0] = FootDiscAlpha[1] = 0.f;
	FootDiscL->SetVisibility(false);
	FootDiscR->SetVisibility(false);
	if (bPerFoot)
	{
		const FLinearColor FootColor = Def->RideEffectColor.Equals(FLinearColor::Black) ? Def->ThemeColor : Def->RideEffectColor;
		UStaticMesh* Custom = Def->RideEffectMesh.IsNull() ? nullptr : Def->RideEffectMesh.LoadSynchronous();
		for (UStaticMeshComponent* Foot : { FootDiscL.Get(), FootDiscR.Get() })
		{
			if (Custom) { Foot->SetStaticMesh(Custom); }
			Foot->SetMaterial(0, ValdoriaMakeGlowMaterial(this, FootColor, 0.55f));
		}
	}
	if (bRide && !bPerFoot)
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
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetMaxHealthAttribute(), (Def->MaxHealth + ShopBonus.MaxHealth) * ValdoriaStatScale * ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::MaxHealth));
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetHealthAttribute(), (Def->MaxHealth + ShopBonus.MaxHealth) * ValdoriaStatScale * ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::MaxHealth));
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetMaxArmorAttribute(), Def->MaxArmor * ValdoriaStatScale * ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::MaxArmor));
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetArmorAttribute(), Def->MaxArmor * ValdoriaStatScale * ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::MaxArmor));
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetMaxShieldAttribute(), Def->MaxShield * ValdoriaStatScale * ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::MaxShield));
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetShieldAttribute(), Def->MaxShield * ValdoriaStatScale * ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::MaxShield));
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
			// InputID = slot (+ 16 * (element set + 1) for element-bound abilities, Kay).
			const int32 Input = static_cast<int32>(Entry.Slot) + (Entry.ElementSet >= 0 ? 16 * (Entry.ElementSet + 1) : 0);
			FGameplayAbilitySpec Spec(Entry.AbilityClass, 1, Input, this);
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
	// Element-bound entry of the current element first (Kay), then the slot's always-available one.
	if (ActiveDefinition->ElementCount > 0)
	{
		const int32 Element = CurrentElement;
		if (const FValdoriaAbilityEntry* E = ActiveDefinition->Abilities.FindByPredicate([Slot, Element](const FValdoriaAbilityEntry& X) { return X.Slot == Slot && X.ElementSet == Element; }))
		{
			return E;
		}
	}
	return ActiveDefinition->Abilities.FindByPredicate([Slot](const FValdoriaAbilityEntry& E) { return E.Slot == Slot && E.ElementSet < 0; });
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
	LookStickAction = MakeAction(TEXT("IA_Valdoria_LookStick"), EInputActionValueType::Axis2D);
	JumpAction = MakeAction(TEXT("IA_Valdoria_Jump"), EInputActionValueType::Boolean);
	FireAction = MakeAction(TEXT("IA_Valdoria_Fire"), EInputActionValueType::Boolean);
	SecondaryAction = MakeAction(TEXT("IA_Valdoria_Secondary"), EInputActionValueType::Boolean);
	Ability1Action = MakeAction(TEXT("IA_Valdoria_Ability1"), EInputActionValueType::Boolean);
	Ability2Action = MakeAction(TEXT("IA_Valdoria_Ability2"), EInputActionValueType::Boolean);
	Ability3Action = MakeAction(TEXT("IA_Valdoria_Ability3"), EInputActionValueType::Boolean);
	AbilityExtraAction = MakeAction(TEXT("IA_Valdoria_AbilityExtra"), EInputActionValueType::Boolean);
	InteractAction = MakeAction(TEXT("IA_Valdoria_Interact"), EInputActionValueType::Boolean);
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
	MapAbilityKeys();
	DefaultMapping->MapKey(SprintAction, EKeys::LeftShift);
	DefaultMapping->MapKey(CrouchAction, EKeys::LeftControl);
	DefaultMapping->MapKey(ReloadAction, EKeys::R);
	DefaultMapping->MapKey(InteractAction, EKeys::G); // hold: seal / reverse-seal a guardian

	// Gamepad (XInput names; on a DualShock 4: Bottom = X, Right = O, Left = Square, Top = Triangle).
	// A DS4 shows up as XInput through Steam Input, DS4Windows or x360ce (see docs/Controle.md).
	{
		FEnhancedActionKeyMapping& Stick = DefaultMapping->MapKey(MoveAction, EKeys::Gamepad_Left2D);
		Stick.Modifiers.Add(NewObject<UInputModifierDeadZone>(DefaultMapping));
		FEnhancedActionKeyMapping& Look = DefaultMapping->MapKey(LookStickAction, EKeys::Gamepad_Right2D);
		Look.Modifiers.Add(NewObject<UInputModifierDeadZone>(DefaultMapping));
	}
	DefaultMapping->MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);
	DefaultMapping->MapKey(SprintAction, EKeys::Gamepad_LeftThumbstick);
	DefaultMapping->MapKey(CrouchAction, EKeys::Gamepad_FaceButton_Right);
	DefaultMapping->MapKey(ReloadAction, EKeys::Gamepad_FaceButton_Left);
	DefaultMapping->MapKey(InteractAction, EKeys::Gamepad_DPad_Right);
}

void AValdoriaHeroCharacter::MapAbilityKeys()
{
	// Teclas das habilidades conforme o esquema escolhido em Configurações > Controles.
	const FValdoriaOptions& Opt = FValdoriaOptions::Get();
	DefaultMapping->MapKey(FireAction, Opt.GetSlotKey(EValdoriaAbilitySlot::PrimaryFire));
	DefaultMapping->MapKey(SecondaryAction, Opt.GetSlotKey(EValdoriaAbilitySlot::SecondaryFire));
	DefaultMapping->MapKey(Ability1Action, Opt.GetSlotKey(EValdoriaAbilitySlot::Ability1));
	DefaultMapping->MapKey(Ability2Action, Opt.GetSlotKey(EValdoriaAbilitySlot::Ability2));
	DefaultMapping->MapKey(Ability3Action, Opt.GetSlotKey(EValdoriaAbilitySlot::Ability3));
	DefaultMapping->MapKey(UltimateAction, Opt.GetSlotKey(EValdoriaAbilitySlot::Ultimate));
	DefaultMapping->MapKey(AbilityExtraAction, Opt.GetSlotKey(EValdoriaAbilitySlot::AbilityExtra));
	DefaultMapping->MapKey(ProneAction, EKeys::X);
	// Gamepad abilities (fixed for now; the owner said skill layout on the pad can be tuned later).
	DefaultMapping->MapKey(FireAction, EKeys::Gamepad_RightTrigger);
	DefaultMapping->MapKey(SecondaryAction, EKeys::Gamepad_LeftTrigger);
	DefaultMapping->MapKey(Ability1Action, EKeys::Gamepad_LeftShoulder);
	DefaultMapping->MapKey(Ability2Action, EKeys::Gamepad_RightShoulder);
	DefaultMapping->MapKey(Ability3Action, EKeys::Gamepad_FaceButton_Top);
	DefaultMapping->MapKey(UltimateAction, EKeys::Gamepad_DPad_Up);
	DefaultMapping->MapKey(AbilityExtraAction, EKeys::Gamepad_DPad_Left);
}

void AValdoriaHeroCharacter::RefreshKeyScheme()
{
	if (!DefaultMapping)
	{
		return;
	}
	for (UInputAction* Action : { FireAction.Get(), SecondaryAction.Get(), Ability1Action.Get(), Ability2Action.Get(), Ability3Action.Get(), AbilityExtraAction.Get(), UltimateAction.Get(), ProneAction.Get() })
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
	Input->BindAction(LookStickAction, ETriggerEvent::Triggered, this, &AValdoriaHeroCharacter::Input_LookStick);
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
	Input->BindAction(SecondaryAction, ETriggerEvent::Completed, this, &AValdoriaHeroCharacter::Input_SecondaryReleased);
	Input->BindAction(Ability1Action, ETriggerEvent::Started, this, &AValdoriaHeroCharacter::Input_Ability1);
	Input->BindAction(Ability2Action, ETriggerEvent::Started, this, &AValdoriaHeroCharacter::Input_Ability2);
	Input->BindAction(Ability3Action, ETriggerEvent::Started, this, &AValdoriaHeroCharacter::Input_Ability3);
	Input->BindAction(AbilityExtraAction, ETriggerEvent::Started, this, &AValdoriaHeroCharacter::Input_AbilityExtra);
	Input->BindAction(InteractAction, ETriggerEvent::Started, this, &AValdoriaHeroCharacter::Input_InteractPressed);
	Input->BindAction(InteractAction, ETriggerEvent::Completed, this, &AValdoriaHeroCharacter::Input_InteractReleased);
	Input->BindAction(UltimateAction, ETriggerEvent::Started, this, &AValdoriaHeroCharacter::Input_Ultimate);
	Input->BindAction(UltimateAction, ETriggerEvent::Completed, this, &AValdoriaHeroCharacter::Input_UltimateReleased);
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

void AValdoriaHeroCharacter::Input_LookStick(const FInputActionValue& Value)
{
	if (IsCursorFree() || !Controller)
	{
		return;
	}
	// A stick is held, so it is a turn rate: scale by frame time. Squared response gives finer aim near the centre.
	FVector2D Axis = Value.Get<FVector2D>();
	const float Tilt = FMath::Min(1.f, Axis.Size());
	Axis = FVector2D(Axis.X * FMath::Abs(Axis.X), Axis.Y * FMath::Abs(Axis.Y));
	const FValdoriaOptions& Options = FValdoriaOptions::Get();
	const float Dt = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;
	float Rate = 120.f * Options.GamepadSensitivity * Dt;
	// AddController*Input is multiplied by the PlayerController's legacy input scale; divide it out so the
	// numbers here are degrees.
	float YawScale = 1.f, PitchScale = 1.f;
	const APlayerController* PC = Cast<APlayerController>(Controller);
	if (PC && GetDefault<UInputSettings>()->bEnableLegacyInputScales)
	{
PRAGMA_DISABLE_DEPRECATION_WARNINGS
		YawScale = FMath::IsNearlyZero(PC->GetDeprecatedInputYawScale()) ? 1.f : PC->GetDeprecatedInputYawScale();
		PitchScale = FMath::IsNearlyZero(PC->GetDeprecatedInputPitchScale()) ? 1.f : PC->GetDeprecatedInputPitchScale();
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

	// Aim assist (gamepad only): friction over a target and a gentle pull toward it while the stick moves.
	float PullYaw = 0.f, PullPitch = 0.f;
	if (Options.bAimAssist && !bIsDead)
	{
		FVector ViewLoc;
		FRotator ViewRot;
		Controller->GetPlayerViewPoint(ViewLoc, ViewRot);
		const FVector ViewDir = ViewRot.Vector();
		const float MaxAngle = 8.f, MaxRange = 4500.f;
		float BestAngle = MaxAngle;
		FVector BestPoint = FVector::ZeroVector;
		for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
		{
			AValdoriaHeroCharacter* Other = *It;
			if (!IsEnemy(Other) || Other->IsHidden())
			{
				continue;
			}
			const FVector Point = Other->GetActorLocation() + FVector(0.f, 0.f, Other->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.35f);
			const FVector To = Point - ViewLoc;
			const float Dist = To.Size();
			if (Dist > MaxRange || Dist < 50.f)
			{
				continue;
			}
			const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(ViewDir, To / Dist), -1.f, 1.f)));
			if (Angle >= BestAngle)
			{
				continue;
			}
			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(ValdoriaAimAssist), false, this);
			if (GetWorld()->LineTraceSingleByChannel(Hit, ViewLoc, Point, ECC_Visibility, Params) && Hit.GetActor() != Other)
			{
				continue; // behind a wall
			}
			BestAngle = Angle;
			BestPoint = Point;
		}
		if (BestAngle < MaxAngle)
		{
			// Friction: up to 45% slower when right on the target.
			Rate *= FMath::Lerp(0.55f, 1.f, BestAngle / MaxAngle);
			// Pull: a fraction of the remaining angle per second, scaled by how much the stick is pushed.
			const FRotator Want = (BestPoint - ViewLoc).Rotation();
			const FRotator Delta = (Want - ViewRot).GetNormalized();
			const float Pull = FMath::Min(1.f, 4.f * Dt) * Tilt * (1.f - BestAngle / MaxAngle);
			PullYaw = Delta.Yaw * Pull;
			PullPitch = Delta.Pitch * Pull;
		}
	}

	// The final numbers below are degrees of real pitch (positive = up): the signed legacy pitch scale is divided
	// out. The stick's Y reading is flipped: the first version used it as-is and the owner found it inverted.
	const float LookY = Options.bInvertY ? Axis.Y : -Axis.Y;
	AddControllerYawInput((Axis.X * Rate + PullYaw) / YawScale);
	AddControllerPitchInput((LookY * Rate + PullPitch) / -FMath::Abs(PitchScale) * (PitchScale < 0.f ? 1.f : -1.f));
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
void AValdoriaHeroCharacter::Input_Secondary() { bSecondaryHeld = true; TryActivateSlot(EValdoriaAbilitySlot::SecondaryFire); }
void AValdoriaHeroCharacter::Input_SecondaryReleased() { bSecondaryHeld = false; }
// Ctrl + 1/2/3/4 spends a level point on that ability (MOBA levels, turn 46) instead of casting it.
static bool ValdoriaCtrlHeld(const AController* C)
{
	const APlayerController* PC = Cast<APlayerController>(C);
	// Gamepad: hold Share (Back) + the ability button.
	return PC && (PC->IsInputKeyDown(EKeys::LeftControl) || PC->IsInputKeyDown(EKeys::RightControl) || PC->IsInputKeyDown(EKeys::Gamepad_Special_Left));
}
void AValdoriaHeroCharacter::Input_Ability1() { if (bUsesLevels && ValdoriaCtrlHeld(Controller)) { TryUpgrade(EValdoriaAbilitySlot::Ability1); return; } TryActivateSlot(EValdoriaAbilitySlot::Ability1); }
void AValdoriaHeroCharacter::Input_Ability2() { if (bUsesLevels && ValdoriaCtrlHeld(Controller)) { TryUpgrade(EValdoriaAbilitySlot::Ability2); return; } TryActivateSlot(EValdoriaAbilitySlot::Ability2); }
void AValdoriaHeroCharacter::Input_Ability3() { if (bUsesLevels && ValdoriaCtrlHeld(Controller)) { TryUpgrade(EValdoriaAbilitySlot::Ability3); return; } TryActivateSlot(EValdoriaAbilitySlot::Ability3); }
void AValdoriaHeroCharacter::Input_AbilityExtra() { TryActivateSlot(EValdoriaAbilitySlot::AbilityExtra); }
void AValdoriaHeroCharacter::Input_Ultimate()
{
	if (bUsesLevels && ValdoriaCtrlHeld(Controller))
	{
		TryUpgrade(EValdoriaAbilitySlot::Ultimate);
		bUltUpgradePress = true; // the release must not cast or switch element
		return;
	}
	bUltUpgradePress = false;
	// Hold-to-cast ultimate (Kay): start counting; a quick tap switches element instead (see Input_UltimateReleased).
	if (ActiveDefinition && ActiveDefinition->UltimateHoldSeconds > 0.f)
	{
		UltHoldStart = GetWorld()->GetTimeSeconds();
		return;
	}
	TryActivateSlot(EValdoriaAbilitySlot::Ultimate);
}

void AValdoriaHeroCharacter::Input_UltimateReleased()
{
	if (UltHoldStart < 0.f || !ActiveDefinition)
	{
		return;
	}
	const float Held = GetWorld()->GetTimeSeconds() - UltHoldStart;
	UltHoldStart = -1.f;
	if (Held < 0.3f)
	{
		SwitchElement(); // quick tap
	}
	else if (IsUltimateReady() && GetCooldownRemaining(EValdoriaAbilitySlot::Ultimate) <= 0.f)
	{
		// Released before the charge completed: cancelled, short cooldown (owner: 3 s).
		StartCooldown(EValdoriaAbilitySlot::Ultimate, ActiveDefinition->UltimateCancelCooldown);
	}
}

void AValdoriaHeroCharacter::TickUltimateHold()
{
	if (UltHoldStart < 0.f || !ActiveDefinition || ActiveDefinition->UltimateHoldSeconds <= 0.f)
	{
		return;
	}
	if (bIsDead || !IsUltimateReady() || GetCooldownRemaining(EValdoriaAbilitySlot::Ultimate) > 0.f)
	{
		return; // holding without a full charge does nothing (a tap still switches element)
	}
	if (GetWorld()->GetTimeSeconds() - UltHoldStart >= ActiveDefinition->UltimateHoldSeconds)
	{
		UltHoldStart = -1.f;
		TryActivateSlot(EValdoriaAbilitySlot::Ultimate);
	}
}

float AValdoriaHeroCharacter::GetUltimateHoldProgress() const
{
	if (UltHoldStart < 0.f || !ActiveDefinition || ActiveDefinition->UltimateHoldSeconds <= 0.f || !IsUltimateReady())
	{
		return -1.f;
	}
	return FMath::Clamp((GetWorld()->GetTimeSeconds() - UltHoldStart) / ActiveDefinition->UltimateHoldSeconds, 0.f, 1.f);
}

void AValdoriaHeroCharacter::SwitchElement()
{
	if (!ActiveDefinition || ActiveDefinition->ElementCount <= 1 || bIsDead)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < ElementSwitchReadyTime && !bNoCooldowns)
	{
		return;
	}
	CurrentElement = (CurrentElement + 1) % ActiveDefinition->ElementCount;
	ElementSwitchReadyTime = Now + ActiveDefinition->ElementSwitchCooldown;
	PlayAreaEffect(GetActorLocation(), 160.f, GetElementColor());
}

float AValdoriaHeroCharacter::GetElementSwitchRemaining() const
{
	return FMath::Max(0.f, ElementSwitchReadyTime - GetWorld()->GetTimeSeconds());
}

FLinearColor AValdoriaHeroCharacter::GetElementColor() const
{
	if (ActiveDefinition && ActiveDefinition->ElementColors.IsValidIndex(CurrentElement))
	{
		return ActiveDefinition->ElementColors[CurrentElement];
	}
	return ActiveDefinition ? ActiveDefinition->ThemeColor : FLinearColor::White;
}

FText AValdoriaHeroCharacter::GetElementName() const
{
	return ActiveDefinition && ActiveDefinition->ElementNames.IsValidIndex(CurrentElement) ? ActiveDefinition->ElementNames[CurrentElement] : FText::GetEmpty();
}

EValdoriaDamageType AValdoriaHeroCharacter::GetElementDamageType(EValdoriaDamageType Fallback) const
{
	return ActiveDefinition && ActiveDefinition->ElementDamageTypes.IsValidIndex(CurrentElement) ? ActiveDefinition->ElementDamageTypes[CurrentElement] : Fallback;
}

void AValdoriaHeroCharacter::SwapToForm(FName FormId)
{
	if (FormId.IsNone() || !HasAuthority())
	{
		return;
	}
	// Next tick: the ability that asked for the swap is still running and its spec is about to be cleared.
	TWeakObjectPtr<AValdoriaHeroCharacter> WeakThis(this);
	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([WeakThis, FormId]()
	{
		AValdoriaHeroCharacter* Self = WeakThis.Get();
		if (!Self || Self->IsDead())
		{
			return;
		}
		const FString Id = FormId.ToString().ToLower();
		FString Asset = TEXT("DA_");
		TArray<FString> Parts;
		Id.ParseIntoArray(Parts, TEXT("_"));
		for (const FString& Part : Parts) { Asset += Part.Left(1).ToUpper() + Part.Mid(1); }
		UValdoriaHeroDefinition* Def = LoadObject<UValdoriaHeroDefinition>(nullptr, *FString::Printf(TEXT("/Game/Valdoria/Heroes/%s.%s"), *Asset, *Asset), nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Def)
		{
			Def = UValdoriaHeroDefinition::CreatePreset(Self, FormId);
		}
		if (!Def)
		{
			return;
		}
		const float HealthFrac = Self->GetHealth() / FMath::Max(1.f, Self->GetMaxHealth());
		const float Ult = Self->GetUltimatePercent();
		const TMap<EValdoriaAbilitySlot, float> Cooldowns = Self->CooldownEndTimes;
		Self->SetHeroDefinition(Def);
		Self->AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetHealthAttribute(), FMath::Max(1.f, Self->GetMaxHealth() * HealthFrac));
		Self->AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetUltimateChargeAttribute(), Def->UltimateCost * Ult);
		Self->CooldownEndTimes = Cooldowns;
		// Ariel passive (turn 45): a bonus on arriving in the new form, from that form's swap entry.
		// Turn 48 (owner: spamming C gave endless speed): the arrival bonus has its own 8 s cooldown (provisional),
		// so swapping back and forth (or with admin no-cooldowns) cannot keep it up forever.
		const float BonusNow = Self->GetWorld()->GetTimeSeconds();
		const FValdoriaAbilityEntry* Swap = BonusNow >= Self->NextFormBonusTime ? Self->FindAbilityEntry(EValdoriaAbilitySlot::AbilityExtra) : nullptr;
		if (Swap)
		{
			Self->NextFormBonusTime = BonusNow + 8.f;
			if (Swap->Magnitude > 0.f) { Self->AddOverheal(Swap->Magnitude * ValdoriaStatScale, Swap->Magnitude * ValdoriaStatScale, Swap->Magnitude * ValdoriaStatScale / 6.f); }
			if (Swap->SecondaryMagnitude > 0.f) { Self->ApplySpeedBuff(Swap->SecondaryMagnitude, 2.f); }
		}
		Self->PlayAreaEffect(Self->GetActorLocation(), 220.f, Def->ThemeColor);
	}));
}

float AValdoriaHeroCharacter::GetVigorPercent() const
{
	const FValdoriaMovementRules& Rules = GetMovementRules();
	return !Rules.bUsesVigor ? -1.f : FMath::Clamp((Vigor < 0.f ? Rules.VigorMax : Vigor) / FMath::Max(1.f, Rules.VigorMax), 0.f, 1.f);
}

void AValdoriaHeroCharacter::TickVigor(float DeltaSeconds)
{
	const FValdoriaMovementRules& Rules = GetMovementRules();
	if (!Rules.bUsesVigor)
	{
		return;
	}
	if (Vigor < 0.f)
	{
		Vigor = Rules.VigorMax;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (bIsSprinting && GetVelocity().Size2D() > 50.f && !bInfiniteDashes)
	{
		Vigor = FMath::Max(0.f, Vigor - Rules.VigorSprintPerSecond * DeltaSeconds);
		VigorLastSpend = Now;
		if (Vigor <= 0.f)
		{
			SetSprinting(false); // out of breath
		}
	}
	else if (Now - VigorLastSpend >= Rules.VigorRegenDelay)
	{
		Vigor = FMath::Min(Rules.VigorMax, Vigor + Rules.VigorRegenPerSecond * DeltaSeconds);
	}
}
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
	// Element-bound ability of the current element first (Kay), then the slot's always-available one.
	const int32 ElementInput = static_cast<int32>(Slot) + 16 * (CurrentElement + 1);
	const bool bElemental = ActiveDefinition && ActiveDefinition->ElementCount > 0;
	const bool bHasElemental = bElemental && AbilitySystem->GetActivatableAbilities().ContainsByPredicate([ElementInput](const FGameplayAbilitySpec& S) { return S.InputID == ElementInput; });
	const int32 WantedInput = bHasElemental ? ElementInput : static_cast<int32>(Slot);
	for (const FGameplayAbilitySpec& Spec : AbilitySystem->GetActivatableAbilities())
	{
		if (Spec.InputID == WantedInput)
		{
			static const bool bCombatLog = FParse::Param(FCommandLine::Get(), TEXT("ValdoriaCombatLog"));
			const bool bActivated = AbilitySystem->TryActivateAbility(Spec.Handle);
			if (bCombatLog && Slot != EValdoriaAbilitySlot::PrimaryFire)
			{
				UE_LOG(LogTemp, Log, TEXT("COMBATE habilidade %s slot=%d %s"), *GetName(), static_cast<int32>(Slot), bActivated ? TEXT("usou") : TEXT("NAO usou"));
			}
			if (bActivated)
			{
				if (bFlying && GetMovementRules().bFlightBlocksActions)
				{
					FlightPauseUntil = GetWorld()->GetTimeSeconds() + 0.6f; // stops in the air to attack, then flies on
				}
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
		Seconds *= 1.f - FMath::Clamp(ShopBonus.CooldownReduction, 0.f, 0.3f); // owner: items cap at 30% (50% only with special effects, later)
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
			if (!IsRailGuardian() && !IsForcedRiding())
			{
				StopRiding(); // hit by an enemy: fall off the trail (Heimdall never falls; forced riders stay on)
			}
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
		const float NewMax = (ActiveDefinition->MaxHealth + ShopBonus.MaxHealth) * ValdoriaStatScale * ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::MaxHealth);
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetMaxHealthAttribute(), NewMax);
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetHealthAttribute(),
			FMath::Clamp(GetHealth() + (ShopBonus.MaxHealth - OldExtraHealth) * ValdoriaStatScale * ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::MaxHealth), 1.f, NewMax));
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
	// Turn 48: global multipliers per mode (Config/Multiplicadores.ini).
	Physical *= ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::PhysicalDamage);
	Magical *= ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::MagicalDamage);

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
	if (Victim->IsInvulnerable() && !Victim->IsPlayerControlled())
	{
		ClientNotifyHit(Victim->GetActorLocation() + FVector(0, 0, 60.f), -1.f, false, false); // shows IMMUNE
		return 0.f;
	}

	// Passivas de quem ataca.
	const FValdoriaPassiveRules& Mine = GetPassive();
	const bool bFromAbility = !DamageSourceLabel.IsEmpty() && DamageSourceLabel != TEXT("Soco (V)");
	float Scale = bFromAbility ? ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::AbilityDamage) : ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::AttackDamage);
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
	const float BuffRed = GetWorld()->GetTimeSeconds() < Victim->BuffReductionUntil ? Victim->BuffReduction : 0.f;
	const float Kept = FMath::Max(0.25f, (1.f - Victim->GetPassiveDamageReduction()) * (1.f - Victim->GetTeamDamageReduction())
		* (1.f - BuffRed) * (1.f - Victim->RailGuardReduction) * (Victim->IsUngido() ? 0.9f : 1.f));
	if (IsRiding() && IsRailGuardian())
	{
		Physical *= 0.7f; // Heimdall on a Bifrost: -30% damage (owner: so he does not camp on it)
		Magical *= 0.7f;
	}
	float Total = (Physical + Magical) * Scale * Kept * (1.f + GetTeamDamageBonus());
	if (ShopBonus.AntiHeal > 0.f)
	{
		Victim->ApplyAntiHeal(ShopBonus.AntiHeal, 3.f); // shop: cuts the target's healing for 3 s
		Victim->SetStack(this, TEXT("antiheal"), TEXT("AC"), FLinearColor(0.75f, 0.2f, 0.25f), FMath::RoundToInt(ShopBonus.AntiHeal * 100.f), 3.f); // Bitter Edge: % of healing cut
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
	// Overheal on the victim soaks damage first.
	if (Victim->Overheal > 0.f)
	{
		const float Soaked = FMath::Min(Victim->Overheal, Total);
		Victim->Overheal -= Soaked;
		Total -= Soaked;
	}
	LastCombatTime = GetWorld()->GetTimeSeconds();
	if (Total > 0.f)
	{
		DealDamageTo(Victim, Total, bHeadshot);
	}
	if (Mine.ExecuteThreshold > 0.f && Victim->IsHeroUnit() && !Victim->IsDead() && Victim->GetHealth() > 0.f
		&& Victim->GetHealth() <= Victim->GetMaxHealth() * Mine.ExecuteThreshold)
	{
		ExecuteEnemy(Victim);
	}
	// Turn 47 (owner): VAMPIRISMO heals from the basic attack and the V punch (weapon value + buffs, max 40%);
	// ROUBO DE VIDA heals from abilities and passives (shop + buffs, max 30%). Caps are provisional.
	const float Rate = bFromAbility ? FMath::Min(0.3f, ShopBonus.LifeSteal + BonusLifeSteal + (IsUngido() ? 0.1f : 0.f)) * ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::LifeSteal)
		: FMath::Min(0.4f, GetWeaponStats().LifeSteal + BonusVampirism + (IsUngido() ? 0.2f : 0.f)) * ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::Vampirism);
	if (Rate > 0.f)
	{
		HealTarget(this, Total * Rate);
	}
	return Total;
}

void AValdoriaHeroCharacter::HealTarget(AActor* Target, float Amount)
{
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	AValdoriaHeroCharacter* Other = Cast<AValdoriaHeroCharacter>(Target);
	const FValdoriaWeaponStats& MyWeapon = GetWeaponStats();
	Amount *= ValdoriaStatScale * ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::Healing); // data values are design units (see ValdoriaStatScale)
	if (!HasAuthority() || !TargetASC || Amount <= 0.f || !Other || Other->IsDead()
		|| (Other->GetHealth() >= Other->GetMaxHealth() && MyWeapon.OverhealMax <= 0.f))
	{
		return;
	}
	FGameplayEffectSpecHandle Spec = AbilitySystem->MakeOutgoingSpec(UValdoriaHealEffect::StaticClass(), 1.f, AbilitySystem->MakeEffectContext());
	if (Spec.IsValid())
	{
		const float Received = Other->GetHealingReceivedMultiplier();
		const float Healed = Amount * (1.f + ShopBonus.HealingPower) * Received;
		// Healing past full health becomes overheal (decaying shield) when this hero's weapon allows it.
		const float Missing = FMath::Max(0.f, Other->GetMaxHealth() - Other->GetHealth());
		if (MyWeapon.OverhealMax > 0.f && Healed > Missing)
		{
			Other->AddOverheal(Healed - Missing, Other->GetMaxHealth() * MyWeapon.OverhealMax, MyWeapon.OverhealDecay);
		}
		Stats.Healing += FMath::Min(Healed, Missing); // scoreboard: real healing, not overheal
		if (Target != this && Missing > 1.f && IsLocallyControlled())
		{
			if (const APlayerController* MyPC = Cast<APlayerController>(Controller))
			{
				if (AValdoriaHUD* MyHUD = Cast<AValdoriaHUD>(MyPC->GetHUD()))
				{
					MyHUD->AddHealNumber(Other->GetActorLocation() + FVector(0, 0, 90.f), FMath::Min(Healed, Missing));
				}
			}
		}
		Spec.Data->SetSetByCallerMagnitude(ValdoriaTags::Data_Heal, Healed);
		static const bool bCombatLogHeal = FParse::Param(FCommandLine::Get(), TEXT("ValdoriaCombatLog"));
		if (bCombatLogHeal && Target != this)
		{
			UE_LOG(LogTemp, Log, TEXT("COMBATE cura %s -> %s %.0f"), *GetName(), *Other->GetName(), Healed);
		}
		// Comunhão (O Nazareno's passive, owner): allies near him receive a part of every heal he receives.
		static bool bSharing = false;
		const float Share = Other->GetPassive().SharedReceivedHeal;
		if (Share > 0.f && !bSharing && Healed > 0.f)
		{
			bSharing = true;
			for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
			{
				if (*It != Other && !It->IsDead() && It->TeamId == Other->TeamId && It->IsHeroUnit()
					&& FVector::Dist(It->GetActorLocation(), Other->GetActorLocation()) <= Other->GetPassive().Radius)
				{
					HealTarget(*It, Healed * Share / ValdoriaStatScale);
				}
			}
			bSharing = false;
		}
		if (Target != this)
		{
			LastCombatTime = GetWorld()->GetTimeSeconds(); // healing others counts as general combat
		}
		AbilitySystem->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
}

void AValdoriaHeroCharacter::AddOverheal(float Amount, float Cap, float DecayPerSecond)
{
	Overheal = FMath::Min(FMath::Max(Overheal, 0.f) + Amount, FMath::Max(Cap, Overheal));
	OverhealDecayRate = DecayPerSecond;
}

void AValdoriaHeroCharacter::ApplySlow(float Fraction, float Seconds)
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < SlowImmuneUntil)
	{
		return; // Caminhar sobre as Águas
	}
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
	VisualMesh->SetOnlyOwnerSee(bStealthHidden);
	WeaponComponent->SetOnlyOwnerSee(bStealthHidden);
	OffhandComponent->SetOnlyOwnerSee(bStealthHidden);
}

void AValdoriaHeroCharacter::SetWeaponOverride(const FValdoriaWeaponStats& NewStats, float Seconds)
{
	if (!bHasWeaponOverride)
	{
		SavedAmmo = CurrentAmmo;
	}
	WeaponOverride = NewStats;
	bHasWeaponOverride = true;
	WeaponOverrideEndTime = GetWorld()->GetTimeSeconds() + Seconds;
	bReloading = false;
	GetWorldTimerManager().ClearTimer(ReloadTimer);
	CurrentAmmo = NewStats.MagazineSize;
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
	// Braced posture also tames the kick (same posture multipliers as the spread, softened: sqrt).
	if (!GetCharacterMovement()->IsFalling() && (bIsProne || bIsCrouched))
	{
		const FValdoriaWeaponStats& Weapon = GetWeaponStats();
		Degrees *= FMath::Sqrt(bIsProne ? Weapon.ProneSpreadMultiplier : Weapon.CrouchSpreadMultiplier);
	}
	FRotator Rot = Controller->GetControlRotation();
	Rot.Pitch += Degrees;
	Rot.Yaw += FMath::FRandRange(-0.3f, 0.3f) * Degrees;
	Controller->SetControlRotation(Rot);
}

float AValdoriaHeroCharacter::GetSpreadMultiplier() const
{
	// Posture (owner, turn 44): crouching tightens the spread, lying down tightens it even more.
	const FValdoriaWeaponStats& Weapon = GetWeaponStats();
	const bool bGrounded = !GetCharacterMovement()->IsFalling();
	float Mult = 1.f;
	if (bGrounded && bIsProne)
	{
		Mult = Weapon.ProneSpreadMultiplier;
	}
	else if (bGrounded && bIsCrouched)
	{
		Mult = Weapon.CrouchSpreadMultiplier;
	}
	// Passives (Kael: standing still or crouched) stack on top.
	const float Still = GetPassive().StillSpreadMultiplier;
	if (Still < 1.f && (bIsCrouched || bIsProne || GetVelocity().Size2D() < 50.f) && bGrounded)
	{
		Mult *= Still;
	}
	return Mult;
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
	if (!bLevelsChecked)
	{
		// Decided here (not in BeginPlay): team and minion flags are set right after spawning.
		bLevelsChecked = true;
		bUsesLevels = bUsesLevels || (IsHeroUnit() && GetWorld()->GetAuthGameMode<AValdoriaMobaMode>() != nullptr);
		if (bUsesLevels && Level == 1 && AbilityPoints == 0 && AbilityRanks[0] + AbilityRanks[1] + AbilityRanks[2] == 0)
		{
			AbilityPoints = 1; // level 1 point
			if (!IsPlayerControlled())
			{
				AutoSpendPoints();
			}
		}
	}
	RailGuardReduction = 0.f;
	if (IsRailGuardian() && !IsRiding())
	{
		for (TActorIterator<AValdoriaLaneRail> It(GetWorld()); It; ++It)
		{
			float Along = 0.f;
			if (It->Team != 253 && It->DistanceToPoint(GetActorLocation(), Along) < 1500.f)
			{
				RailGuardReduction = 0.15f; // near a Bifrost but off it: 15% less damage
				break;
			}
		}
	}
	if (IsHeroUnit())
	{
		GainXP(2.f * Step);       // passive XP (provisional)
		Stats.Gold += 4.f * Step * ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::Gold); // same as the shop's credits per second
	}
	if (GetTeamRegen() > 0.f)
	{
		HealTarget(this, GetTeamRegen() * Step);
	}
	// Turn 46 (owner): out of combat every hero regenerates 1 HP every 0.5 s (real HP, so / ValdoriaStatScale).
	// Minions and guardians have their own rules.
	if (IsHeroUnit() && GetHealth() < GetMaxHealth() && !IsInPassiveCombat(5.f) && !IsInCombat(5.f))
	{
		HealTarget(this, OutOfCombatRegenHP * Step / ValdoriaStatScale * (IsUngido() ? 3.f : 1.f));
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
	if (HasAuthority() && Victim && Victim != this)
	{
		Stats.Damage += Damage;
		Victim->Stats.DamageTaken += Damage;
		Victim->RecentDamagers.Add(this, GetWorld()->GetTimeSeconds());
		if (Victim->IsHeroUnit())
		{
			FDamageLogEntry E;
			E.Attacker = GetUnitName();
			E.Source = !DamageSourceLabel.IsEmpty() ? DamageSourceLabel
				: (!GetWeaponStats().WeaponName.IsEmpty() ? GetWeaponStats().WeaponName.ToString() : FString(TEXT("Arma / projétil")));
			E.Amount = Damage;
			E.Time = GetWorld()->GetTimeSeconds();
			E.bAttackerIsHero = IsHeroUnit();
			Victim->DamageLog.Add(E);
			if (Victim->DamageLog.Num() > 80)
			{
				Victim->DamageLog.RemoveAt(0, 20);
			}
		}
		if (const UValdoriaShopCatalog* Catalog = GetDefault<UValdoriaShopCatalog>())
		{
			Stats.Gold += Damage / ValdoriaStatScale * Catalog->CreditsPerDamage * ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::Gold);
		}
	}
	if (ActiveDefinition && HasAuthority() && !IsUltimateReady())
	{
		const float Charge = Attributes->GetUltimateCharge() + Damage / ValdoriaStatScale * ActiveDefinition->UltimateChargePerDamage * ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::UltimateCharge);
		AbilitySystem->SetNumericAttributeBase(UValdoriaAttributeSet::GetUltimateChargeAttribute(), FMath::Min(Charge, ActiveDefinition->UltimateCost));
	}
	const bool bKill = Victim && Victim->GetHealth() <= 0.f;
	// Loja: roubo de vida e créditos por dano/abate.
	if (HasAuthority() && Victim && Victim != this)
	{
		// (shop life steal now applies in DealTypedDamage, to abilities only - turn 47)
		if (AValdoriaPlayerController* PC = Cast<AValdoriaPlayerController>(Controller))
		{
			PC->OnHeroDamageDealt(Damage / ValdoriaStatScale, bKill); // credits use design units
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
	AwardDeath(Killer);
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

void AValdoriaHeroCharacter::TickFallOutOfMap()
{
	// Owner (turn 44): falling out of the map kills the hero (normal respawn) and is logged.
	if (!HasAuthority() || bIsDead || IsRiding())
	{
		return;
	}
	if (FallKillZ == TNumericLimits<float>::Lowest())
	{
		// 30 m below the lowest player start of this map.
		float Lowest = GetActorLocation().Z;
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			Lowest = FMath::Min(Lowest, It->GetActorLocation().Z);
		}
		FallKillZ = Lowest - 3000.f;
	}
	const FVector P = GetActorLocation();
	if (P.Z >= FallKillZ)
	{
		return;
	}
	UE_LOG(LogValdoria, Warning, TEXT("PLAYTEST PROBLEMA caiu do mapa: %s (%s) em (%.0f,%.0f,%.0f)"), *GetName(),
		ActiveDefinition ? *ActiveDefinition->GetName() : TEXT("-"), P.X, P.Y, P.Z);
	if (bGodMode)
	{
		// Immortal (admin): back to the spawn instead of dying.
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			TeleportTo(It->GetActorLocation(), GetActorRotation());
			GetCharacterMovement()->Velocity = FVector::ZeroVector;
			break;
		}
		return;
	}
	HandleDeath(nullptr);
}

void AValdoriaHeroCharacter::StartTelekineticDash(AValdoriaHeroCharacter* Target, float Speed, float Damage, float Push, AActor* Shield)
{
	if (!Target || bIsDead)
	{
		return;
	}
	TkDashTarget = Target;
	TkDashShield = Shield;
	TkDashSpeed = FMath::Max(800.f, Speed);
	TkDashDamage = Damage;
	TkDashPush = Push;
	TkDashEnd = GetWorld()->GetTimeSeconds() + 1.5f;
	GetCharacterMovement()->SetMovementMode(MOVE_Flying); // no gravity while she flies to the target
}

void AValdoriaHeroCharacter::TickTelekineticDash()
{
	AValdoriaHeroCharacter* Target = TkDashTarget.Get();
	if (!Target)
	{
		return;
	}
	UCharacterMovementComponent* Move = GetCharacterMovement();
	auto Finish = [this, Move]()
	{
		if (AActor* Shield = TkDashShield.Get())
		{
			Shield->Destroy(); // the shield lasts until she arrives
		}
		TkDashTarget = nullptr;
		TkDashShield = nullptr;
		Move->Velocity *= 0.15f;
		if (!bFlying && !bNoclip && Move->MovementMode == MOVE_Flying)
		{
			Move->SetMovementMode(MOVE_Falling);
		}
	};
	if (bIsDead || Target->IsDead() || GetWorld()->GetTimeSeconds() > TkDashEnd || HasStatus(EValdoriaStatus::Stunned) || HasStatus(EValdoriaStatus::Rooted))
	{
		Finish();
		return;
	}
	const FVector To = Target->GetActorLocation() - GetActorLocation();
	const float Reach = GetCapsuleComponent()->GetScaledCapsuleRadius() + Target->GetCapsuleComponent()->GetScaledCapsuleRadius() + 60.f;
	if (To.Size() <= Reach)
	{
		if (HasAuthority() && IsEnemy(Target))
		{
			// Arrives with a telekinetic shove: damage, pushed away and briefly off balance.
			DealTypedDamage(Target, TkDashDamage, EValdoriaDamageType::Physical, 0.5f, false);
			Target->LaunchCharacter(To.GetSafeNormal2D() * TkDashPush + FVector(0.f, 0.f, 250.f), true, true);
			Target->ApplyStatus(EValdoriaStatus::Stunned, 0.4f);
			PlayAreaEffect(Target->GetActorLocation(), 150.f, ActiveDefinition ? ActiveDefinition->ThemeColor : FLinearColor::White);
		}
		Finish();
		return;
	}
	Move->Velocity = To.GetSafeNormal() * TkDashSpeed;
	SetActorRotation(FRotator(0.f, To.Rotation().Yaw, 0.f));
}

void AValdoriaHeroCharacter::TickMapBounds()
{
	if (bIsDead || IsRiding())
	{
		return; // the Bifrost flies above the roofs on purpose
	}
	if (!bMapBoundsReady)
	{
		bMapBoundsReady = true;
		FBox Geometry(ForceInit);
		for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
		{
			const UStaticMeshComponent* C = It->GetStaticMeshComponent();
			if (!C || C->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
			{
				continue;
			}
			const FBox B = C->Bounds.GetBox();
			if (B.IsValid && B.GetExtent().GetMax() < 100000.f) // skip sky domes / giant helper meshes
			{
				Geometry += B;
			}
		}
		// Turn 45: the side limits follow the GROUND (large flat pieces near street level), a little inside its edge,
		// instead of 5 m past every mesh: heroes could glide over the border walls or walk off the floor edge and fall.
		FBox Ground(ForceInit);
		for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
		{
			const UStaticMeshComponent* C = It->GetStaticMeshComponent();
			if (!C || C->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
			{
				continue;
			}
			const FBox B = C->Bounds.GetBox();
			const FVector E = B.GetExtent();
			if (B.IsValid && E.GetMax() < 100000.f && E.Z < 120.f && FMath::Max(E.X, E.Y) >= 400.f && B.Max.Z < 200.f)
			{
				Ground += B; // floors, streets, platforms (thin and wide)
			}
		}
		if (Geometry.IsValid)
		{
			MapBounds = Geometry.ExpandBy(FVector(MapBoundsPadding, MapBoundsPadding, 0.f));
			MapCeilingZ = Geometry.Max.Z + MapCeilingAbove;
		}
		if (Ground.IsValid && Geometry.IsValid)
		{
			const FBox Inner = Ground.ExpandBy(FVector(-MapGroundInset, -MapGroundInset, 0.f));
			MapBounds = FBox(FVector(Inner.Min.X, Inner.Min.Y, MapBounds.Min.Z), FVector(Inner.Max.X, Inner.Max.Y, MapBounds.Max.Z));
		}
	}
	if (!MapBounds.IsValid)
	{
		return;
	}
	UCharacterMovementComponent* Move = GetCharacterMovement();
	FVector P = GetActorLocation();
	bool bClamped = false;
	// Sides: invisible walls (stop the outward speed, keep sliding along the wall).
	for (int32 Axis = 0; Axis < 2; ++Axis)
	{
		if (P[Axis] < MapBounds.Min[Axis]) { P[Axis] = MapBounds.Min[Axis]; Move->Velocity[Axis] = FMath::Max(0.f, Move->Velocity[Axis]); bClamped = true; }
		if (P[Axis] > MapBounds.Max[Axis]) { P[Axis] = MapBounds.Max[Axis]; Move->Velocity[Axis] = FMath::Min(0.f, Move->Velocity[Axis]); bClamped = true; }
	}
	// Sky: soft ceiling. Upward speed is capped lower and lower inside the band, reaching 0 at the limit.
	if (Move->Velocity.Z > 0.f && P.Z > MapCeilingZ - MapCeilingSoft)
	{
		const float Alpha = FMath::Clamp((MapCeilingZ - P.Z) / MapCeilingSoft, 0.f, 1.f);
		Move->Velocity.Z = FMath::Min(Move->Velocity.Z, 1500.f * Alpha);
	}
	if (P.Z > MapCeilingZ)
	{
		P.Z = MapCeilingZ;
		Move->Velocity.Z = FMath::Min(0.f, Move->Velocity.Z);
		bClamped = true;
	}
	if (bClamped)
	{
		SetActorLocation(P, false, nullptr, ETeleportType::None);
	}
}

void AValdoriaHeroCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TickVisualAttachments();
	{
		// -ValdoriaJumpLog: logs any frame where the hero moved much more than its speed allows (teleport hunting).
		static const bool bJumpLog = FParse::Param(FCommandLine::Get(), TEXT("ValdoriaJumpLog"));
		if (bJumpLog && IsLocallyControlled())
		{
			const FVector Now = GetActorLocation();
			const float Step = FVector::Dist2D(Now, JumpLogLast);
			const float Allowed = GetVelocity().Size2D() * DeltaSeconds * 1.5f + 3.f;
			if (JumpLogLast != FVector::ZeroVector && Step > Allowed && Step < 2000.f)
			{
				UE_LOG(LogValdoria, Display, TEXT("JUMPLOG salto %.1f cm (permitido %.1f) vel=%.0f agachado=%d deitado=%d modo=%d dz=%.1f"),
					Step, Allowed, GetVelocity().Size2D(), bIsCrouched, bIsProne, (int32)GetCharacterMovement()->MovementMode.GetValue(), Now.Z - JumpLogLast.Z);
			}
			JumpLogLast = Now;
			// The body itself: the pelvis offset from the capsule must not snap between frames.
			if (GetMesh() && GetMesh()->GetBoneIndex(TEXT("pelvis")) != INDEX_NONE)
			{
				const FVector Off = GetMesh()->GetBoneLocation(TEXT("pelvis")) - Now;
				const float Snap = FVector::Dist2D(Off, JumpLogPelvis);
				if (JumpLogPelvis != FVector::ZeroVector && Snap > 12.f)
				{
					UE_LOG(LogValdoria, Display, TEXT("JUMPLOG pelvis pulou %.1f cm agachado=%d deitado=%d"), Snap, bIsCrouched, bIsProne);
				}
				JumpLogPelvis = Off;
			}
		}
	}
	if (Overheal > 0.f)
	{
		Overheal = FMath::Max(0.f, Overheal - OverhealDecayRate * DeltaSeconds);
	}
	TickFallOutOfMap();
	TickTelekineticDash();
	TickUltimateHold();
	TickVigor(DeltaSeconds);
	TickMapBounds();
	TickMovementFeatures(DeltaSeconds);
	TickStatusEffects(DeltaSeconds);
	TickHover(DeltaSeconds);
	TickFootDiscs(DeltaSeconds);
	TickFootsteps(DeltaSeconds);
	TickFlightPose(DeltaSeconds);
	// Telekinetic weapons (Vex): they bob gently around their socket, out of phase, as if held by the mind.
	if (ActiveDefinition && ActiveDefinition->bWeaponsFloat && !IsRiding())
	{
		const float T = GetWorld()->GetTimeSeconds();
		WeaponComponent->SetRelativeLocation(ActiveDefinition->WeaponOffset.GetLocation() + FVector(FMath::Sin(T * 2.1f) * 3.f, 0.f, FMath::Sin(T * 1.7f) * 2.f));
		OffhandComponent->SetRelativeLocation(ActiveDefinition->OffhandOffset.GetLocation() + FVector(FMath::Sin(T * 2.1f + 1.6f) * 3.f, 0.f, FMath::Sin(T * 1.7f + 1.1f) * 2.f));
	}
	TickAutopilot(DeltaSeconds);
	TickRiding(DeltaSeconds);
	TickChannel(DeltaSeconds);
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
	VisualMesh->SetOwnerNoSee(bEnable);
	WeaponComponent->SetOwnerNoSee(bEnable);
	OffhandComponent->SetOwnerNoSee(bEnable);
}

void AValdoriaHeroCharacter::SetNoclip(bool bEnable)
{
	bNoclip = bEnable;
	SetActorEnableCollision(!bEnable);
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (bEnable)
	{
		Move->SetMovementMode(MOVE_Flying);
		Move->MaxFlySpeed = 1500.f * FMath::Max(1.f, AdminSpeedMultiplier);
		Move->BrakingDecelerationFlying = 4000.f;
	}
	else
	{
		Move->SetMovementMode(MOVE_Falling);
	}
}

void AValdoriaHeroCharacter::DealRawDamageTo(AActor* Target, float RealDamage)
{
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!HasAuthority() || !TargetASC || RealDamage <= 0.f)
	{
		return;
	}
	FGameplayEffectContextHandle Context = AbilitySystem->MakeEffectContext();
	Context.AddInstigator(this, this);
	FGameplayEffectSpecHandle Spec = AbilitySystem->MakeOutgoingSpec(UValdoriaDamageEffect::StaticClass(), 1.f, Context);
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(ValdoriaTags::Data_Damage, RealDamage);
		AbilitySystem->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
}

bool AValdoriaHeroCharacter::IsInvulnerable() const
{
	return bGodMode || (GetWorld() && GetWorld()->GetTimeSeconds() < DamageImmuneUntil);
}

bool AValdoriaHeroCharacter::IsUngido() const
{
	return GetWorld() && GetWorld()->GetTimeSeconds() < UngidoUntil;
}

void AValdoriaHeroCharacter::Cleanse()
{
	StatusEndTimes.Empty();
	SlowFraction = 0.f;
	SlowEndTime = 0.f;
	UpdateMovementSpeed();
}

bool AValdoriaHeroCharacter::TrySurviveLethal(float HitDamage)
{
	UWorld* W = GetWorld();
	if (!W || bLastStandUsed || W->GetTimeSeconds() >= LastStandGuardUntil || HitDamage > GetMaxHealth() * 0.3f)
	{
		return false;
	}
	// Crucificado (owner): falls with 1 HP, takes 1 s to stand up, immune meanwhile; after that any damage kills.
	bLastStandUsed = true;
	DamageImmuneUntil = W->GetTimeSeconds() + 1.f;
	StatusEndTimes.FindOrAdd(EValdoriaStatus::Stunned) = W->GetTimeSeconds() + 1.f;
	return true;
}

void AValdoriaHeroCharacter::ExecuteEnemy(AValdoriaHeroCharacter* Victim)
{
	if (!HasAuthority() || !Victim || Victim->IsDead() || Victim->IsInvulnerable())
	{
		return; // damage immunity (Crucificado, god mode) also stops the execution
	}
	UWorld* W = GetWorld();
	const float Now = W->GetTimeSeconds();
	// Credit (owner, round 4): the ally who hurt this enemy most in the last 10 s lands the kill; he gets the assist.
	AValdoriaHeroCharacter* Best = nullptr;
	float BestDamage = 0.f;
	for (TActorIterator<AValdoriaHeroCharacter> It(W); It; ++It)
	{
		AValdoriaHeroCharacter* A = *It;
		if (A == this || A->IsDead() || A->TeamId != TeamId || !A->IsHeroUnit())
		{
			continue;
		}
		const FString Name = A->GetUnitName();
		float Sum = 0.f;
		for (const FDamageLogEntry& E : Victim->DamageLog)
		{
			if (E.Attacker == Name && Now - E.Time <= 10.f)
			{
				Sum += E.Amount;
			}
		}
		if (Sum > BestDamage)
		{
			BestDamage = Sum;
			Best = A;
		}
	}
	AValdoriaHeroCharacter* Executor = Best ? Best : this;
	Victim->LastStandGuardUntil = 0.f; // the execution is not a "hit" the Crucificado guard can save
	Executor->DamageSourceLabel = TEXT("Deus do Velho Testamento");
	Executor->DealRawDamageTo(Victim, Victim->GetHealth() + Victim->GetShield() + Victim->GetArmor() * 3.f + 100.f);
	Executor->DamageSourceLabel.Reset();
	if (!Victim->IsDead())
	{
		return;
	}
	PlayAreaEffect(Victim->GetActorLocation(), 300.f, FLinearColor(1.f, 0.88f, 0.55f));
	// Heal: nearest ally within range heals 35% of missing health; nobody near -> himself.
	const FValdoriaPassiveRules& Mine = GetPassive();
	AValdoriaHeroCharacter* Healed = this;
	float BestDist = Mine.ExecuteHealRadius;
	for (TActorIterator<AValdoriaHeroCharacter> It(W); It; ++It)
	{
		AValdoriaHeroCharacter* A = *It;
		const float D = FVector::Dist(A->GetActorLocation(), GetActorLocation());
		if (A != this && !A->IsDead() && A->TeamId == TeamId && A->IsHeroUnit() && D <= BestDist)
		{
			BestDist = D;
			Healed = A;
		}
	}
	const float Missing = FMath::Max(0.f, Healed->GetMaxHealth() - Healed->GetHealth());
	if (Mine.ExecuteHealMissing > 0.f && Missing > 0.f)
	{
		HealTarget(Healed, Missing * Mine.ExecuteHealMissing / ValdoriaStatScale);
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
		Spec.Data->SetSetByCallerMagnitude(ValdoriaTags::Data_Damage, Damage * ValdoriaStatScale * FMath::Max(0.f, GetDamageMultiplier()) * ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::Damage));
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
	float Speed = ActiveDefinition->MoveSpeed * AdminSpeedMultiplier * (1.f - SlowFraction) * (1.f + BuffFraction) * (1.f + ShopBonus.MoveSpeed) * ValdoriaMultipliers::Get(GetWorld(), EValdoriaMult::MoveSpeed);
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
	const bool bOutOfVigor = GetMovementRules().bUsesVigor && Vigor >= 0.f && Vigor < 5.f; // Lume: needs breath to run
	bNewSprinting = bNewSprinting && bAllowed && !bIsDead && !bOutOfVigor;
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
	const FValdoriaMovementRules& DashRules = GetMovementRules();
	if (DashRules.bUsesVigor && !bInfiniteDashes && Vigor >= 0.f && Vigor < DashRules.VigorDashCost)
	{
		return; // Lume: not enough vigor for a dash
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
		if (GetMovementRules().bUsesVigor)
		{
			Vigor = FMath::Max(0.f, (Vigor < 0.f ? GetMovementRules().VigorMax : Vigor) - GetMovementRules().VigorDashCost);
			VigorLastSpend = GetWorld()->GetTimeSeconds();
		}
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
	// Lie-down / get-up transition clip (plays once, full body, a bit faster than the clip's own pace).
	if (ActiveDefinition && !bIsDead)
	{
		const TSoftObjectPtr<UAnimSequenceBase>& Trans = bIsProne ? ActiveDefinition->Animations.ProneEnter : ActiveDefinition->Animations.ProneExit;
		UValdoriaAnimInstance* TransAnim = GetMesh() ? Cast<UValdoriaAnimInstance>(GetMesh()->GetAnimInstance()) : nullptr;
		if (TransAnim && !Trans.IsNull())
		{
			TransAnim->PlayAction(Trans.LoadSynchronous(), 1.8f, true);
		}
	}
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
		if (!IsForcedRiding())
		{
			StopRiding(); // tap Space again to get off (not while carried by Chamado da Bifrost)
		}
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
	// Kay (owner, turn 48): in the air, Space again = an elemental jump with fire (burst up) or air (dash forward),
	// once until landing. Earth and water have none.
	if (ActiveDefinition && ActiveDefinition->ElementCount > 0 && GetCharacterMovement()->IsFalling() && !bElementJumpUsed
		&& (CurrentElement == 0 || CurrentElement == 3))
	{
		bElementJumpUsed = true;
		const FVector Fwd = FRotator(0.f, GetControlRotation().Yaw, 0.f).Vector();
		if (CurrentElement == 0)
		{
			LaunchCharacter(Fwd * 250.f + FVector(0.f, 0.f, 950.f), false, true); // fire: burst upward
		}
		else
		{
			LaunchCharacter(Fwd * 1100.f + FVector(0.f, 0.f, 450.f), true, true); // air: gust forward
		}
		const FLinearColor C = ActiveDefinition->ElementColors.IsValidIndex(CurrentElement) ? ActiveDefinition->ElementColors[CurrentElement] : ActiveDefinition->ThemeColor;
		PlayAreaEffect(GetActorLocation() - FVector(0.f, 0.f, 80.f), 180.f, C);
		return;
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
	// Admin flight (any hero): behaves like the fuel mode with infinite fuel, and keeps collision (no noclip).
	const float FuelSeconds = bAdminFlight ? FMath::Max(Rules.FlightFuelSeconds, 10.f) : Rules.FlightFuelSeconds;
	if (FlightFuel < 0.f || bInfiniteFlight || bAdminFlight || Rules.bUnlimitedFlight)
	{
		FlightFuel = FuelSeconds;
	}
	if (Move->IsMovingOnGround())
	{
		FlightFuel = FMath::Min(FuelSeconds, FlightFuel + Rules.FlightFuelRegen * DeltaSeconds);
	}
	const bool bAbilityFlight = Rules.FlightMode == EValdoriaFlightMode::Ability && Now < AbilityFlightEnd;
	// Turn 48 (owner, Vex): with unlimited flight she keeps levitating after Space is released and comes down the same
	// way she goes up, until she reaches the ground.
	bool bNearGround = false;
	if (bFlying && Rules.bUnlimitedFlight)
	{
		FHitResult Down;
		FCollisionQueryParams Q(SCENE_QUERY_STAT(ValdoriaLevitate), false, this);
		const float Half = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		bNearGround = GetWorld()->LineTraceSingleByChannel(Down, GetActorLocation(), GetActorLocation() - FVector(0.f, 0.f, Half + 12.f), ECC_WorldStatic, Q);
	}
	const bool bLevitating = bFlying && Rules.bUnlimitedFlight && !bAdminFlight && !(bNearGround && !bJumpHeld);
	const bool bFuelFlight = (Rules.FlightMode == EValdoriaFlightMode::Fuel || bAdminFlight)
		&& ((bJumpHeld && (Move->IsFalling() || bFlying)) || bLevitating) && FlightFuel > 0.f && !IsFacingWall(45.f);
	if (bAbilityFlight || bFuelFlight)
	{
		if (!bFlying)
		{
			bFlying = true;
			Move->SetMovementMode(MOVE_Flying);
			Move->MaxFlySpeed = bAdminFlight ? FMath::Max(Rules.FlightSpeed, 900.f) : Rules.FlightSpeed;
			Move->BrakingDecelerationFlying = 1500.f;
		}
		if (Now < FlightPauseUntil)
		{
			Move->Velocity = FVector::ZeroVector; // hovering still while she attacks from the sky
			bGliding = false;
			return;
		}
		if (bFuelFlight)
		{
			FlightFuel = FMath::Max(0.f, FlightFuel - DeltaSeconds);
			if (bJumpHeld)
			{
				AddMovementInput(FVector::UpVector, 1.f); // segurar Espaço sobe
			}
			else if (bCrouchHeld)
			{
				AddMovementInput(FVector::UpVector, -1.f);
			}
			else
			{
				AddMovementInput(FVector::UpVector, -0.6f); // released: comes down levitating, like going up
			}
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
		Move->AirControl = AdminAirControl >= 0.f ? AdminAirControl : ActiveDefinition->AirControl;
	}
	// Aim glide (Recruit): aiming or shooting while falling slows the fall, no Space needed.
	bAimGliding = !bGliding && Rules.bAimGlide && (bSecondaryHeld || bFireHeld) && Move->IsFalling() && Move->Velocity.Z < -Rules.AimGlideFallSpeed;
	if (bAimGliding)
	{
		Move->Velocity.Z = -Rules.AimGlideFallSpeed;
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
	bElementJumpUsed = false;
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
	if (IsRiding() && bRideVertical && !Set.RideVerticalPose.IsNull())
	{
		return Load(Set.RideVerticalPose); // rising upright at the spawn
	}
	if (IsRiding() && (!Set.RidePose.IsNull() || !Set.FallLoop.IsNull()))
	{
		return Load(!Set.RidePose.IsNull() ? Set.RidePose : Set.FallLoop); // leaning forward, arms back
	}
	if (bGliding && !Set.Glide.IsNull()) // aim glide keeps the aiming pose
	{
		const float GFwd = FVector::DotProduct(Velocity, GetActorForwardVector());
		const float GRight = FVector::DotProduct(Velocity, GetActorRightVector());
		if (Speed > 150.f && FMath::Abs(GRight) > FMath::Abs(GFwd) * 0.8f)
		{
			const TSoftObjectPtr<UAnimSequenceBase>& Side = GRight >= 0.f ? Set.GlideRight : Set.GlideLeft;
			if (!Side.IsNull())
			{
				return Load(Side);
			}
		}
		return Load(Set.Glide);
	}
	if (IsSkating() && GetCharacterMovement()->IsMovingOnGround() && !Set.CrouchIdle.IsNull())
	{
		return Load(Set.CrouchIdle); // knees bent, no steps (reference video: sliding on a light trail)
	}
	if (bFlying)
	{
		// Levitate: calm upright pose (owner reference video), superhero: flying loop.
		const bool bLevitate = Set.FlightStyle == EValdoriaFlightStyle::Levitate && !Set.Hover.IsNull();
		if (!bLevitate)
		{
			// Turn 45 (owner: more flight poses for the Recruit): diving, moving and hovering differ.
			if (Velocity.Z < -350.f && !Set.FlyDown.IsNull())
			{
				return Load(Set.FlyDown);
			}
			if (Speed > 250.f && !Set.FlyMove.IsNull())
			{
				return Load(Set.FlyMove);
			}
		}
		const TSoftObjectPtr<UAnimSequenceBase>& FlyClip = bLevitate ? Set.Hover : Set.Fly;
		if (!FlyClip.IsNull())
		{
			return Load(FlyClip);
		}
	}
	// In the air after a jump: rising / falling poses (Lume was stiff in the air, turn 45).
	if (GetCharacterMovement()->IsFalling() && !bFlying)
	{
		const TSoftObjectPtr<UAnimSequenceBase>& AirClip = Velocity.Z > 60.f ? Set.AirRise : Set.AirFall;
		if (!AirClip.IsNull())
		{
			return Load(AirClip);
		}
	}
	// Flutuar: pairando sem passos (qualquer velocidade), a não ser agachado.
	if (ActiveDefinition->Locomotion == EValdoriaLocomotion::Float && !bIsCrouched && !bSliding && !Set.Hover.IsNull())
	{
		return Load(Set.Hover);
	}
	if (bIsProne)
	{
		if (Speed > 20.f && !Set.ProneCrawl.IsNull())
		{
			// Crawl in the direction of travel (backwards / sideways clips are optional).
			const float PFwd = FVector::DotProduct(Velocity, GetActorForwardVector());
			const float PRight = FVector::DotProduct(Velocity, GetActorRightVector());
			const TSoftObjectPtr<UAnimSequenceBase>* Pick = &Set.ProneCrawl;
			if (FMath::Abs(PFwd) >= FMath::Abs(PRight) * 0.8f)
			{
				if (PFwd < 0.f && !Set.ProneCrawlBackward.IsNull()) { Pick = &Set.ProneCrawlBackward; }
			}
			else if (PRight >= 0.f)
			{
				if (!Set.ProneCrawlRight.IsNull()) { Pick = &Set.ProneCrawlRight; }
			}
			else if (!Set.ProneCrawlLeft.IsNull())
			{
				Pick = &Set.ProneCrawlLeft;
			}
			return Load(*Pick);
		}
		return Load(Set.ProneIdle);
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
	if (GetWorld()->GetTimeSeconds() < CCImmuneUntil
		&& (Status == EValdoriaStatus::Stunned || Status == EValdoriaStatus::Rooted || Status == EValdoriaStatus::Silenced))
	{
		return; // control immunity (Gjallarhorn and future casting elevations)
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
	// Turn 48 (owner): Vex may attack from the sky; the flight pauses for a moment instead (see TryActivateSlot).
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
	if (!DebugBaseAnim.IsNull())
	{
		bOutLoop = true;
		return DebugBaseAnim.LoadSynchronous(); // pose gallery test
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

void AValdoriaHeroCharacter::TickVisualAttachments()
{
	// Mannequin bone -> VRoid (VRM humanoid) bone, for the sockets the heroes use.
	auto ToVroid = [](FName Bone) -> FName
	{
		const FString B = Bone.ToString().ToLower();
		if (B == TEXT("hand_r")) return TEXT("J_Bip_R_Hand");
		if (B == TEXT("hand_l")) return TEXT("J_Bip_L_Hand");
		if (B == TEXT("lowerarm_r")) return TEXT("J_Bip_R_LowerArm");
		if (B == TEXT("lowerarm_l")) return TEXT("J_Bip_L_LowerArm");
		if (B == TEXT("spine_01") || B == TEXT("spine_02")) return TEXT("J_Bip_C_Spine");
		if (B == TEXT("spine_03")) return TEXT("J_Bip_C_Chest");
		if (B == TEXT("spine_04") || B == TEXT("spine_05")) return TEXT("J_Bip_C_UpperChest");
		if (B == TEXT("neck_01") || B == TEXT("neck_02")) return TEXT("J_Bip_C_Neck");
		if (B == TEXT("head")) return TEXT("J_Bip_C_Head");
		if (B == TEXT("pelvis")) return TEXT("J_Bip_C_Hips");
		return NAME_None;
	};
	const bool bVisual = VisualMesh && VisualMesh->IsVisible() && VisualMesh->GetSkeletalMeshAsset() != nullptr;
	UStaticMeshComponent* Items[2] = { WeaponComponent, OffhandComponent };
	for (int32 i = 0; i < 2; ++i)
	{
		UStaticMeshComponent* C = Items[i];
		if (!C || C->GetAttachParent() != GetMesh())
		{
			continue;
		}
		// Someone else (attach, ability, pose) placed the item: that is the new base offset.
		const FVector Rel = C->GetRelativeLocation();
		if (!Rel.Equals(VisualWrittenRel[i], 0.01f))
		{
			VisualBaseRel[i] = Rel;
		}
		FVector Want = VisualBaseRel[i];
		if (bVisual)
		{
			const FName Socket = C->GetAttachSocketName();
			const FName Bone = GetMesh()->GetSocketBoneName(Socket);
			const FName VBone = ToVroid(Bone);
			if (!VBone.IsNone() && VisualMesh->GetBoneIndex(VBone) != INDEX_NONE && GetMesh()->GetBoneIndex(Bone) != INDEX_NONE)
			{
				// Same orientation (the retargeter copies rotations), only the position differs: move by the gap
				// between the hidden hand and the visible hand, expressed in the socket's space.
				const FVector Gap = VisualMesh->GetBoneLocation(VBone) - GetMesh()->GetBoneLocation(Bone);
				Want += GetMesh()->GetSocketTransform(Socket).InverseTransformVector(Gap);
			}
		}
		if (!Want.Equals(Rel, 0.01f))
		{
			C->SetRelativeLocation(Want);
		}
		VisualWrittenRel[i] = Want;
	}
}

void AValdoriaHeroCharacter::TickFootDiscs(float DeltaSeconds)
{
	if (!ActiveDefinition || !ActiveDefinition->bRideEffectPerFoot || ActiveDefinition->Locomotion != EValdoriaLocomotion::RideEffect)
	{
		return;
	}
	// Owner (turn 45, Vex): a small telekinetic disc under each foot, created just before the foot lands
	// and fading after it lifts. The disc stays where it was made (absolute location), like a stepping stone.
	const bool bGrounded = !bIsDead && GetCharacterMovement()->IsMovingOnGround() && !bFlying;
	const float Sole = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float DiscZ = Sole + ActiveDefinition->HoverHeight * 0.5f;
	static const FName Bones[2] = { TEXT("foot_l"), TEXT("foot_r") };
	UStaticMeshComponent* Discs[2] = { FootDiscL.Get(), FootDiscR.Get() };
	// Turn 48 (owner): when she runs fast the feet outpace the discs, so the next disc appears ahead of the step
	// (where the foot is going to land) and a little earlier while the foot is still coming down.
	const FVector Vel2D = FVector(GetVelocity().X, GetVelocity().Y, 0.f);
	const float Speed = Vel2D.Size();
	const FVector Lead = Vel2D * FMath::Clamp(Speed / 1500.f, 0.f, 1.f) * 0.12f;
	const float LowThreshold = 22.f + FMath::Min(Speed * 0.03f, 30.f);
	for (int32 i = 0; i < 2; ++i)
	{
		UStaticMeshComponent* Disc = Discs[i];
		if (!Disc)
		{
			continue;
		}
		const FVector Foot = GetMesh()->GetSocketLocation(Bones[i]);
		// The ankle bone sits ~10 cm above the sole: "low" = the foot is landing or planted.
		const bool bLow = bGrounded && Foot.Z - (Sole + ActiveDefinition->HoverHeight) < LowThreshold;
		float& Alpha = FootDiscAlpha[i];
		if (bLow)
		{
			const FVector Target(Foot.X, Foot.Y, DiscZ);
			// New step: appear where the foot will land (ahead when running fast); while planted, follow it (fast
			// enough to keep up at high speed, gently when turning in place).
			const FVector Where = Alpha < 0.05f ? Target + Lead : FMath::VInterpTo(Disc->GetComponentLocation(), Target + Lead * 0.5f, DeltaSeconds, Speed > 700.f ? 25.f : 10.f);
			Disc->SetWorldLocation(Where);
			Alpha = FMath::Min(1.f, Alpha + DeltaSeconds * 14.f);
		}
		else
		{
			Alpha = FMath::Max(0.f, Alpha - DeltaSeconds * (bGrounded ? 5.f : 9.f));
		}
		const bool bShow = Alpha > 0.02f;
		Disc->SetVisibility(bShow);
		if (bShow)
		{
			const FVector Base = ActiveDefinition->RideEffectScale;
			const float Grow = 0.55f + 0.45f * Alpha;
			Disc->SetWorldScale3D(FVector(Base.X * Grow, Base.Y * Grow, Base.Z));
			Disc->SetWorldRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
		}
	}
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
	if (!ActiveDefinition || bIsDead || ActiveDefinition->FootstepVolume <= 0.f || ActiveDefinition->bSilentMovement
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
		// Holding the primary fire: upper-body channel pose (Lume's draining beam).
		const bool bHold = !bSkating && !IsRiding() && bFireHeld && !Set.FireHoldPose.IsNull();
		if (Anim && (bHold || bWasFireHold))
		{
			Anim->SetUpperPose(bHold ? Load(Set.FireHoldPose) : nullptr, Set.FireHoldPoseTime, bHold ? 1.f : 0.f);
		}
		bWasFireHold = bHold;
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

	// Levitate: upright when still, tilts forward/back with the movement (owner, turn 48: the tilt was only 12 degrees,
	// it looked like no tilt at all). Superhero: lies down flying fast (FlightLean, 80 by default).
	const float MaxLean = bSuperhero ? Set.FlightLean : FMath::Min(Set.FlightLean, FMath::Max(ActiveDefinition->HoverLean, 32.f));
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

FVector AValdoriaHeroCharacter::AutopilotNavDirection(const FVector& Goal)
{
	const float Now = GetWorld()->GetTimeSeconds();
	const FVector P = GetActorLocation();
	if (Now >= NavRepathTime || FVector::DistSquared(Goal, NavGoal) > FMath::Square(400.f))
	{
		NavRepathTime = Now + NavRepathInterval;
		NavGoal = Goal;
		NavPath.Reset();
		NavPathIndex = 0;
		if (UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), P, Goal, this))
		{
			if (Path->IsValid() && Path->PathPoints.Num() > 1)
			{
				NavPath = Path->PathPoints;
				NavPathIndex = 1;
			}
		}
	}
	while (NavPath.IsValidIndex(NavPathIndex) && FVector::Dist2D(NavPath[NavPathIndex], P) < 120.f)
	{
		++NavPathIndex;
	}
	const FVector Next = NavPath.IsValidIndex(NavPathIndex) ? NavPath[NavPathIndex] : Goal;
	const FVector Dir = (Next - P).GetSafeNormal2D();
	return Dir.IsNearlyZero() ? GetActorForwardVector().GetSafeNormal2D() : Dir;
}

void AValdoriaHeroCharacter::TickAutopilot(float DeltaSeconds)
{
	if (!bAutopilot || bIsDead || !ActiveDefinition)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	const FVector Eye = GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);
	const UValdoriaVisionSubsystem* FogVision = GetWorld()->GetSubsystem<UValdoriaVisionSubsystem>();
	auto CanSee = [this, &Eye, FogVision](const AValdoriaHeroCharacter* Other)
	{
		if (FogVision && FogVision->IsHiddenInFogFrom(Other, TeamId))
		{
			return false; // hidden in a fog area (turn 47)
		}
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ValdoriaAutopilot), false, this);
		IgnoreFriendlyBarriers(Params);
		return !GetWorld()->LineTraceSingleByChannel(Hit, Eye, Other->GetActorLocation() + FVector(0.f, 0.f, 40.f), ECC_Visibility, Params)
			|| Hit.GetActor() == Other;
	};

	// Target: nearest visible enemy, re-checked twice a second.
	if (Now >= AutopilotNextPick)
	{
		AutopilotNextPick = Now + AutopilotPickInterval;
		AutopilotTarget = nullptr;
		// Turn 46 (owner, MOBA logic): clear the minion wave first, then fight the heroes, then clear the next wave,
		// and only then go and seal (sealing breaks when hit, so nobody may be shooting at you).
		// Priority: 1) a hero who hit me in the last 2 s (self-defence), 2) nearest minion within 2500,
		// 3) nearest hero, 4) anything else (guardians). Guardians themselves (leashed) keep "nearest".
		const bool bLaneBrain = LeashRadius <= 0.f;
		AValdoriaHeroCharacter* Attacker = LastAttacker.Get();
		if (bLaneBrain && Attacker && Now - LastAttackedTime < 2.f && IsEnemy(Attacker) && !Attacker->IsDead()
			&& Attacker->IsHeroUnit() && !Attacker->IsHiddenByStealth()
			&& FVector::Dist(GetActorLocation(), Attacker->GetActorLocation()) < 3000.f && CanSee(Attacker))
		{
			AutopilotTarget = Attacker;
		}
		float BestTier[3] = { 2500.f, 5000.f, 5000.f }; // minion, hero, other
		AValdoriaHeroCharacter* Pick[3] = { nullptr, nullptr, nullptr };
		for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It && !AutopilotTarget.IsValid(); ++It)
		{
			if (!IsEnemy(*It) || It->IsHiddenByStealth() || It->IsInvulnerable()
				|| (LeashRadius > 0.f && FVector::Dist2D(It->GetActorLocation(), LeashCenter) > LeashRadius))
			{
				continue;
			}
			const int32 Tier = !bLaneBrain ? 1 : It->IsMinionUnit() ? 0 : It->IsHeroUnit() ? 1 : 2;
			const float Dist = FVector::Dist(GetActorLocation(), It->GetActorLocation());
			if (Dist < BestTier[Tier] && CanSee(*It))
			{
				BestTier[Tier] = Dist;
				Pick[Tier] = *It;
			}
		}
		if (!AutopilotTarget.IsValid())
		{
			AutopilotTarget = Pick[0] ? Pick[0] : Pick[1] ? Pick[1] : Pick[2];
			// Wave and heroes cleared: a semi-sealed guardian nearby beats hitting another guardian -> go seal.
			if (bLaneBrain && !Pick[0] && !Pick[1] && Pick[2])
			{
				for (TActorIterator<AValdoriaGuardian> G(GetWorld()); G; ++G)
				{
					if (G->CanBeChanneledBy(this) && G->GuardTeam != TeamId && FVector::Dist2D(G->GetActorLocation(), GetActorLocation()) < 2500.f)
					{
						AutopilotTarget = nullptr;
						break;
					}
				}
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
		// A semi-sealed guardian nearby: go and seal it (enemy) or free it (ally).
		if (LeashRadius <= 0.f)
		{
			for (TActorIterator<AValdoriaGuardian> It(GetWorld()); It; ++It)
			{
				if (It->CanBeChanneledBy(this) && FVector::Dist2D(It->GetActorLocation(), GetActorLocation()) < 2500.f)
				{
					const FVector To = It->GetActorLocation() - GetActorLocation();
					bInteractHeld = To.Size2D() < 400.f;
					if (!bInteractHeld)
					{
						AutopilotAim = FRotator(0.f, To.Rotation().Yaw, 0.f);
						SetActorRotation(AutopilotAim);
						AddMovementInput(To.GetSafeNormal2D(), 1.f);
					}
					return;
				}
			}
			bInteractHeld = bInteractHeld && IsPlayerControlled();
		}
		// Follow the route (MOBA lane) if there is one, along navmesh paths (turn 44: out of the metro, around walls).
		if (AutopilotRoute.IsValidIndex(AutopilotRouteIndex))
		{
			// Stuck rescue (turn 45): barely moved in 3 s -> new path; twice in a row -> skip this route point.
			if (Now >= AutopilotStuckCheck)
			{
				const bool bStuck = AutopilotStuckCheck > 0.f && FVector::Dist2D(GetActorLocation(), AutopilotStuckPos) < 150.f;
				AutopilotStuckStrikes = bStuck ? AutopilotStuckStrikes + 1 : 0;
				if (bStuck)
				{
					NavRepathTime = 0.f;
					Jump();
					if (AutopilotStuckStrikes >= 2 && AutopilotRouteIndex < AutopilotRoute.Num() - 1)
					{
						++AutopilotRouteIndex;
						AutopilotStuckStrikes = 0;
					}
				}
				AutopilotStuckPos = GetActorLocation();
				AutopilotStuckCheck = Now + 3.f;
			}
			const FVector ToPoint = AutopilotRoute[AutopilotRouteIndex] - GetActorLocation();
			if (ToPoint.Size2D() < 500.f)
			{
				++AutopilotRouteIndex;
			}
			const FVector Step = AutopilotNavDirection(AutopilotRoute[FMath::Min(AutopilotRouteIndex, AutopilotRoute.Num() - 1)]);
			AutopilotAim = FRotator(0.f, Step.Rotation().Yaw, 0.f);
			SetActorRotation(AutopilotAim);
			AddMovementInput(Step, 1.f);
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
		// Nobody in sight: hunt the nearest living enemy along the navmesh (turn 44: before, bots spun in place
		// and never left closed rooms). Guardians (leashed) never hunt.
		AValdoriaHeroCharacter* Prey = nullptr;
		if (LeashRadius <= 0.f)
		{
			float Best = 40000.f;
			for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
			{
				if (IsEnemy(*It) && !It->IsDead() && !It->IsHiddenByStealth() && !It->IsInvulnerable() && !It->IsA<AValdoriaGuardian>())
				{
					const float D = FVector::Dist(GetActorLocation(), It->GetActorLocation());
					if (D < Best) { Best = D; Prey = *It; }
				}
			}
		}
		if (Prey)
		{
			const FVector Step = AutopilotNavDirection(Prey->GetActorLocation());
			AutopilotAim = FRotator(0.f, Step.Rotation().Yaw, 0.f);
			SetActorRotation(AutopilotAim);
			AddMovementInput(Step, 0.9f);
		}
		else
		{
			// Roam to random reachable spots.
			if (Now >= AutopilotRoamTime || FVector::Dist2D(AutopilotRoamGoal, GetActorLocation()) < 200.f)
			{
				AutopilotRoamTime = Now + 6.f;
				FNavLocation Spot;
				UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
				AutopilotRoamGoal = (Nav && Nav->GetRandomReachablePointInRadius(GetActorLocation(), 3000.f, Spot)) ? Spot.Location
					: GetActorLocation() + FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f).Vector() * 1500.f;
			}
			const FVector Step = AutopilotNavDirection(AutopilotRoamGoal);
			AutopilotAim = FRotator(0.f, Step.Rotation().Yaw, 0.f);
			SetActorRotation(AutopilotAim);
			AddMovementInput(Step, 0.6f);
		}
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
		AddMovementInput(LeashRadius > 0.f ? Fwd : AutopilotNavDirection(Target->GetActorLocation()), 1.f);
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
	if (Dist < 200.f && AimError < 30.f && !IsMinionUnit() && !IsGuardianUnit())
	{
		TryMeleePunch(); // V punch up close (turn 46)
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
		if ((It->Team != TeamId && !bCanRideAnyRail && !IsRailGuardian()) || It->Team == 253)
		{
			continue; // 253 = Gjallarhorn arc, only a picture
		}
		if (const float* Until = RailLockouts.Find(*It); Until && GetWorld()->GetTimeSeconds() < *Until)
		{
			continue; // just got carried by this bridge: 8 s before taking it back (owner)
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
	// Start in the direction the camera looks along the trail.
	MountRailAt(Best, BestAlong, FVector::DotProduct(Dir.GetSafeNormal2D(), Best->GetTangentAt(BestAlong).GetSafeNormal2D()) >= 0.f ? 1.f : -1.f);
	return true;
}

void AValdoriaHeroCharacter::MountRailAt(AValdoriaLaneRail* Best, float BestAlong, float Sign)
{
	if (!Best)
	{
		return;
	}
	RideRail = Best;
	RideDistance = BestAlong;
	RideSign = Sign;
	// Weapon goes on the back while riding.
	if (WeaponComponent->GetStaticMesh() && GetMesh()->DoesSocketExist(TEXT("spine_03")))
	{
		WeaponComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("spine_03"));
		WeaponComponent->SetRelativeLocationAndRotation(FVector(-10.f, -25.f, 0.f), FRotator(0.f, 0.f, 45.f));
	}
	if (OffhandComponent->GetStaticMesh() && GetMesh()->DoesSocketExist(TEXT("spine_03")))
	{
		OffhandComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("spine_03"));
		OffhandComponent->SetRelativeLocationAndRotation(FVector(-10.f, -25.f, 20.f), FRotator(0.f, 0.f, -45.f));
	}
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	GetCharacterMovement()->StopMovementImmediately();
	ValdoriaAudio::PlayAt(this, ValdoriaAudio::Load(TEXT("SciFi"), TEXT("forceField_000")), GetActorLocation(), 0.6f);
}

void AValdoriaHeroCharacter::StopRiding()
{
	if (!RideRail.IsValid())
	{
		return;
	}
	RideRail = nullptr;
	bRideVertical = false;
	// Draw the weapon back into the hand quickly (equip gesture).
	if (ActiveDefinition && WeaponComponent->GetStaticMesh())
	{
		WeaponComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, ActiveDefinition->WeaponSocket);
		WeaponComponent->SetRelativeTransform(ActiveDefinition->WeaponOffset);
		if (OffhandComponent->GetStaticMesh())
		{
			OffhandComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, ActiveDefinition->OffhandSocket);
			OffhandComponent->SetRelativeTransform(ActiveDefinition->OffhandOffset);
		}
		if (UValdoriaAnimInstance* DrawAnim = Cast<UValdoriaAnimInstance>(GetMesh()->GetAnimInstance()))
		{
			const TSoftObjectPtr<UAnimSequenceBase>& Draw = ActiveDefinition->Animations.AbilityAction;
			DrawAnim->PlayAction(Draw.IsNull() ? nullptr : Draw.LoadSynchronous(), 2.f);
		}
	}
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
	if (ForcedRideUntil > 0.f && GetWorld()->GetTimeSeconds() >= ForcedRideUntil)
	{
		RailLockouts.Add(Rail, GetWorld()->GetTimeSeconds() + 8.f);
		ForcedRideUntil = 0.f;
		StopRiding(); // dropped near the end of the carry; cannot take this bridge back for 8 s
		return;
	}
	float SpeedMult = 1.f;
	for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
	{
		if (*It != this && !IsEnemy(*It) && It->IsRailGuardian() && !It->IsDead() && FVector::Dist(It->GetActorLocation(), GetActorLocation()) < 3000.f)
		{
			SpeedMult = 1.2f; // allies near Heimdall ride 20% faster (owner's idea)
			break;
		}
	}
	RideDistance += Sign * RideSpeed * SpeedMult * DeltaSeconds;
	if (RideDistance <= 0.f || RideDistance >= Rail->GetLength())
	{
		StopRiding(); // end of the trail: drop off
		return;
	}
	// Owner: it looked like a zipline (the hero hung 60 cm below the ribbon). Now the feet are on top of the
	// ribbon on the horizontal part; on the vertical part the hero rises upright inside the light column.
	bRideVertical = FMath::Abs(Tangent.Z) > 0.7f;
	const float Lift = bRideVertical ? 0.f : (ActiveDefinition ? ActiveDefinition->CapsuleHalfHeight : 96.f) + 15.f;
	SetActorLocation(Rail->GetPointAt(RideDistance) + FVector(0.f, 0.f, Lift));
	if (!bRideVertical)
	{
		SetActorRotation(FRotator(0.f, (Tangent * Sign).Rotation().Yaw, 0.f)); // the vertical part has no useful yaw
	}
	// Flying pose (owner): body leaning forward, looking ahead and a little down; weapon on the back.
	// Vertical part of the trail (rising at the spawn): upright, no lean.
	const FRotator Lie(bRideVertical ? 0.f : -ActiveDefinition->Animations.RideLean, 0.f, 0.f);
	const FVector FeetOffset(0.f, 0.f, -ActiveDefinition->CapsuleHalfHeight);
	GetMesh()->SetRelativeLocationAndRotation(Lie.RotateVector(FeetOffset), Lie.Quaternion() * FRotator(0.f, -90.f, 0.f).Quaternion());
}

FString AValdoriaHeroCharacter::GetRideLaneName() const
{
	return RideRail.IsValid() ? RideRail->LaneName : FString();
}

// ------------------------------------------------------------------ seal / reverse-seal channel (hold G)

AValdoriaGuardian* AValdoriaHeroCharacter::FindChannelTarget() const
{
	AValdoriaGuardian* Best = nullptr;
	float BestDist = 450.f;
	for (TActorIterator<AValdoriaGuardian> It(GetWorld()); It; ++It)
	{
		const float Dist = FVector::Dist2D(It->GetActorLocation(), GetActorLocation());
		if (Dist < BestDist && It->CanBeChanneledBy(this))
		{
			BestDist = Dist;
			Best = *It;
		}
	}
	return Best;
}

void AValdoriaHeroCharacter::TickChannel(float DeltaSeconds)
{
	UValdoriaAnimInstance* Anim = GetMesh() ? Cast<UValdoriaAnimInstance>(GetMesh()->GetAnimInstance()) : nullptr;
	AValdoriaGuardian* Target = (bInteractHeld && !bIsDead && !IsRiding()) ? FindChannelTarget() : nullptr;
	// Must stay out of passive combat: taking damage or control breaks the incantation.
	if (!Target || IsInPassiveCombat(2.f) || (ChannelTarget.IsValid() && ChannelTarget.Get() != Target))
	{
		if (ChannelTarget.IsValid() && Anim)
		{
			Anim->SetUpperPose(nullptr, 0.f, 0.f);
		}
		ChannelTarget = nullptr;
		ChannelProgress = 0.f;
		return;
	}
	ChannelTarget = Target;
	const bool bRevert = Target->GuardTeam == TeamId;
	const float Needed = bRevert ? Target->RevertSeconds : Target->SealSeconds;
	const bool bDone = Target->AddChannel(this, DeltaSeconds / FMath::Max(0.5f, Needed)); // shared: several heroes go faster
	ChannelProgress = bDone ? 1.f : Target->GetChannelProgressFor(this);
	// Pose: open hand towards the guardian (frozen "arm out" frame).
	SetActorRotation(FRotator(0.f, (Target->GetActorLocation() - GetActorLocation()).Rotation().Yaw, 0.f));
	if (Anim)
	{
		Anim->SetUpperPose(LoadObject<UAnimSequenceBase>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01")), 0.25f, 1.f);
	}
	if (bDone)
	{
		ChannelTarget = nullptr;
		ChannelProgress = 0.f;
		bInteractHeld = false;
		if (Anim)
		{
			Anim->SetUpperPose(nullptr, 0.f, 0.f);
		}
	}
}

FString AValdoriaHeroCharacter::GetChannelSubtitle() const
{
	const AValdoriaGuardian* Target = ChannelTarget.Get();
	if (!Target)
	{
		return FString();
	}
	// The incantation every hero knows (lore). Subtitles only until there is voice.
	return Target->GuardTeam == TeamId
		? TEXT("\"Solvo vincula tua, custos. Redi ad nos, surge et vigila.\"  (I loosen your bonds, guardian. Come back to us, rise and keep watch.)")
		: TEXT("\"Sigillo te, custos. In silentio maneas.\"  (I seal you, guardian. Remain in silence.)");
}

// ------------------------------------------------------------------ stacks (turn 45)

void AValdoriaHeroCharacter::SetStack(AValdoriaHeroCharacter* Source, FName Key, const FString& Label, FLinearColor Color, int32 Count, float Duration)
{
	const float Now = GetWorld()->GetTimeSeconds();
	Stacks.RemoveAll([this, Now](const FValdoriaStack& S) { return !S.Source.IsValid() || !IsStackLive(S); });
	FValdoriaStack* Found = Stacks.FindByPredicate([Source, Key](const FValdoriaStack& S) { return S.Source.Get() == Source && S.Key == Key; });
	if (Count <= 0)
	{
		if (Found)
		{
			Stacks.RemoveAll([Source, Key](const FValdoriaStack& S) { return S.Source.Get() == Source && S.Key == Key; });
		}
		return;
	}
	if (!Found)
	{
		Found = &Stacks.AddDefaulted_GetRef();
		Found->Source = Source;
		Found->Key = Key;
	}
	Found->Label = Label;
	Found->Color = Color;
	Found->Count = Count;
	Found->EndTime = Duration > 0.f ? Now + Duration : 0.f;
}

void AValdoriaHeroCharacter::AddStack(AValdoriaHeroCharacter* Source, FName Key, const FString& Label, FLinearColor Color, int32 Add, int32 MaxCount, float Duration)
{
	int32 Current = 0;
	for (const FValdoriaStack& S : Stacks)
	{
		if (S.Source.Get() == Source && S.Key == Key && IsStackLive(S))
		{
			Current = S.Count;
		}
	}
	SetStack(Source, Key, Label, Color, FMath::Clamp(Current + Add, 0, MaxCount), Duration);
}

// ------------------------------------------------------------------ Heimdall helpers (turn 47)

bool AValdoriaHeroCharacter::IsRailGuardian() const
{
	return ActiveDefinition && ActiveDefinition->bRailGuardian;
}

bool AValdoriaHeroCharacter::IsForcedRiding() const
{
	return IsRiding() && ForcedRideUntil > 0.f;
}

bool AValdoriaHeroCharacter::StartForcedRide(float Seconds, float MaxDistance)
{
	AValdoriaLaneRail* Best = nullptr;
	float BestDist = MaxDistance;
	float BestAlong = 0.f;
	for (TActorIterator<AValdoriaLaneRail> It(GetWorld()); It; ++It)
	{
		if (It->Team == 253)
		{
			continue;
		}
		float Along = 0.f;
		const float D = It->DistanceToPoint(GetActorLocation(), Along);
		if (D < BestDist)
		{
			BestDist = D;
			Best = *It;
			BestAlong = Along;
		}
	}
	if (!Best)
	{
		ApplyStatus(EValdoriaStatus::Stunned, 1.f); // no bridge close: just a short stun
		return false;
	}
	if (IsRiding())
	{
		StopRiding();
	}
	// Rails start at their team's base: own rail -> back toward the start; enemy rail -> toward its end.
	MountRailAt(Best, BestAlong, Best->Team == TeamId ? -1.f : 1.f);
	ForcedRideUntil = GetWorld()->GetTimeSeconds() + Seconds;
	ApplyStatus(EValdoriaStatus::CannotAttack, Seconds);
	ApplyStatus(EValdoriaStatus::CannotUseAbilities, Seconds);
	return true;
}
