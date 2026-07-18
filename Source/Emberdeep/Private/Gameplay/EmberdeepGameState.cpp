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

void AEmberdeepGameState::OnRep_EncounterState()
{
	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_ENCOUNTER Replicated Remaining=%d Started=%s"),
		RemainingEnemies, bEncounterStarted ? TEXT("true") : TEXT("false"));
}

void AEmberdeepGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEmberdeepGameState, RemainingEnemies);
	DOREPLIFETIME(AEmberdeepGameState, bEncounterStarted);
}
