#include "Environment/EmberdeepDungeonEnvironment.h"

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
	enum class EDungeonPalette : uint8
	{
		FloorDark,
		FloorMid,
		WallDark,
		WallStone,
		Iron,
		Bone,
		Crimson,
		Fire,
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
		{ EDungeonPalette::FloorDark, TEXT("FloorDark"), FLinearColor::FromSRGBColor(FColor(23, 20, 17)) },
		{ EDungeonPalette::FloorMid, TEXT("FloorMid"), FLinearColor::FromSRGBColor(FColor(40, 36, 31)) },
		{ EDungeonPalette::WallDark, TEXT("WallDark"), FLinearColor::FromSRGBColor(FColor(30, 27, 24)) },
		{ EDungeonPalette::WallStone, TEXT("WallStone"), FLinearColor::FromSRGBColor(FColor(70, 62, 52)) },
		{ EDungeonPalette::Iron, TEXT("Iron"), FLinearColor::FromSRGBColor(FColor(78, 77, 73)) },
		{ EDungeonPalette::Bone, TEXT("Bone"), FLinearColor::FromSRGBColor(FColor(164, 148, 117)) },
		{ EDungeonPalette::Crimson, TEXT("Crimson"), FLinearColor::FromSRGBColor(FColor(83, 13, 12)) },
		{ EDungeonPalette::Fire, TEXT("Fire"), FLinearColor::FromSRGBColor(FColor(232, 84, 22)) }
	};

	static_assert(
		UE_ARRAY_COUNT(GDungeonPaletteDefinitions) == static_cast<int32>(EDungeonPalette::Count),
		"Every dungeon palette entry must have a material definition.");

	int32 PositiveModulo(int32 Value, int32 Divisor)
	{
		const int32 Result = Value % Divisor;
		return Result < 0 ? Result + Divisor : Result;
	}

	uint32 HashCell(int32 X, int32 Y, int32 Z, int32 Seed)
	{
		uint32 Hash = 2166136261u;
		Hash = (Hash ^ static_cast<uint32>(X)) * 16777619u;
		Hash = (Hash ^ static_cast<uint32>(Y)) * 16777619u;
		Hash = (Hash ^ static_cast<uint32>(Z)) * 16777619u;
		return (Hash ^ static_cast<uint32>(Seed)) * 16777619u;
	}

	class FDungeonVoxelBuilder
	{
	public:
		explicit FDungeonVoxelBuilder(TArray<TArray<FTransform>>& InTransformsByPalette)
			: TransformsByPalette(InTransformsByPalette)
		{
			TransformsByPalette.SetNum(static_cast<int32>(EDungeonPalette::Count));
		}

		void AddCell(EDungeonPalette Palette, int32 X, int32 Y, int32 Z)
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

		void AddFilledBox(EDungeonPalette Palette, const FIntVector& Min, const FIntVector& Max)
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

		void AddSurfaceBox(EDungeonPalette Palette, const FIntVector& Min, const FIntVector& Max)
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

		void AddLine(EDungeonPalette Palette, const FIntVector& Start, const FIntVector& End, int32 Radius = 0)
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

	EDungeonPalette SelectMasonryPalette(int32 Along, int32 Z, int32 Seed)
	{
		const int32 BrickRow = PositiveModulo(Z - 1, 8);
		const int32 RowOffset = ((Z - 1) / 8 & 1) != 0 ? 8 : 0;
		const int32 BrickColumn = PositiveModulo(Along + RowOffset, 16);
		if (BrickRow == 0 || BrickColumn == 0)
		{
			return EDungeonPalette::WallDark;
		}
		return HashCell(Along / 4, Z / 4, Seed, 41) % 13u < 3u
			? EDungeonPalette::WallDark
			: EDungeonPalette::WallStone;
	}

	void AddPavedFloor(FDungeonVoxelBuilder& Builder)
	{
		// The room remains exactly 18 x 14 metres. Forty-to-fifty centimetre
		// staggered pavers replace the old one-metre checkerboard, while every
		// visible solid is still one shared four-centimetre cell.
		for (int32 Y = -175; Y <= 174; ++Y)
		{
			const int32 SlabRow = FMath::FloorToInt(static_cast<float>(Y + 175) / 10.0f);
			const int32 RowOffset = (SlabRow & 1) != 0 ? 6 : 0;
			for (int32 X = -225; X <= 224; ++X)
			{
				const int32 LocalX = PositiveModulo(X + 225 + RowOffset, 12);
				const int32 LocalY = PositiveModulo(Y + 175, 10);
				const bool bMortar = LocalX == 0 || LocalY == 0;
				const uint32 Wear = HashCell(X / 5, Y / 5, 0, 7);
				const EDungeonPalette Palette = bMortar || Wear % 29u == 0u
					? EDungeonPalette::FloorDark
					: EDungeonPalette::FloorMid;
				Builder.AddCell(Palette, X, Y, 0);
			}
		}

		// Flat, non-colliding stains add history without cluttering combat lanes.
		for (const FIntVector& StainCenter : {
			FIntVector(-88, -38, 1), FIntVector(74, 48, 1), FIntVector(118, -102, 1) })
		{
			for (int32 OffsetY = -8; OffsetY <= 8; ++OffsetY)
			{
				for (int32 OffsetX = -10; OffsetX <= 10; ++OffsetX)
				{
					const int32 Distance = OffsetX * OffsetX + OffsetY * OffsetY;
					if (Distance < 64 + static_cast<int32>(HashCell(OffsetX, OffsetY, 0, 91) % 44u))
					{
						Builder.AddCell(EDungeonPalette::Crimson, StainCenter.X + OffsetX, StainCenter.Y + OffsetY, 1);
					}
				}
			}
		}
	}

	void AddFarSouthWall(FDungeonVoxelBuilder& Builder)
	{
		for (int32 Z = 1; Z <= 75; ++Z)
		{
			for (int32 Y = -175; Y <= -166; ++Y)
			{
				for (int32 X = -225; X <= 224; ++X)
				{
					const bool bDoorOpening = FMath::Abs(X) <= 23 && Z <= 55;
					const bool bSurface = X == -225 || X == 224 || Y == -175 || Y == -166 || Z == 1 || Z == 75;
					if (!bDoorOpening && bSurface)
					{
						Builder.AddCell(SelectMasonryPalette(X, Z, 3), X, Y, Z);
					}
				}
			}
		}

		// Deep doorway, heavy lintel, and an iron-strapped closed door establish
		// a clear architectural focal point behind the party entrance.
		Builder.AddSurfaceBox(EDungeonPalette::WallStone, FIntVector(-31, -165, 1), FIntVector(-24, -156, 64));
		Builder.AddSurfaceBox(EDungeonPalette::WallStone, FIntVector(24, -165, 1), FIntVector(31, -156, 64));
		Builder.AddSurfaceBox(EDungeonPalette::WallStone, FIntVector(-31, -165, 55), FIntVector(31, -156, 65));
		Builder.AddSurfaceBox(EDungeonPalette::WallDark, FIntVector(-22, -164, 1), FIntVector(22, -163, 53));
		for (const int32 StrapZ : { 12, 28, 44 })
		{
			Builder.AddFilledBox(EDungeonPalette::Iron, FIntVector(-22, -162, StrapZ), FIntVector(22, -162, StrapZ + 1));
		}
		Builder.AddFilledBox(EDungeonPalette::Iron, FIntVector(-1, -162, 2), FIntVector(1, -162, 52));

		// A restrained crimson standard borrows the strong value grouping of the
		// target without copying its heraldry.
		Builder.AddFilledBox(EDungeonPalette::Iron, FIntVector(-18, -162, 70), FIntVector(18, -162, 72));
		Builder.AddFilledBox(EDungeonPalette::Crimson, FIntVector(-15, -161, 60), FIntVector(15, -161, 70));
	}

	void AddFarEastWall(FDungeonVoxelBuilder& Builder)
	{
		for (int32 Z = 1; Z <= 75; ++Z)
		{
			for (int32 X = 215; X <= 224; ++X)
			{
				for (int32 Y = -175; Y <= 174; ++Y)
				{
					const bool bSurface = X == 215 || X == 224 || Y == -175 || Y == 174 || Z == 1 || Z == 75;
					if (bSurface)
					{
						Builder.AddCell(SelectMasonryPalette(Y, Z, 11), X, Y, Z);
					}
				}
			}
		}
	}

	void AddBrokenNearWalls(FDungeonVoxelBuilder& Builder)
	{
		// Camera-side walls remain low and broken, preserving the dark perimeter
		// silhouette without hiding combatants behind three metres of masonry.
		for (int32 X = -225; X <= 224; ++X)
		{
			if (FMath::Abs(X) <= 26)
			{
				continue;
			}
			const int32 Segment = FMath::FloorToInt(static_cast<float>(X + 225) / 18.0f);
			const int32 Height = 15 + static_cast<int32>(HashCell(Segment, 0, 0, 17) % 12u);
			for (int32 Y = 165; Y <= 174; ++Y)
			{
				for (int32 Z = 1; Z <= Height; ++Z)
				{
					if (Y == 165 || Y == 174 || Z == 1 || Z == Height)
					{
						Builder.AddCell(SelectMasonryPalette(X, Z, 19), X, Y, Z);
					}
				}
			}
		}

		for (int32 Y = -175; Y <= 174; ++Y)
		{
			const int32 Segment = FMath::FloorToInt(static_cast<float>(Y + 175) / 18.0f);
			const int32 Height = 14 + static_cast<int32>(HashCell(Segment, 0, 0, 23) % 11u);
			for (int32 X = -225; X <= -216; ++X)
			{
				for (int32 Z = 1; Z <= Height; ++Z)
				{
					if (X == -225 || X == -216 || Z == 1 || Z == Height)
					{
						Builder.AddCell(SelectMasonryPalette(Y, Z, 29), X, Y, Z);
					}
				}
			}
		}

		// A broken arch frames the reward portal on the northern wall.
		Builder.AddSurfaceBox(EDungeonPalette::WallStone, FIntVector(-34, 154, 1), FIntVector(-27, 174, 55));
		Builder.AddSurfaceBox(EDungeonPalette::WallStone, FIntVector(27, 154, 1), FIntVector(34, 174, 55));
		Builder.AddSurfaceBox(EDungeonPalette::WallStone, FIntVector(-34, 154, 49), FIntVector(34, 174, 58));
	}

	void AddButtresses(FDungeonVoxelBuilder& Builder)
	{
		for (const int32 X : { -174, -112, 112, 174 })
		{
			Builder.AddSurfaceBox(EDungeonPalette::WallStone, FIntVector(X - 6, -165, 1), FIntVector(X + 6, -151, 82));
			Builder.AddSurfaceBox(EDungeonPalette::Iron, FIntVector(X - 8, -150, 56), FIntVector(X + 8, -148, 61));
		}
		for (const int32 Y : { -132, -66, 0, 66, 132 })
		{
			Builder.AddSurfaceBox(EDungeonPalette::WallStone, FIntVector(201, Y - 6, 1), FIntVector(214, Y + 6, 82));
			Builder.AddSurfaceBox(EDungeonPalette::Iron, FIntVector(198, Y - 8, 56), FIntVector(200, Y + 8, 61));
		}
	}

	void AddWallTorch(FDungeonVoxelBuilder& Builder, int32 X, int32 Y)
	{
		Builder.AddFilledBox(EDungeonPalette::Iron, FIntVector(X - 1, Y - 1, 28), FIntVector(X + 1, Y + 1, 45));
		Builder.AddFilledBox(EDungeonPalette::Iron, FIntVector(X - 4, Y - 4, 43), FIntVector(X + 4, Y + 4, 46));
		Builder.AddFilledBox(EDungeonPalette::Fire, FIntVector(X - 3, Y - 3, 47), FIntVector(X + 3, Y + 3, 52));
		Builder.AddFilledBox(EDungeonPalette::Fire, FIntVector(X - 1, Y - 2, 53), FIntVector(X + 2, Y + 2, 57));
	}

	void AddBarrel(FDungeonVoxelBuilder& Builder, int32 CenterX, int32 CenterY)
	{
		constexpr int32 Radius = 8;
		constexpr int32 Height = 17;
		for (int32 Z = 1; Z <= Height; ++Z)
		{
			for (int32 Y = -Radius; Y <= Radius; ++Y)
			{
				for (int32 X = -Radius; X <= Radius; ++X)
				{
					const int32 DistanceSquared = X * X + Y * Y;
					const bool bSide = DistanceSquared >= 43 && DistanceSquared <= 68;
					const bool bTop = Z == Height && DistanceSquared <= 64;
					if (bSide || bTop)
					{
						const bool bIronBand = Z == 4 || Z == 13;
						Builder.AddCell(bIronBand ? EDungeonPalette::Iron : EDungeonPalette::WallDark, CenterX + X, CenterY + Y, Z);
					}
				}
			}
		}
	}

	void AddCage(FDungeonVoxelBuilder& Builder, int32 CenterX, int32 CenterY)
	{
		constexpr int32 HalfX = 12;
		constexpr int32 HalfY = 9;
		constexpr int32 Height = 32;
		for (int32 X = -HalfX; X <= HalfX; X += 4)
		{
			Builder.AddLine(EDungeonPalette::Iron, FIntVector(CenterX + X, CenterY - HalfY, 1), FIntVector(CenterX + X, CenterY - HalfY, Height));
			Builder.AddLine(EDungeonPalette::Iron, FIntVector(CenterX + X, CenterY + HalfY, 1), FIntVector(CenterX + X, CenterY + HalfY, Height));
		}
		for (int32 Y = -HalfY; Y <= HalfY; Y += 4)
		{
			Builder.AddLine(EDungeonPalette::Iron, FIntVector(CenterX - HalfX, CenterY + Y, 1), FIntVector(CenterX - HalfX, CenterY + Y, Height));
			Builder.AddLine(EDungeonPalette::Iron, FIntVector(CenterX + HalfX, CenterY + Y, 1), FIntVector(CenterX + HalfX, CenterY + Y, Height));
		}
		for (const int32 Z : { 1, Height })
		{
			Builder.AddLine(EDungeonPalette::Iron, FIntVector(CenterX - HalfX, CenterY - HalfY, Z), FIntVector(CenterX + HalfX, CenterY - HalfY, Z), 1);
			Builder.AddLine(EDungeonPalette::Iron, FIntVector(CenterX - HalfX, CenterY + HalfY, Z), FIntVector(CenterX + HalfX, CenterY + HalfY, Z), 1);
			Builder.AddLine(EDungeonPalette::Iron, FIntVector(CenterX - HalfX, CenterY - HalfY, Z), FIntVector(CenterX - HalfX, CenterY + HalfY, Z), 1);
			Builder.AddLine(EDungeonPalette::Iron, FIntVector(CenterX + HalfX, CenterY - HalfY, Z), FIntVector(CenterX + HalfX, CenterY + HalfY, Z), 1);
		}
	}

	void AddBonePile(FDungeonVoxelBuilder& Builder, int32 CenterX, int32 CenterY, int32 Seed)
	{
		Builder.AddSurfaceBox(EDungeonPalette::Bone, FIntVector(CenterX - 3, CenterY - 3, 1), FIntVector(CenterX + 3, CenterY + 3, 6));
		for (int32 BoneIndex = 0; BoneIndex < 5; ++BoneIndex)
		{
			const int32 StartX = CenterX - 14 + static_cast<int32>(HashCell(BoneIndex, Seed, 0, 5) % 17u);
			const int32 StartY = CenterY - 10 + static_cast<int32>(HashCell(BoneIndex, Seed, 0, 7) % 15u);
			const int32 EndX = StartX + 10 + static_cast<int32>(HashCell(BoneIndex, Seed, 0, 11) % 8u);
			const int32 EndY = StartY - 7 + static_cast<int32>(HashCell(BoneIndex, Seed, 0, 13) % 15u);
			Builder.AddLine(EDungeonPalette::Bone, FIntVector(StartX, StartY, 2), FIntVector(EndX, EndY, 2), BoneIndex == 0 ? 1 : 0);
		}
	}

	void AddPerimeterDressing(FDungeonVoxelBuilder& Builder)
	{
		AddBarrel(Builder, -190, -104);
		AddBarrel(Builder, -172, -119);
		AddBarrel(Builder, 184, -120);
		AddCage(Builder, -188, 102);
		AddBonePile(Builder, -126, 142, 1);
		AddBonePile(Builder, 146, 132, 2);
		AddBonePile(Builder, 172, -42, 3);

		// Chunky wall-side rubble. It is deliberately non-colliding and never
		// enters the central 900 x 700 cm combat rectangle.
		for (int32 RubbleIndex = 0; RubbleIndex < 42; ++RubbleIndex)
		{
			const bool bAlongSide = (RubbleIndex & 1) == 0;
			const int32 X = bAlongSide
				? (RubbleIndex % 4 < 2 ? -205 : 194)
				: -185 + static_cast<int32>(HashCell(RubbleIndex, 1, 0, 47) % 370u);
			const int32 Y = bAlongSide
				? -148 + static_cast<int32>(HashCell(RubbleIndex, 2, 0, 53) % 296u)
				: (RubbleIndex % 4 < 2 ? -151 : 148);
			const int32 Radius = 1 + static_cast<int32>(HashCell(RubbleIndex, 3, 0, 59) % 4u);
			Builder.AddSurfaceBox(
				RubbleIndex % 3 == 0 ? EDungeonPalette::WallDark : EDungeonPalette::WallStone,
				FIntVector(X - Radius, Y - Radius, 1),
				FIntVector(X + Radius, Y + Radius, Radius + 2));
		}

		// A few readable broken timbers and iron braces, all snapped to the same
		// lattice, sell collapse without turning the room into an obstacle course.
		Builder.AddLine(EDungeonPalette::WallDark, FIntVector(-198, 48, 2), FIntVector(-162, 67, 6), 2);
		Builder.AddLine(EDungeonPalette::Iron, FIntVector(-196, 42, 3), FIntVector(-170, 71, 4), 0);
		Builder.AddLine(EDungeonPalette::WallDark, FIntVector(158, 146, 2), FIntVector(196, 124, 7), 2);
	}
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
		TEXT("/Game/Emberdeep/Materials/M_VoxelSurface.M_VoxelSurface"));
	BaseBlockMaterial = MaterialAsset.Object;

	for (const FDungeonPaletteDefinition& PaletteDefinition : GDungeonPaletteDefinitions)
	{
		const FName ComponentName(*FString::Printf(TEXT("Dungeon%sInstances"), PaletteDefinition.Name));
		UInstancedStaticMeshComponent* PaletteMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(ComponentName);
		PaletteMesh->SetupAttachment(DungeonRoot);
		PaletteMesh->SetMobility(EComponentMobility::Static);
		PaletteMesh->SetStaticMesh(CubeAsset.Object);
		PaletteMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PaletteMesh->SetGenerateOverlapEvents(false);
		PaletteMesh->SetCanEverAffectNavigation(false);
		PaletteMesh->SetCastShadow(PaletteDefinition.Palette != EDungeonPalette::FloorDark
			&& PaletteDefinition.Palette != EDungeonPalette::FloorMid
			&& PaletteDefinition.Palette != EDungeonPalette::Crimson);
		PaletteMeshes.Add(PaletteMesh);
	}

	TArray<TArray<FTransform>> TransformsByPalette;
	FDungeonVoxelBuilder Builder(TransformsByPalette);
	AddPavedFloor(Builder);
	AddFarSouthWall(Builder);
	AddFarEastWall(Builder);
	AddBrokenNearWalls(Builder);
	AddButtresses(Builder);
	AddWallTorch(Builder, -118, -154);
	AddWallTorch(Builder, 118, -154);
	AddWallTorch(Builder, -118, 154);
	AddWallTorch(Builder, 118, 154);
	AddPerimeterDressing(Builder);

	for (int32 PaletteIndex = 0; PaletteIndex < PaletteMeshes.Num(); ++PaletteIndex)
	{
		PaletteMeshes[PaletteIndex]->AddInstances(TransformsByPalette[PaletteIndex], false, false, false);
	}

	const FVector TorchLocations[] =
	{
		FVector(-472.0f, -616.0f, 218.0f),
		FVector(472.0f, -616.0f, 218.0f),
		FVector(-472.0f, 616.0f, 218.0f),
		FVector(472.0f, 616.0f, 218.0f)
	};
	for (int32 TorchIndex = 0; TorchIndex < UE_ARRAY_COUNT(TorchLocations); ++TorchIndex)
	{
		UPointLightComponent* TorchLight = CreateDefaultSubobject<UPointLightComponent>(
			FName(*FString::Printf(TEXT("DungeonTorchLight%02d"), TorchIndex)));
		TorchLight->SetupAttachment(DungeonRoot);
		TorchLight->SetRelativeLocation(TorchLocations[TorchIndex]);
		TorchLight->SetMobility(EComponentMobility::Movable);
		TorchLight->SetIntensity(15400.0f);
		TorchLight->SetLightColor(FLinearColor::FromSRGBColor(FColor(255, 105, 35)));
		TorchLight->SetAttenuationRadius(620.0f);
		TorchLight->SetSourceRadius(12.0f);
		TorchLight->SetSoftSourceRadius(4.0f);
		TorchLight->SetCastShadows(true);
		LocalLights.Add(TorchLight);
	}

	struct FCollisionDefinition
	{
		FVector Center;
		FVector Size;
	};
	// Gameplay footprint, party/enemy positions, and the proven five-proxy
	// collision contract are intentionally unchanged by this visual overhaul.
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
		Material->SetScalarParameterValue(
			TEXT("EmissiveStrength"),
			PaletteIndex == static_cast<int32>(EDungeonPalette::Fire) ? 4.5f : 0.14f);
		PaletteMeshes[PaletteIndex]->SetMaterial(0, Material);
	}
}
