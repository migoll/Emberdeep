#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EmberdeepHealthComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_ThreeParams(FEmberdeepHealthChanged, float, float, float);
DECLARE_MULTICAST_DELEGATE(FEmberdeepDeath);

UCLASS(ClassGroup = (Emberdeep), meta = (BlueprintSpawnableComponent))
class EMBERDEEP_API UEmberdeepHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEmberdeepHealthComponent();

	float ApplyDamage(float DamageAmount);
	void ResetHealth();
	void SetMaxHealth(float NewMaxHealth, bool bFullyHeal = true);

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthNormalized() const;

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return CurrentHealth <= 0.0f; }

	FEmberdeepHealthChanged OnHealthChanged;
	FEmberdeepDeath OnDeath;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_CurrentHealth(float PreviousHealth);

	UPROPERTY(EditDefaultsOnly, Replicated, Category = "Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth)
	float CurrentHealth = 100.0f;
};
