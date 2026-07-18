#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EmberdeepGameMode.generated.h"

class AEmberdeepCampEnvironment;
class AEmberdeepCharacter;

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

	UFUNCTION(BlueprintPure, Category = "Encounter")
	int32 GetRemainingEnemies() const;

	UFUNCTION(BlueprintPure, Category = "Encounter")
	bool IsEncounterComplete() const;

private:
	void SpawnCampEnvironment();
	void SpawnCombatEncounter();

	UPROPERTY()
	TObjectPtr<AEmberdeepCampEnvironment> CampEnvironment;

};
