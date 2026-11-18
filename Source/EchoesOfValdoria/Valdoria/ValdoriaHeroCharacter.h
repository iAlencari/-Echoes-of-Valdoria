// Echoes of Valdoria - personagem jogável genérico. Tudo que é específico de
// cada herói vem do UValdoriaHeroDefinition (Data Asset).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpecHandle.h"
#include "ValdoriaTypes.h"
#include "ValdoriaHeroCharacter.generated.h"

class UAbilitySystemComponent;
class UValdoriaAttributeSet;
class UValdoriaHeroDefinition;
class UAnimSequenceBase;
class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogValdoria, Log, All);

UCLASS()
class ECHOESOFVALDORIA_API AValdoriaHeroCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AValdoriaHeroCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void Tick(float DeltaSeconds) override;

	// --- Herói
	/** Data Asset do herói. Vazio = "Recruta" de teste criado em código. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_HeroDefinition, Category = "Herói")
	TObjectPtr<UValdoriaHeroDefinition> HeroDefinition;

	/** Troca o herói em tempo real (servidor). */
	UFUNCTION(BlueprintCallable, Category = "Herói")
	void SetHeroDefinition(UValdoriaHeroDefinition* NewDefinition);

	const UValdoriaHeroDefinition* GetHeroDefinition() const { return ActiveDefinition; }
	const FValdoriaAbilityEntry* FindAbilityEntry(EValdoriaAbilitySlot Slot) const;
	const FValdoriaWeaponStats& GetWeaponStats() const;
	float GetArmorDamageReduction() const;

	/** Time: inimigos são todos com TeamId diferente. 255 = alvo de treino. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Herói")
	uint8 TeamId = 0;

	bool IsEnemy(const AActor* Other) const;

	// --- Arma
	bool CanFireWeapon() const;
	void ConsumeAmmo();
	void GetAimRay(FVector& OutStart, FVector& OutDirection) const;
	static bool IsHeadshot(const FHitResult& Hit);
	int32 GetAmmo() const { return CurrentAmmo; }
	bool IsReloading() const { return bReloading; }
	void StartReload();

	// --- Recargas e ultimate
	float GetCooldownRemaining(EValdoriaAbilitySlot Slot) const;
	void StartCooldown(EValdoriaAbilitySlot Slot, float Seconds);
	bool IsUltimateReady() const;
	float GetUltimatePercent() const;
	void ConsumeUltimate();

	// --- Vida e morte
	bool IsDead() const { return bIsDead; }
	bool IsInvulnerable() const { return bGodMode; }
	void HandleDeath(AValdoriaHeroCharacter* Killer);
	virtual void OnDamageTaken(AValdoriaHeroCharacter* DamageInstigator, float Damage, bool bHeadshot);
	virtual void OnDamageDealt(AValdoriaHeroCharacter* Victim, float Damage, bool bHeadshot);
	float GetHealth() const;
	float GetMaxHealth() const;
	float GetArmor() const;
	float GetMaxArmor() const;
	float GetShield() const;
	float GetMaxShield() const;

	// --- Efeitos (cosméticos)
	void PlayShotEffects(const FVector& Impact, bool bHit);
	void PlayAreaEffect(const FVector& Center, float Radius, FLinearColor Color);

	// --- Painel de administrador (só servidor)
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Admin")
	bool bGodMode = false;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Admin")
	bool bNoCooldowns = false;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Admin")
	bool bInfiniteAmmo = false;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Admin")
	bool bInfiniteDashes = false;
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Admin")
	float AdminSpeedMultiplier = 1.f;
	void AdminSetSpeedMultiplier(float Multiplier);
	float GetDamageMultiplier() const;
	void AdminSetDamageMultiplier(float Multiplier);
	void AdminFillUltimate();
	void AdminRestoreAll();

	/** Troca a arma visível (painel de admin). */
	void SetWeaponMesh(UStaticMesh* NewWeapon);

	/** Câmera em primeira pessoa (tecla V alterna). */
	void SetFirstPerson(bool bEnable);
	bool IsFirstPerson() const { return bFirstPerson; }

	/** Voar e atravessar paredes (painel de admin). */
	void SetNoclip(bool bEnable);
	bool IsNoclip() const { return bNoclip; }

	/** Aplica dano em outro personagem pelo GAS, como se fosse um tiro deste (bots usam isto). */
	void DealDamageTo(AActor* Target, float Damage, bool bHeadshot = false);

	/** Volta ao estado inicial (vida cheia, munição, sem recargas). */
	void ResetCombatState();

	int32 GetDashCharges() const { return DashChargesLeft; }
	bool IsSprinting() const { return bIsSprinting; }
	bool IsProne() const { return bIsProne; }
	bool IsSliding() const { return bSliding; }
	bool IsWallClimbing() const { return bWallClimbing; }
	bool IsMantling() const { return bMantling; }

	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;

	UFUNCTION() void OnRep_HeroDefinition();
	UFUNCTION() void OnRep_IsDead();

	/** Lê o Data Asset e aplica tudo: atributos, movimento, câmera, malha e habilidades. */
	void ApplyHeroDefinition();
	void GrantAbilities();
	void BuildDefaultInput();
	void TryActivateSlot(EValdoriaAbilitySlot Slot);
	void FinishReload();
	void EnterRagdoll();
	void LeaveRagdoll();

	// Entrada
	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	bool IsCursorFree() const;
	void Input_FirePressed();
	void Input_FireReleased();
	void Input_Secondary();
	void Input_Ability1();
	void Input_Ability2();
	void Input_Ultimate();
	void Input_Reload();
	void Input_JumpPressed();
	void Input_JumpReleased();
	void Input_SprintPressed();
	void Input_SprintReleased();
	void Input_CrouchPressed();
	void Input_CrouchReleased();
	void Input_Prone();

	// --- Movimento avançado (regras por herói em FValdoriaMovementRules)
	const FValdoriaMovementRules& GetMovementRules() const;
	void UpdateMovementSpeed();
	void SetSprinting(bool bNewSprinting);
	void SetProne(bool bNewProne);
	void TryDash();
	UFUNCTION(Server, Reliable) void ServerDash(FVector_NetQuantizeNormal Direction);
	void PerformDash(const FVector& Direction);
	int32 DashChargesLeft = 0;
	float DashRechargeProgress = 0.f;
	float LastShiftPressTime = -10.f;
	void StartSlide();
	void EndSlide();
	bool FindMantleTarget(FVector& OutTarget) const;
	bool IsFacingWall(float Distance) const;
	void StartMantle(const FVector& Target);
	void TickMovementFeatures(float DeltaSeconds);
	UAnimSequenceBase* PickMovementAnimation(bool& bOutLoop) const;
	void UpdateMovementAnimation();
	void PlayDashAnimation(const FVector& Direction);
	TWeakObjectPtr<UAnimSequenceBase> CurrentMovementAnim;
	virtual void Landed(const FHitResult& Hit) override;

	UFUNCTION(Server, Reliable) void ServerSetSprinting(bool bNewSprinting);
	UFUNCTION(Server, Reliable) void ServerSetProne(bool bNewProne);
	UFUNCTION(Server, Reliable) void ServerSetJumpHeld(bool bHeld);
	UFUNCTION(Server, Reliable) void ServerStartMantle(FVector_NetQuantize Target);
	UFUNCTION(Server, Reliable) void ServerStartSlide();
	UFUNCTION() void OnRep_IsProne();

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Movimento")
	bool bIsSprinting = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsProne, BlueprintReadOnly, Category = "Movimento")
	bool bIsProne = false;

	bool bNoclip = false;
	bool bFirstPerson = false;
	bool bJumpHeld = false;
	bool bCrouchHeld = false;
	bool bPendingRecrouch = false;
	bool bSliding = false;
	float SlideEndTime = 0.f;
	float LastSlideTime = -100.f;
	float WallClimbTimeUsed = 0.f;
	bool bWallClimbing = false;
	bool bMantling = false;
	float MantleElapsed = 0.f;
	FVector MantleStart = FVector::ZeroVector;
	FVector MantleTarget = FVector::ZeroVector;

	UPROPERTY(Transient) TObjectPtr<UInputAction> SprintAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> CrouchAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> ProneAction;

	UFUNCTION(Server, Reliable) void ServerReload();
	UFUNCTION(NetMulticast, Unreliable) void MulticastShotEffects(FVector_NetQuantize Impact, bool bHit);
	UFUNCTION(NetMulticast, Unreliable) void MulticastAreaEffect(FVector_NetQuantize Center, float Radius, FLinearColor Color);
	UFUNCTION(Client, Unreliable) void ClientNotifyHit(FVector_NetQuantize Location, float Damage, bool bHeadshot, bool bKill);

	void DrawShot(const FVector& Impact, bool bHit) const;
	void DrawArea(const FVector& Center, float Radius, FLinearColor Color) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componentes")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componentes")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componentes")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componentes")
	TObjectPtr<UStaticMeshComponent> WeaponComponent;

	UPROPERTY()
	TObjectPtr<UValdoriaAttributeSet> Attributes;

	/** Definição em uso (o Data Asset, ou o Recruta de teste). */
	UPROPERTY(Transient)
	TObjectPtr<UValdoriaHeroDefinition> ActiveDefinition;

	// Entrada criada em código (não depende de arquivos do editor).
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> DefaultMapping;
	UPROPERTY(Transient) TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> LookAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> FireAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> SecondaryAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Ability1Action;
	UPROPERTY(Transient) TObjectPtr<UInputAction> Ability2Action;
	UPROPERTY(Transient) TObjectPtr<UInputAction> UltimateAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool bIsDead = false;

	UPROPERTY(Replicated)
	int32 CurrentAmmo = 30;

	UPROPERTY(Replicated)
	bool bReloading = false;

	TMap<EValdoriaAbilitySlot, float> CooldownEndTimes;
	TArray<FGameplayAbilitySpecHandle> GrantedAbilities;
	FTimerHandle ReloadTimer;
	float LastDamageTakenTime = -100.f;
	bool bFireHeld = false;
	bool bAbilitiesGranted = false;
	FTransform DefaultMeshRelativeTransform;

	friend class AValdoriaGameMode;
	friend class AValdoriaHUD;
};
