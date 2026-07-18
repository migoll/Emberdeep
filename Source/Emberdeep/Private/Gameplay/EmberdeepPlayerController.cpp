#include "Gameplay/EmberdeepPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Emberdeep.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Gameplay/EmberdeepCharacter.h"
#include "Gameplay/EmberdeepInteractable.h"
#include "Gameplay/EmberdeepLootContainer.h"
#include "Gameplay/EmberdeepPlayerState.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/EmberdeepInventoryWidget.h"
#include "UI/EmberdeepLootWidget.h"
#include "UI/EmberdeepMainMenuWidget.h"
#include "TimerManager.h"

AEmberdeepPlayerController::AEmberdeepPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AEmberdeepPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController() && GetNetMode() == NM_Standalone && !FParse::Param(FCommandLine::Get(), TEXT("SkipMainMenu")))
	{
		ShowMainMenu();
	}
	else if (IsLocalController() && FParse::Param(FCommandLine::Get(), TEXT("AutoInventoryPreview")))
	{
		FTimerHandle PreviewTimer;
		GetWorldTimerManager().SetTimer(PreviewTimer, this, &AEmberdeepPlayerController::ToggleInventory, 1.0f, false);
	}

	// Visual QA can request a settled gameplay frame instead of capturing the
	// first render tick while shaders, materials, AI, and camera state are still
	// warming up. This path is inert in every normal player launch.
	if (IsLocalController() && FParse::Param(FCommandLine::Get(), TEXT("AutoScreenshot")))
	{
		FTimerHandle ScreenshotTimer;
		FTimerDelegate ScreenshotDelegate = FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_QA Capturing settled gameplay frame"));
			ConsoleCommand(TEXT("HighResShot 1"), true);
		});
		GetWorldTimerManager().SetTimer(ScreenshotTimer, ScreenshotDelegate, 4.0f, false);
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
	InputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AEmberdeepPlayerController::TryInteract);
	InputComponent->BindAction(TEXT("Inventory"), IE_Pressed, this, &AEmberdeepPlayerController::ToggleInventory);
	InputComponent->BindAction(TEXT("QuitGame"), IE_Pressed, this, &AEmberdeepPlayerController::QuitGame);
}

void AEmberdeepPlayerController::TryInteract()
{
	if (IsLocalController())
	{
		ServerInteract();
	}
}

void AEmberdeepPlayerController::ServerInteract_Implementation()
{
	AEmberdeepCharacter* PlayerCharacter = Cast<AEmberdeepCharacter>(GetPawn());
	AEmberdeepInteractable* Interactable = FindClosestInteractable();
	if (!PlayerCharacter || PlayerCharacter->IsDead() || !Interactable)
	{
		return;
	}
	const float Distance = FVector::Dist2D(PlayerCharacter->GetActorLocation(), Interactable->GetActorLocation());
	if (Distance <= Interactable->GetInteractionRange())
	{
		Interactable->Interact(PlayerCharacter, this);
	}
}

AEmberdeepInteractable* AEmberdeepPlayerController::FindClosestInteractable() const
{
	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return nullptr;
	}

	AEmberdeepInteractable* Closest = nullptr;
	float ClosestDistanceSquared = TNumericLimits<float>::Max();
	for (TActorIterator<AEmberdeepInteractable> It(GetWorld()); It; ++It)
	{
		AEmberdeepInteractable* Candidate = *It;
		if (!IsValid(Candidate))
		{
			continue;
		}
		const float DistanceSquared = FVector::DistSquared2D(ControlledPawn->GetActorLocation(), Candidate->GetActorLocation());
		const float Range = Candidate->GetInteractionRange();
		if (DistanceSquared <= Range * Range && DistanceSquared < ClosestDistanceSquared)
		{
			Closest = Candidate;
			ClosestDistanceSquared = DistanceSquared;
		}
	}
	return Closest;
}

bool AEmberdeepPlayerController::GetClosestInteractionPrompt(FString& OutPrompt) const
{
	if (const AEmberdeepInteractable* Interactable = FindClosestInteractable())
	{
		OutPrompt = Interactable->GetInteractionPrompt(Cast<AEmberdeepCharacter>(GetPawn()));
		return !OutPrompt.IsEmpty();
	}
	return false;
}

void AEmberdeepPlayerController::ShowLootWindowFromServer(
	AEmberdeepLootContainer* Container,
	const FString& Title,
	const TArray<FEmberdeepItemInstance>& Items)
{
	if (!HasAuthority())
	{
		return;
	}
	CurrentLootContainer = Container;
	ClientShowLootWindow(Container, Title, Items);
}

void AEmberdeepPlayerController::ClientShowLootWindow_Implementation(
	AEmberdeepLootContainer* Container,
	const FString& Title,
	const TArray<FEmberdeepItemInstance>& Items)
{
	CurrentLootContainer = Container;
	if (!LootWidget)
	{
		LootWidget = CreateWidget<UEmberdeepLootWidget>(this, UEmberdeepLootWidget::StaticClass());
	}
	if (LootWidget)
	{
		LootWidget->ShowLoot(Title, Items);
	}
}

void AEmberdeepPlayerController::RequestTakeLoot(int32 EntryId)
{
	if (CurrentLootContainer)
	{
		ServerTakeLoot(CurrentLootContainer, EntryId);
	}
}

void AEmberdeepPlayerController::ServerTakeLoot_Implementation(AEmberdeepLootContainer* Container, int32 EntryId)
{
	AEmberdeepCharacter* PlayerCharacter = Cast<AEmberdeepCharacter>(GetPawn());
	if (!PlayerCharacter || PlayerCharacter->IsDead() || !Container || Container != CurrentLootContainer)
	{
		return;
	}
	if (FVector::DistSquared2D(PlayerCharacter->GetActorLocation(), Container->GetActorLocation()) > FMath::Square(Container->GetInteractionRange() + 20.0f))
	{
		return;
	}
	Container->TryTakeEntry(this, EntryId);
}

void AEmberdeepPlayerController::ToggleInventory()
{
	if (!IsLocalController())
	{
		return;
	}
	if (InventoryWidget && InventoryWidget->IsInViewport())
	{
		CloseGameplayWindows();
		return;
	}
	if (!InventoryWidget)
	{
		InventoryWidget = CreateWidget<UEmberdeepInventoryWidget>(this, UEmberdeepInventoryWidget::StaticClass());
	}
	if (InventoryWidget)
	{
		InventoryWidget->ShowInventory();
	}
}

void AEmberdeepPlayerController::RequestEquipItem(int32 InstanceId)
{
	ServerEquipItem(InstanceId);
}

void AEmberdeepPlayerController::ServerEquipItem_Implementation(int32 InstanceId)
{
	AEmberdeepPlayerState* RunState = GetPlayerState<AEmberdeepPlayerState>();
	if (RunState && RunState->EquipItem(InstanceId))
	{
		if (AEmberdeepCharacter* PlayerCharacter = Cast<AEmberdeepCharacter>(GetPawn()))
		{
			PlayerCharacter->ApplyEquipmentStats();
		}
		UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_EQUIPMENT Equipped Player=%d Instance=%d Damage=%.1f Health=%.1f Armor=%.1f"),
			RunState->GetPlayerId(), InstanceId, RunState->GetTotalDamageBonus(),
			RunState->GetTotalMaxHealthBonus(), RunState->GetTotalArmorBonus());
		NotifyInventoryChanged();
	}
}

void AEmberdeepPlayerController::NotifyInventoryChanged()
{
	if (HasAuthority())
	{
		ClientInventoryChanged();
	}
	else if (InventoryWidget && InventoryWidget->IsInViewport())
	{
		InventoryWidget->RefreshInventory();
	}
}

void AEmberdeepPlayerController::ClientInventoryChanged_Implementation()
{
	if (InventoryWidget && InventoryWidget->IsInViewport())
	{
		InventoryWidget->RefreshInventory();
	}
}

void AEmberdeepPlayerController::NotifyInventoryFull()
{
	if (HasAuthority())
	{
		ClientInventoryFull();
	}
}

void AEmberdeepPlayerController::ClientInventoryFull_Implementation()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.5f, FColor::Orange, TEXT("Inventory full - equip or leave an item behind."));
	}
}

void AEmberdeepPlayerController::CloseGameplayWindows()
{
	ServerCloseLootWindow();
	CloseGameplayWindowsLocal();
}

void AEmberdeepPlayerController::ServerCloseLootWindow_Implementation()
{
	if (CurrentLootContainer)
	{
		CurrentLootContainer->RemoveViewer(this);
	}
	CurrentLootContainer = nullptr;
}

void AEmberdeepPlayerController::CloseGameplayWindowsFromServer()
{
	if (CurrentLootContainer)
	{
		CurrentLootContainer->RemoveViewer(this);
	}
	CurrentLootContainer = nullptr;
	ClientCloseGameplayWindows();
}

void AEmberdeepPlayerController::ClientCloseGameplayWindows_Implementation()
{
	CloseGameplayWindowsLocal();
}

void AEmberdeepPlayerController::CloseGameplayWindowsLocal()
{
	if (LootWidget)
	{
		LootWidget->RemoveFromParent();
	}
	if (InventoryWidget)
	{
		InventoryWidget->RemoveFromParent();
	}
	CurrentLootContainer = nullptr;
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = true;
}

void AEmberdeepPlayerController::QuitGame()
{
	if ((LootWidget && LootWidget->IsInViewport()) || (InventoryWidget && InventoryWidget->IsInViewport()))
	{
		CloseGameplayWindows();
		return;
	}
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
