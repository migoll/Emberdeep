#include "Gameplay/EmberdeepInteractable.h"

AEmberdeepInteractable::AEmberdeepInteractable()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
}

FString AEmberdeepInteractable::GetInteractionPrompt(const AEmberdeepCharacter* Character) const
{
	return TEXT("Interact");
}

void AEmberdeepInteractable::Interact(AEmberdeepCharacter* Character, AEmberdeepPlayerController* Controller)
{
}
