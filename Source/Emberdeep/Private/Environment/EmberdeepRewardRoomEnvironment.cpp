#include "Environment/EmberdeepRewardRoomEnvironment.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Visual/EmberdeepVoxelStyle.h"

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
		{ ERewardPalette::FloorDark, TEXT("FloorDark"), FLinearColor::FromSRGBColor(FColor(9, 10, 13)) },
		{ ERewardPalette::FloorMid, TEXT("FloorMid"), FLinearColor::FromSRGBColor(FColor(25, 24, 27)) },
		{ ERewardPalette::WallDark, TEXT("WallDark"), FLinearColor::FromSRGBColor(FColor(23, 21, 24)) },
		{ ERewardPalette::WallStone, TEXT("WallStone"), FLinearColor::FromSRGBColor(FColor(57, 52, 51)) },
		{ ERewardPalette::Iron, TEXT("Iron"), FLinearColor::FromSRGBColor(FColor(76, 76, 80)) },
		{ ERewardPalette::Bone, TEXT("Bone"), FLinearColor::FromSRGBColor(FColor(160, 144, 111)) },
		{ ERewardPalette::Crimson, TEXT("Crimson"), FLinearColor::FromSRGBColor(FColor(76, 9, 12)) },
		{ ERewardPalette::RewardGold, TEXT("RewardGold"), FLinearColor::FromSRGBColor(FColor(230, 133, 25)) },
		{ ERewardPalette::Frost, TEXT("Frost"), FLinearColor::FromSRGBColor(FColor(62, 85, 111)) }
	};

	static_assert(
		UE_ARRAY_COUNT(GRewardPaletteDefinitions) == static_cast<int32>(ERewardPalette::Count),
		"Every reward-room palette entry must have a material definition.");

	int32 PositiveRewardModulo(int32 Value, int32 Divisor)
	{
		const int32 Result = Value % Divisor;
		return Result < 0 ? Result + Divisor : Result;
	}

	uint32 HashRewardCell(int32 X, int32 Y, int32 Z, int32 Seed)
	{
		uint32 Hash = 2166136261u;
		Hash = (Hash ^ static_cast<uint32>(X)) * 16777619u;
		Hash = (Hash ^ static_cast<uint32>(Y)) * 16777619u;
		Hash = (Hash ^ static_cast<uint32>(Z)) * 16777619u;
		return (Hash ^ static_cast<uint32>(Seed)) * 16777619u;
	}

	class FRewardVoxelBuilder
	{
	public:
		explicit FRewardVoxelBuilder(TArray<TArray<FTransform>>& InTransformsByPalette)
			: TransformsByPalette(InTransformsByPalette)
		{
			TransformsByPalette.SetNum(static_cast<int32>(ERewardPalette::Count));
		}

		void AddCell(ERewardPalette Palette, int32 X, int32 Y, int32 Z)
		{
			const FIntVector Cell(X, Y, Z);
			if (OccupiedCells.Contains(Cell))
			{
				return;
			}
			OccupiedCells.Add(Cell);
			TransformsByPalette[static_cast<int32>(Palette)].Emplace(
				FQuat::Identity,
				EmberdeepVoxelStyle::CellCenter(X, Y, Z),
				EmberdeepVoxelStyle::InstanceScale());
		}

		void AddFilledBox(ERewardPalette Palette, const FIntVector& Min, const FIntVector& Max)
		{
			for (int32 Z = Min.Z; Z <= Max.Z; ++Z)
			{
				for (int32 Y = Min.Y; Y <= Max.Y; ++Y)
				{
					for (int32 X = Min.X; X <= Max.X; ++X)
					{
						AddCell(Palette, X, Y, Z);
					}
				}
			}
		}

		void AddSurfaceBox(ERewardPalette Palette, const FIntVector& Min, const FIntVector& Max)
		{
			for (int32 Z = Min.Z; Z <= Max.Z; ++Z)
			{
				for (int32 Y = Min.Y; Y <= Max.Y; ++Y)
				{
					for (int32 X = Min.X; X <= Max.X; ++X)
					{
						if (X == Min.X || X == Max.X || Y == Min.Y || Y == Max.Y || Z == Min.Z || Z == Max.Z)
						{
							AddCell(Palette, X, Y, Z);
						}
					}
				}
			}
		}

		void AddLine(ERewardPalette Palette, const FIntVector& Start, const FIntVector& End, int32 Radius = 0)
		{
			const FIntVector Delta = End - Start;
			const int32 StepCount = FMath::Max3(FMath::Abs(Delta.X), FMath::Abs(Delta.Y), FMath::Abs(Delta.Z));
			for (int32 Step = 0; Step <= StepCount; ++Step)
			{
				const float Alpha = StepCount > 0 ? static_cast<float>(Step) / static_cast<float>(StepCount) : 0.0f;
				const FIntVector Center(
					FMath::RoundToInt(FMath::Lerp(static_cast<float>(Start.X), static_cast<float>(End.X), Alpha)),
					FMath::RoundToInt(FMath::Lerp(static_cast<float>(Start.Y), static_cast<float>(End.Y), Alpha)),
					FMath::RoundToInt(FMath::Lerp(static_cast<float>(Start.Z), static_cast<float>(End.Z), Alpha)));
				AddFilledBox(Palette, Center - FIntVector(Radius), Center + FIntVector(Radius));
			}
		}

	private:
		TArray<TArray<FTransform>>& TransformsByPalette;
		TSet<FIntVector> OccupiedCells;
	};

	ERewardPalette SelectRewardMasonryPalette(int32 Along, int32 Z, int32 Seed)
	{
		const int32 BrickRow = PositiveRewardModulo(Z - 1, 8);
		const int32 RowOffset = ((Z - 1) / 8 & 1) != 0 ? 8 : 0;
		const int32 BrickColumn = PositiveRewardModulo(Along + RowOffset, 16);
		if (BrickRow == 0 || BrickColumn == 0)
		{
			return ERewardPalette::WallDark;
		}
		return HashRewardCell(Along / 4, Z / 4, Seed, 37) % 14u < 3u
			? ERewardPalette::WallDark
			: ERewardPalette::WallStone;
	}

	void AddPavedFloor(FRewardVoxelBuilder& Builder)
	{
		// A compact 12 x 9 metre floor made from staggered 48 x 40 cm slabs.
		// The footprint differs by only two centimetres at each Y edge because
		// nine metres cannot be symmetrically centered on the shared 4 cm lattice.
		for (int32 Y = -112; Y <= 112; ++Y)
		{
			const int32 SlabRow = FMath::FloorToInt(static_cast<float>(Y + 112) / 10.0f);
			const int32 RowOffset = (SlabRow & 1) != 0 ? 6 : 0;
			for (int32 X = -150; X <= 149; ++X)
			{
				const int32 LocalX = PositiveRewardModulo(X + 150 + RowOffset, 12);
				const int32 LocalY = PositiveRewardModulo(Y + 112, 10);
				const bool bMortar = LocalX == 0 || LocalY == 0;
				const bool bWorn = HashRewardCell(X / 5, Y / 5, 0, 13) % 31u == 0u;
				Builder.AddCell(bMortar || bWorn ? ERewardPalette::FloorDark : ERewardPalette::FloorMid, X, Y, 0);
			}
		}

		// A narrow, dark-crimson runner leads from the party spawn to the reliquary.
		for (int32 Y = -74; Y <= 27; ++Y)
		{
			const int32 HalfWidth = 11 + ((Y + 74) / 26);
			for (int32 X = -HalfWidth; X <= HalfWidth; ++X)
			{
				if (FMath::Abs(X) == HalfWidth || PositiveRewardModulo(Y, 18) != 0 || FMath::Abs(X) > 2)
				{
					Builder.AddCell(ERewardPalette::Crimson, X, Y, 1);
				}
			}
		}
	}

	void AddFarWalls(FRewardVoxelBuilder& Builder)
	{
		// South wall with a deep, closed processional door.
		for (int32 Z = 1; Z <= 70; ++Z)
		{
			for (int32 Y = -112; Y <= -103; ++Y)
			{
				for (int32 X = -150; X <= 149; ++X)
				{
					const bool bDoorOpening = FMath::Abs(X) <= 20 && Z <= 50;
					const bool bSurface = X == -150 || X == 149 || Y == -112 || Y == -103 || Z == 1 || Z == 70;
					if (!bDoorOpening && bSurface)
					{
						Builder.AddCell(SelectRewardMasonryPalette(X, Z, 3), X, Y, Z);
					}
				}
			}
		}

		Builder.AddSurfaceBox(ERewardPalette::WallStone, FIntVector(-28, -102, 1), FIntVector(-21, -93, 59));
		Builder.AddSurfaceBox(ERewardPalette::WallStone, FIntVector(21, -102, 1), FIntVector(28, -93, 59));
		Builder.AddSurfaceBox(ERewardPalette::WallStone, FIntVector(-28, -102, 50), FIntVector(28, -93, 60));
		Builder.AddSurfaceBox(ERewardPalette::WallDark, FIntVector(-19, -101, 1), FIntVector(19, -100, 48));
		for (const int32 StrapZ : { 11, 26, 41 })
		{
			Builder.AddFilledBox(ERewardPalette::Iron, FIntVector(-19, -99, StrapZ), FIntVector(19, -99, StrapZ + 1));
		}
		Builder.AddFilledBox(ERewardPalette::RewardGold, FIntVector(-2, -98, 23), FIntVector(2, -98, 30));

		// East wall supplies the second tall value mass in the fixed camera view.
		for (int32 Z = 1; Z <= 70; ++Z)
		{
			for (int32 X = 140; X <= 149; ++X)
			{
				for (int32 Y = -112; Y <= 112; ++Y)
				{
					const bool bSurface = X == 140 || X == 149 || Y == -112 || Y == 112 || Z == 1 || Z == 70;
					if (bSurface)
					{
						Builder.AddCell(SelectRewardMasonryPalette(Y, Z, 7), X, Y, Z);
					}
				}
			}
		}
	}

	void AddBrokenNearWalls(FRewardVoxelBuilder& Builder)
	{
		for (int32 X = -150; X <= 149; ++X)
		{
			const int32 Segment = FMath::FloorToInt(static_cast<float>(X + 150) / 16.0f);
			const int32 Height = 13 + static_cast<int32>(HashRewardCell(Segment, 0, 0, 17) % 10u);
			for (int32 Y = 103; Y <= 112; ++Y)
			{
				for (int32 Z = 1; Z <= Height; ++Z)
				{
					if (Y == 103 || Y == 112 || Z == 1 || Z == Height)
					{
						Builder.AddCell(SelectRewardMasonryPalette(X, Z, 19), X, Y, Z);
					}
				}
			}
		}

		for (int32 Y = -112; Y <= 112; ++Y)
		{
			const int32 Segment = FMath::FloorToInt(static_cast<float>(Y + 112) / 16.0f);
			const int32 Height = 12 + static_cast<int32>(HashRewardCell(Segment, 0, 0, 23) % 10u);
			for (int32 X = -150; X <= -141; ++X)
			{
				for (int32 Z = 1; Z <= Height; ++Z)
				{
					if (X == -150 || X == -141 || Z == 1 || Z == Height)
					{
						Builder.AddCell(SelectRewardMasonryPalette(Y, Z, 29), X, Y, Z);
					}
				}
			}
		}
	}

	void AddButtresses(FRewardVoxelBuilder& Builder)
	{
		for (const int32 X : { -112, -58, 58, 112 })
		{
			Builder.AddSurfaceBox(ERewardPalette::WallStone, FIntVector(X - 5, -102, 1), FIntVector(X + 5, -90, 76));
			Builder.AddSurfaceBox(ERewardPalette::Iron, FIntVector(X - 7, -89, 49), FIntVector(X + 7, -87, 54));
		}
		for (const int32 Y : { -74, -20, 34, 88 })
		{
			Builder.AddSurfaceBox(ERewardPalette::WallStone, FIntVector(128, Y - 5, 1), FIntVector(139, Y + 5, 76));
			Builder.AddSurfaceBox(ERewardPalette::Iron, FIntVector(125, Y - 7, 49), FIntVector(127, Y + 7, 54));
		}
	}

	void AddReliquary(FRewardVoxelBuilder& Builder)
	{
		// Three shallow tiers preserve the current walkable collision plane. The
		// open central plinth frames a chest at roughly (0, 220, 70) while leaving
		// the currently spawned chest at the front edge completely visible.
		Builder.AddSurfaceBox(ERewardPalette::WallStone, FIntVector(-54, 18, 2), FIntVector(54, 88, 3));
		Builder.AddSurfaceBox(ERewardPalette::WallDark, FIntVector(-47, 28, 4), FIntVector(47, 88, 6));
		Builder.AddSurfaceBox(ERewardPalette::WallStone, FIntVector(-38, 39, 7), FIntVector(38, 88, 10));
		// The existing reward actor spawns at (0, 105, 20). This front plinth ends
		// at Z=32 cm, exactly under its current chest base instead of leaving it
		// visibly suspended above the ceremonial steps.
		Builder.AddSurfaceBox(ERewardPalette::WallStone, FIntVector(-24, 15, 1), FIntVector(24, 38, 7));

		// Back shrine and recessed niche create a ceremonial focal silhouette.
		Builder.AddSurfaceBox(ERewardPalette::WallStone, FIntVector(-53, 82, 1), FIntVector(-39, 101, 58));
		Builder.AddSurfaceBox(ERewardPalette::WallStone, FIntVector(39, 82, 1), FIntVector(53, 101, 58));
		Builder.AddSurfaceBox(ERewardPalette::WallStone, FIntVector(-53, 82, 50), FIntVector(53, 101, 61));
		Builder.AddSurfaceBox(ERewardPalette::WallDark, FIntVector(-32, 81, 15), FIntVector(32, 89, 49));

		// The chest pedestal is deliberately modest: gold is reserved for the
		// reward itself and a thin architectural halo, not sprayed across the room.
		Builder.AddSurfaceBox(ERewardPalette::WallStone, FIntVector(-28, 43, 7), FIntVector(28, 70, 14));
		Builder.AddFilledBox(ERewardPalette::Iron, FIntVector(-29, 42, 14), FIntVector(29, 71, 16));
		Builder.AddFilledBox(ERewardPalette::RewardGold, FIntVector(-24, 42, 17), FIntVector(24, 43, 18));

		// Original sunburst motif behind the chest.
		Builder.AddSurfaceBox(ERewardPalette::Bone, FIntVector(-7, 79, 25), FIntVector(7, 80, 39));
		for (const FIntVector& End : {
			FIntVector(-25, 79, 32), FIntVector(25, 79, 32), FIntVector(0, 79, 53),
			FIntVector(-19, 79, 48), FIntVector(19, 79, 48) })
		{
			Builder.AddLine(ERewardPalette::RewardGold, FIntVector(0, 78, 32), End, 1);
		}

		// Frost insets are dark value separators rather than competing light sources.
		Builder.AddFilledBox(ERewardPalette::Frost, FIntVector(-49, 80, 18), FIntVector(-42, 80, 43));
		Builder.AddFilledBox(ERewardPalette::Frost, FIntVector(42, 80, 18), FIntVector(49, 80, 43));
	}

	void AddBrazier(FRewardVoxelBuilder& Builder, int32 CenterX, int32 CenterY)
	{
		Builder.AddFilledBox(ERewardPalette::Iron, FIntVector(CenterX - 2, CenterY - 2, 1), FIntVector(CenterX + 2, CenterY + 2, 28));
		Builder.AddFilledBox(ERewardPalette::Iron, FIntVector(CenterX - 7, CenterY - 7, 27), FIntVector(CenterX + 7, CenterY + 7, 31));
		Builder.AddFilledBox(ERewardPalette::RewardGold, FIntVector(CenterX - 4, CenterY - 4, 32), FIntVector(CenterX + 4, CenterY + 4, 38));
		Builder.AddFilledBox(ERewardPalette::RewardGold, FIntVector(CenterX - 2, CenterY - 3, 39), FIntVector(CenterX + 2, CenterY + 2, 43));
	}

	void AddUrn(FRewardVoxelBuilder& Builder, int32 CenterX, int32 CenterY, bool bBone)
	{
		const ERewardPalette Palette = bBone ? ERewardPalette::Bone : ERewardPalette::WallStone;
		Builder.AddSurfaceBox(Palette, FIntVector(CenterX - 5, CenterY - 5, 1), FIntVector(CenterX + 5, CenterY + 5, 12));
		Builder.AddFilledBox(ERewardPalette::Iron, FIntVector(CenterX - 3, CenterY - 3, 13), FIntVector(CenterX + 3, CenterY + 3, 15));
	}

	void AddPerimeterDressing(FRewardVoxelBuilder& Builder)
	{
		AddBrazier(Builder, -55, 63);
		AddBrazier(Builder, 55, 63);
		AddUrn(Builder, -126, -74, true);
		AddUrn(Builder, -128, 34, false);
		AddUrn(Builder, 121, -72, true);

		// Two wall-side sarcophagi and sparse offering debris make the chamber feel
		// authored without compromising the central party route.
		Builder.AddSurfaceBox(ERewardPalette::WallStone, FIntVector(-136, -62, 1), FIntVector(-112, -25, 13));
		Builder.AddSurfaceBox(ERewardPalette::Bone, FIntVector(-132, -58, 14), FIntVector(-116, -29, 16));
		Builder.AddSurfaceBox(ERewardPalette::WallStone, FIntVector(109, -23, 1), FIntVector(133, 17, 13));
		Builder.AddSurfaceBox(ERewardPalette::Bone, FIntVector(113, -19, 14), FIntVector(129, 13, 16));

		for (int32 DebrisIndex = 0; DebrisIndex < 28; ++DebrisIndex)
		{
			const bool bSide = (DebrisIndex & 1) == 0;
			const int32 X = bSide
				? (DebrisIndex % 4 < 2 ? -137 : 133)
				: -128 + static_cast<int32>(HashRewardCell(DebrisIndex, 1, 0, 61) % 256u);
			const int32 Y = bSide
				? -92 + static_cast<int32>(HashRewardCell(DebrisIndex, 2, 0, 67) % 176u)
				: (DebrisIndex % 4 < 2 ? -94 : 94);
			const int32 Radius = 1 + static_cast<int32>(HashRewardCell(DebrisIndex, 3, 0, 71) % 3u);
			Builder.AddSurfaceBox(
				DebrisIndex % 4 == 0 ? ERewardPalette::Bone : ERewardPalette::WallStone,
				FIntVector(X - Radius, Y - Radius, 1),
				FIntVector(X + Radius, Y + Radius, Radius + 2));
		}
	}
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
		TEXT("/Game/Emberdeep/Materials/M_VoxelSurface.M_VoxelSurface"));
	BaseBlockMaterial = MaterialAsset.Object;

	for (const FRewardPaletteDefinition& PaletteDefinition : GRewardPaletteDefinitions)
	{
		const FName ComponentName(*FString::Printf(TEXT("Reward%sInstances"), PaletteDefinition.Name));
		UInstancedStaticMeshComponent* PaletteMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(ComponentName);
		PaletteMesh->SetupAttachment(RewardRoomRoot);
		PaletteMesh->SetMobility(EComponentMobility::Static);
		PaletteMesh->SetStaticMesh(CubeAsset.Object);
		PaletteMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PaletteMesh->SetGenerateOverlapEvents(false);
		PaletteMesh->SetCanEverAffectNavigation(false);
		PaletteMesh->SetCastShadow(PaletteDefinition.Palette != ERewardPalette::FloorDark
			&& PaletteDefinition.Palette != ERewardPalette::FloorMid
			&& PaletteDefinition.Palette != ERewardPalette::Crimson);
		PaletteMeshes.Add(PaletteMesh);
	}

	TArray<TArray<FTransform>> TransformsByPalette;
	FRewardVoxelBuilder Builder(TransformsByPalette);
	AddPavedFloor(Builder);
	AddFarWalls(Builder);
	AddBrokenNearWalls(Builder);
	AddButtresses(Builder);
	AddReliquary(Builder);
	AddPerimeterDressing(Builder);

	for (int32 PaletteIndex = 0; PaletteIndex < PaletteMeshes.Num(); ++PaletteIndex)
	{
		PaletteMeshes[PaletteIndex]->AddInstances(TransformsByPalette[PaletteIndex], false, false, false);
	}

	const FVector GoldLightLocations[] =
	{
		FVector(-220.0f, 252.0f, 174.0f),
		FVector(220.0f, 252.0f, 174.0f)
	};
	for (int32 LightIndex = 0; LightIndex < UE_ARRAY_COUNT(GoldLightLocations); ++LightIndex)
	{
		UPointLightComponent* GoldLight = CreateDefaultSubobject<UPointLightComponent>(
			FName(*FString::Printf(TEXT("RewardGoldLight%02d"), LightIndex)));
		GoldLight->SetupAttachment(RewardRoomRoot);
		GoldLight->SetRelativeLocation(GoldLightLocations[LightIndex]);
		GoldLight->SetMobility(EComponentMobility::Movable);
		GoldLight->SetIntensity(6400.0f);
		GoldLight->SetLightColor(FLinearColor::FromSRGBColor(FColor(255, 127, 43)));
		GoldLight->SetAttenuationRadius(430.0f);
		GoldLight->SetSourceRadius(12.0f);
		GoldLight->SetSoftSourceRadius(4.0f);
		GoldLight->SetCastShadows(true);
		LocalLights.Add(GoldLight);
	}

	struct FCollisionDefinition
	{
		FVector Center;
		FVector Size;
	};
	// Preserve the established twelve-by-nine-metre gameplay footprint and its
	// proven floor/perimeter-only collision contract.
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
		Material->SetScalarParameterValue(
			TEXT("Roughness"),
			PaletteIndex == static_cast<int32>(ERewardPalette::Iron) ? 0.72f : 0.93f);
		Material->SetScalarParameterValue(
			TEXT("Specular"),
			PaletteIndex == static_cast<int32>(ERewardPalette::Iron) ? 0.18f : 0.06f);
		Material->SetScalarParameterValue(
			TEXT("EmissiveStrength"),
			PaletteIndex == static_cast<int32>(ERewardPalette::RewardGold) ? 0.55f
			: PaletteIndex == static_cast<int32>(ERewardPalette::Frost) ? 0.14f
			: 0.10f);
		PaletteMeshes[PaletteIndex]->SetMaterial(0, Material);
	}
}
