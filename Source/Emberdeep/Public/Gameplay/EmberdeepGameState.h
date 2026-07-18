#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "EmberdeepGameState.generated.h"

UCLASS()
class EMBERDEEP_API AEmberdeepGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	void SetEncounterState(int32 NewRemainingEnemies, bool bNewEncounterStarted);
	void SetRemainingEnemies(int32 NewRemainingEnemies);

	UFUNCTION(BlueprintPure, Category = "Encounter")
	int32 GetRemainingEnemies() const { return RemainingEnemies; }

	UFUNCTION(BlueprintPure, Category = "Encounter")
	bool IsEncounterComplete() const { return bEncounterStarted && RemainingEnemies <= 0; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_EncounterState();

	UPROPERTY(ReplicatedUsing = OnRep_EncounterState)
	int32 RemainingEnemies = 0;

	UPROPERTY(ReplicatedUsing = OnRep_EncounterState)
	bool bEncounterStarted = false;
};
