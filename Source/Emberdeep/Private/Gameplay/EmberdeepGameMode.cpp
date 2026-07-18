#include "Gameplay/EmberdeepGameMode.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Emberdeep.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMeshActor.h"
#include "Gameplay/EmberdeepCharacter.h"
#include "Gameplay/EmberdeepPlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
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
	const FLinearColor FloorColor(0.13f, 0.105f, 0.085f);
	const FLinearColor WallColor(0.16f, 0.175f, 0.19f);
	const FLinearColor PillarColor(0.22f, 0.19f, 0.15f);

	SpawnBlock(FVector(0.0f, 0.0f, -35.0f), FVector(12.0f, 9.0f, 0.6f), FloorColor);

	SpawnBlock(FVector(0.0f, 890.0f, 75.0f), FVector(12.0f, 0.45f, 1.6f), WallColor);
	SpawnBlock(FVector(0.0f, -890.0f, 75.0f), FVector(12.0f, 0.45f, 1.6f), WallColor);
	SpawnBlock(FVector(1190.0f, 0.0f, 75.0f), FVector(0.45f, 8.5f, 1.6f), WallColor);
	SpawnBlock(FVector(-1190.0f, 0.0f, 75.0f), FVector(0.45f, 8.5f, 1.6f), WallColor);

	for (const FVector PillarLocation : {
		FVector(-650.0f, -470.0f, 120.0f), FVector(-650.0f, 470.0f, 120.0f),
		FVector(650.0f, -470.0f, 120.0f), FVector(650.0f, 470.0f, 120.0f)})
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
		ExposureVolume->Settings.AutoExposureBias = 0.0f;
	}

	ADirectionalLight* MoonLight = GetWorld()->SpawnActor<ADirectionalLight>(
		FVector::ZeroVector,
		FRotator(-52.0f, -38.0f, 0.0f));
	if (MoonLight)
	{
		MoonLight->SetMobility(EComponentMobility::Movable);
		MoonLight->GetLightComponent()->SetIntensity(8.0f);
		MoonLight->GetLightComponent()->SetLightColor(FLinearColor(0.38f, 0.48f, 0.70f));
	}

	APointLight* AmbientFill = GetWorld()->SpawnActor<APointLight>(FVector(0.0f, 0.0f, 900.0f), FRotator::ZeroRotator);
	if (AmbientFill)
	{
		AmbientFill->SetMobility(EComponentMobility::Movable);
		AmbientFill->GetLightComponent()->SetIntensity(32000.0f);
		AmbientFill->GetLightComponent()->SetLightColor(FLinearColor(0.30f, 0.40f, 0.58f));
		if (UPointLightComponent* FillComponent = Cast<UPointLightComponent>(AmbientFill->GetLightComponent()))
		{
			FillComponent->SetAttenuationRadius(3200.0f);
			FillComponent->SetCastShadows(false);
		}
	}

	for (const FVector TorchLocation : {FVector(-760.0f, 0.0f, 180.0f), FVector(760.0f, 0.0f, 180.0f)})
	{
		APointLight* Torch = GetWorld()->SpawnActor<APointLight>(TorchLocation, FRotator::ZeroRotator);
		if (Torch)
		{
			Torch->SetMobility(EComponentMobility::Movable);
			Torch->GetLightComponent()->SetIntensity(18000.0f);
			Torch->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.22f, 0.035f));
			if (UPointLightComponent* PointLightComponent = Cast<UPointLightComponent>(Torch->GetLightComponent()))
			{
				PointLightComponent->SetAttenuationRadius(950.0f);
			}
		}
	}

	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_FOUNDATION ArenaGenerated"));
}

void AEmberdeepGameMode::SpawnBlock(
	const FVector& Location,
	const FVector& Scale,
	const FLinearColor& Color,
	const FRotator& Rotation)
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
	MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
	if (BlockMaterial)
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BlockMaterial, Block);
		Material->SetVectorParameterValue(TEXT("Color"), Color);
		MeshComponent->SetMaterial(0, Material);
	}
}
