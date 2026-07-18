#include "Gameplay/EmberdeepGameMode.h"

#include "Emberdeep.h"
#include "EngineUtils.h"
#include "Environment/EmberdeepCampEnvironment.h"
#include "Environment/EmberdeepDungeonEnvironment.h"
#include "Environment/EmberdeepRewardRoomEnvironment.h"
#include "Gameplay/EmberdeepCharacter.h"
#include "Gameplay/EmberdeepEnemy.h"
#include "Gameplay/EmberdeepGameState.h"
#include "Gameplay/EmberdeepGoldPickup.h"
#include "Gameplay/EmberdeepItemTypes.h"
#include "Gameplay/EmberdeepLootContainer.h"
#include "Gameplay/EmberdeepPlayerController.h"
#include "Gameplay/EmberdeepPlayerState.h"
#include "Gameplay/EmberdeepPortal.h"
#include "GameFramework/PlayerState.h"
#include "UI/EmberdeepHUD.h"
#include "TimerManager.h"

AEmberdeepGameMode::AEmberdeepGameMode()
{
	DefaultPawnClass = AEmberdeepCharacter::StaticClass();
	PlayerControllerClass = AEmberdeepPlayerController::StaticClass();
	PlayerStateClass = AEmberdeepPlayerState::StaticClass();
	HUDClass = AEmberdeepHUD::StaticClass();
	GameStateClass = AEmberdeepGameState::StaticClass();
}

void AEmberdeepGameMode::StartPlay()
{
	Super::StartPlay();
	if (HasAuthority())
	{
		SpawnWorldLighting();
		EnterRunStage(EEmberdeepRunStage::Camp);
		if (FParse::Param(FCommandLine::Get(), TEXT("AutoRunSmoke")))
		{
			EnterRunStage(EEmberdeepRunStage::Dungeon);
		}
		else if (FParse::Param(FCommandLine::Get(), TEXT("AutoRewardSmoke")))
		{
			EnterRunStage(EEmberdeepRunStage::RewardRoom);
		}
	}
}

void AEmberdeepGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	AEmberdeepPlayerState* RunState = NewPlayer ? NewPlayer->GetPlayerState<AEmberdeepPlayerState>() : nullptr;
	if (RunState)
	{
		RunState->InitializeStarterLoadout();
	}
	if (AEmberdeepCharacter* Character = NewPlayer ? Cast<AEmberdeepCharacter>(NewPlayer->GetPawn()) : nullptr)
	{
		Character->ApplyEquipmentStats();
		if (RunState && FParse::Param(FCommandLine::Get(), TEXT("AutoLootSmoke")))
		{
			const int32 SmokeEntryId = NextLootEntryId++;
			TArray<FEmberdeepItemInstance> SmokeItems;
			SmokeItems.Add(FEmberdeepItemCatalog::MakeLootItem(TEXT("WardenCleaver"), SmokeEntryId, 2));
			if (AEmberdeepLootContainer* SmokeDrop = GetWorld()->SpawnActor<AEmberdeepLootContainer>(
				Character->GetActorLocation() + FVector(60.0f, 0.0f, 20.0f), FRotator::ZeroRotator))
			{
				SmokeDrop->ConfigureLoot(TEXT("Automated Loot Proof"), SmokeItems, false, false);
				RegisterStageActor(SmokeDrop);
				if (AEmberdeepPlayerController* EmberdeepController = Cast<AEmberdeepPlayerController>(NewPlayer))
				{
					EmberdeepController->ShowLootWindowFromServer(SmokeDrop, SmokeDrop->GetContainerTitle(), SmokeDrop->GetLootEntries());
					EmberdeepController->RequestTakeLoot(SmokeEntryId);
					RunState->EquipItem(SmokeEntryId);
					Character->ApplyEquipmentStats();
					UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_LOOT AutoLootEquipped Player=%d Inventory=%d Damage=%.1f"),
						RunState->GetPlayerId(), RunState->GetInventory().Num(), RunState->GetTotalDamageBonus());
				}
			}
		}
	}
	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_NETWORK PlayerJoined Id=%d Players=%d"),
		NewPlayer && NewPlayer->PlayerState ? NewPlayer->PlayerState->GetPlayerId() : INDEX_NONE,
		GetNumPlayers());
}

void AEmberdeepGameMode::PreLogin(
	const FString& Options,
	const FString& Address,
	const FUniqueNetIdRepl& UniqueId,
	FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (ErrorMessage.IsEmpty() && GetNumPlayers() >= 5)
	{
		ErrorMessage = TEXT("Emberdeep party is full (maximum five players).");
	}
}

void AEmberdeepGameMode::Logout(AController* Exiting)
{
	const int32 PlayerId = Exiting && Exiting->PlayerState ? Exiting->PlayerState->GetPlayerId() : INDEX_NONE;
	const int32 RemainingPlayers = FMath::Max(0, GetNumPlayers() - 1);
	Super::Logout(Exiting);
	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_NETWORK PlayerLeft Id=%d Players=%d"), PlayerId, RemainingPlayers);
}

void AEmberdeepGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);
	if (!NewPlayer || !NewPlayer->GetPawn())
	{
		return;
	}

	const int32 PlayerId = NewPlayer->PlayerState ? NewPlayer->PlayerState->GetPlayerId() : 0;
	const int32 SpawnIndex = FMath::Abs(PlayerId) % 5;
	NewPlayer->GetPawn()->SetActorLocation(GetStageSpawnLocation(SpawnIndex), false, nullptr, ETeleportType::TeleportPhysics);
	if (AEmberdeepCharacter* Character = Cast<AEmberdeepCharacter>(NewPlayer->GetPawn()))
	{
		Character->ApplyEquipmentStats();
	}
	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_NETWORK PlayerSpawned Id=%d Slot=%d"), PlayerId, SpawnIndex);
}

void AEmberdeepGameMode::SchedulePlayerRespawn(AEmberdeepCharacter* DefeatedCharacter)
{
	if (!HasAuthority() || !DefeatedCharacter)
	{
		return;
	}

	TWeakObjectPtr<AController> Controller = DefeatedCharacter->GetController();
	TWeakObjectPtr<AEmberdeepCharacter> Character = DefeatedCharacter;
	FTimerDelegate RespawnDelegate = FTimerDelegate::CreateWeakLambda(this, [this, Controller, Character]()
	{
		if (!Controller.IsValid())
		{
			return;
		}
		if (Character.IsValid())
		{
			Character->Destroy();
		}
		RestartPlayer(Controller.Get());
		const AEmberdeepPlayerState* RunState = Controller->GetPlayerState<AEmberdeepPlayerState>();
		UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_NETWORK PlayerRespawned Id=%d Inventory=%d Level=%d Gold=%d"),
			Controller->PlayerState ? Controller->PlayerState->GetPlayerId() : INDEX_NONE,
			RunState ? RunState->GetInventory().Num() : 0,
			RunState ? RunState->GetCharacterLevel() : 1,
			RunState ? RunState->GetGold() : 0);
	});
	FTimerHandle RespawnTimer;
	GetWorldTimerManager().SetTimer(RespawnTimer, RespawnDelegate, 2.0f, false);
}

void AEmberdeepGameMode::EnterRunStage(EEmberdeepRunStage NewStage)
{
	if (!HasAuthority() || bTransitioningStage)
	{
		return;
	}
	bTransitioningStage = true;

	AEmberdeepGameState* RunGameState = GetGameState<AEmberdeepGameState>();
	const EEmberdeepRunStage PreviousStage = RunGameState ? RunGameState->GetRunStage() : EEmberdeepRunStage::Camp;
	if (RunGameState && PreviousStage == EEmberdeepRunStage::RewardRoom && NewStage == EEmberdeepRunStage::Camp)
	{
		RunGameState->SetRunTier(RunGameState->GetRunTier() + 1);
	}

	ClosePartyWindows();
	ClearCurrentStage();
	if (RunGameState)
	{
		RunGameState->SetRunStage(NewStage);
		RunGameState->SetRewardAvailable(false);
		RunGameState->SetEncounterState(0, false);
	}

	switch (NewStage)
	{
	case EEmberdeepRunStage::Dungeon:
		SpawnDungeonStage();
		break;
	case EEmberdeepRunStage::RewardRoom:
		SpawnRewardStage();
		break;
	default:
		SpawnCampStage();
		break;
	}

	TeleportPartyToStage();
	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_RUN StageEntered Stage=%s Tier=%d"),
		*EmberdeepRunStageName(NewStage), RunGameState ? RunGameState->GetRunTier() : 1);
	bTransitioningStage = false;
}

void AEmberdeepGameMode::ClearCurrentStage()
{
	for (AActor* Actor : StageActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	StageActors.Empty();
	if (IsValid(CurrentEnvironment))
	{
		CurrentEnvironment->Destroy();
	}
	CurrentEnvironment = nullptr;

	// The Entry map can still contain prototype enemies placed before the staged
	// run loop existed. Treat enemies as disposable stage content so the camp is
	// always a genuinely safe hub, even when opening an older saved map.
	for (TActorIterator<AEmberdeepEnemy> EnemyIt(GetWorld()); EnemyIt; ++EnemyIt)
	{
		EnemyIt->Destroy();
	}

	for (TActorIterator<AEmberdeepGoldPickup> PickupIt(GetWorld()); PickupIt; ++PickupIt)
	{
		PickupIt->Destroy();
	}
}

void AEmberdeepGameMode::SpawnCampStage()
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CurrentEnvironment = GetWorld()->SpawnActor<AEmberdeepCampEnvironment>(
		AEmberdeepCampEnvironment::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	SpawnPortal(FVector(0.0f, -245.0f, 20.0f), EEmberdeepRunStage::Dungeon, TEXT("Enter the Ashen Crypt"));
}

void AEmberdeepGameMode::SpawnDungeonStage()
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CurrentEnvironment = GetWorld()->SpawnActor<AEmberdeepDungeonEnvironment>(
		AEmberdeepDungeonEnvironment::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);

	TArray<FEmberdeepItemInstance> CacheItems;
	const int32 Tier = GetGameState<AEmberdeepGameState>() ? GetGameState<AEmberdeepGameState>()->GetRunTier() : 1;
	CacheItems.Add(FEmberdeepItemCatalog::MakeRandomDrop(NextLootEntryId++, Tier, false));
	CacheItems.Add(FEmberdeepItemCatalog::MakeRandomDrop(NextLootEntryId++, Tier, false));
	if (AEmberdeepLootContainer* Cache = GetWorld()->SpawnActor<AEmberdeepLootContainer>(FVector(-585.0f, 325.0f, 20.0f), FRotator::ZeroRotator))
	{
		Cache->ConfigureLoot(TEXT("Crypt Cache"), CacheItems, true, false);
		RegisterStageActor(Cache);
	}
	SpawnCombatEncounter();
}

void AEmberdeepGameMode::SpawnRewardStage()
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CurrentEnvironment = GetWorld()->SpawnActor<AEmberdeepRewardRoomEnvironment>(
		AEmberdeepRewardRoomEnvironment::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);

	const int32 Tier = GetGameState<AEmberdeepGameState>() ? GetGameState<AEmberdeepGameState>()->GetRunTier() : 1;
	TArray<FEmberdeepItemInstance> RewardItems;
	RewardItems.Add(FEmberdeepItemCatalog::MakeLegendaryReward(NextLootEntryId++, Tier));
	if (AEmberdeepLootContainer* Chest = GetWorld()->SpawnActor<AEmberdeepLootContainer>(FVector(0.0f, 105.0f, 20.0f), FRotator::ZeroRotator))
	{
		Chest->ConfigureLoot(TEXT("Cinder Vault Reliquary"), RewardItems, true, true);
		RegisterStageActor(Chest);
	}
	SpawnPortal(FVector(0.0f, 330.0f, 20.0f), EEmberdeepRunStage::Camp, TEXT("Return to Camp and descend deeper"));
}

void AEmberdeepGameMode::SpawnCombatEncounter()
{
	int32 SpawnedEnemyCount = 0;
	const TArray<FVector> EnemyLocations = {
		FVector(-440.0f, 25.0f, 110.0f),
		FVector(440.0f, 25.0f, 110.0f),
		FVector(0.0f, 330.0f, 110.0f)};
	const int32 Tier = GetGameState<AEmberdeepGameState>() ? GetGameState<AEmberdeepGameState>()->GetRunTier() : 1;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	for (int32 EnemyIndex = 0; EnemyIndex < EnemyLocations.Num(); ++EnemyIndex)
	{
		if (AEmberdeepEnemy* Enemy = GetWorld()->SpawnActor<AEmberdeepEnemy>(
			AEmberdeepEnemy::StaticClass(), EnemyLocations[EnemyIndex], FRotator::ZeroRotator, SpawnParameters))
		{
			Enemy->ConfigureForRun(Tier, EnemyIndex == EnemyLocations.Num() - 1);
			RegisterStageActor(Enemy);
			++SpawnedEnemyCount;
		}
	}
	if (AEmberdeepGameState* RunGameState = GetGameState<AEmberdeepGameState>())
	{
		RunGameState->SetEncounterState(SpawnedEnemyCount, SpawnedEnemyCount > 0);
	}
	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_ENCOUNTER Started Tier=%d Enemies=%d"), Tier, SpawnedEnemyCount);
}

void AEmberdeepGameMode::SpawnLootForEnemy(const FVector& Location, bool bElite)
{
	const int32 Tier = GetGameState<AEmberdeepGameState>() ? GetGameState<AEmberdeepGameState>()->GetRunTier() : 1;
	TArray<FEmberdeepItemInstance> Entries;
	Entries.Add(FEmberdeepItemCatalog::MakeRandomDrop(NextLootEntryId++, Tier, bElite));
	if (bElite)
	{
		Entries.Add(FEmberdeepItemCatalog::MakeRandomDrop(NextLootEntryId++, Tier + 1, true));
	}
	if (AEmberdeepLootContainer* Drop = GetWorld()->SpawnActor<AEmberdeepLootContainer>(Location, FRotator::ZeroRotator))
	{
		Drop->ConfigureLoot(bElite ? TEXT("Bone Warden Spoils") : TEXT("Fallen Enemy"), Entries, false, false);
		RegisterStageActor(Drop);
	}
}

void AEmberdeepGameMode::NotifyEnemyDefeated()
{
	if (AEmberdeepGameState* RunGameState = GetGameState<AEmberdeepGameState>())
	{
		RunGameState->SetRemainingEnemies(RunGameState->GetRemainingEnemies() - 1);
		const int32 Remaining = RunGameState->GetRemainingEnemies();
		UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_ENCOUNTER EnemyDefeated Remaining=%d"), Remaining);
		if (Remaining <= 0 && RunGameState->GetRunStage() == EEmberdeepRunStage::Dungeon && !RunGameState->IsRewardAvailable())
		{
			RunGameState->SetRewardAvailable(true);
			SpawnPortal(FVector(0.0f, 505.0f, 20.0f), EEmberdeepRunStage::RewardRoom, TEXT("Enter the Cinder Vault"));
			UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_RUN RewardRoomUnlocked"));
		}
	}
}

int32 AEmberdeepGameMode::GetRemainingEnemies() const
{
	const AEmberdeepGameState* RunGameState = GetGameState<AEmberdeepGameState>();
	return RunGameState ? RunGameState->GetRemainingEnemies() : 0;
}

bool AEmberdeepGameMode::IsEncounterComplete() const
{
	const AEmberdeepGameState* RunGameState = GetGameState<AEmberdeepGameState>();
	return RunGameState && RunGameState->IsEncounterComplete();
}

void AEmberdeepGameMode::TeleportPartyToStage()
{
	int32 Slot = 0;
	for (FConstPlayerControllerIterator ControllerIt = GetWorld()->GetPlayerControllerIterator(); ControllerIt; ++ControllerIt)
	{
		APlayerController* Controller = ControllerIt->Get();
		if (Controller && Controller->GetPawn())
		{
			Controller->GetPawn()->SetActorLocation(GetStageSpawnLocation(Slot++), false, nullptr, ETeleportType::TeleportPhysics);
		}
	}
}

void AEmberdeepGameMode::ClosePartyWindows()
{
	for (FConstPlayerControllerIterator ControllerIt = GetWorld()->GetPlayerControllerIterator(); ControllerIt; ++ControllerIt)
	{
		if (AEmberdeepPlayerController* Controller = Cast<AEmberdeepPlayerController>(ControllerIt->Get()))
		{
			Controller->CloseGameplayWindowsFromServer();
		}
	}
}

FVector AEmberdeepGameMode::GetStageSpawnLocation(int32 Slot) const
{
	static const FVector PartyOffsets[] = {
		FVector(-150.0f, 0.0f, 0.0f), FVector(150.0f, 0.0f, 0.0f),
		FVector(-150.0f, 120.0f, 0.0f), FVector(150.0f, 120.0f, 0.0f), FVector::ZeroVector};
	const AEmberdeepGameState* RunGameState = GetGameState<AEmberdeepGameState>();
	const EEmberdeepRunStage Stage = RunGameState ? RunGameState->GetRunStage() : EEmberdeepRunStage::Camp;
	const FVector Base = Stage == EEmberdeepRunStage::Camp
		? FVector(0.0f, -470.0f, 85.0f)
		: Stage == EEmberdeepRunStage::Dungeon
			? FVector(0.0f, -430.0f, 85.0f)
			: FVector(0.0f, -275.0f, 85.0f);
	return Base + PartyOffsets[FMath::Abs(Slot) % UE_ARRAY_COUNT(PartyOffsets)];
}

AEmberdeepPortal* AEmberdeepGameMode::SpawnPortal(
	const FVector& Location,
	EEmberdeepRunStage Destination,
	const FString& Label)
{
	AEmberdeepPortal* Portal = GetWorld()->SpawnActor<AEmberdeepPortal>(Location, FRotator::ZeroRotator);
	if (Portal)
	{
		Portal->Configure(Destination, Label);
		RegisterStageActor(Portal);
	}
	return Portal;
}

void AEmberdeepGameMode::RegisterStageActor(AActor* Actor)
{
	if (Actor)
	{
		StageActors.AddUnique(Actor);
	}
}

void AEmberdeepGameMode::SpawnWorldLighting()
{
	// The visual baseline is installed by each local HUD. GameMode only exists on
	// authority, so owning presentation here would make remote clients diverge.
}
