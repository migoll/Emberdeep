#pragma once

#include "CoreMinimal.h"
#include "EmberdeepRunTypes.generated.h"

UENUM(BlueprintType)
enum class EEmberdeepRunStage : uint8
{
	Camp,
	Dungeon,
	RewardRoom
};

inline FString EmberdeepRunStageName(EEmberdeepRunStage Stage)
{
	switch (Stage)
	{
	case EEmberdeepRunStage::Dungeon:
		return TEXT("THE ASHEN CRYPT");
	case EEmberdeepRunStage::RewardRoom:
		return TEXT("THE CINDER VAULT");
	default:
		return TEXT("BROKEN CARAVAN CAMP");
	}
}
