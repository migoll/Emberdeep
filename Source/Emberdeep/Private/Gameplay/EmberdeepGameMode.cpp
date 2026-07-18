#include "Gameplay/EmberdeepGameMode.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Emberdeep.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/PostProcessVolume.h"
#include "Environment/EmberdeepCampEnvironment.h"
#include "Gameplay/EmberdeepCharacter.h"
#include "Gameplay/EmberdeepEnemy.h"
#include "Gameplay/EmberdeepGameState.h"
#include "Gameplay/EmberdeepPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "UI/EmberdeepHUD.h"
#include "TimerManager.h"

AEmberdeepGameMode::AEmberdeepGameMode()
{
	DefaultPawnClass = AEmberdeepCharacter::StaticClass();
	PlayerControllerClass = AEmberdeepPlayerController::StaticClass();
	HUDClass = AEmberdeepHUD::StaticClass();
	GameStateClass = AEmberdeepGameState::StaticClass();
}

void AEmberdeepGameMode::StartPlay()
{
	Super::StartPlay();

	if (HasAuthority())
	{
		SpawnCampEnvironment();
		SpawnCombatEncounter();
	}
}

void AEmberdeepGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
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

	// Stable party slots place freshly connected players together on the camp's
	// open southern approach without stacking them on its single PlayerStart.
	static const FVector PartySpawnLocations[] =
	{
		FVector(-150.0f, -470.0f, 85.0f),
		FVector(150.0f, -470.0f, 85.0f),
		FVector(-150.0f, -350.0f, 85.0f),
		FVector(150.0f, -350.0f, 85.0f),
		FVector(0.0f, -410.0f, 85.0f)
	};
	const int32 PlayerId = NewPlayer->PlayerState ? NewPlayer->PlayerState->GetPlayerId() : 0;
	const int32 SpawnIndex = FMath::Abs(PlayerId) % UE_ARRAY_COUNT(PartySpawnLocations);
	NewPlayer->GetPawn()->SetActorLocation(
		PartySpawnLocations[SpawnIndex],
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
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
		UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_NETWORK PlayerRespawned Id=%d"),
			Controller->PlayerState ? Controller->PlayerState->GetPlayerId() : INDEX_NONE);
	});

	FTimerHandle RespawnTimer;
	GetWorldTimerManager().SetTimer(RespawnTimer, RespawnDelegate, 2.0f, false);
}

void AEmberdeepGameMode::SpawnCombatEncounter()
{
	// The Phase 0A enemies steer directly toward players, so start them in the
	// camp's open southern lanes rather than behind blocking work props.
	int32 SpawnedEnemyCount = 0;
	const TArray<FVector> EnemyLocations = {
		FVector(-450.0f, -525.0f, 110.0f),
		FVector(450.0f, -525.0f, 110.0f),
		FVector(400.0f, -175.0f, 110.0f)};

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	for (int32 EnemyIndex = 0; EnemyIndex < EnemyLocations.Num(); ++EnemyIndex)
	{
		if (AEmberdeepEnemy* Enemy = GetWorld()->SpawnActor<AEmberdeepEnemy>(
			AEmberdeepEnemy::StaticClass(),
			EnemyLocations[EnemyIndex],
			FRotator::ZeroRotator,
			SpawnParameters))
		{
			if (EnemyIndex == EnemyLocations.Num() - 1)
			{
				Enemy->ConfigureAsElite();
			}
			++SpawnedEnemyCount;
		}
	}

	if (AEmberdeepGameState* EmberdeepGameState = GetGameState<AEmberdeepGameState>())
	{
		EmberdeepGameState->SetEncounterState(SpawnedEnemyCount, SpawnedEnemyCount > 0);
	}
	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_ENCOUNTER Started Enemies=%d"), SpawnedEnemyCount);
}

void AEmberdeepGameMode::NotifyEnemyDefeated()
{
	if (AEmberdeepGameState* EmberdeepGameState = GetGameState<AEmberdeepGameState>())
	{
		EmberdeepGameState->SetRemainingEnemies(EmberdeepGameState->GetRemainingEnemies() - 1);
		UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_ENCOUNTER EnemyDefeated Remaining=%d"), EmberdeepGameState->GetRemainingEnemies());
	}
}

int32 AEmberdeepGameMode::GetRemainingEnemies() const
{
	const AEmberdeepGameState* EmberdeepGameState = GetGameState<AEmberdeepGameState>();
	return EmberdeepGameState ? EmberdeepGameState->GetRemainingEnemies() : 0;
}

bool AEmberdeepGameMode::IsEncounterComplete() const
{
	const AEmberdeepGameState* EmberdeepGameState = GetGameState<AEmberdeepGameState>();
	return EmberdeepGameState && EmberdeepGameState->IsEncounterComplete();
}

void AEmberdeepGameMode::SpawnCampEnvironment()
{
	FActorSpawnParameters CampSpawnParameters;
	CampSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CampEnvironment = GetWorld()->SpawnActor<AEmberdeepCampEnvironment>(
		AEmberdeepCampEnvironment::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		CampSpawnParameters);

	APostProcessVolume* ExposureVolume = GetWorld()->SpawnActor<APostProcessVolume>();
	if (ExposureVolume)
	{
		ExposureVolume->bUnbound = true;
		ExposureVolume->Settings.bOverride_AutoExposureMethod = true;
		ExposureVolume->Settings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
		ExposureVolume->Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
		ExposureVolume->Settings.AutoExposureApplyPhysicalCameraExposure = 0;
		ExposureVolume->Settings.bOverride_AutoExposureBias = true;
		ExposureVolume->Settings.AutoExposureBias = 0.5f;
	}

	ADirectionalLight* MoonLight = GetWorld()->SpawnActor<ADirectionalLight>(
		FVector::ZeroVector,
		FRotator(-52.0f, -38.0f, 0.0f));
	if (MoonLight)
	{
		MoonLight->SetMobility(EComponentMobility::Movable);
		MoonLight->GetLightComponent()->SetIntensity(11.0f);
		MoonLight->GetLightComponent()->SetLightColor(FLinearColor(0.48f, 0.56f, 0.72f));
	}

	APointLight* AmbientFill = GetWorld()->SpawnActor<APointLight>(FVector(0.0f, 0.0f, 900.0f), FRotator::ZeroRotator);
	if (AmbientFill)
	{
		AmbientFill->SetMobility(EComponentMobility::Movable);
		AmbientFill->GetLightComponent()->SetIntensity(18000.0f);
		AmbientFill->GetLightComponent()->SetLightColor(FLinearColor(0.38f, 0.46f, 0.62f));
		if (UPointLightComponent* FillComponent = Cast<UPointLightComponent>(AmbientFill->GetLightComponent()))
		{
			FillComponent->SetAttenuationRadius(5000.0f);
			FillComponent->SetCastShadows(false);
		}
	}

	UE_LOG(
		LogEmberdeep,
		Display,
		TEXT("EMBERDEEP_FOUNDATION CampGenerated Instances=%d CollisionProxies=%d"),
		CampEnvironment ? CampEnvironment->GetVoxelInstanceCount() : 0,
		CampEnvironment ? CampEnvironment->GetCollisionProxyCount() : 0);
}
