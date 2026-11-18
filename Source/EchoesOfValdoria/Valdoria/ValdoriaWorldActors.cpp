// Echoes of Valdoria - projéteis, zonas e barreiras.

#include "ValdoriaWorldActors.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ValdoriaHeroCharacter.h"
#include "ValdoriaAudio.h"
#include "ValdoriaSettings.h"

UMaterialInstanceDynamic* ValdoriaMakeGlowMaterial(UObject* Outer, const FLinearColor& Color, float Opacity)
{
	// M_Valdoria_Energia é criado por Scripts/create_heroes.py (translúcido e brilhante).
	static const TCHAR* EnergyPath = TEXT("/Game/Valdoria/Materials/M_Valdoria_Energia.M_Valdoria_Energia");
	static const TCHAR* BlockoutPath = TEXT("/Game/Maps/NP_Materials/M_NP_Blockout.M_NP_Blockout");
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, EnergyPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	const bool bEnergy = Base != nullptr;
	if (!Base)
	{
		Base = LoadObject<UMaterialInterface>(nullptr, BlockoutPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	if (!Base)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, Outer);
	if (bEnergy)
	{
		// O material multiplica a cor por 3 (brilho); aqui baixa para não estourar em branco.
		MID->SetVectorParameterValue(TEXT("Cor"), Color * 0.35f);
		MID->SetScalarParameterValue(TEXT("Opacidade"), Opacity);
	}
	else
	{
		MID->SetVectorParameterValue(TEXT("Cor"), Color);
		MID->SetScalarParameterValue(TEXT("Brilho"), 2.f);
	}
	return MID;
}

static UStaticMesh* LoadShape(const TCHAR* Name)
{
	return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Name, Name));
}

// ------------------------------------------------------------------ zona

AValdoriaZone::AValdoriaZone()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	Disc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Disc"));
	RootComponent = Disc;
	Disc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Disc->SetCastShadow(false);
	Disc->SetStaticMesh(LoadShape(TEXT("Cylinder")));
}

void AValdoriaZone::Setup(AValdoriaHeroCharacter* InOwner, float InRadius, float InDuration, float InHealPerSecond,
	float InDamagePerSecond, EValdoriaDamageType InType, float InSlow, FLinearColor InColor, bool bInFollowOwner)
{
	OwnerHero = InOwner;
	Radius = FMath::Max(50.f, InRadius);
	HealPerSecond = InHealPerSecond;
	DamagePerSecond = InDamagePerSecond;
	DamageType = InType;
	Slow = InSlow;
	Color = InColor;
	bFollowOwner = bInFollowOwner;
	EndTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.1f, InDuration);
	// Cilindro de 100 cm de diâmetro e 100 de altura: vira um disco fino no chão.
	Disc->SetWorldScale3D(FVector(Radius / 50.f, Radius / 50.f, 0.04f));
	if (UMaterialInstanceDynamic* MID = ValdoriaMakeGlowMaterial(this, Color, 0.25f))
	{
		Disc->SetMaterial(0, MID);
	}
}

void AValdoriaZone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Now = GetWorld()->GetTimeSeconds();
	AValdoriaHeroCharacter* Src = OwnerHero.Get();
	if (Now >= EndTime || !Src)
	{
		Destroy();
		return;
	}
	if (bFollowOwner)
	{
		SetActorLocation(Src->GetActorLocation() - FVector(0.f, 0.f, 90.f));
	}
#if ENABLE_DRAW_DEBUG
	DrawDebugCircle(GetWorld(), GetActorLocation() + FVector(0, 0, 5.f), Radius, 48, Color.ToFColor(true), false, -1.f, 0, 4.f,
		FVector(1, 0, 0), FVector(0, 1, 0), false);
#endif
	PulseTimer += DeltaSeconds;
	if (PulseTimer < 0.25f || !HasAuthority())
	{
		return;
	}
	const float Step = PulseTimer;
	PulseTimer = 0.f;
	for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
	{
		AValdoriaHeroCharacter* Hero = *It;
		if (Hero->IsDead() || FVector::Dist2D(Hero->GetActorLocation(), GetActorLocation()) > Radius
			|| FMath::Abs(Hero->GetActorLocation().Z - GetActorLocation().Z) > 400.f)
		{
			continue;
		}
		if (Src->IsEnemy(Hero))
		{
			if (DamagePerSecond > 0.f)
			{
				Src->DealTypedDamage(Hero, DamagePerSecond * Step, DamageType, 0.5f, false);
			}
			if (Slow > 0.f)
			{
				Hero->ApplySlow(Slow, 0.5f);
			}
		}
		else if (Hero->TeamId == Src->TeamId)
		{
			if (HealPerSecond > 0.f)
			{
				Src->HealTarget(Hero, HealPerSecond * Step);
			}
			if (AllySpeedBuff > 0.f)
			{
				Hero->ApplySpeedBuff(AllySpeedBuff, 0.5f);
			}
		}
	}
}

// ------------------------------------------------------------------ projétil

AValdoriaProjectile::AValdoriaProjectile()
{
	bReplicates = true;
	SetReplicatingMovement(true);
	InitialLifeSpan = 4.f;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(12.f);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	Collision->SetNotifyRigidBodyCollision(true);
	RootComponent = Collision;

	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Collision);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetCastShadow(false);
	Visual->SetStaticMesh(LoadShape(TEXT("Sphere")));

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->bRotationFollowsVelocity = true;
	Movement->bShouldBounce = false;
}

AValdoriaProjectile* AValdoriaProjectile::Launch(AValdoriaHeroCharacter* Src, const FVector& Start, const FVector& Direction, const FParams& Params)
{
	if (!Src || !Src->HasAuthority())
	{
		return nullptr;
	}
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Src;
	SpawnParams.Instigator = Src;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AValdoriaProjectile* Proj = Src->GetWorld()->SpawnActor<AValdoriaProjectile>(AValdoriaProjectile::StaticClass(),
		Start, Direction.Rotation(), SpawnParams);
	if (!Proj)
	{
		return nullptr;
	}
	Proj->OwnerHero = Src;
	Proj->P = Params;
	Proj->Collision->IgnoreActorWhenMoving(Src, true);
	Src->IgnoreFriendlyBarriers(Proj->Collision);
	Proj->Collision->OnComponentBeginOverlap.AddDynamic(Proj, &AValdoriaProjectile::OnOverlap);
	Proj->Collision->OnComponentHit.AddDynamic(Proj, &AValdoriaProjectile::OnHit);
	Proj->Visual->SetRelativeScale3D(FVector(Params.Size));
	if (UMaterialInstanceDynamic* MID = ValdoriaMakeGlowMaterial(Proj, Params.Color, 0.9f))
	{
		Proj->Visual->SetMaterial(0, MID);
	}
	Proj->Movement->InitialSpeed = Params.Speed;
	Proj->Movement->MaxSpeed = Params.Speed;
	Proj->Movement->ProjectileGravityScale = Params.Gravity;
	Proj->Movement->Velocity = Direction.GetSafeNormal() * Params.Speed;
	return Proj;
}

void AValdoriaProjectile::OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AValdoriaHeroCharacter* Src = OwnerHero.Get();
	AValdoriaHeroCharacter* Other = Cast<AValdoriaHeroCharacter>(OtherActor);
	if (bDone || !Src || !Other || Other == Src || Other->IsDead())
	{
		return;
	}
	if (Src->IsEnemy(Other) && Other->IsReflecting())
	{
		// Refletido: volta para quem atirou, agora do time de quem refletiu.
		OwnerHero = Other;
		Collision->ClearMoveIgnoreActors();
		Collision->IgnoreActorWhenMoving(Other, true);
		Movement->Velocity = -Movement->Velocity;
		Other->PlayAreaEffect(GetActorLocation(), 60.f, FLinearColor(1.f, 0.9f, 0.5f));
		return;
	}
	if (Src->IsEnemy(Other))
	{
		// Cabeça: o ponto do projétil está na parte de cima da cápsula.
		FHitResult Fake;
		Fake.ImpactPoint = GetActorLocation();
		Fake.HitObjectHandle = FActorInstanceHandle(Other);
		const bool bHead = AValdoriaHeroCharacter::IsHeadshot(Fake);
		Src->DealTypedDamage(Other, P.Damage, P.DamageType, P.MagicShare, bHead);
		ValdoriaAudio::PlayAt(this, ValdoriaAudio::Load(TEXT("Impacts"), TEXT("impactPunch_medium_002")), GetActorLocation(), 0.6f);
		Explode(GetActorLocation(), Other);
	}
	else if (P.AllyHeal > 0.f && Other->TeamId == Src->TeamId)
	{
		Src->HealTarget(Other, P.AllyHeal);
		Explode(GetActorLocation(), Other);
	}
}

void AValdoriaProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (bDone)
	{
		return;
	}
	if (AValdoriaHeroCharacter* Src = OwnerHero.Get())
	{
		if (AValdoriaBarrier* Barrier = Cast<AValdoriaBarrier>(OtherActor))
		{
			Src->DealTypedDamage(Barrier, P.Damage, P.DamageType, P.MagicShare, false);
		}
	}
	if (P.bExplodeOnWorld)
	{
		Explode(Hit.ImpactPoint + Hit.ImpactNormal * 10.f, nullptr);
	}
	else
	{
		bDone = true;
		Destroy();
	}
}

void AValdoriaProjectile::Explode(const FVector& Where, AActor* DirectHit)
{
	bDone = true;
	AValdoriaHeroCharacter* Src = OwnerHero.Get();
	if (Src && P.SplashRadius > 0.f)
	{
		ValdoriaAudio::PlayAt(this, ValdoriaAudio::Load(TEXT("SciFi"), TEXT("explosionCrunch_001")), Where, 0.6f);
		Src->PlayAreaEffect(Where, P.SplashRadius, P.Color);
		for (TActorIterator<AValdoriaHeroCharacter> It(GetWorld()); It; ++It)
		{
			AValdoriaHeroCharacter* Hero = *It;
			if (Hero == DirectHit || Hero->IsDead() || FVector::Dist(Hero->GetActorLocation(), Where) > P.SplashRadius)
			{
				continue;
			}
			if (Src->IsEnemy(Hero))
			{
				Src->DealTypedDamage(Hero, P.Damage * 0.6f, P.DamageType, P.MagicShare, false);
			}
			else if (P.AllyHeal > 0.f && Hero->TeamId == Src->TeamId)
			{
				Src->HealTarget(Hero, P.AllyHeal * 0.6f);
			}
		}
	}
	if (Src && P.ZoneDuration > 0.f)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		// A zona fica no chão abaixo do ponto da explosão.
		FVector Ground = Where;
		FHitResult Down;
		if (GetWorld()->LineTraceSingleByChannel(Down, Where + FVector(0, 0, 20.f), Where - FVector(0, 0, 600.f), ECC_WorldStatic))
		{
			Ground = Down.ImpactPoint;
		}
		if (AValdoriaZone* Zone = GetWorld()->SpawnActor<AValdoriaZone>(AValdoriaZone::StaticClass(), Ground, FRotator::ZeroRotator, SpawnParams))
		{
			Zone->Setup(Src, P.ZoneRadius, P.ZoneDuration, P.ZoneHealPerSecond, P.ZoneDamagePerSecond, P.DamageType, P.ZoneSlow, P.Color);
		}
	}
	Destroy();
}

// ------------------------------------------------------------------ barreira

AValdoriaBarrier::AValdoriaBarrier()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	Wall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wall"));
	RootComponent = Wall;
	Wall->SetStaticMesh(LoadShape(TEXT("Cube")));
	Wall->SetCastShadow(false);
	// Bloqueia tiros (Visibility) e projéteis; pessoas atravessam.
	Wall->SetCollisionObjectType(ECC_WorldDynamic);
	Wall->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Wall->SetCollisionResponseToAllChannels(ECR_Block);
	Wall->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Wall->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
}

void AValdoriaBarrier::Setup(AValdoriaHeroCharacter* InOwner, float InHealth, float InWidth, float InHeight, float InDuration,
	FLinearColor InColor, bool bAttachToOwner)
{
	OwnerHero = InOwner;
	TeamId = InOwner ? InOwner->TeamId : 0;
	Health = MaxHealth = FMath::Max(1.f, InHealth);
	Color = InColor;
	EndTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.1f, InDuration);
	// Cubo de 100 cm: fica fino (12 cm) e com a largura/altura pedidas.
	Wall->SetWorldScale3D(FVector(0.12f, InWidth / 100.f, InHeight / 100.f));
	Material = ValdoriaMakeGlowMaterial(this, Color, 0.3f);
	if (Material)
	{
		Wall->SetMaterial(0, Material);
	}
	if (InOwner && bAttachToOwner)
	{
		AttachToActor(InOwner, FAttachmentTransformRules::KeepWorldTransform);
	}
}

void AValdoriaBarrier::AbsorbDamage(float Amount)
{
	Health -= Amount;
	FlashTime = 0.12f;
	if (Health <= 0.f)
	{
		if (AValdoriaHeroCharacter* Src = OwnerHero.Get())
		{
			Src->PlayAreaEffect(GetActorLocation(), 250.f, Color);
		}
		Destroy();
	}
}

void AValdoriaBarrier::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetWorld()->GetTimeSeconds() >= EndTime || !OwnerHero.IsValid() || OwnerHero->IsDead())
	{
		Destroy();
		return;
	}
	FlashTime -= DeltaSeconds;
	if (Material)
	{
		// Pisca ao levar tiro e fica mais apagada quando está quase quebrando.
		const float Life = FMath::Clamp(Health / MaxHealth, 0.f, 1.f);
		Material->SetScalarParameterValue(TEXT("Opacidade"), FlashTime > 0.f ? 0.6f : 0.12f + 0.25f * Life);
	}
#if ENABLE_DRAW_DEBUG
	// Moldura hexagonal desenhada na borda.
	const FVector C = GetActorLocation();
	const FVector R = GetActorRightVector() * Wall->GetComponentScale().Y * 50.f;
	const FVector U = GetActorUpVector() * Wall->GetComponentScale().Z * 50.f;
	const FVector Pts[6] = { C + R * 0.5f + U, C + R + U * 0.f, C + R * 0.5f - U, C - R * 0.5f - U, C - R, C - R * 0.5f + U };
	for (int32 i = 0; i < 6; ++i)
	{
		DrawDebugLine(GetWorld(), Pts[i], Pts[(i + 1) % 6], Color.ToFColor(true), false, -1.f, 0, 3.f);
	}
#endif
}

// ------------------------------------------------------------------ efeitos visuais

AValdoriaFx::AValdoriaFx()
{
	PrimaryActorTick.bCanEverTick = true;
	Shape = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shape"));
	RootComponent = Shape;
	Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Shape->SetCastShadow(false);
}

AValdoriaFx* AValdoriaFx::SpawnFx(UWorld* World, const FVector& Where, EKind InKind, const FLinearColor& Color, float InLife)
{
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AValdoriaFx* Fx = World->SpawnActor<AValdoriaFx>(AValdoriaFx::StaticClass(), Where, FRotator::ZeroRotator, Params);
	if (!Fx)
	{
		return nullptr;
	}
	Fx->Kind = InKind;
	Fx->Life = FMath::Max(0.03f, InLife);
	Fx->Shape->SetStaticMesh(LoadShape(InKind == EKind::Burst ? TEXT("Sphere") : TEXT("Cylinder")));
	Fx->Material = ValdoriaMakeGlowMaterial(Fx, InKind == EKind::Beam ? Color * 1.6f : Color, Fx->StartOpacity);
	if (Fx->Material)
	{
		Fx->Shape->SetMaterial(0, Fx->Material);
	}
	return Fx;
}

void AValdoriaFx::Beam(UWorld* World, const FVector& From, const FVector& To, const FLinearColor& Color, float Thickness, float InLife)
{
	const FVector Delta = To - From;
	const float Len = Delta.Size();
	if (Len < 1.f)
	{
		return;
	}
	if (AValdoriaFx* Fx = SpawnFx(World, (From + To) * 0.5f, EKind::Beam, Color, InLife))
	{
		// Cilindro de 100 cm em pé: deita na direção do tiro.
		Fx->SetActorRotation(FRotationMatrix::MakeFromZ(Delta / Len).Rotator());
		Fx->StartScale = FVector(Thickness / 100.f, Thickness / 100.f, Len / 100.f);
		Fx->EndScale = FVector(Thickness / 300.f, Thickness / 300.f, Len / 100.f);
		Fx->StartOpacity = 0.85f;
		Fx->SetActorScale3D(Fx->StartScale);
	}
}

void AValdoriaFx::Ring(UWorld* World, const FVector& Center, float Radius, const FLinearColor& Color, float InLife)
{
	if (AValdoriaFx* Fx = SpawnFx(World, Center, EKind::Ring, Color, InLife))
	{
		Fx->StartScale = FVector(Radius / 50.f * 0.25f, Radius / 50.f * 0.25f, 0.02f);
		Fx->EndScale = FVector(Radius / 50.f, Radius / 50.f, 0.02f);
		Fx->StartOpacity = 0.55f;
		Fx->SetActorScale3D(Fx->StartScale);
	}
	// Uma "bolha" bem fraca junto, para a área ter volume sem tampar a visão.
	AValdoriaFx* Dome = FValdoriaOptions::Get().EffectsQuality >= 2 ? SpawnFx(World, Center, EKind::Burst, Color, InLife * 0.7f) : nullptr;
	if (Dome)
	{
		Dome->StartScale = FVector(Radius / 50.f * 0.2f);
		Dome->EndScale = FVector(Radius / 50.f * 0.9f, Radius / 50.f * 0.9f, Radius / 50.f * 0.35f);
		Dome->StartOpacity = 0.1f;
		Dome->SetActorScale3D(Dome->StartScale);
	}
}

void AValdoriaFx::Burst(UWorld* World, const FVector& Center, float Size, const FLinearColor& Color, float InLife)
{
	if (FValdoriaOptions::Get().EffectsQuality < 1)
	{
		return; // low: no impact bursts
	}
	if (AValdoriaFx* Fx = SpawnFx(World, Center, EKind::Burst, Color, InLife))
	{
		Fx->StartScale = FVector(Size / 100.f * 0.3f);
		Fx->EndScale = FVector(Size / 100.f);
		Fx->StartOpacity = 0.35f;
		Fx->SetActorScale3D(Fx->StartScale);
	}
}

void AValdoriaFx::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	const float T = FMath::Clamp(Age / Life, 0.f, 1.f);
	if (T >= 1.f)
	{
		Destroy();
		return;
	}
	// Cresce rápido no começo e apaga no fim.
	const float Ease = 1.f - FMath::Square(1.f - T);
	SetActorScale3D(FMath::Lerp(StartScale, EndScale, Ease));
	if (Material)
	{
		Material->SetScalarParameterValue(TEXT("Opacidade"), StartOpacity * (1.f - T * T));
	}
}

// ------------------------------------------------------------------ portal

AValdoriaPortal::AValdoriaPortal()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	Trigger = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Trigger"));
	Trigger->InitCapsuleSize(70.f, 110.f);
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
	RootComponent = Trigger;
	Ring = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ring"));
	Ring->SetupAttachment(Trigger);
	Ring->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ring->SetCastShadow(false);
	Ring->SetStaticMesh(LoadShape(TEXT("Cylinder")));
	// Upright flat disc (a doorway of light) facing the actor's forward axis.
	Ring->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	Ring->SetRelativeScale3D(FVector(2.2f, 1.4f, 0.03f));
}

void AValdoriaPortal::Setup(AValdoriaHeroCharacter* InOwner, EValdoriaPortalAccess InAccess, float InDuration, FLinearColor InColor)
{
	OwnerHero = InOwner;
	Access = InAccess;
	Color = InColor;
	EndTime = GetWorld()->GetTimeSeconds() + FMath::Max(1.f, InDuration);
	if (UMaterialInstanceDynamic* MID = ValdoriaMakeGlowMaterial(this, Color, 0.45f))
	{
		Ring->SetMaterial(0, MID);
	}
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AValdoriaPortal::OnOverlap);
}

void AValdoriaPortal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetWorld()->GetTimeSeconds() >= EndTime || !OwnerHero.IsValid())
	{
		Destroy();
		return;
	}
	// Slow spin so it reads as "active"; dimmer while it has no partner yet.
	Ring->AddLocalRotation(FRotator(0.f, 0.f, 90.f * DeltaSeconds));
	Ring->SetVisibility(true);
	Ring->SetScalarParameterValueOnMaterials(TEXT("Opacidade"), Linked.IsValid() ? 0.45f : 0.18f);
}

void AValdoriaPortal::OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AValdoriaPortal* Exit = Linked.Get();
	AValdoriaHeroCharacter* Traveller = Cast<AValdoriaHeroCharacter>(OtherActor);
	AValdoriaHeroCharacter* Src = OwnerHero.Get();
	if (!HasAuthority() || !Exit || !Traveller || !Src || Traveller->IsDead())
	{
		return;
	}
	if (Access == EValdoriaPortalAccess::TeamOnly && Traveller->TeamId != Src->TeamId)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (const float* Until = Cooldowns.Find(Traveller); Until && *Until > Now)
	{
		return;
	}
	// Come out a bit in front of the exit, keeping the speed.
	const FVector Out = Exit->GetActorLocation() + Exit->GetActorForwardVector() * 120.f;
	Exit->Cooldowns.Add(Traveller, Now + 1.f);
	Traveller->TeleportTo(Out, Traveller->GetActorRotation());
	Src->PlayAreaEffect(Out, 140.f, Color);
	ValdoriaAudio::PlayAt(this, ValdoriaAudio::Load(TEXT("Magic"), TEXT("teleport")), Out, 0.7f);
}
