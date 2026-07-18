#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EmberdeepCharacter.generated.h"

class UCameraComponent;
class UEmberdeepHealthComponent;
class UInstancedStaticMeshComponent;
class USceneComponent;
class USpringArmComponent;

UCLASS()
class EMBERDEEP_API AEmberdeepCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEmberdeepCharacter();
	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	void AddGold(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Combat")
	UEmberdeepHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintPure, Category = "Loot")
	int32 GetGold() const { return Gold; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetDodgeCooldownNormalized() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsDead() const;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void UpdateMouseAim();
	void BasicAttack();
	void HeavyAttack();
	void Dodge();
	void PerformAttack(float Damage, float Radius, float Reach, float KnockbackStrength, float Cooldown);
	void ResetAttackVisual();
	void EndDodge();
	void HandleDeath();
	void RestartEncounter();

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> IsometricCamera;

	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<USceneComponent> ThorgrimAxeRoot;

	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<USceneComponent> ThorgrimShieldRoot;

	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> ThorgrimPaletteMeshes;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TObjectPtr<UEmberdeepHealthComponent> HealthComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float BasicAttackCooldown = 0.38f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float HeavyAttackCooldown = 1.15f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float DodgeCooldown = 1.0f;

	UPROPERTY(Replicated)
	int32 Gold = 0;

	float NextAttackTime = 0.0f;
	float NextDodgeTime = 0.0f;
	bool bInvulnerable = false;
	FRotator ThorgrimAxeRestingRotation;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
