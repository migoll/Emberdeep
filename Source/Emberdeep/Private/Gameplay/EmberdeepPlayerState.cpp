#include "Gameplay/EmberdeepPlayerState.h"

#include "Gameplay/EmberdeepPlayerController.h"
#include "Net/UnrealNetwork.h"

AEmberdeepPlayerState::AEmberdeepPlayerState()
{
	bReplicates = true;
}

void AEmberdeepPlayerState::AddGold(int32 Amount)
{
	if (!HasAuthority() || Amount <= 0)
	{
		return;
	}

	Gold += Amount;
	OnProgressionChanged.Broadcast();
	ForceNetUpdate();
}

void AEmberdeepPlayerState::AddExperience(int32 Amount)
{
	if (!HasAuthority() || Amount <= 0)
	{
		return;
	}

	CurrentExperience += Amount;
	while (CurrentExperience >= ExperienceToNextLevel)
	{
		CurrentExperience -= ExperienceToNextLevel;
		++CharacterLevel;
		ExperienceToNextLevel = FMath::Max(
			1,
			FMath::RoundToInt(static_cast<float>(ExperienceToNextLevel) * 1.25f));
	}

	OnProgressionChanged.Broadcast();
	ForceNetUpdate();
}

bool AEmberdeepPlayerState::AddItem(const FEmberdeepItemInstance& Item)
{
	if (!HasAuthority() || !Item.IsValid() || Inventory.Num() >= MaxInventorySize || FindItem(Item.InstanceId))
	{
		return false;
	}

	Inventory.Add(Item);
	OnInventoryChanged.Broadcast();
	ForceNetUpdate();
	return true;
}

bool AEmberdeepPlayerState::EquipItem(int32 InstanceId)
{
	if (!HasAuthority())
	{
		return false;
	}

	const FEmberdeepItemInstance* Item = FindItem(InstanceId);
	if (!Item)
	{
		return false;
	}

	switch (Item->Slot)
	{
	case EEmberdeepItemSlot::Weapon:
		EquippedWeaponId = InstanceId;
		break;
	case EEmberdeepItemSlot::Armor:
		EquippedArmorId = InstanceId;
		break;
	case EEmberdeepItemSlot::Trinket:
		EquippedTrinketId = InstanceId;
		break;
	case EEmberdeepItemSlot::None:
	default:
		return false;
	}

	OnInventoryChanged.Broadcast();
	ForceNetUpdate();
	return true;
}

void AEmberdeepPlayerState::InitializeStarterLoadout()
{
	if (!HasAuthority() || Inventory.Num() > 0)
	{
		return;
	}

	const FEmberdeepItemInstance StarterWeapon =
		FEmberdeepItemCatalog::MakeLootItem(TEXT("NotchedIronAxe"), -100, 1);
	const FEmberdeepItemInstance StarterArmor =
		FEmberdeepItemCatalog::MakeLootItem(TEXT("PatchedHide"), -101, 1);
	if (AddItem(StarterWeapon))
	{
		EquipItem(StarterWeapon.InstanceId);
	}
	if (AddItem(StarterArmor))
	{
		EquipItem(StarterArmor.InstanceId);
	}
}

float AEmberdeepPlayerState::GetExperienceNormalized() const
{
	return ExperienceToNextLevel > 0
		? FMath::Clamp(
			static_cast<float>(CurrentExperience) / static_cast<float>(ExperienceToNextLevel),
			0.0f,
			1.0f)
		: 0.0f;
}

const FEmberdeepItemInstance* AEmberdeepPlayerState::FindItem(int32 InstanceId) const
{
	return Inventory.FindByPredicate([InstanceId](const FEmberdeepItemInstance& Item)
	{
		return Item.InstanceId == InstanceId;
	});
}

int32 AEmberdeepPlayerState::GetEquippedItemId(EEmberdeepItemSlot Slot) const
{
	switch (Slot)
	{
	case EEmberdeepItemSlot::Weapon:
		return EquippedWeaponId;
	case EEmberdeepItemSlot::Armor:
		return EquippedArmorId;
	case EEmberdeepItemSlot::Trinket:
		return EquippedTrinketId;
	case EEmberdeepItemSlot::None:
	default:
		return INDEX_NONE;
	}
}

const FEmberdeepItemInstance* AEmberdeepPlayerState::GetEquippedItem(EEmberdeepItemSlot Slot) const
{
	return FindItem(GetEquippedItemId(Slot));
}

bool AEmberdeepPlayerState::IsItemEquipped(int32 InstanceId) const
{
	return InstanceId != INDEX_NONE &&
		(InstanceId == EquippedWeaponId || InstanceId == EquippedArmorId || InstanceId == EquippedTrinketId);
}

float AEmberdeepPlayerState::GetTotalDamageBonus() const
{
	float Total = 0.0f;
	for (const FEmberdeepItemInstance& Item : Inventory)
	{
		if (IsItemEquipped(Item.InstanceId))
		{
			Total += Item.DamageBonus;
		}
	}
	return Total;
}

float AEmberdeepPlayerState::GetTotalMaxHealthBonus() const
{
	float Total = 0.0f;
	for (const FEmberdeepItemInstance& Item : Inventory)
	{
		if (IsItemEquipped(Item.InstanceId))
		{
			Total += Item.MaxHealthBonus;
		}
	}
	return Total;
}

float AEmberdeepPlayerState::GetTotalArmorBonus() const
{
	float Total = 0.0f;
	for (const FEmberdeepItemInstance& Item : Inventory)
	{
		if (IsItemEquipped(Item.InstanceId))
		{
			Total += Item.ArmorBonus;
		}
	}
	return Total;
}

void AEmberdeepPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);
	CopyRunStateTo(Cast<AEmberdeepPlayerState>(PlayerState));
}

void AEmberdeepPlayerState::OverrideWith(APlayerState* PlayerState)
{
	Super::OverrideWith(PlayerState);
	if (const AEmberdeepPlayerState* Source = Cast<AEmberdeepPlayerState>(PlayerState))
	{
		Source->CopyRunStateTo(this);
	}
}

void AEmberdeepPlayerState::CopyRunStateTo(AEmberdeepPlayerState* Target) const
{
	if (!Target)
	{
		return;
	}

	Target->Gold = Gold;
	Target->CharacterLevel = CharacterLevel;
	Target->CurrentExperience = CurrentExperience;
	Target->ExperienceToNextLevel = ExperienceToNextLevel;
	Target->Inventory = Inventory;
	Target->EquippedWeaponId = EquippedWeaponId;
	Target->EquippedArmorId = EquippedArmorId;
	Target->EquippedTrinketId = EquippedTrinketId;
	Target->OnProgressionChanged.Broadcast();
	Target->OnInventoryChanged.Broadcast();
}

void AEmberdeepPlayerState::OnRep_InventoryState()
{
	OnInventoryChanged.Broadcast();
	if (AEmberdeepPlayerController* Controller = Cast<AEmberdeepPlayerController>(GetOwner()))
	{
		Controller->NotifyInventoryChanged();
	}
}

void AEmberdeepPlayerState::OnRep_ProgressionState()
{
	OnProgressionChanged.Broadcast();
}

void AEmberdeepPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AEmberdeepPlayerState, Gold, COND_OwnerOnly);
	DOREPLIFETIME(AEmberdeepPlayerState, CharacterLevel);
	DOREPLIFETIME_CONDITION(AEmberdeepPlayerState, CurrentExperience, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AEmberdeepPlayerState, ExperienceToNextLevel, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AEmberdeepPlayerState, Inventory, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AEmberdeepPlayerState, EquippedWeaponId, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AEmberdeepPlayerState, EquippedArmorId, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AEmberdeepPlayerState, EquippedTrinketId, COND_OwnerOnly);
}
