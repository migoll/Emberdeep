#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Gameplay/EmberdeepItemTypes.h"
#include "EmberdeepPlayerController.generated.h"

class AEmberdeepInteractable;
class AEmberdeepLootContainer;
class UEmberdeepInventoryWidget;
class UEmberdeepLootWidget;
class UEmberdeepMainMenuWidget;

UCLASS()
class EMBERDEEP_API AEmberdeepPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AEmberdeepPlayerController();

	void RequestTakeLoot(int32 EntryId);
	void RequestEquipItem(int32 InstanceId);
	void CloseGameplayWindows();
	void ShowLootWindowFromServer(AEmberdeepLootContainer* Container, const FString& Title, const TArray<FEmberdeepItemInstance>& Items);
	void NotifyInventoryChanged();
	void NotifyInventoryFull();
	void CloseGameplayWindowsFromServer();
	bool GetClosestInteractionPrompt(FString& OutPrompt) const;

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;

private:
	void ShowMainMenu();
	void TryInteract();
	void ToggleInventory();
	void QuitGame();
	AEmberdeepInteractable* FindClosestInteractable() const;
	void CloseGameplayWindowsLocal();

	UFUNCTION(Server, Reliable)
	void ServerInteract();

	UFUNCTION(Server, Reliable)
	void ServerTakeLoot(AEmberdeepLootContainer* Container, int32 EntryId);

	UFUNCTION(Server, Reliable)
	void ServerEquipItem(int32 InstanceId);

	UFUNCTION(Server, Reliable)
	void ServerCloseLootWindow();

	UFUNCTION(Client, Reliable)
	void ClientShowLootWindow(AEmberdeepLootContainer* Container, const FString& Title, const TArray<FEmberdeepItemInstance>& Items);

	UFUNCTION(Client, Reliable)
	void ClientInventoryChanged();

	UFUNCTION(Client, Reliable)
	void ClientInventoryFull();

	UFUNCTION(Client, Reliable)
	void ClientCloseGameplayWindows();

	UPROPERTY()
	TObjectPtr<UEmberdeepMainMenuWidget> MainMenuWidget;

	UPROPERTY()
	TObjectPtr<UEmberdeepLootWidget> LootWidget;

	UPROPERTY()
	TObjectPtr<UEmberdeepInventoryWidget> InventoryWidget;

	UPROPERTY()
	TObjectPtr<AEmberdeepLootContainer> CurrentLootContainer;
};
