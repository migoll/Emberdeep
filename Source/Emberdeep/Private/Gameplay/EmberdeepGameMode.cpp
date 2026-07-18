#include "Gameplay/EmberdeepGameMode.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Emberdeep.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMeshActor.h"
#include "Gameplay/EmberdeepCharacter.h"
#include "Gameplay/EmberdeepPlayerController.h"
#include "UObject/ConstructorHelpers.h"

AEmberdeepGameMode::AEmberdeepGameMode()
{
	DefaultPawnClass = AEmberdeepCharacter::StaticClass();
	PlayerControllerClass = AEmberdeepPlayerController::StaticClass();

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
	}
}

void AEmberdeepGameMode::SpawnBlockoutArena()
{
	SpawnBlock(FVector(0.0f, 0.0f, -35.0f), FVector(12.0f, 9.0f, 0.6f));

	SpawnBlock(FVector(0.0f, 890.0f, 155.0f), FVector(12.0f, 0.45f, 3.2f));
	SpawnBlock(FVector(0.0f, -890.0f, 155.0f), FVector(12.0f, 0.45f, 3.2f));
	SpawnBlock(FVector(1190.0f, 0.0f, 155.0f), FVector(0.45f, 8.5f, 3.2f));
	SpawnBlock(FVector(-1190.0f, 0.0f, 155.0f), FVector(0.45f, 8.5f, 3.2f));

	for (const FVector PillarLocation : {
		FVector(-650.0f, -470.0f, 120.0f), FVector(-650.0f, 470.0f, 120.0f),
		FVector(650.0f, -470.0f, 120.0f), FVector(650.0f, 470.0f, 120.0f)})
	{
		SpawnBlock(PillarLocation, FVector(0.65f, 0.65f, 2.5f));
	}

	ADirectionalLight* MoonLight = GetWorld()->SpawnActor<ADirectionalLight>(
		FVector::ZeroVector,
		FRotator(-52.0f, -38.0f, 0.0f));
	if (MoonLight)
	{
		MoonLight->GetLightComponent()->SetIntensity(2.2f);
		MoonLight->GetLightComponent()->SetLightColor(FLinearColor(0.30f, 0.38f, 0.52f));
		MoonLight->SetMobility(EComponentMobility::Movable);
	}

	for (const FVector TorchLocation : {FVector(-760.0f, 0.0f, 180.0f), FVector(760.0f, 0.0f, 180.0f)})
	{
		APointLight* Torch = GetWorld()->SpawnActor<APointLight>(TorchLocation, FRotator::ZeroRotator);
		if (Torch)
		{
			Torch->GetLightComponent()->SetIntensity(4200.0f);
			Torch->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.28f, 0.045f));
			if (UPointLightComponent* PointLightComponent = Cast<UPointLightComponent>(Torch->GetLightComponent()))
			{
				PointLightComponent->SetAttenuationRadius(620.0f);
			}
		}
	}

	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_FOUNDATION ArenaGenerated"));
}

void AEmberdeepGameMode::SpawnBlock(const FVector& Location, const FVector& Scale, const FRotator& Rotation)
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
	MeshComponent->SetStaticMesh(CubeMesh);
	MeshComponent->SetWorldScale3D(Scale);
	MeshComponent->SetMobility(EComponentMobility::Static);
	MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
	if (BlockMaterial)
	{
		MeshComponent->SetMaterial(0, BlockMaterial);
	}
}
