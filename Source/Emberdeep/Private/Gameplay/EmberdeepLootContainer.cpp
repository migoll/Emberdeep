#include "Gameplay/EmberdeepLootContainer.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Emberdeep.h"
#include "Engine/StaticMesh.h"
#include "Gameplay/EmberdeepPlayerController.h"
#include "Gameplay/EmberdeepPlayerState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace EmberdeepLootVisuals
{
	constexpr float VoxelScale = 0.04f;
	constexpr float VoxelSize = 4.0f;

	void AddVoxel(UInstancedStaticMeshComponent* Mesh, int32 X, int32 Y, int32 Z)
	{
		if (Mesh)
		{
			Mesh->AddInstance(FTransform(
				FRotator::ZeroRotator,
				FVector(X * VoxelSize, Y * VoxelSize, Z * VoxelSize),
				FVector(VoxelScale)));
		}
	}

	void AddSurfaceBox(
		UInstancedStaticMeshComponent* Mesh,
		int32 MinX,
		int32 MaxX,
		int32 MinY,
		int32 MaxY,
		int32 MinZ,
		int32 MaxZ)
	{
		for (int32 X = MinX; X <= MaxX; ++X)
		{
			for (int32 Y = MinY; Y <= MaxY; ++Y)
			{
				for (int32 Z = MinZ; Z <= MaxZ; ++Z)
				{
					if (X == MinX || X == MaxX || Y == MinY || Y == MaxY || Z == MinZ || Z == MaxZ)
					{
						AddVoxel(Mesh, X, Y, Z);
					}
				}
			}
		}
	}

	UMaterialInstanceDynamic* SetVoxelMaterial(
		UInstancedStaticMeshComponent* Mesh,
		const FLinearColor& Color,
		float EmissiveStrength = 0.08f,
		float Roughness = 0.95f)
	{
		if (!Mesh)
		{
			return nullptr;
		}

		UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0));
		if (!Material)
		{
			Material = Mesh->CreateDynamicMaterialInstance(0);
		}
		if (Material)
		{
			Material->SetVectorParameterValue(TEXT("Color"), Color);
			Material->SetScalarParameterValue(TEXT("Roughness"), Roughness);
			Material->SetScalarParameterValue(TEXT("Specular"), 0.045f);
			Material->SetScalarParameterValue(TEXT("EmissiveStrength"), EmissiveStrength);
		}
		return Material;
	}

	void SetVoxelHidden(UInstancedStaticMeshComponent* Mesh, bool bHidden)
	{
		if (Mesh)
		{
			Mesh->SetHiddenInGame(bHidden);
		}
	}
}

AEmberdeepLootContainer::AEmberdeepLootContainer()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	InteractionRange = 250.0f;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("LootRoot"));
	SetRootComponent(Root);
	Root->SetMobility(EComponentMobility::Movable);

	ChestLidPivot = CreateDefaultSubobject<USceneComponent>(TEXT("ChestLidPivot"));
	ChestLidPivot->SetupAttachment(Root);
	ChestLidPivot->SetRelativeLocation(FVector(0.0f, 0.0f, 36.0f));
	ChestLidPivot->SetMobility(EComponentMobility::Movable);

	LootGlowRoot = CreateDefaultSubobject<USceneComponent>(TEXT("LootGlowRoot"));
	LootGlowRoot->SetupAttachment(Root);
	LootGlowRoot->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> VoxelMaterial(
		TEXT("/Game/Emberdeep/Materials/M_VoxelSurface.M_VoxelSurface"));

	const auto CreateVoxelMesh = [this](const TCHAR* Name, USceneComponent* Parent, bool bCastShadows)
	{
		UInstancedStaticMeshComponent* Mesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Mesh->SetupAttachment(Parent);
		Mesh->SetStaticMesh(CubeMesh.Object);
		if (VoxelMaterial.Succeeded())
		{
			Mesh->SetMaterial(0, VoxelMaterial.Object);
		}
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->SetCastShadow(bCastShadows);
		Mesh->SetReceivesDecals(bCastShadows);
		Mesh->SetMobility(EComponentMobility::Movable);
		return Mesh;
	};

	ChestWoodVoxels = CreateVoxelMesh(TEXT("ChestWoodVoxels"), Root, true);
	ChestPanelVoxels = CreateVoxelMesh(TEXT("ChestPanelVoxels"), Root, true);
	ChestIronVoxels = CreateVoxelMesh(TEXT("ChestIronVoxels"), Root, true);
	ChestLegendaryVoxels = CreateVoxelMesh(TEXT("ChestLegendaryVoxels"), Root, true);
	ChestLidWoodVoxels = CreateVoxelMesh(TEXT("ChestLidWoodVoxels"), ChestLidPivot, true);
	ChestLidIronVoxels = CreateVoxelMesh(TEXT("ChestLidIronVoxels"), ChestLidPivot, true);
	ChestLidLegendaryVoxels = CreateVoxelMesh(TEXT("ChestLidLegendaryVoxels"), ChestLidPivot, true);
	ChestGlowVoxels = CreateVoxelMesh(TEXT("ChestGlowVoxels"), LootGlowRoot, false);
	DropLeatherVoxels = CreateVoxelMesh(TEXT("DropLeatherVoxels"), Root, true);
	DropMetalVoxels = CreateVoxelMesh(TEXT("DropMetalVoxels"), Root, true);
	DropLegendaryVoxels = CreateVoxelMesh(TEXT("DropLegendaryVoxels"), Root, true);
	DropGlowVoxels = CreateVoxelMesh(TEXT("DropGlowVoxels"), LootGlowRoot, false);

	using namespace EmberdeepLootVisuals;

	// Chest body: a compact 60 x 84 cm silhouette, assembled cell by cell.
	AddSurfaceBox(ChestWoodVoxels, -7, 7, -10, 10, 0, 8);
	for (int32 Y = -8; Y <= 8; ++Y)
	{
		for (int32 Z = 2; Z <= 6; ++Z)
		{
			if ((Y + Z) % 3 != 0)
			{
				AddVoxel(ChestPanelVoxels, -8, Y, Z);
				AddVoxel(ChestPanelVoxels, 8, Y, Z);
			}
		}
	}

	const auto AddBodyMetal = [this](int32 X, int32 Y, int32 Z)
	{
		EmberdeepLootVisuals::AddVoxel(ChestIronVoxels, X, Y, Z);
		EmberdeepLootVisuals::AddVoxel(ChestLegendaryVoxels, X, Y, Z);
	};
	for (int32 X = -7; X <= 7; ++X)
	{
		for (int32 Y : {-10, 10})
		{
			AddBodyMetal(X, Y, 0);
			AddBodyMetal(X, Y, 8);
		}
	}
	for (int32 Y = -10; Y <= 10; ++Y)
	{
		for (int32 X : {-7, 7})
		{
			AddBodyMetal(X, Y, 0);
			AddBodyMetal(X, Y, 8);
		}
	}
	for (int32 Z = 0; Z <= 8; ++Z)
	{
		for (int32 X : {-7, 7})
		{
			for (int32 Y : {-10, 10})
			{
				AddBodyMetal(X, Y, Z);
			}
		}
	}
	for (int32 Z = 3; Z <= 6; ++Z)
	{
		for (int32 Y = -1; Y <= 1; ++Y)
		{
			AddBodyMetal(-8, Y, Z);
			AddBodyMetal(8, Y, Z);
		}
	}
	for (int32 X : {-5, 5})
	{
		for (int32 Y : {-8, 8})
		{
			AddBodyMetal(X, Y, -1);
		}
	}

	// Domed lid. The whole rigid cell cluster rotates around ChestLidPivot when open.
	for (int32 X = -7; X <= 7; ++X)
	{
		const int32 TopZ = 1 + FMath::RoundToInt(4.0f * (1.0f - FMath::Abs(static_cast<float>(X)) / 7.0f));
		for (int32 Y = -10; Y <= 10; ++Y)
		{
			AddVoxel(ChestLidWoodVoxels, X, Y, TopZ);
			if (Y == -10 || Y == 10)
			{
				for (int32 Z = 0; Z < TopZ; ++Z)
				{
					AddVoxel(ChestLidWoodVoxels, X, Y, Z);
				}
			}
		}
	}

	const auto AddLidMetal = [this](int32 X, int32 Y, int32 Z)
	{
		EmberdeepLootVisuals::AddVoxel(ChestLidIronVoxels, X, Y, Z);
		EmberdeepLootVisuals::AddVoxel(ChestLidLegendaryVoxels, X, Y, Z);
	};
	for (int32 X = -7; X <= 7; ++X)
	{
		const int32 TopZ = 1 + FMath::RoundToInt(4.0f * (1.0f - FMath::Abs(static_cast<float>(X)) / 7.0f));
		for (int32 Y : {-5, 0, 5})
		{
			AddLidMetal(X, Y, TopZ + 1);
		}
	}
	for (int32 Y = -10; Y <= 10; ++Y)
	{
		for (int32 X : {-7, 7})
		{
			AddLidMetal(X, Y, 1);
		}
	}

	// Lock glow and just a handful of suspended reward cells.
	for (int32 Y = -1; Y <= 1; ++Y)
	{
		AddVoxel(ChestGlowVoxels, -9, Y, 5);
		AddVoxel(ChestGlowVoxels, 9, Y, 5);
	}
	AddVoxel(ChestGlowVoxels, 0, 0, 14);
	AddVoxel(ChestGlowVoxels, -1, -3, 17);
	AddVoxel(ChestGlowVoxels, 1, 4, 19);

	// Ground loot is a small leather spoil bundle with a readable diagonal weapon.
	for (int32 Z = 0; Z <= 4; ++Z)
	{
		const int32 RadiusX = 4 - Z / 2;
		const int32 RadiusY = 5 - Z;
		for (int32 X = -RadiusX; X <= RadiusX; ++X)
		{
			for (int32 Y = -RadiusY; Y <= RadiusY; ++Y)
			{
				if (Z == 0 || Z == 4 || FMath::Abs(X) == RadiusX || FMath::Abs(Y) == RadiusY)
				{
					AddVoxel(DropLeatherVoxels, X, Y, Z);
				}
			}
		}
	}
	for (int32 Step = -5; Step <= 5; ++Step)
	{
		AddVoxel(DropMetalVoxels, Step, Step, 5);
		AddVoxel(DropLegendaryVoxels, Step, Step, 5);
	}
	for (const FIntVector& Cell : {
		FIntVector(-6, -6, 5), FIntVector(-7, -7, 5), FIntVector(-6, -8, 5),
		FIntVector(-8, -6, 5), FIntVector(5, 4, 5), FIntVector(4, 5, 5),
		FIntVector(6, 6, 5), FIntVector(5, 5, 6)})
	{
		AddVoxel(DropMetalVoxels, Cell.X, Cell.Y, Cell.Z);
		AddVoxel(DropLegendaryVoxels, Cell.X, Cell.Y, Cell.Z);
	}
	for (const FIntVector& Cell : {
		FIntVector(0, 0, 7), FIntVector(-3, 2, 6), FIntVector(3, -2, 6),
		FIntVector(-1, -1, 10), FIntVector(2, 2, 13), FIntVector(-2, 1, 16)})
	{
		AddVoxel(DropGlowVoxels, Cell.X, Cell.Y, Cell.Z);
	}

	LootLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LootLight"));
	LootLight->SetupAttachment(LootGlowRoot);
	LootLight->SetRelativeLocation(FVector(0.0f, 0.0f, 68.0f));
	LootLight->SetIntensity(0.0f);
	LootLight->SetAttenuationRadius(220.0f);
	LootLight->SetCastShadows(false);
	LootLight->SetVolumetricScatteringIntensity(0.12f);
}

void AEmberdeepLootContainer::BeginPlay()
{
	Super::BeginPlay();
	ApplyVisualState();
}

void AEmberdeepLootContainer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	LootVisualTime = FMath::Fmod(LootVisualTime + DeltaSeconds, 1000.0f);
	const float Pulse = 0.90f + 0.10f * FMath::Sin(LootVisualTime * 3.4f);
	const float Hover = bChest ? 0.0f : 1.1f * FMath::Sin(LootVisualTime * 2.1f);

	if (LootGlowRoot)
	{
		LootGlowRoot->SetRelativeLocation(FVector(0.0f, 0.0f, Hover));
	}
	if (LootLight && !bDepleted)
	{
		LootLight->SetIntensity(LootLightBaseIntensity * Pulse);
	}
	const float GlowBase = bLegendaryChest ? 1.65f : 0.72f;
	if (ChestGlowMaterial)
	{
		ChestGlowMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), GlowBase * Pulse);
	}
	if (DropGlowMaterial)
	{
		DropGlowMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), GlowBase * Pulse);
	}
}

void AEmberdeepLootContainer::ConfigureLoot(
	const FString& NewTitle,
	const TArray<FEmberdeepItemInstance>& NewEntries,
	bool bAsChest,
	bool bLegendary)
{
	if (!HasAuthority())
	{
		return;
	}
	ContainerTitle = NewTitle;
	LootEntries = NewEntries;
	bChest = bAsChest;
	bLegendaryChest = bLegendary;
	bDepleted = LootEntries.IsEmpty();
	ApplyVisualState();
}

FString AEmberdeepLootContainer::GetInteractionPrompt(const AEmberdeepCharacter* Character) const
{
	return bDepleted
		? FString::Printf(TEXT("%s (empty)"), *ContainerTitle)
		: FString::Printf(TEXT("Open %s"), *ContainerTitle);
}

void AEmberdeepLootContainer::Interact(AEmberdeepCharacter* Character, AEmberdeepPlayerController* Controller)
{
	if (HasAuthority() && Controller && !bDepleted)
	{
		AddViewer(Controller);
		Controller->ShowLootWindowFromServer(this, ContainerTitle, LootEntries);
	}
}

bool AEmberdeepLootContainer::TryTakeEntry(AEmberdeepPlayerController* Controller, int32 EntryId)
{
	if (!HasAuthority() || !Controller || bDepleted)
	{
		return false;
	}

	const int32 EntryIndex = LootEntries.IndexOfByPredicate([EntryId](const FEmberdeepItemInstance& Entry)
	{
		return Entry.InstanceId == EntryId;
	});
	if (!LootEntries.IsValidIndex(EntryIndex))
	{
		return false;
	}

	AEmberdeepPlayerState* RunState = Controller->GetPlayerState<AEmberdeepPlayerState>();
	if (!RunState || !RunState->AddItem(LootEntries[EntryIndex]))
	{
		Controller->NotifyInventoryFull();
		return false;
	}

	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_LOOT Claimed Player=%d Item=%s"),
		RunState->GetPlayerId(), *LootEntries[EntryIndex].DisplayName);
	LootEntries.RemoveAt(EntryIndex);
	bDepleted = LootEntries.IsEmpty();
	ApplyVisualState();
	if (bDepleted && !bChest)
	{
		SetLifeSpan(1.25f);
	}
	Controller->NotifyInventoryChanged();
	RefreshViewers();
	return true;
}

void AEmberdeepLootContainer::AddViewer(AEmberdeepPlayerController* Controller)
{
	if (Controller)
	{
		Viewers.AddUnique(Controller);
	}
}

void AEmberdeepLootContainer::RemoveViewer(AEmberdeepPlayerController* Controller)
{
	Viewers.Remove(Controller);
}

void AEmberdeepLootContainer::RefreshViewers()
{
	for (int32 ViewerIndex = Viewers.Num() - 1; ViewerIndex >= 0; --ViewerIndex)
	{
		AEmberdeepPlayerController* Viewer = Viewers[ViewerIndex].Get();
		if (!Viewer)
		{
			Viewers.RemoveAtSwap(ViewerIndex);
			continue;
		}
		Viewer->ShowLootWindowFromServer(this, ContainerTitle, LootEntries);
	}
}

void AEmberdeepLootContainer::OnRep_VisualState()
{
	ApplyVisualState();
}

void AEmberdeepLootContainer::ApplyVisualState()
{
	using namespace EmberdeepLootVisuals;
	const float DepletedScale = bDepleted ? 0.62f : 1.0f;
	const FLinearColor WoodDark(0.032f * DepletedScale, 0.012f * DepletedScale, 0.006f * DepletedScale);
	const FLinearColor WoodPanel(0.105f * DepletedScale, 0.029f * DepletedScale, 0.009f * DepletedScale);
	const FLinearColor Iron(0.105f * DepletedScale, 0.090f * DepletedScale, 0.075f * DepletedScale);
	const FLinearColor Leather(0.050f * DepletedScale, 0.020f * DepletedScale, 0.012f * DepletedScale);
	const FLinearColor Steel(0.24f * DepletedScale, 0.22f * DepletedScale, 0.18f * DepletedScale);
	const FLinearColor LegendaryGold(0.88f, 0.32f, 0.018f);
	const FLinearColor NormalGlow(0.44f, 0.095f, 0.008f);
	const FLinearColor LegendaryGlow(1.0f, 0.39f, 0.018f);
	const FLinearColor GlowColor = bLegendaryChest ? LegendaryGlow : NormalGlow;

	SetVoxelMaterial(ChestWoodVoxels, WoodDark);
	SetVoxelMaterial(ChestPanelVoxels, WoodPanel);
	SetVoxelMaterial(ChestLidWoodVoxels, WoodDark);
	SetVoxelMaterial(ChestIronVoxels, Iron, 0.0f, 0.88f);
	SetVoxelMaterial(ChestLidIronVoxels, Iron, 0.0f, 0.88f);
	SetVoxelMaterial(ChestLegendaryVoxels, LegendaryGold, 0.22f, 0.90f);
	SetVoxelMaterial(ChestLidLegendaryVoxels, LegendaryGold, 0.22f, 0.90f);
	SetVoxelMaterial(DropLeatherVoxels, Leather);
	SetVoxelMaterial(DropMetalVoxels, Steel, 0.0f, 0.84f);
	SetVoxelMaterial(DropLegendaryVoxels, LegendaryGold, 0.26f, 0.90f);
	ChestGlowMaterial = SetVoxelMaterial(ChestGlowVoxels, GlowColor, bLegendaryChest ? 1.65f : 0.72f);
	DropGlowMaterial = SetVoxelMaterial(DropGlowVoxels, GlowColor, bLegendaryChest ? 1.65f : 0.72f);

	SetVoxelHidden(ChestWoodVoxels, !bChest);
	SetVoxelHidden(ChestPanelVoxels, !bChest);
	SetVoxelHidden(ChestLidWoodVoxels, !bChest);
	SetVoxelHidden(ChestIronVoxels, !bChest || bLegendaryChest);
	SetVoxelHidden(ChestLidIronVoxels, !bChest || bLegendaryChest);
	SetVoxelHidden(ChestLegendaryVoxels, !bChest || !bLegendaryChest);
	SetVoxelHidden(ChestLidLegendaryVoxels, !bChest || !bLegendaryChest);
	SetVoxelHidden(ChestGlowVoxels, !bChest || bDepleted);

	SetVoxelHidden(DropLeatherVoxels, bChest);
	SetVoxelHidden(DropMetalVoxels, bChest || bLegendaryChest);
	SetVoxelHidden(DropLegendaryVoxels, bChest || !bLegendaryChest);
	SetVoxelHidden(DropGlowVoxels, bChest || bDepleted);

	if (ChestLidPivot)
	{
		ChestLidPivot->SetRelativeRotation(bDepleted
			? FRotator(-38.0f, 0.0f, 0.0f)
			: FRotator::ZeroRotator);
	}

	LootLightBaseIntensity = bDepleted
		? 0.0f
		: bLegendaryChest ? 1850.0f : bChest ? 720.0f : 470.0f;
	if (LootLight)
	{
		LootLight->SetVisibility(!bDepleted);
		LootLight->SetLightColor(GlowColor);
		LootLight->SetIntensity(LootLightBaseIntensity);
		LootLight->SetRelativeLocation(bChest
			? FVector(0.0f, 0.0f, 74.0f)
			: FVector(0.0f, 0.0f, 52.0f));
	}
}

void AEmberdeepLootContainer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEmberdeepLootContainer, ContainerTitle);
	DOREPLIFETIME(AEmberdeepLootContainer, bChest);
	DOREPLIFETIME(AEmberdeepLootContainer, bLegendaryChest);
	DOREPLIFETIME(AEmberdeepLootContainer, bDepleted);
}
