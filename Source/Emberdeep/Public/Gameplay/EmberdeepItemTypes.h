#pragma once

#include "CoreMinimal.h"
#include "EmberdeepItemTypes.generated.h"

UENUM(BlueprintType)
enum class EEmberdeepItemSlot : uint8
{
	None UMETA(DisplayName = "None"),
	Weapon UMETA(DisplayName = "Weapon"),
	Armor UMETA(DisplayName = "Armor"),
	Trinket UMETA(DisplayName = "Trinket")
};

UENUM(BlueprintType)
enum class EEmberdeepItemRarity : uint8
{
	Common UMETA(DisplayName = "Common"),
	Magic UMETA(DisplayName = "Magic"),
	Rare UMETA(DisplayName = "Rare"),
	Legendary UMETA(DisplayName = "Legendary")
};

USTRUCT(BlueprintType)
struct EMBERDEEP_API FEmberdeepItemInstance
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	int32 InstanceId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FName DefinitionId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FString DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	EEmberdeepItemSlot Slot = EEmberdeepItemSlot::None;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	EEmberdeepItemRarity Rarity = EEmberdeepItemRarity::Common;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	int32 ItemLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	float DamageBonus = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	float MaxHealthBonus = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	float ArmorBonus = 0.0f;

	bool IsValid() const
	{
		return InstanceId != INDEX_NONE && DefinitionId != NAME_None && Slot != EEmberdeepItemSlot::None;
	}
};

/**
 * Small Phase 2 vertical-slice catalog. Item instances remain plain replicated
 * values; presentation can later move to Data Assets without changing the
 * inventory or networking contract.
 */
class EMBERDEEP_API FEmberdeepItemCatalog
{
public:
	static FEmberdeepItemInstance MakeLootItem(FName DefinitionId, int32 EntryId, int32 ItemLevel);
	static FEmberdeepItemInstance MakeRandomDrop(int32 EntryId, int32 ItemLevel, bool bElite);
	static FEmberdeepItemInstance MakeLegendaryReward(int32 EntryId, int32 ItemLevel);
	static FString GetRarityDisplayName(EEmberdeepItemRarity Rarity);
	static FLinearColor GetRarityColor(EEmberdeepItemRarity Rarity);
};
