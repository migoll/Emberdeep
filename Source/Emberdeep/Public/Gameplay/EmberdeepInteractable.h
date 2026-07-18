#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EmberdeepInteractable.generated.h"

class AEmberdeepCharacter;
class AEmberdeepPlayerController;

UCLASS(Abstract)
class EMBERDEEP_API AEmberdeepInteractable : public AActor
{
	GENERATED_BODY()

public:
	AEmberdeepInteractable();

	virtual FString GetInteractionPrompt(const AEmberdeepCharacter* Character) const;
	virtual void Interact(AEmberdeepCharacter* Character, AEmberdeepPlayerController* Controller);

	UFUNCTION(BlueprintPure, Category = "Interaction")
	float GetInteractionRange() const { return InteractionRange; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionRange = 230.0f;
};
