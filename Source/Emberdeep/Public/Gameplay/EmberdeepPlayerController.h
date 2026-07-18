#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EmberdeepPlayerController.generated.h"

UCLASS()
class EMBERDEEP_API AEmberdeepPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AEmberdeepPlayerController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
};
