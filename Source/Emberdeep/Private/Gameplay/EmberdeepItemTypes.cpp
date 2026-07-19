#include "Gameplay/EmberdeepItemTypes.h"

namespace
{
	FEmberdeepItemInstance MakeBaseItem(
		FName DefinitionId,
		int32 EntryId,
		int32 ItemLevel,
		const TCHAR* DisplayName,
		EEmberdeepItemSlot Slot,
		EEmberdeepItemRarity Rarity)
	{
		FEmberdeepItemInstance Item;
		Item.InstanceId = EntryId;
		Item.DefinitionId = DefinitionId;
		Item.DisplayName = DisplayName;
		Item.Slot = Slot;
		Item.Rarity = Rarity;
		Item.ItemLevel = FMath::Max(1, ItemLevel);
		return Item;
	}
}

FEmberdeepItemInstance FEmberdeepItemCatalog::MakeLootItem(
	FName DefinitionId,
	int32 EntryId,
	int32 ItemLevel)
{
	const int32 SafeLevel = FMath::Max(1, ItemLevel);

	if (DefinitionId == FName(TEXT("NotchedIronSword")))
	{
		FEmberdeepItemInstance Item = MakeBaseItem(
			DefinitionId,
			EntryId,
			SafeLevel,
			TEXT("Notched Iron Sword"),
			EEmberdeepItemSlot::Weapon,
			EEmberdeepItemRarity::Common);
		Item.DamageBonus = 4.0f + SafeLevel * 1.5f;
		return Item;
	}

	if (DefinitionId == FName(TEXT("NotchedIronAxe")))
	{
		FEmberdeepItemInstance Item = MakeBaseItem(
			DefinitionId,
			EntryId,
			SafeLevel,
			TEXT("Notched Iron Axe"),
			EEmberdeepItemSlot::Weapon,
			EEmberdeepItemRarity::Common);
		Item.DamageBonus = 4.0f + SafeLevel * 1.5f;
		return Item;
	}

	if (DefinitionId == FName(TEXT("BonecarverAxe")))
	{
		FEmberdeepItemInstance Item = MakeBaseItem(
			DefinitionId,
			EntryId,
			SafeLevel,
			TEXT("Bonecarver Axe"),
			EEmberdeepItemSlot::Weapon,
			EEmberdeepItemRarity::Magic);
		Item.DamageBonus = 9.0f + SafeLevel * 2.25f;
		return Item;
	}

	if (DefinitionId == FName(TEXT("WardenCleaver")))
	{
		FEmberdeepItemInstance Item = MakeBaseItem(
			DefinitionId,
			EntryId,
			SafeLevel,
			TEXT("Warden's Cleaver"),
			EEmberdeepItemSlot::Weapon,
			EEmberdeepItemRarity::Rare);
		Item.DamageBonus = 16.0f + SafeLevel * 3.0f;
		Item.ArmorBonus = 2.0f + SafeLevel * 0.4f;
		return Item;
	}

	if (DefinitionId == FName(TEXT("PatchedHide")))
	{
		FEmberdeepItemInstance Item = MakeBaseItem(
			DefinitionId,
			EntryId,
			SafeLevel,
			TEXT("Patched Caravan Hide"),
			EEmberdeepItemSlot::Armor,
			EEmberdeepItemRarity::Common);
		Item.MaxHealthBonus = 15.0f + SafeLevel * 4.0f;
		Item.ArmorBonus = 2.0f + SafeLevel * 0.8f;
		return Item;
	}

	if (DefinitionId == FName(TEXT("BoneguardHauberk")))
	{
		FEmberdeepItemInstance Item = MakeBaseItem(
			DefinitionId,
			EntryId,
			SafeLevel,
			TEXT("Boneguard Hauberk"),
			EEmberdeepItemSlot::Armor,
			EEmberdeepItemRarity::Magic);
		Item.MaxHealthBonus = 34.0f + SafeLevel * 6.0f;
		Item.ArmorBonus = 6.0f + SafeLevel * 1.25f;
		return Item;
	}

	if (DefinitionId == FName(TEXT("WardenPlate")))
	{
		FEmberdeepItemInstance Item = MakeBaseItem(
			DefinitionId,
			EntryId,
			SafeLevel,
			TEXT("Plate of the Bone Warden"),
			EEmberdeepItemSlot::Armor,
			EEmberdeepItemRarity::Rare);
		Item.MaxHealthBonus = 58.0f + SafeLevel * 8.0f;
		Item.ArmorBonus = 11.0f + SafeLevel * 1.75f;
		return Item;
	}

	if (DefinitionId == FName(TEXT("CharredCharm")))
	{
		FEmberdeepItemInstance Item = MakeBaseItem(
			DefinitionId,
			EntryId,
			SafeLevel,
			TEXT("Charred Wayfarer's Charm"),
			EEmberdeepItemSlot::Trinket,
			EEmberdeepItemRarity::Magic);
		Item.DamageBonus = 3.0f + SafeLevel * 0.75f;
		Item.MaxHealthBonus = 10.0f + SafeLevel * 2.5f;
		return Item;
	}

	if (DefinitionId == FName(TEXT("AshenSignet")))
	{
		FEmberdeepItemInstance Item = MakeBaseItem(
			DefinitionId,
			EntryId,
			SafeLevel,
			TEXT("Ashen Signet"),
			EEmberdeepItemSlot::Trinket,
			EEmberdeepItemRarity::Rare);
		Item.DamageBonus = 7.0f + SafeLevel * 1.2f;
		Item.MaxHealthBonus = 22.0f + SafeLevel * 3.5f;
		Item.ArmorBonus = 3.0f + SafeLevel * 0.5f;
		return Item;
	}

	if (DefinitionId == FName(TEXT("EmberdeepOath")))
	{
		FEmberdeepItemInstance Item = MakeBaseItem(
			DefinitionId,
			EntryId,
			SafeLevel,
			TEXT("Emberdeep Oath"),
			EEmberdeepItemSlot::Weapon,
			EEmberdeepItemRarity::Legendary);
		Item.DamageBonus = 28.0f + SafeLevel * 4.5f;
		Item.MaxHealthBonus = 35.0f + SafeLevel * 5.0f;
		Item.ArmorBonus = 8.0f + SafeLevel;
		return Item;
	}

	FEmberdeepItemInstance Fallback = MakeBaseItem(
		DefinitionId.IsNone() ? FName(TEXT("UnknownRelic")) : DefinitionId,
		EntryId,
		SafeLevel,
		TEXT("Unmarked Relic"),
		EEmberdeepItemSlot::Trinket,
		EEmberdeepItemRarity::Common);
	Fallback.MaxHealthBonus = 5.0f + SafeLevel * 2.0f;
	return Fallback;
}

FEmberdeepItemInstance FEmberdeepItemCatalog::MakeRandomDrop(
	int32 EntryId,
	int32 ItemLevel,
	bool bElite)
{
	if (bElite)
	{
		static const FName EliteDefinitions[] =
		{
			TEXT("WardenCleaver"),
			TEXT("WardenPlate"),
			TEXT("AshenSignet")
		};
		return MakeLootItem(
			EliteDefinitions[FMath::RandRange(0, UE_ARRAY_COUNT(EliteDefinitions) - 1)],
			EntryId,
			ItemLevel);
	}

	static const FName CommonDefinitions[] =
	{
		TEXT("NotchedIronSword"),
		TEXT("NotchedIronAxe"),
		TEXT("BonecarverAxe"),
		TEXT("PatchedHide"),
		TEXT("BoneguardHauberk"),
		TEXT("CharredCharm")
	};
	return MakeLootItem(
		CommonDefinitions[FMath::RandRange(0, UE_ARRAY_COUNT(CommonDefinitions) - 1)],
		EntryId,
		ItemLevel);
}

FEmberdeepItemInstance FEmberdeepItemCatalog::MakeLegendaryReward(int32 EntryId, int32 ItemLevel)
{
	return MakeLootItem(TEXT("EmberdeepOath"), EntryId, FMath::Max(1, ItemLevel));
}

FString FEmberdeepItemCatalog::GetRarityDisplayName(EEmberdeepItemRarity Rarity)
{
	switch (Rarity)
	{
	case EEmberdeepItemRarity::Magic:
		return TEXT("Magic");
	case EEmberdeepItemRarity::Rare:
		return TEXT("Rare");
	case EEmberdeepItemRarity::Legendary:
		return TEXT("Legendary");
	case EEmberdeepItemRarity::Common:
	default:
		return TEXT("Common");
	}
}

FLinearColor FEmberdeepItemCatalog::GetRarityColor(EEmberdeepItemRarity Rarity)
{
	switch (Rarity)
	{
	case EEmberdeepItemRarity::Magic:
		return FLinearColor(0.22f, 0.48f, 1.0f);
	case EEmberdeepItemRarity::Rare:
		return FLinearColor(0.78f, 0.30f, 0.95f);
	case EEmberdeepItemRarity::Legendary:
		return FLinearColor(1.0f, 0.55f, 0.08f);
	case EEmberdeepItemRarity::Common:
	default:
		return FLinearColor(0.82f, 0.80f, 0.74f);
	}
}
