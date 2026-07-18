#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Gameplay/EmberdeepItemTypes.h"
#include "EmberdeepPlayerState.generated.h"

DECLARE_MULTICAST_DELEGATE(FEmberdeepInventoryChanged);
DECLARE_MULTICAST_DELEGATE(FEmberdeepProgressionChanged);

UCLASS()
class EMBERDEEP_API AEmberdeepPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AEmberdeepPlayerState();

	void AddGold(int32 Amount);
	void AddExperience(int32 Amount);
	bool AddItem(const FEmberdeepItemInstance& Item);
	bool EquipItem(int32 InstanceId);
	void InitializeStarterLoadout();

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetGold() const { return Gold; }

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetCharacterLevel() const { return CharacterLevel; }

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetCurrentExperience() const { return CurrentExperience; }

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetExperienceToNextLevel() const { return ExperienceToNextLevel; }

	UFUNCTION(BlueprintPure, Category = "Progression")
	float GetExperienceNormalized() const;

	const TArray<FEmberdeepItemInstance>& GetInventory() const { return Inventory; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetInventoryCapacity() const { return MaxInventorySize; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetEquippedWeaponId() const { return EquippedWeaponId; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetEquippedArmorId() const { return EquippedArmorId; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetEquippedTrinketId() const { return EquippedTrinketId; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetTotalDamageBonus() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetTotalMaxHealthBonus() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetTotalArmorBonus() const;

	const FEmberdeepItemInstance* FindItem(int32 InstanceId) const;
	int32 GetEquippedItemId(EEmberdeepItemSlot Slot) const;
	const FEmberdeepItemInstance* GetEquippedItem(EEmberdeepItemSlot Slot) const;
	bool IsItemEquipped(int32 InstanceId) const;

	FEmberdeepInventoryChanged OnInventoryChanged;
	FEmberdeepProgressionChanged OnProgressionChanged;

	virtual void CopyProperties(APlayerState* PlayerState) override;
	virtual void OverrideWith(APlayerState* PlayerState) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_InventoryState();

	UFUNCTION()
	void OnRep_ProgressionState();

	void CopyRunStateTo(AEmberdeepPlayerState* Target) const;

	UPROPERTY(ReplicatedUsing = OnRep_ProgressionState)
	int32 Gold = 0;

	UPROPERTY(ReplicatedUsing = OnRep_ProgressionState)
	int32 CharacterLevel = 1;

	UPROPERTY(ReplicatedUsing = OnRep_ProgressionState)
	int32 CurrentExperience = 0;

	UPROPERTY(ReplicatedUsing = OnRep_ProgressionState)
	int32 ExperienceToNextLevel = 100;

	UPROPERTY(ReplicatedUsing = OnRep_InventoryState)
	TArray<FEmberdeepItemInstance> Inventory;

	UPROPERTY(ReplicatedUsing = OnRep_InventoryState)
	int32 EquippedWeaponId = INDEX_NONE;

	UPROPERTY(ReplicatedUsing = OnRep_InventoryState)
	int32 EquippedArmorId = INDEX_NONE;

	UPROPERTY(ReplicatedUsing = OnRep_InventoryState)
	int32 EquippedTrinketId = INDEX_NONE;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory", meta = (ClampMin = "1", ClampMax = "64"))
	int32 MaxInventorySize = 12;
};
