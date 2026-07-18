#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Gameplay/EmberdeepRunTypes.h"
#include "EmberdeepGameState.generated.h"

UCLASS()
class EMBERDEEP_API AEmberdeepGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	void SetEncounterState(int32 NewRemainingEnemies, bool bNewEncounterStarted);
	void SetRemainingEnemies(int32 NewRemainingEnemies);
	void SetRunStage(EEmberdeepRunStage NewStage);
	void SetRunTier(int32 NewTier);
	void SetRewardAvailable(bool bNewRewardAvailable);

	UFUNCTION(BlueprintPure, Category = "Encounter")
	int32 GetRemainingEnemies() const { return RemainingEnemies; }

	UFUNCTION(BlueprintPure, Category = "Encounter")
	bool IsEncounterComplete() const { return bEncounterStarted && RemainingEnemies <= 0; }

	UFUNCTION(BlueprintPure, Category = "Run")
	EEmberdeepRunStage GetRunStage() const { return RunStage; }

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetRunTier() const { return RunTier; }

	UFUNCTION(BlueprintPure, Category = "Run")
	bool IsRewardAvailable() const { return bRewardAvailable; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_EncounterState();

	UPROPERTY(ReplicatedUsing = OnRep_EncounterState)
	int32 RemainingEnemies = 0;

	UPROPERTY(ReplicatedUsing = OnRep_EncounterState)
	bool bEncounterStarted = false;

	UPROPERTY(ReplicatedUsing = OnRep_EncounterState)
	EEmberdeepRunStage RunStage = EEmberdeepRunStage::Camp;

	UPROPERTY(ReplicatedUsing = OnRep_EncounterState)
	int32 RunTier = 1;

	UPROPERTY(ReplicatedUsing = OnRep_EncounterState)
	bool bRewardAvailable = false;
};
