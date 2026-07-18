#include "Environment/EmberdeepRewardRoomEnvironment.h"

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
	enum class ERewardPalette : uint8
	{
		FloorDark,
		FloorMid,
		WallDark,
		WallStone,
		Iron,
		Bone,
		Crimson,
		RewardGold,
		Frost,
		Count
	};

	struct FRewardPaletteDefinition
	{
		ERewardPalette Palette;
		const TCHAR* Name;
		FLinearColor Color;
	};

	const FRewardPaletteDefinition GRewardPaletteDefinitions[] =
	{
		{ ERewardPalette::FloorDark, TEXT("FloorDark"), FLinearColor::FromSRGBColor(FColor(20, 21, 24)) },
		{ ERewardPalette::FloorMid, TEXT("FloorMid"), FLinearColor::FromSRGBColor(FColor(38, 38, 41)) },
		{ ERewardPalette::WallDark, TEXT("WallDark"), FLinearColor::FromSRGBColor(FColor(40, 39, 42)) },
		{ ERewardPalette::WallStone, TEXT("WallStone"), FLinearColor::FromSRGBColor(FColor(78, 76, 76)) },
		{ ERewardPalette::Iron, TEXT("Iron"), FLinearColor::FromSRGBColor(FColor(105, 106, 108)) },
		{ ERewardPalette::Bone, TEXT("Bone"), FLinearColor::FromSRGBColor(FColor(188, 174, 143)) },
		{ ERewardPalette::Crimson, TEXT("Crimson"), FLinearColor::FromSRGBColor(FColor(100, 18, 19)) },
		{ ERewardPalette::RewardGold, TEXT("RewardGold"), FLinearColor::FromSRGBColor(FColor(220, 144, 36)) },
		{ ERewardPalette::Frost, TEXT("Frost"), FLinearColor::FromSRGBColor(FColor(119, 146, 173)) }
	};

	static_assert(
		UE_ARRAY_COUNT(GRewardPaletteDefinitions) == static_cast<int32>(ERewardPalette::Count),
		"Every reward-room palette entry must have a material definition.");
}

AEmberdeepRewardRoomEnvironment::AEmberdeepRewardRoomEnvironment()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);
	SetReplicateMovement(false);
	bAlwaysRelevant = true;

	RewardRoomRoot = CreateDefaultSubobject<USceneComponent>(TEXT("RewardRoomRoot"));
	RewardRoomRoot->SetMobility(EComponentMobility::Static);
	SetRootComponent(RewardRoomRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseBlockMaterial = MaterialAsset.Object;

	for (const FRewardPaletteDefinition& PaletteDefinition : GRewardPaletteDefinitions)
	{
		const FName ComponentName(*FString::Printf(TEXT("Reward%sInstances"), PaletteDefinition.Name));
		UInstancedStaticMeshComponent* PaletteMesh =
			CreateDefaultSubobject<UInstancedStaticMeshComponent>(ComponentName);
		PaletteMesh->SetupAttachment(RewardRoomRoot);
		PaletteMesh->SetMobility(EComponentMobility::Static);
		PaletteMesh->SetStaticMesh(CubeAsset.Object);
		PaletteMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PaletteMesh->SetGenerateOverlapEvents(false);
		PaletteMesh->SetCanEverAffectNavigation(false);
		PaletteMeshes.Add(PaletteMesh);
	}

	const auto AddBlock = [this](
		ERewardPalette Palette,
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

	// Compact 12x9 chamber. Its quiet, open center gives the reward chest a
	// strong silhouette and keeps the return route readable for a full party.
	for (int32 TileX = 0; TileX < 12; ++TileX)
	{
		for (int32 TileY = 0; TileY < 9; ++TileY)
		{
			const ERewardPalette FloorPalette = (TileX + TileY) % 2 == 0
				? ERewardPalette::FloorDark
				: ERewardPalette::FloorMid;
			AddBlock(
				FloorPalette,
				FVector(-550.0f + TileX * 100.0f, -400.0f + TileY * 100.0f, -4.0f),
				FVector(100.0f, 100.0f, 18.0f));
		}
	}

	for (int32 BlockX = 0; BlockX < 12; ++BlockX)
	{
		const float X = -550.0f + BlockX * 100.0f;
		for (int32 HeightLayer = 0; HeightLayer < 2; ++HeightLayer)
		{
			const ERewardPalette WallPalette = (BlockX + HeightLayer) % 4 == 0
				? ERewardPalette::WallStone
				: ERewardPalette::WallDark;
			const float Z = 55.0f + HeightLayer * 100.0f;
			AddBlock(WallPalette, FVector(X, -430.0f, Z), FVector(100.0f, 60.0f, 100.0f));
			AddBlock(WallPalette, FVector(X, 430.0f, Z), FVector(100.0f, 60.0f, 100.0f));
		}
	}
	for (int32 BlockY = 0; BlockY < 9; ++BlockY)
	{
		const float Y = -400.0f + BlockY * 100.0f;
		for (int32 HeightLayer = 0; HeightLayer < 2; ++HeightLayer)
		{
			const ERewardPalette WallPalette = (BlockY + HeightLayer) % 4 == 0
				? ERewardPalette::WallStone
				: ERewardPalette::WallDark;
			const float Z = 55.0f + HeightLayer * 100.0f;
			AddBlock(WallPalette, FVector(-580.0f, Y, Z), FVector(60.0f, 100.0f, 100.0f));
			AddBlock(WallPalette, FVector(580.0f, Y, Z), FVector(60.0f, 100.0f, 100.0f));
		}
	}

	// Crimson runner points directly from the party entrance to the reward dais.
	// The actual gold palette is reserved for the altar and legendary framing.
	for (int32 RunnerY = -3; RunnerY <= 2; ++RunnerY)
	{
		AddBlock(ERewardPalette::Crimson, FVector(0.0f, RunnerY * 100.0f, 7.0f), FVector(150.0f, 94.0f, 5.0f));
	}

	// A broad non-colliding stone dais leaves room for the separate interactive
	// reward chest actor at roughly (0, 220, 70).
	AddBlock(ERewardPalette::WallStone, FVector(0.0f, 250.0f, 20.0f), FVector(430.0f, 260.0f, 35.0f));
	AddBlock(ERewardPalette::WallDark, FVector(0.0f, 315.0f, 48.0f), FVector(340.0f, 120.0f, 55.0f));
	AddBlock(ERewardPalette::Iron, FVector(0.0f, 327.0f, 82.0f), FVector(290.0f, 20.0f, 18.0f));
	AddBlock(ERewardPalette::RewardGold, FVector(0.0f, 317.0f, 101.0f), FVector(210.0f, 16.0f, 18.0f));
	for (const float Side : { -1.0f, 1.0f })
	{
		AddBlock(ERewardPalette::WallStone, FVector(Side * 245.0f, 335.0f, 95.0f), FVector(68.0f, 86.0f, 180.0f));
		AddBlock(ERewardPalette::Iron, FVector(Side * 245.0f, 328.0f, 126.0f), FVector(82.0f, 18.0f, 22.0f));
		AddBlock(ERewardPalette::RewardGold, FVector(Side * 245.0f, 312.0f, 170.0f), FVector(34.0f, 20.0f, 48.0f));
		AddBlock(ERewardPalette::Bone, FVector(Side * 420.0f, 350.0f, 45.0f), FVector(70.0f, 42.0f, 70.0f));
	}

	// Four shallow wall pilasters and gold caps make the room feel ceremonial
	// without taking any playable floor away from the party.
	for (const float X : { -420.0f, 420.0f })
	{
		for (const float Side : { -1.0f, 1.0f })
		{
			AddBlock(ERewardPalette::WallStone, FVector(X, Side * 385.0f, 82.0f), FVector(72.0f, 82.0f, 150.0f));
			AddBlock(ERewardPalette::RewardGold, FVector(X, Side * 376.0f, 158.0f), FVector(84.0f, 18.0f, 16.0f));
		}
	}

	// Frost-blue insets prevent the room from becoming uniformly warm and make
	// the truly limited gold accents read as a reward rather than ordinary trim.
	for (const float Side : { -1.0f, 1.0f })
	{
		AddBlock(ERewardPalette::Frost, FVector(Side * 535.0f, 0.0f, 125.0f), FVector(20.0f, 150.0f, 90.0f));
		AddBlock(ERewardPalette::Iron, FVector(Side * 525.0f, 0.0f, 125.0f), FVector(16.0f, 180.0f, 120.0f));
	}

	const FVector GoldLightLocations[] =
	{
		FVector(-220.0f, 255.0f, 190.0f),
		FVector(220.0f, 255.0f, 190.0f)
	};
	for (int32 LightIndex = 0; LightIndex < UE_ARRAY_COUNT(GoldLightLocations); ++LightIndex)
	{
		UPointLightComponent* GoldLight = CreateDefaultSubobject<UPointLightComponent>(
			FName(*FString::Printf(TEXT("RewardGoldLight%02d"), LightIndex)));
		GoldLight->SetupAttachment(RewardRoomRoot);
		GoldLight->SetRelativeLocation(GoldLightLocations[LightIndex]);
		GoldLight->SetMobility(EComponentMobility::Movable);
		GoldLight->SetIntensity(11500.0f);
		GoldLight->SetLightColor(FLinearColor(1.0f, 0.42f, 0.055f));
		GoldLight->SetAttenuationRadius(480.0f);
		GoldLight->SetCastShadows(false);
		LocalLights.Add(GoldLight);
	}

	for (int32 SideIndex = 0; SideIndex < 2; ++SideIndex)
	{
		const float Side = SideIndex == 0 ? -1.0f : 1.0f;
		UPointLightComponent* FrostLight = CreateDefaultSubobject<UPointLightComponent>(
			FName(*FString::Printf(TEXT("RewardFrostLight%02d"), SideIndex)));
		FrostLight->SetupAttachment(RewardRoomRoot);
		FrostLight->SetRelativeLocation(FVector(Side * 430.0f, -80.0f, 180.0f));
		FrostLight->SetMobility(EComponentMobility::Movable);
		FrostLight->SetIntensity(4500.0f);
		FrostLight->SetLightColor(FLinearColor(0.12f, 0.30f, 0.62f));
		FrostLight->SetAttenuationRadius(390.0f);
		FrostLight->SetCastShadows(false);
		LocalLights.Add(FrostLight);
	}

	struct FCollisionDefinition
	{
		FVector Center;
		FVector Size;
	};
	const FCollisionDefinition CollisionDefinitions[] =
	{
		{ FVector(0.0f, 0.0f, -25.0f), FVector(1200.0f, 900.0f, 60.0f) },
		{ FVector(0.0f, -430.0f, 105.0f), FVector(1200.0f, 40.0f, 260.0f) },
		{ FVector(0.0f, 430.0f, 105.0f), FVector(1200.0f, 40.0f, 260.0f) },
		{ FVector(-580.0f, 0.0f, 105.0f), FVector(40.0f, 820.0f, 260.0f) },
		{ FVector(580.0f, 0.0f, 105.0f), FVector(40.0f, 820.0f, 260.0f) }
	};
	for (int32 CollisionIndex = 0; CollisionIndex < UE_ARRAY_COUNT(CollisionDefinitions); ++CollisionIndex)
	{
		const FCollisionDefinition& Definition = CollisionDefinitions[CollisionIndex];
		UBoxComponent* Collision = CreateDefaultSubobject<UBoxComponent>(
			FName(*FString::Printf(TEXT("RewardRoomCollision%02d"), CollisionIndex)));
		Collision->SetupAttachment(RewardRoomRoot);
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

void AEmberdeepRewardRoomEnvironment::BeginPlay()
{
	Super::BeginPlay();
	ApplyPaletteMaterials();
}

int32 AEmberdeepRewardRoomEnvironment::GetVisualBlockCount() const
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

void AEmberdeepRewardRoomEnvironment::ApplyPaletteMaterials()
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
		Material->SetVectorParameterValue(TEXT("Color"), GRewardPaletteDefinitions[PaletteIndex].Color);
		PaletteMeshes[PaletteIndex]->SetMaterial(0, Material);
	}
}
