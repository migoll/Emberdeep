#include "Gameplay/EmberdeepPlayerController.h"

#include "Emberdeep.h"
#include "Kismet/KismetSystemLibrary.h"

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

void AEmberdeepPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindAction(TEXT("QuitGame"), IE_Pressed, this, &AEmberdeepPlayerController::QuitGame);
}

void AEmberdeepPlayerController::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
