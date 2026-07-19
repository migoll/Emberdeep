#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EmberdeepCharacter.generated.h"

class UCameraComponent;
class UEmberdeepHealthComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class USceneComponent;
class USpringArmComponent;

UCLASS()
class EMBERDEEP_API AEmberdeepCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEmberdeepCharacter();
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	void AddGold(int32 Amount);
	void AddExperience(int32 Amount);
	void ApplyEquipmentStats();

	UFUNCTION(BlueprintPure, Category = "Combat")
	UEmberdeepHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintPure, Category = "Loot")
	int32 GetGold() const;

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetCharacterLevel() const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	float GetEquipmentDamageBonus() const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	float GetEquipmentArmorBonus() const;

	UFUNCTION(BlueprintPure, Category = "Progression")
	float GetExperienceNormalized() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetDodgeCooldownNormalized() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetBasicAttackCooldownNormalized() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetBasicAttackFeedbackNormalized() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetBasicAttacksPerSecond() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetAttackSpeedMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsBasicAttackQueued() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsDead() const;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void ZoomCamera(float Value);
	void UpdateMouseAim();
	void BasicAttack();
	void StopBasicAttack();
	void HeavyAttack();
	void Dodge();
	void TogglePlayableCharacter();
	void RebuildPlayableVoxelCharacter(bool bUseThorgrim);
	void PollLiveSyncData();
	void RequestAttack(bool bHeavyAttack);
	void ExecuteAttack(bool bHeavyAttack, const FVector& AttackDirection);
	void ResolvePendingAttack();
	float GetAttackClipDuration(bool bHeavyAttack) const;
	float GetAttackDuration(bool bHeavyAttack) const;
	void PlayAttackVisual(bool bHeavyAttack, int32 ComboStep, float AttackDuration);
	void PlayAttackImpact(bool bHeavyAttack, int32 HitCount, int32 ComboStep);
	void PlayHurtVisual(float Damage, bool bFatal);
	void StartLocalCameraShake(float Strength, float Duration);
	void UpdateLocalCameraShake(float DeltaSeconds);
	void UpdateVisualPresentation(float DeltaSeconds);
	void ApplySteppedVisualPose(bool bForce = false);
	void ResetHitVisual();
	void UpdateThorgrimAnimation(float DeltaSeconds);
	void ExecuteDodge(const FVector& DodgeDirection);
	void ResetAttackVisual();
	void EndDodge();
	void HandleDeath();
	float GetBasicAttackInterval() const;

	UFUNCTION(Server, Unreliable)
	void ServerSetAimDirection(FVector_NetQuantizeNormal NewAimDirection);

	UFUNCTION(Server, Reliable)
	void ServerPerformAttack(bool bHeavyAttack, FVector_NetQuantizeNormal RequestedAimDirection);

	UFUNCTION(Server, Reliable)
	void ServerSetBasicAttackHeld(bool bHeld, FVector_NetQuantizeNormal RequestedAimDirection);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayAttackVisual(bool bHeavyAttack, int32 ComboStep, float AttackDuration);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayAttackImpact(bool bHeavyAttack, int32 HitCount, int32 ComboStep);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayDodgeVisual(FVector_NetQuantizeNormal DodgeDirection);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayHurtVisual(float Damage, bool bFatal);

	UFUNCTION(Server, Reliable)
	void ServerDodge(FVector_NetQuantizeNormal RequestedDodgeDirection);

	UFUNCTION()
	void OnRep_AimDirection();

	UFUNCTION()
	void OnRep_EquippedWeaponVisual();

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> IsometricCamera;

	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<USceneComponent> ThorgrimVisualRoot;

	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<USceneComponent> ThorgrimBodyRoot;

	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<USceneComponent> ThorgrimAxeRoot;

	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<USceneComponent> ThorgrimShieldRoot;

	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> ThorgrimPaletteMeshes;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> ThorgrimMaterials;

	TArray<FLinearColor> ThorgrimMaterialBaseColors;

	// Runtime-only rest data for the rigid voxel rig. Each visible cell remains
	// uniformly scaled and is assigned wholly to one procedural body zone.
	TArray<TArray<FTransform>> ThorgrimRestTransformsByMesh;
	TArray<TArray<uint8>> ThorgrimRigidBonesByMesh;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TObjectPtr<UEmberdeepHealthComponent> HealthComponent;

	// Blockbench supplies the base duration. Future equipment and buffs modify
	// this single scalar so animation, hit frame, and recovery stay aligned.
	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0.25", ClampMax = "3.0"))
	float AttackSpeedMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0.05", ClampMax = "0.95"))
	float BasicAttackHitNormalizedTime = 0.40f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0.05", ClampMax = "0.95"))
	float HeavyAttackHitNormalizedTime = 0.50f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float DodgeCooldown = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Zoom")
	float MinimumOrthoWidth = 420.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Zoom")
	float MaximumOrthoWidth = 1550.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Zoom")
	float ZoomStep = 140.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Zoom")
	float ZoomInterpolationSpeed = 10.0f;

	UPROPERTY(ReplicatedUsing = OnRep_AimDirection)
	FVector_NetQuantizeNormal ReplicatedAimDirection = FVector::ForwardVector;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeaponVisual)
	FName EquippedWeaponVisualId = TEXT("NotchedIronSword");

	float NextAttackTime = 0.0f;
	float LastBasicAttackTime = -10.0f;
	float BasicAttackCooldownVisualEndTime = 0.0f;
	float BasicAttackFeedbackStartTime = -1.0f;
	float NextDodgeTime = 0.0f;
	float NextAimReplicationTime = 0.0f;
	float TargetOrthoWidth = 1100.0f;
	float CameraShakeRemaining = 0.0f;
	float CameraShakeDuration = 0.0f;
	float CameraShakeStrength = 0.0f;
	FVector LastSentAimDirection = FVector::ForwardVector;
	bool bInvulnerable = false;
	int32 AttackVisualSequence = 0;
	int32 BasicComboStep = 0;
	int32 LastVisualPoseStep = INDEX_NONE;
	float VisualAttackStartTime = -100.0f;
	float VisualAttackDuration = 0.0f;
	float VisualDodgeStartTime = -100.0f;
	float VisualHurtStartTime = -100.0f;
	float VisualDeathStartTime = -100.0f;
	float VisualFacingYaw = 0.0f;
	bool bVisualHeavyAttack = false;
	bool bVisualDead = false;
	int32 VisualComboStep = 0;
	FVector VisualDodgeDirection = FVector::ForwardVector;
	FVector ThorgrimVisualRestingLocation = FVector::ZeroVector;
	FRotator ThorgrimVisualRestingRotation = FRotator::ZeroRotator;
	FVector ThorgrimBodyRestingLocation = FVector::ZeroVector;
	FRotator ThorgrimAxeRestingRotation;
	FRotator ThorgrimShieldRestingRotation = FRotator::ZeroRotator;
	bool bBasicAttackHeld = false;
	bool bBasicAttackQueued = false;
	FVector QueuedBasicAttackDirection = FVector::ForwardVector;
	FVector PlayableAxePivot = FVector::ZeroVector;
	FVector PlayableShieldPivot = FVector::ZeroVector;
	FString PlayableVoxelCharacterName;
	float ThorgrimAnimationTime = 0.0f;
	float ThorgrimPoseAccumulator = 0.0f;
	float ThorgrimAttackTimeRemaining = 0.0f;
	float ThorgrimAttackDuration = 0.0f;
	float ThorgrimDodgeTimeRemaining = 0.0f;
	float ThorgrimDodgeDuration = 0.28f;
	FTimerHandle PendingAttackTimer;
	FTimerHandle AttackVisualTimer;
	FVector PendingAttackDirection = FVector::ForwardVector;
	float PendingAttackDamage = 0.0f;
	float PendingAttackRadius = 0.0f;
	float PendingAttackReach = 0.0f;
	float PendingAttackKnockbackStrength = 0.0f;
	int32 PendingAttackComboStep = INDEX_NONE;
	bool bPendingAttack = false;
	bool bPendingHeavyAttack = false;
	bool bThorgrimHeavyAttack = false;
	bool bUsingThorgrimVisual = false;
	int32 AppliedLiveSyncRevision = INDEX_NONE;
	float NextLiveSyncCheckTime = 0.0f;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
