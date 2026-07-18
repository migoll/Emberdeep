#include "Gameplay/EmberdeepPlayerController.h"

#include "Emberdeep.h"

AEmberdeepPlayerController::AEmberdeepPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
}

void AEmberdeepPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (InPawn)
	{
		SetViewTarget(InPawn);
		UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_CAMERA ViewTarget=%s"), *InPawn->GetName());
	}
}
