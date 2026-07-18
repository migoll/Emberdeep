#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EmberdeepEnemy.generated.h"

class UEmberdeepHealthComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class EMBERDEEP_API AEmberdeepEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	AEmberdeepEnemy();
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	void ConfigureAsElite();
	void ConfigureForRun(int32 Tier, bool bElite);

	UEmberdeepHealthComponent* GetHealthComponent() const { return HealthComponent; }
	bool IsElite() const { return bIsElite; }
	bool IsAttackWindingUp() const { return bAttackWindingUp; }

protected:
	virtual void BeginPlay() override;

private:
	void UpdateServerAI();
	void AttackTarget(ACharacter* TargetCharacter);
	void ResolveEliteAttack();
	void HandleDeath();
	void ResetHitFlash();
	void ApplyBoneColor(const FLinearColor& Color);
	void ApplyEnemyStyle();
	void UpdateVisualPresentation(float DeltaSeconds);
	void ApplySteppedVisualPose(bool bForce = false);

	UFUNCTION()
	void OnRep_EnemyStyle();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayHitFeedback(float Damage, FVector_NetQuantizeNormal HitDirection, bool bFatal);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastSetAttackTelegraph(bool bVisible, bool bEliteAttack);

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TObjectPtr<UEmberdeepHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TObjectPtr<UStaticMeshComponent> AttackTelegraph;

	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<USceneComponent> EnemyVisualRoot;

	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<USceneComponent> WeaponRoot;

	UPROPERTY()
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> BoneVoxelMeshes;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> BoneMaterials;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> WeaponSteelVoxels;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> WeaponGripVoxels;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> AccentVoxels;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> WeaponSteelMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> WeaponGripMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> AccentMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float AggroRange = 520.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float AttackRange = 125.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float AttackDamage = 18.0f;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float AttackCooldown = 1.15f;

	float NextAttackTime = 0.0f;
	bool bHasAggro = false;
	UPROPERTY(ReplicatedUsing = OnRep_EnemyStyle)
	bool bIsElite = false;
	bool bAttackWindingUp = false;
	bool bVisualDead = false;
	int32 GoldDropValue = 7;
	int32 RunTier = 1;
	int32 LastVisualPoseStep = INDEX_NONE;
	float VisualWindupStartTime = -100.0f;
	float VisualAttackRecoveryStartTime = -100.0f;
	float VisualHitStartTime = -100.0f;
	float VisualDeathStartTime = -100.0f;
	float VisualFacingYaw = 0.0f;
	FVector VisualRestingLocation = FVector::ZeroVector;
	FRotator VisualRestingRotation = FRotator::ZeroRotator;
	FVector WeaponRestingLocation = FVector::ZeroVector;
	FRotator WeaponRestingRotation = FRotator::ZeroRotator;
	TWeakObjectPtr<ACharacter> PendingAttackTarget;
};
