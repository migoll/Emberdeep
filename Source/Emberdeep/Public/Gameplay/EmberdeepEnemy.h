#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EmberdeepEnemy.generated.h"

class UEmberdeepHealthComponent;
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

	UEmberdeepHealthComponent* GetHealthComponent() const { return HealthComponent; }

protected:
	virtual void BeginPlay() override;

private:
	void UpdateServerAI();
	void AttackTarget(ACharacter* TargetCharacter);
	void HandleDeath();
	void ResetHitFlash();

	UStaticMeshComponent* CreateBoneBlock(
		FName Name,
		const FVector& Location,
		const FVector& Scale,
		const FRotator& Rotation = FRotator::ZeroRotator);

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TObjectPtr<UEmberdeepHealthComponent> HealthComponent;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> BoneBlocks;

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
};
