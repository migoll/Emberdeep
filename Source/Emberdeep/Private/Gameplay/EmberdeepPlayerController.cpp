#include "Gameplay/EmberdeepPlayerController.h"

#include "Emberdeep.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/EmberdeepMainMenuWidget.h"
#include "Blueprint/UserWidget.h"

AEmberdeepPlayerController::AEmberdeepPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
}

void AEmberdeepPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController() && GetNetMode() == NM_Standalone && !FParse::Param(FCommandLine::Get(), TEXT("SkipMainMenu")))
	{
		ShowMainMenu();
	}
}

void AEmberdeepPlayerController::ShowMainMenu()
{
	MainMenuWidget = CreateWidget<UEmberdeepMainMenuWidget>(this, UEmberdeepMainMenuWidget::StaticClass());
	if (MainMenuWidget)
	{
		MainMenuWidget->AddToViewport(100);
		SetPause(true);
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
	}
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
