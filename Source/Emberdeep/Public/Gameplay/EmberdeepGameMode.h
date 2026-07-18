#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EmberdeepGameMode.generated.h"

class AEmberdeepCampEnvironment;

UCLASS()
class EMBERDEEP_API AEmberdeepGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AEmberdeepGameMode();
	virtual void StartPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	void NotifyEnemyDefeated();

	UFUNCTION(BlueprintPure, Category = "Encounter")
	int32 GetRemainingEnemies() const { return RemainingEnemies; }

	UFUNCTION(BlueprintPure, Category = "Encounter")
	bool IsEncounterComplete() const { return bEncounterStarted && RemainingEnemies <= 0; }

private:
	void SpawnCampEnvironment();
	void SpawnCombatEncounter();

	UPROPERTY()
	TObjectPtr<AEmberdeepCampEnvironment> CampEnvironment;

	int32 RemainingEnemies = 0;
	bool bEncounterStarted = false;
};
