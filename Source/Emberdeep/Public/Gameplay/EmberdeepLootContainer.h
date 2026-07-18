#pragma once

#include "CoreMinimal.h"
#include "Gameplay/EmberdeepInteractable.h"
#include "Gameplay/EmberdeepItemTypes.h"
#include "EmberdeepLootContainer.generated.h"

class AEmberdeepPlayerController;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class USceneComponent;

UCLASS()
class EMBERDEEP_API AEmberdeepLootContainer : public AEmberdeepInteractable
{
	GENERATED_BODY()

public:
	AEmberdeepLootContainer();
	virtual void Tick(float DeltaSeconds) override;

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
	TObjectPtr<USceneComponent> ChestLidPivot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> LootGlowRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> ChestWoodVoxels;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> ChestPanelVoxels;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> ChestIronVoxels;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> ChestLegendaryVoxels;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> ChestLidWoodVoxels;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> ChestLidIronVoxels;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> ChestLidLegendaryVoxels;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> ChestGlowVoxels;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> DropLeatherVoxels;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> DropMetalVoxels;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> DropLegendaryVoxels;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> DropGlowVoxels;

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

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> ChestGlowMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DropGlowMaterial;

	float LootVisualTime = 0.0f;
	float LootLightBaseIntensity = 0.0f;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
