// Echoes of Valdoria - coisas que as habilidades colocam no mundo: projéteis, zonas
// (cura/fogo/redemoinho) e barreiras. Todas genéricas: os números vêm de quem cria.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ValdoriaTypes.h"
#include "ValdoriaWorldActors.generated.h"

class AValdoriaHeroCharacter;
class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UMaterialInstanceDynamic;

/** Cria um material de energia (translúcido) ou, se não houver, o do blockout, na cor pedida. */
UMaterialInstanceDynamic* ValdoriaMakeGlowMaterial(UObject* Outer, const FLinearColor& Color, float Opacity = 0.35f);

/** Zona no chão: cura aliados e/ou fere e deixa lentos os inimigos enquanto durar. */
UCLASS()
class ECHOESOFVALDORIA_API AValdoriaZone : public AActor
{
	GENERATED_BODY()
public:
	AValdoriaZone();
	virtual void Tick(float DeltaSeconds) override;

	/** Configura e liga a zona (chame logo depois de criar). */
	void Setup(AValdoriaHeroCharacter* InOwner, float InRadius, float InDuration, float InHealPerSecond,
		float InDamagePerSecond, EValdoriaDamageType InType, float InSlow, FLinearColor InColor, bool bInFollowOwner = false);

	/** Aliados dentro ficam mais rápidos (0.4 = +40%). Usado por rastros de deslizar. */
	float AllySpeedBuff = 0.f;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Disc;
	TWeakObjectPtr<AValdoriaHeroCharacter> OwnerHero;
	float Radius = 400.f;
	float HealPerSecond = 0.f;
	float DamagePerSecond = 0.f;
	float Slow = 0.f;
	EValdoriaDamageType DamageType = EValdoriaDamageType::Magical;
	FLinearColor Color = FLinearColor::White;
	bool bFollowOwner = false;
	float EndTime = 0.f;
	float PulseTimer = 0.f;
};

/** Projétil com velocidade, gravidade, explosão, cura em aliados e zona opcional ao cair. */
UCLASS()
class ECHOESOFVALDORIA_API AValdoriaProjectile : public AActor
{
	GENERATED_BODY()
public:
	AValdoriaProjectile();

	struct FParams
	{
		float Damage = 20.f;
		EValdoriaDamageType DamageType = EValdoriaDamageType::Physical;
		float MagicShare = 0.5f;
		float Speed = 5000.f;
		float Gravity = 0.f;
		float SplashRadius = 0.f;
		float AllyHeal = 0.f;
		float Size = 0.12f;
		FLinearColor Color = FLinearColor::White;
		// Zona deixada ao explodir (Duration 0 = nenhuma).
		float ZoneRadius = 0.f;
		float ZoneDuration = 0.f;
		float ZoneHealPerSecond = 0.f;
		float ZoneDamagePerSecond = 0.f;
		float ZoneSlow = 0.f;
		/** Explode ao bater em qualquer coisa (se falso, só some). */
		bool bExplodeOnWorld = true;
	};

	/** Cria e lança um projétil (só no servidor). */
	static AValdoriaProjectile* Launch(AValdoriaHeroCharacter* Owner, const FVector& Start, const FVector& Direction, const FParams& Params);

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Visual;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProjectileMovementComponent> Movement;

	UFUNCTION() void OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION() void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	void Explode(const FVector& Where, AActor* DirectHit);

	TWeakObjectPtr<AValdoriaHeroCharacter> OwnerHero;
	FParams P;
	bool bDone = false;
};

/** Barreira de energia: bloqueia tiros e projéteis inimigos (aliados atravessam). */
UCLASS()
class ECHOESOFVALDORIA_API AValdoriaBarrier : public AActor
{
	GENERATED_BODY()
public:
	AValdoriaBarrier();
	virtual void Tick(float DeltaSeconds) override;

	void Setup(AValdoriaHeroCharacter* InOwner, float InHealth, float InWidth, float InHeight, float InDuration, FLinearColor InColor, bool bAttachToOwner);
	void AbsorbDamage(float Amount);

	uint8 TeamId = 0;
	float Health = 800.f;
	float MaxHealth = 800.f;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Wall;
	TWeakObjectPtr<AValdoriaHeroCharacter> OwnerHero;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Material;
	FLinearColor Color = FLinearColor::White;
	float EndTime = 0.f;
	float FlashTime = 0.f;
};
