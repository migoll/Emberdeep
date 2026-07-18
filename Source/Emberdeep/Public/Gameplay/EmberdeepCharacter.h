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
	void AddExperience(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Combat")
	UEmberdeepHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintPure, Category = "Loot")
	int32 GetGold() const { return Gold; }

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetCharacterLevel() const { return CharacterLevel; }

	UFUNCTION(BlueprintPure, Category = "Progression")
	float GetExperienceNormalized() const;

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
	void ZoomCamera(float Value);
	void UpdateMouseAim();
	void BasicAttack();
	void HeavyAttack();
	void Dodge();
	void RequestAttack(bool bHeavyAttack);
	void ExecuteAttack(bool bHeavyAttack, const FVector& AttackDirection);
	void PlayAttackVisual(bool bHeavyAttack);
	void ExecuteDodge(const FVector& DodgeDirection);
	void ResetAttackVisual();
	void EndDodge();
	void HandleDeath();
	void RestartEncounter();

	UFUNCTION(Server, Unreliable)
	void ServerSetAimDirection(FVector_NetQuantizeNormal NewAimDirection);

	UFUNCTION(Server, Reliable)
	void ServerPerformAttack(bool bHeavyAttack, FVector_NetQuantizeNormal RequestedAimDirection);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayAttackVisual(bool bHeavyAttack);

	UFUNCTION(Server, Reliable)
	void ServerDodge(FVector_NetQuantizeNormal RequestedDodgeDirection);

	UFUNCTION()
	void OnRep_AimDirection();

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

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Zoom")
	float MinimumOrthoWidth = 420.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Zoom")
	float MaximumOrthoWidth = 1550.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Zoom")
	float ZoomStep = 140.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera|Zoom")
	float ZoomInterpolationSpeed = 10.0f;

	UPROPERTY(Replicated)
	int32 Gold = 0;

	UPROPERTY(Replicated)
	int32 CharacterLevel = 1;

	UPROPERTY(Replicated)
	int32 CurrentExperience = 0;

	UPROPERTY(Replicated)
	int32 ExperienceToNextLevel = 100;

	UPROPERTY(ReplicatedUsing = OnRep_AimDirection)
	FVector_NetQuantizeNormal ReplicatedAimDirection = FVector::ForwardVector;

	float NextAttackTime = 0.0f;
	float NextDodgeTime = 0.0f;
	float NextAimReplicationTime = 0.0f;
	float TargetOrthoWidth = 1100.0f;
	FVector LastSentAimDirection = FVector::ForwardVector;
	bool bInvulnerable = false;
	FRotator ThorgrimAxeRestingRotation;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
