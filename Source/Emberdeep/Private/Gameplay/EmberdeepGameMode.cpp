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
#include "Gameplay/EmberdeepPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "UI/EmberdeepHUD.h"

AEmberdeepGameMode::AEmberdeepGameMode()
{
	DefaultPawnClass = AEmberdeepCharacter::StaticClass();
	PlayerControllerClass = AEmberdeepPlayerController::StaticClass();
	HUDClass = AEmberdeepHUD::StaticClass();
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

void AEmberdeepGameMode::Logout(AController* Exiting)
{
	const int32 PlayerId = Exiting && Exiting->PlayerState ? Exiting->PlayerState->GetPlayerId() : INDEX_NONE;
	Super::Logout(Exiting);
	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_NETWORK PlayerLeft Id=%d Players=%d"), PlayerId, GetNumPlayers());
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

void AEmberdeepGameMode::SpawnCombatEncounter()
{
	RemainingEnemies = 0;
	// The Phase 0A enemies steer directly toward players, so start them in the
	// camp's open southern lanes rather than behind blocking work props.
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
			++RemainingEnemies;
		}
	}

	bEncounterStarted = RemainingEnemies > 0;
	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_ENCOUNTER Started Enemies=%d"), RemainingEnemies);
}

void AEmberdeepGameMode::NotifyEnemyDefeated()
{
	RemainingEnemies = FMath::Max(0, RemainingEnemies - 1);
	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_ENCOUNTER EnemyDefeated Remaining=%d"), RemainingEnemies);
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
