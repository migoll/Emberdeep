#include "Environment/EmberdeepDungeonEnvironment.h"

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
	enum class EDungeonPalette : uint8
	{
		FloorDark,
		FloorMid,
		WallDark,
		WallStone,
		Iron,
		Bone,
		Ember,
		Rift,
		Count
	};

	struct FDungeonPaletteDefinition
	{
		EDungeonPalette Palette;
		const TCHAR* Name;
		FLinearColor Color;
	};

	const FDungeonPaletteDefinition GDungeonPaletteDefinitions[] =
	{
		{ EDungeonPalette::FloorDark, TEXT("FloorDark"), FLinearColor::FromSRGBColor(FColor(18, 20, 24)) },
		{ EDungeonPalette::FloorMid, TEXT("FloorMid"), FLinearColor::FromSRGBColor(FColor(31, 33, 37)) },
		{ EDungeonPalette::WallDark, TEXT("WallDark"), FLinearColor::FromSRGBColor(FColor(35, 35, 39)) },
		{ EDungeonPalette::WallStone, TEXT("WallStone"), FLinearColor::FromSRGBColor(FColor(67, 66, 69)) },
		{ EDungeonPalette::Iron, TEXT("Iron"), FLinearColor::FromSRGBColor(FColor(91, 94, 99)) },
		{ EDungeonPalette::Bone, TEXT("Bone"), FLinearColor::FromSRGBColor(FColor(170, 157, 129)) },
		{ EDungeonPalette::Ember, TEXT("Ember"), FLinearColor::FromSRGBColor(FColor(154, 45, 18)) },
		{ EDungeonPalette::Rift, TEXT("Rift"), FLinearColor::FromSRGBColor(FColor(74, 35, 111)) }
	};

	static_assert(
		UE_ARRAY_COUNT(GDungeonPaletteDefinitions) == static_cast<int32>(EDungeonPalette::Count),
		"Every dungeon palette entry must have a material definition.");
}

AEmberdeepDungeonEnvironment::AEmberdeepDungeonEnvironment()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);
	SetReplicateMovement(false);
	bAlwaysRelevant = true;

	DungeonRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DungeonRoot"));
	DungeonRoot->SetMobility(EComponentMobility::Static);
	SetRootComponent(DungeonRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseBlockMaterial = MaterialAsset.Object;

	for (const FDungeonPaletteDefinition& PaletteDefinition : GDungeonPaletteDefinitions)
	{
		const FName ComponentName(*FString::Printf(TEXT("Dungeon%sInstances"), PaletteDefinition.Name));
		UInstancedStaticMeshComponent* PaletteMesh =
			CreateDefaultSubobject<UInstancedStaticMeshComponent>(ComponentName);
		PaletteMesh->SetupAttachment(DungeonRoot);
		PaletteMesh->SetMobility(EComponentMobility::Static);
		PaletteMesh->SetStaticMesh(CubeAsset.Object);
		PaletteMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PaletteMesh->SetGenerateOverlapEvents(false);
		PaletteMesh->SetCanEverAffectNavigation(false);
		PaletteMeshes.Add(PaletteMesh);
	}

	const auto AddBlock = [this](
		EDungeonPalette Palette,
		const FVector& Center,
		const FVector& Size,
		const FRotator& Rotation = FRotator::ZeroRotator)
	{
		const int32 PaletteIndex = static_cast<int32>(Palette);
		if (PaletteMeshes.IsValidIndex(PaletteIndex))
		{
			PaletteMeshes[PaletteIndex]->AddInstance(FTransform(Rotation, Center, Size / 100.0f));
		}
	};

	// An 18x14 block floor creates a readable low-resolution grid without using
	// per-block collision. The single collision slab below follows the same edge.
	for (int32 TileX = 0; TileX < 18; ++TileX)
	{
		for (int32 TileY = 0; TileY < 14; ++TileY)
		{
			const EDungeonPalette FloorPalette = (TileX + TileY) % 2 == 0
				? EDungeonPalette::FloorDark
				: EDungeonPalette::FloorMid;
			AddBlock(
				FloorPalette,
				FVector(-850.0f + TileX * 100.0f, -650.0f + TileY * 100.0f, -4.0f),
				FVector(100.0f, 100.0f, 18.0f));
		}
	}

	// Two-block-high perimeter walls. All substantial decoration remains against
	// these walls so the center stays unobstructed for direct-steering enemies.
	for (int32 BlockX = 0; BlockX < 18; ++BlockX)
	{
		const float X = -850.0f + BlockX * 100.0f;
		for (int32 HeightLayer = 0; HeightLayer < 2; ++HeightLayer)
		{
			const EDungeonPalette WallPalette = (BlockX + HeightLayer) % 3 == 0
				? EDungeonPalette::WallStone
				: EDungeonPalette::WallDark;
			const float Z = 55.0f + HeightLayer * 100.0f;
			AddBlock(WallPalette, FVector(X, -680.0f, Z), FVector(100.0f, 60.0f, 100.0f));
			AddBlock(WallPalette, FVector(X, 680.0f, Z), FVector(100.0f, 60.0f, 100.0f));
		}
	}
	for (int32 BlockY = 0; BlockY < 14; ++BlockY)
	{
		const float Y = -650.0f + BlockY * 100.0f;
		for (int32 HeightLayer = 0; HeightLayer < 2; ++HeightLayer)
		{
			const EDungeonPalette WallPalette = (BlockY + HeightLayer) % 3 == 0
				? EDungeonPalette::WallStone
				: EDungeonPalette::WallDark;
			const float Z = 55.0f + HeightLayer * 100.0f;
			AddBlock(WallPalette, FVector(-880.0f, Y, Z), FVector(60.0f, 100.0f, 100.0f));
			AddBlock(WallPalette, FVector(880.0f, Y, Z), FVector(60.0f, 100.0f, 100.0f));
		}
	}

	// Chunky buttresses, iron straps, and ritual slabs give the room a distinct
	// silhouette while hugging the non-playable perimeter.
	for (const float X : { -600.0f, 0.0f, 600.0f })
	{
		for (const float Side : { -1.0f, 1.0f })
		{
			AddBlock(EDungeonPalette::WallStone, FVector(X, Side * 625.0f, 75.0f), FVector(82.0f, 90.0f, 140.0f));
			AddBlock(EDungeonPalette::Iron, FVector(X, Side * 616.0f, 155.0f), FVector(92.0f, 18.0f, 20.0f));
		}
	}
	for (const float Y : { -420.0f, 0.0f, 420.0f })
	{
		for (const float Side : { -1.0f, 1.0f })
		{
			AddBlock(EDungeonPalette::WallStone, FVector(Side * 825.0f, Y, 75.0f), FVector(90.0f, 82.0f, 140.0f));
			AddBlock(EDungeonPalette::Iron, FVector(Side * 816.0f, Y, 155.0f), FVector(18.0f, 92.0f, 20.0f));
		}
	}

	// A flat crimson-violet seal makes the combat center legible without adding
	// collision or introducing reward-gold into an ordinary dungeon.
	for (int32 RuneIndex = -2; RuneIndex <= 2; ++RuneIndex)
	{
		AddBlock(EDungeonPalette::Ember, FVector(RuneIndex * 72.0f, 0.0f, 7.0f), FVector(38.0f, 250.0f, 5.0f));
		AddBlock(EDungeonPalette::Rift, FVector(0.0f, RuneIndex * 72.0f, 8.0f), FVector(250.0f, 24.0f, 6.0f));
	}

	// Bone piles and torch brackets are deliberately shallow wall dressing.
	for (const FVector& BoneLocation : {
		FVector(-710.0f, 590.0f, 18.0f), FVector(710.0f, 590.0f, 18.0f),
		FVector(-710.0f, -590.0f, 18.0f), FVector(710.0f, -590.0f, 18.0f) })
	{
		AddBlock(EDungeonPalette::Bone, BoneLocation, FVector(72.0f, 24.0f, 18.0f), FRotator(0.0f, 25.0f, 0.0f));
		AddBlock(EDungeonPalette::Bone, BoneLocation + FVector(22.0f, 8.0f, 12.0f), FVector(22.0f), FRotator(0.0f, -15.0f, 0.0f));
	}

	const FVector TorchLocations[] =
	{
		FVector(-470.0f, -615.0f, 150.0f),
		FVector(470.0f, -615.0f, 150.0f),
		FVector(-470.0f, 615.0f, 150.0f),
		FVector(470.0f, 615.0f, 150.0f)
	};
	for (int32 TorchIndex = 0; TorchIndex < UE_ARRAY_COUNT(TorchLocations); ++TorchIndex)
	{
		const FVector& TorchLocation = TorchLocations[TorchIndex];
		AddBlock(EDungeonPalette::Iron, TorchLocation, FVector(18.0f, 18.0f, 90.0f));
		AddBlock(EDungeonPalette::Ember, TorchLocation + FVector(0.0f, 0.0f, 58.0f), FVector(34.0f, 34.0f, 42.0f));

		UPointLightComponent* TorchLight = CreateDefaultSubobject<UPointLightComponent>(
			FName(*FString::Printf(TEXT("DungeonTorchLight%02d"), TorchIndex)));
		TorchLight->SetupAttachment(DungeonRoot);
		TorchLight->SetRelativeLocation(TorchLocation + FVector(0.0f, 0.0f, 62.0f));
		TorchLight->SetMobility(EComponentMobility::Movable);
		TorchLight->SetIntensity(9500.0f);
		TorchLight->SetLightColor(FLinearColor(1.0f, 0.12f, 0.025f));
		TorchLight->SetAttenuationRadius(510.0f);
		TorchLight->SetCastShadows(false);
		LocalLights.Add(TorchLight);
	}

	UPointLightComponent* RiftLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("DungeonRiftLight"));
	RiftLight->SetupAttachment(DungeonRoot);
	RiftLight->SetRelativeLocation(FVector(0.0f, 0.0f, 220.0f));
	RiftLight->SetMobility(EComponentMobility::Movable);
	RiftLight->SetIntensity(5200.0f);
	RiftLight->SetLightColor(FLinearColor(0.25f, 0.035f, 0.55f));
	RiftLight->SetAttenuationRadius(650.0f);
	RiftLight->SetCastShadows(false);
	LocalLights.Add(RiftLight);

	struct FCollisionDefinition
	{
		FVector Center;
		FVector Size;
	};
	const FCollisionDefinition CollisionDefinitions[] =
	{
		{ FVector(0.0f, 0.0f, -25.0f), FVector(1800.0f, 1400.0f, 60.0f) },
		{ FVector(0.0f, -680.0f, 105.0f), FVector(1800.0f, 40.0f, 260.0f) },
		{ FVector(0.0f, 680.0f, 105.0f), FVector(1800.0f, 40.0f, 260.0f) },
		{ FVector(-880.0f, 0.0f, 105.0f), FVector(40.0f, 1320.0f, 260.0f) },
		{ FVector(880.0f, 0.0f, 105.0f), FVector(40.0f, 1320.0f, 260.0f) }
	};
	for (int32 CollisionIndex = 0; CollisionIndex < UE_ARRAY_COUNT(CollisionDefinitions); ++CollisionIndex)
	{
		const FCollisionDefinition& Definition = CollisionDefinitions[CollisionIndex];
		UBoxComponent* Collision = CreateDefaultSubobject<UBoxComponent>(
			FName(*FString::Printf(TEXT("DungeonCollision%02d"), CollisionIndex)));
		Collision->SetupAttachment(DungeonRoot);
		Collision->SetMobility(EComponentMobility::Static);
		Collision->SetRelativeLocation(Definition.Center);
		Collision->SetBoxExtent(Definition.Size * 0.5f);
		Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Collision->SetCollisionObjectType(ECC_WorldStatic);
		Collision->SetCollisionResponseToAllChannels(ECR_Block);
		Collision->SetGenerateOverlapEvents(false);
		Collision->SetHiddenInGame(true);
		CollisionProxies.Add(Collision);
	}
}

void AEmberdeepDungeonEnvironment::BeginPlay()
{
	Super::BeginPlay();
	ApplyPaletteMaterials();
}

int32 AEmberdeepDungeonEnvironment::GetVisualBlockCount() const
{
	int32 BlockCount = 0;
	for (const UInstancedStaticMeshComponent* PaletteMesh : PaletteMeshes)
	{
		if (PaletteMesh)
		{
			BlockCount += PaletteMesh->GetInstanceCount();
		}
	}
	return BlockCount;
}

void AEmberdeepDungeonEnvironment::ApplyPaletteMaterials()
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
		Material->SetVectorParameterValue(TEXT("Color"), GDungeonPaletteDefinitions[PaletteIndex].Color);
		PaletteMeshes[PaletteIndex]->SetMaterial(0, Material);
	}
}
