#include "Gameplay/EmberdeepGameMode.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Emberdeep.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMeshActor.h"
#include "Gameplay/EmberdeepCharacter.h"
#include "Gameplay/EmberdeepEnemy.h"
#include "Gameplay/EmberdeepPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UI/EmberdeepHUD.h"
#include "UObject/ConstructorHelpers.h"

AEmberdeepGameMode::AEmberdeepGameMode()
{
	DefaultPawnClass = AEmberdeepCharacter::StaticClass();
	PlayerControllerClass = AEmberdeepPlayerController::StaticClass();
	HUDClass = AEmberdeepHUD::StaticClass();

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	CubeMesh = CubeAsset.Object;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BlockMaterial = MaterialAsset.Object;
}

void AEmberdeepGameMode::StartPlay()
{
	Super::StartPlay();

	if (HasAuthority())
	{
		SpawnBlockoutArena();
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

	// Stable party slots prevent freshly connected players from spawning inside
	// each other while the blockout map still has only one engine PlayerStart.
	static const FVector PartySpawnLocations[] =
	{
		FVector(-180.0f, -120.0f, 75.0f),
		FVector(180.0f, -120.0f, 75.0f),
		FVector(-180.0f, 120.0f, 75.0f),
		FVector(180.0f, 120.0f, 75.0f),
		FVector(0.0f, 0.0f, 75.0f)
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
	const TArray<FVector> EnemyLocations = {
		FVector(-620.0f, -360.0f, 110.0f),
		FVector(640.0f, 330.0f, 110.0f),
		FVector(0.0f, 500.0f, 110.0f)};

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

void AEmberdeepGameMode::SpawnBlockoutArena()
{
	const FLinearColor FloorColor(0.16f, 0.13f, 0.10f);
	const FLinearColor WallColor(0.22f, 0.235f, 0.26f);
	const FLinearColor PillarColor(0.30f, 0.24f, 0.16f);

	SpawnBlock(
		FVector(0.0f, 0.0f, -105.0f),
		FVector(60.0f, 60.0f, 0.25f),
		FLinearColor(0.025f, 0.03f, 0.04f),
		FRotator::ZeroRotator,
		false);
	SpawnBlock(FVector(0.0f, 0.0f, -35.0f), FVector(18.0f, 14.0f, 0.6f), FloorColor);

	// Low curbs share the floor's exact outer dimensions. Tall replacement walls
	// can later sit on these footprints without changing gameplay collision.
	// The engine cube is 100 units wide, so this floor's half-extents are
	// 900 x 700. Each curb ends exactly on that outer edge.
	SpawnBlock(FVector(0.0f, 680.0f, 32.5f), FVector(18.0f, 0.40f, 0.75f), WallColor);
	SpawnBlock(FVector(0.0f, -680.0f, 32.5f), FVector(18.0f, 0.40f, 0.75f), WallColor);
	SpawnBlock(FVector(880.0f, 0.0f, 32.5f), FVector(0.40f, 13.2f, 0.75f), WallColor);
	SpawnBlock(FVector(-880.0f, 0.0f, 32.5f), FVector(0.40f, 13.2f, 0.75f), WallColor);

	// Gameplay boundaries do not depend on the current wall art or imported camp kit.
	SpawnBoundary(FVector(0.0f, 680.0f, 95.0f), FVector(900.0f, 20.0f, 140.0f));
	SpawnBoundary(FVector(0.0f, -680.0f, 95.0f), FVector(900.0f, 20.0f, 140.0f));
	SpawnBoundary(FVector(880.0f, 0.0f, 95.0f), FVector(20.0f, 660.0f, 140.0f));
	SpawnBoundary(FVector(-880.0f, 0.0f, 95.0f), FVector(20.0f, 660.0f, 140.0f));

	for (const FVector PillarLocation : {
		FVector(-867.5f, -667.5f, 120.0f), FVector(-867.5f, 667.5f, 120.0f),
		FVector(867.5f, -667.5f, 120.0f), FVector(867.5f, 667.5f, 120.0f)})
	{
		SpawnBlock(PillarLocation, FVector(0.65f, 0.65f, 2.5f), PillarColor);
	}

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
		AmbientFill->GetLightComponent()->SetIntensity(50000.0f);
		AmbientFill->GetLightComponent()->SetLightColor(FLinearColor(0.38f, 0.46f, 0.62f));
		if (UPointLightComponent* FillComponent = Cast<UPointLightComponent>(AmbientFill->GetLightComponent()))
		{
			FillComponent->SetAttenuationRadius(5000.0f);
			FillComponent->SetCastShadows(false);
		}
	}

	for (const FVector TorchLocation : {
		FVector(-700.0f, -500.0f, 210.0f), FVector(-700.0f, 500.0f, 210.0f),
		FVector(700.0f, -500.0f, 210.0f), FVector(700.0f, 500.0f, 210.0f)})
	{
		APointLight* Torch = GetWorld()->SpawnActor<APointLight>(TorchLocation, FRotator::ZeroRotator);
		if (Torch)
		{
			Torch->SetMobility(EComponentMobility::Movable);
			Torch->GetLightComponent()->SetIntensity(24000.0f);
			Torch->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.22f, 0.035f));
			if (UPointLightComponent* PointLightComponent = Cast<UPointLightComponent>(Torch->GetLightComponent()))
			{
				PointLightComponent->SetAttenuationRadius(1100.0f);
			}
		}
	}

	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_FOUNDATION ArenaGenerated"));
}

void AEmberdeepGameMode::SpawnBoundary(const FVector& Location, const FVector& Extent)
{
	// A bare AActor has no root during SpawnActor, so its spawn transform cannot
	// be applied to the component that we create afterward. Build the root first,
	// register it, then explicitly place the finished boundary actor.
	AActor* BoundaryActor = GetWorld()->SpawnActor<AActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!BoundaryActor)
	{
		return;
	}

	UBoxComponent* Boundary = NewObject<UBoxComponent>(BoundaryActor, TEXT("ArenaBoundary"));
	BoundaryActor->SetRootComponent(Boundary);
	Boundary->SetBoxExtent(Extent);
	Boundary->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Boundary->SetCollisionObjectType(ECC_WorldStatic);
	Boundary->SetCollisionResponseToAllChannels(ECR_Block);
	Boundary->SetHiddenInGame(true);
	Boundary->RegisterComponent();
	BoundaryActor->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
}

void AEmberdeepGameMode::SpawnBlock(
	const FVector& Location,
	const FVector& Scale,
	const FLinearColor& Color,
	const FRotator& Rotation,
	bool bEnableCollision)
{
	if (!CubeMesh)
	{
		return;
	}

	AStaticMeshActor* Block = GetWorld()->SpawnActor<AStaticMeshActor>(Location, Rotation);
	if (!Block)
	{
		return;
	}

	UStaticMeshComponent* MeshComponent = Block->GetStaticMeshComponent();
	// Runtime-generated actors start with Static mobility. Unreal rejects mesh
	// assignment on a registered Static component, so make it movable first.
	MeshComponent->SetMobility(EComponentMobility::Movable);
	MeshComponent->SetStaticMesh(CubeMesh);
	MeshComponent->SetWorldScale3D(Scale);
	MeshComponent->SetCollisionEnabled(bEnableCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	if (bEnableCollision)
	{
		MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
	}
	if (BlockMaterial)
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BlockMaterial, Block);
		Material->SetVectorParameterValue(TEXT("Color"), Color);
		MeshComponent->SetMaterial(0, Material);
	}
}
