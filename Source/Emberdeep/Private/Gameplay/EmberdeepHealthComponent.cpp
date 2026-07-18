#include "Gameplay/EmberdeepHealthComponent.h"

#include "Net/UnrealNetwork.h"

UEmberdeepHealthComponent::UEmberdeepHealthComponent()
{
	SetIsReplicatedByDefault(true);
}

float UEmberdeepHealthComponent::ApplyDamage(float DamageAmount)
{
	if (DamageAmount <= 0.0f || IsDead())
	{
		return 0.0f;
	}

	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.0f, MaxHealth);
	const float AppliedDamage = PreviousHealth - CurrentHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, -AppliedDamage);

	if (CurrentHealth <= 0.0f)
	{
		OnDeath.Broadcast();
	}

	return AppliedDamage;
}

void UEmberdeepHealthComponent::ResetHealth()
{
	const float PreviousHealth = CurrentHealth;
	CurrentHealth = MaxHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, CurrentHealth - PreviousHealth);
}

void UEmberdeepHealthComponent::SetMaxHealth(float NewMaxHealth, bool bFullyHeal)
{
	MaxHealth = FMath::Max(1.0f, NewMaxHealth);
	CurrentHealth = bFullyHeal ? MaxHealth : FMath::Min(CurrentHealth, MaxHealth);
}

float UEmberdeepHealthComponent::GetHealthNormalized() const
{
	return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
}

void UEmberdeepHealthComponent::OnRep_CurrentHealth(float PreviousHealth)
{
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, CurrentHealth - PreviousHealth);
	if (CurrentHealth <= 0.0f && PreviousHealth > 0.0f)
	{
		OnDeath.Broadcast();
	}
}

void UEmberdeepHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UEmberdeepHealthComponent, CurrentHealth);
}
