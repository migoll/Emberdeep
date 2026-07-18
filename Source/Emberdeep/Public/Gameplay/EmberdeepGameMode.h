#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Gameplay/EmberdeepRunTypes.h"
#include "EmberdeepGameMode.generated.h"

class AEmberdeepCharacter;
class AEmberdeepPortal;

UCLASS()
class EMBERDEEP_API AEmberdeepGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AEmberdeepGameMode();
	virtual void StartPlay() override;
	virtual void PreLogin(
		const FString& Options,
		const FString& Address,
		const FUniqueNetIdRepl& UniqueId,
		FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	void NotifyEnemyDefeated();
	void SchedulePlayerRespawn(AEmberdeepCharacter* DefeatedCharacter);
	void SpawnLootForEnemy(const FVector& Location, bool bElite);
	void EnterRunStage(EEmberdeepRunStage NewStage);

	UFUNCTION(BlueprintPure, Category = "Encounter")
	int32 GetRemainingEnemies() const;

	UFUNCTION(BlueprintPure, Category = "Encounter")
	bool IsEncounterComplete() const;

private:
	void ClearCurrentStage();
	void SpawnCampStage();
	void SpawnDungeonStage();
	void SpawnRewardStage();
	void SpawnCombatEncounter();
	void SpawnWorldLighting();
	void TeleportPartyToStage();
	void ClosePartyWindows();
	FVector GetStageSpawnLocation(int32 Slot) const;
	AEmberdeepPortal* SpawnPortal(const FVector& Location, EEmberdeepRunStage Destination, const FString& Label);
	void RegisterStageActor(AActor* Actor);

	UPROPERTY()
	TObjectPtr<AActor> CurrentEnvironment;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> StageActors;

	bool bTransitioningStage = false;
	int32 NextLootEntryId = 1;
};
