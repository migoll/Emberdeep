#include "Gameplay/EmberdeepGameState.h"

#include "Emberdeep.h"
#include "Net/UnrealNetwork.h"

void AEmberdeepGameState::SetEncounterState(int32 NewRemainingEnemies, bool bNewEncounterStarted)
{
	if (HasAuthority())
	{
		RemainingEnemies = FMath::Max(0, NewRemainingEnemies);
		bEncounterStarted = bNewEncounterStarted;
	}
}

void AEmberdeepGameState::SetRemainingEnemies(int32 NewRemainingEnemies)
{
	if (HasAuthority())
	{
		RemainingEnemies = FMath::Max(0, NewRemainingEnemies);
	}
}

void AEmberdeepGameState::SetRunStage(EEmberdeepRunStage NewStage)
{
	if (HasAuthority())
	{
		RunStage = NewStage;
	}
}

void AEmberdeepGameState::SetRunTier(int32 NewTier)
{
	if (HasAuthority())
	{
		RunTier = FMath::Max(1, NewTier);
	}
}

void AEmberdeepGameState::SetRewardAvailable(bool bNewRewardAvailable)
{
	if (HasAuthority())
	{
		bRewardAvailable = bNewRewardAvailable;
	}
}

void AEmberdeepGameState::OnRep_EncounterState()
{
	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_RUN Replicated Stage=%s Tier=%d Remaining=%d Reward=%s"),
		*EmberdeepRunStageName(RunStage), RunTier, RemainingEnemies,
		bRewardAvailable ? TEXT("true") : TEXT("false"));
}

void AEmberdeepGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEmberdeepGameState, RemainingEnemies);
	DOREPLIFETIME(AEmberdeepGameState, bEncounterStarted);
	DOREPLIFETIME(AEmberdeepGameState, RunStage);
	DOREPLIFETIME(AEmberdeepGameState, RunTier);
	DOREPLIFETIME(AEmberdeepGameState, bRewardAvailable);
}
