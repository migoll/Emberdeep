#pragma once

#include "CoreMinimal.h"
#include "Gameplay/EmberdeepInteractable.h"
#include "Gameplay/EmberdeepItemTypes.h"
#include "EmberdeepLootContainer.generated.h"

class AEmberdeepPlayerController;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;

UCLASS()
class EMBERDEEP_API AEmberdeepLootContainer : public AEmberdeepInteractable
{
	GENERATED_BODY()

public:
	AEmberdeepLootContainer();

	void ConfigureLoot(const FString& NewTitle, const TArray<FEmberdeepItemInstance>& NewEntries, bool bAsChest, bool bLegendary);
	virtual FString GetInteractionPrompt(const AEmberdeepCharacter* Character) const override;
	virtual void Interact(AEmberdeepCharacter* Character, AEmberdeepPlayerController* Controller) override;

	bool TryTakeEntry(AEmberdeepPlayerController* Controller, int32 EntryId);
	void AddViewer(AEmberdeepPlayerController* Controller);
	void RemoveViewer(AEmberdeepPlayerController* Controller);
	void RefreshViewers();
	const TArray<FEmberdeepItemInstance>& GetLootEntries() const { return LootEntries; }
	const FString& GetContainerTitle() const { return ContainerTitle; }
	bool IsDepleted() const { return bDepleted; }

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_VisualState();

	void ApplyVisualState();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ContainerBase;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> ContainerLid;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> LootGlow;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> LootLight;

	UPROPERTY(ReplicatedUsing = OnRep_VisualState)
	FString ContainerTitle = TEXT("Forgotten Spoils");

	UPROPERTY(ReplicatedUsing = OnRep_VisualState)
	bool bChest = false;

	UPROPERTY(ReplicatedUsing = OnRep_VisualState)
	bool bLegendaryChest = false;

	UPROPERTY(ReplicatedUsing = OnRep_VisualState)
	bool bDepleted = false;

	// Contents are intentionally server-only. Viewers receive validated snapshots
	// through their owning PlayerController instead of globally replicated loot.
	TArray<FEmberdeepItemInstance> LootEntries;
	TArray<TWeakObjectPtr<AEmberdeepPlayerController>> Viewers;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
