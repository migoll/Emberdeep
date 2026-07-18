#include "Environment/EmberdeepCampEnvironment.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	#include "CampVoxelData.inl"

	struct FCampPaletteDefinition
	{
		ECampPalette Palette;
		const TCHAR* Name;
		FLinearColor Color;
	};

	const FCampPaletteDefinition GCampPaletteDefinitions[] =
	{
		{ ECampPalette::GroundDark, TEXT("GroundDark"), FLinearColor::FromSRGBColor(FColor(23, 25, 29)) },
		{ ECampPalette::GroundMid, TEXT("GroundMid"), FLinearColor::FromSRGBColor(FColor(40, 40, 42)) },
		{ ECampPalette::StoneDark, TEXT("StoneDark"), FLinearColor::FromSRGBColor(FColor(52, 54, 58)) },
		{ ECampPalette::Stone, TEXT("Stone"), FLinearColor::FromSRGBColor(FColor(86, 88, 91)) },
		{ ECampPalette::WoodDark, TEXT("WoodDark"), FLinearColor::FromSRGBColor(FColor(53, 37, 28)) },
		{ ECampPalette::Wood, TEXT("Wood"), FLinearColor::FromSRGBColor(FColor(90, 58, 34)) },
		{ ECampPalette::WoodLight, TEXT("WoodLight"), FLinearColor::FromSRGBColor(FColor(138, 87, 40)) },
		{ ECampPalette::CanvasDark, TEXT("CanvasDark"), FLinearColor::FromSRGBColor(FColor(40, 52, 59)) },
		{ ECampPalette::Canvas, TEXT("Canvas"), FLinearColor::FromSRGBColor(FColor(75, 90, 94)) },
		{ ECampPalette::Iron, TEXT("Iron"), FLinearColor::FromSRGBColor(FColor(115, 119, 123)) },
		{ ECampPalette::Frost, TEXT("Frost"), FLinearColor::FromSRGBColor(FColor(170, 183, 193)) },
		{ ECampPalette::Ember, TEXT("Ember"), FLinearColor::FromSRGBColor(FColor(180, 71, 22)) },
		{ ECampPalette::Fire, TEXT("Fire"), FLinearColor::FromSRGBColor(FColor(242, 161, 58)) }
	};

	const FCampCollisionBox GCampStructuralCollisionBoxes[] =
	{
		{ FVector(0.0f, 0.0f, -25.0f), FVector(1800.0f, 1400.0f, 60.0f), FRotator::ZeroRotator },
		{ FVector(0.0f, 680.0f, 95.0f), FVector(1800.0f, 40.0f, 280.0f), FRotator::ZeroRotator },
		{ FVector(0.0f, -680.0f, 95.0f), FVector(1800.0f, 40.0f, 280.0f), FRotator::ZeroRotator },
		{ FVector(880.0f, 0.0f, 95.0f), FVector(40.0f, 1320.0f, 280.0f), FRotator::ZeroRotator },
		{ FVector(-880.0f, 0.0f, 95.0f), FVector(40.0f, 1320.0f, 280.0f), FRotator::ZeroRotator }
	};
}

AEmberdeepCampEnvironment::AEmberdeepCampEnvironment()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.0f / 12.0f;
	SetReplicates(true);
	SetReplicateMovement(false);
	bAlwaysRelevant = true;

	CampRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CampRoot"));
	CampRoot->SetMobility(EComponentMobility::Static);
	SetRootComponent(CampRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseBlockMaterial = MaterialAsset.Object;

	for (const FCampPaletteDefinition& PaletteDefinition : GCampPaletteDefinitions)
	{
		const FName ComponentName(*FString::Printf(TEXT("Camp%sInstances"), PaletteDefinition.Name));
		UInstancedStaticMeshComponent* PaletteMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(ComponentName);
		PaletteMesh->SetupAttachment(CampRoot);
		PaletteMesh->SetMobility(EComponentMobility::Static);
		PaletteMesh->SetStaticMesh(CubeAsset.Object);
		PaletteMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PaletteMesh->SetGenerateOverlapEvents(false);
		PaletteMesh->SetCanEverAffectNavigation(false);
		PaletteMeshes.Add(PaletteMesh);
	}

	TArray<TArray<FTransform>> InstanceTransformsByPalette;
	InstanceTransformsByPalette.SetNum(PaletteMeshes.Num());
	for (const FCampVoxelBlock& Block : GCampVoxelBlocks)
	{
		const int32 PaletteIndex = static_cast<int32>(Block.Palette);
		if (InstanceTransformsByPalette.IsValidIndex(PaletteIndex))
		{
			InstanceTransformsByPalette[PaletteIndex].Add(FTransform(Block.Rotation, Block.Center, Block.Size / 100.0f));
		}
	}

	// Keep the camera's off-camp view dark when it follows a character near the
	// perimeter. This sits well below the playable floor and has no collision.
	const int32 BackdropPaletteIndex = static_cast<int32>(ECampPalette::GroundDark);
	if (InstanceTransformsByPalette.IsValidIndex(BackdropPaletteIndex))
	{
		InstanceTransformsByPalette[BackdropPaletteIndex].Add(
			FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, -105.0f), FVector(60.0f, 60.0f, 0.25f)));
	}
	for (int32 PaletteIndex = 0; PaletteIndex < PaletteMeshes.Num(); ++PaletteIndex)
	{
		PaletteMeshes[PaletteIndex]->AddInstances(
			InstanceTransformsByPalette[PaletteIndex],
			false,
			false,
			false);
	}

	for (int32 CollisionIndex = 0; CollisionIndex < UE_ARRAY_COUNT(GCampStructuralCollisionBoxes); ++CollisionIndex)
	{
		const FCampCollisionBox& CollisionData = GCampStructuralCollisionBoxes[CollisionIndex];
		const FName ComponentName(*FString::Printf(TEXT("CampStructuralCollision%02d"), CollisionIndex));
		UBoxComponent* Collision = CreateDefaultSubobject<UBoxComponent>(ComponentName);
		Collision->SetupAttachment(CampRoot);
		Collision->SetMobility(EComponentMobility::Static);
		Collision->SetRelativeLocation(CollisionData.Center);
		Collision->SetRelativeRotation(CollisionData.Rotation);
		Collision->SetBoxExtent(CollisionData.Size * 0.5f);
		Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Collision->SetCollisionObjectType(ECC_WorldStatic);
		Collision->SetCollisionResponseToAllChannels(ECR_Block);
		Collision->SetGenerateOverlapEvents(false);
		Collision->SetHiddenInGame(true);
		CollisionProxies.Add(Collision);
	}

	for (int32 CollisionIndex = 0; CollisionIndex < UE_ARRAY_COUNT(GCampCollisionBoxes); ++CollisionIndex)
	{
		const FCampCollisionBox& CollisionData = GCampCollisionBoxes[CollisionIndex];
		const FName ComponentName(*FString::Printf(TEXT("CampPropCollision%02d"), CollisionIndex));
		UBoxComponent* Collision = CreateDefaultSubobject<UBoxComponent>(ComponentName);
		Collision->SetupAttachment(CampRoot);
		Collision->SetMobility(EComponentMobility::Static);
		Collision->SetRelativeLocation(CollisionData.Center);
		Collision->SetRelativeRotation(CollisionData.Rotation);
		Collision->SetBoxExtent(CollisionData.Size * 0.5f);
		Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Collision->SetCollisionObjectType(ECC_WorldStatic);
		Collision->SetCollisionResponseToAllChannels(ECR_Block);
		Collision->SetGenerateOverlapEvents(false);
		Collision->SetHiddenInGame(true);
		CollisionProxies.Add(Collision);
	}

	CampfireLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CampfireLight"));
	CampfireLight->SetupAttachment(CampRoot);
	CampfireLight->SetRelativeLocation(GCampPrimaryFireLocation);
	CampfireLight->SetMobility(EComponentMobility::Movable);
	CampfireLight->SetIntensity(36000.0f);
	CampfireLight->SetLightColor(FLinearColor(1.0f, 0.19f, 0.025f));
	CampfireLight->SetAttenuationRadius(920.0f);
	CampfireLight->SetCastShadows(true);

	WagonLanternLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("WagonLanternLight"));
	WagonLanternLight->SetupAttachment(CampRoot);
	WagonLanternLight->SetRelativeLocation(GCampWagonLanternLocation);
	WagonLanternLight->SetMobility(EComponentMobility::Movable);
	WagonLanternLight->SetIntensity(9500.0f);
	WagonLanternLight->SetLightColor(FLinearColor(1.0f, 0.24f, 0.045f));
	WagonLanternLight->SetAttenuationRadius(520.0f);
	WagonLanternLight->SetCastShadows(false);

	ShelterLanternLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ShelterLanternLight"));
	ShelterLanternLight->SetupAttachment(CampRoot);
	ShelterLanternLight->SetRelativeLocation(GCampShelterLanternLocation);
	ShelterLanternLight->SetMobility(EComponentMobility::Movable);
	ShelterLanternLight->SetIntensity(7500.0f);
	ShelterLanternLight->SetLightColor(FLinearColor(1.0f, 0.20f, 0.035f));
	ShelterLanternLight->SetAttenuationRadius(460.0f);
	ShelterLanternLight->SetCastShadows(false);

	WorkstationLanternLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("WorkstationLanternLight"));
	WorkstationLanternLight->SetupAttachment(CampRoot);
	WorkstationLanternLight->SetRelativeLocation(GCampWorkstationLanternLocation);
	WorkstationLanternLight->SetMobility(EComponentMobility::Movable);
	WorkstationLanternLight->SetIntensity(6500.0f);
	WorkstationLanternLight->SetLightColor(FLinearColor(1.0f, 0.18f, 0.03f));
	WorkstationLanternLight->SetAttenuationRadius(420.0f);
	WorkstationLanternLight->SetCastShadows(false);
}

void AEmberdeepCampEnvironment::BeginPlay()
{
	Super::BeginPlay();

	ApplyPaletteMaterials();
	UpdateCampfireFlicker();
}

void AEmberdeepCampEnvironment::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateCampfireFlicker();
}

int32 AEmberdeepCampEnvironment::GetVoxelInstanceCount() const
{
	int32 InstanceCount = 0;
	for (const UInstancedStaticMeshComponent* PaletteMesh : PaletteMeshes)
	{
		if (PaletteMesh)
		{
			InstanceCount += PaletteMesh->GetInstanceCount();
		}
	}
	return InstanceCount;
}

void AEmberdeepCampEnvironment::ApplyPaletteMaterials()
{
	if (!BaseBlockMaterial)
	{
		return;
	}

	for (int32 PaletteIndex = 0; PaletteIndex < PaletteMeshes.Num(); ++PaletteIndex)
	{
		if (!PaletteMeshes[PaletteIndex])
		{
			continue;
		}

		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseBlockMaterial, this);
		Material->SetVectorParameterValue(TEXT("Color"), GCampPaletteDefinitions[PaletteIndex].Color);
		PaletteMeshes[PaletteIndex]->SetMaterial(0, Material);
	}
}

void AEmberdeepCampEnvironment::UpdateCampfireFlicker()
{
	if (!CampfireLight || !GetWorld())
	{
		return;
	}

	const int32 FlickerStep = FMath::FloorToInt(GetWorld()->GetTimeSeconds() * 12.0f);
	if (FlickerStep == LastFlickerStep)
	{
		return;
	}
	LastFlickerStep = FlickerStep;

	static constexpr float FlickerPattern[] = { 0.94f, 1.03f, 0.98f, 1.08f, 0.96f, 1.01f, 0.91f, 1.05f };
	CampfireLight->SetIntensity(36000.0f * FlickerPattern[FlickerStep % UE_ARRAY_COUNT(FlickerPattern)]);
}
