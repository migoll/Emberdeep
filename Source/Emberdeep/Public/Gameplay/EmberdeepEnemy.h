#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EmberdeepEnemy.generated.h"

class UEmberdeepHealthComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

UCLASS()
class EMBERDEEP_API AEmberdeepEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	AEmberdeepEnemy();
	virtual void Tick(float DeltaSeconds) override;
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

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TObjectPtr<UEmberdeepHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TObjectPtr<UStaticMeshComponent> AttackTelegraph;

	UPROPERTY()
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> BoneVoxelMeshes;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> BoneMaterials;

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
	bool bIsElite = false;
	bool bAttackWindingUp = false;
	int32 GoldDropValue = 7;
	int32 RunTier = 1;
	TWeakObjectPtr<ACharacter> PendingAttackTarget;
};
