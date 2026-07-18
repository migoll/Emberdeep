#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EmberdeepGameMode.generated.h"

class UMaterialInterface;
class UStaticMesh;

UCLASS()
class EMBERDEEP_API AEmberdeepGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AEmberdeepGameMode();
	virtual void StartPlay() override;
	void NotifyEnemyDefeated();

	UFUNCTION(BlueprintPure, Category = "Encounter")
	int32 GetRemainingEnemies() const { return RemainingEnemies; }

	UFUNCTION(BlueprintPure, Category = "Encounter")
	bool IsEncounterComplete() const { return bEncounterStarted && RemainingEnemies <= 0; }

private:
	void SpawnBlockoutArena();
	void SpawnCombatEncounter();
	void SpawnBoundary(const FVector& Location, const FVector& Extent);
	void SpawnBlock(
		const FVector& Location,
		const FVector& Scale,
		const FLinearColor& Color,
		const FRotator& Rotation = FRotator::ZeroRotator,
		bool bEnableCollision = true);

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BlockMaterial;

	int32 RemainingEnemies = 0;
	bool bEncounterStarted = false;
};
