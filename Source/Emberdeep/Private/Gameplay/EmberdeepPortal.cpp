#include "Gameplay/EmberdeepPortal.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Gameplay/EmberdeepGameMode.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace EmberdeepPortalVisuals
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

	void AddSolidBox(
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
					AddVoxel(Mesh, X, Y, Z);
				}
			}
		}
	}

	UMaterialInstanceDynamic* SetVoxelMaterial(
		UInstancedStaticMeshComponent* Mesh,
		const FLinearColor& Color,
		float EmissiveStrength = 0.08f)
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
			Material->SetScalarParameterValue(TEXT("Roughness"), 0.96f);
			Material->SetScalarParameterValue(TEXT("Specular"), 0.035f);
			Material->SetScalarParameterValue(TEXT("EmissiveStrength"), EmissiveStrength);
		}
		return Material;
	}
}

AEmberdeepPortal::AEmberdeepPortal()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	InteractionRange = 270.0f;

	PortalVisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PortalRoot"));
	SetRootComponent(PortalVisualRoot);
	PortalVisualRoot->SetMobility(EComponentMobility::Movable);

	PortalGlowRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PortalGlowRoot"));
	PortalGlowRoot->SetupAttachment(PortalVisualRoot);
	PortalGlowRoot->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> VoxelMaterial(
		TEXT("/Game/Emberdeep/Materials/M_VoxelSurface.M_VoxelSurface"));

	const auto CreateVoxelMesh = [this](
		const TCHAR* Name,
		USceneComponent* Parent,
		bool bCastShadows)
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

	StoneDarkVoxels = CreateVoxelMesh(TEXT("StoneDarkVoxels"), PortalVisualRoot, true);
	StoneMidVoxels = CreateVoxelMesh(TEXT("StoneMidVoxels"), PortalVisualRoot, true);
	StoneEdgeVoxels = CreateVoxelMesh(TEXT("StoneEdgeVoxels"), PortalVisualRoot, true);
	PortalCoreVoxels = CreateVoxelMesh(TEXT("PortalCoreVoxels"), PortalGlowRoot, false);
	PortalSparkVoxels = CreateVoxelMesh(TEXT("PortalSparkVoxels"), PortalGlowRoot, false);

	using namespace EmberdeepPortalVisuals;
	const auto AddStone = [this](int32 X, int32 Y, int32 Z, int32 Salt = 0)
	{
		const uint32 Hash = static_cast<uint32>(
			(X * 73856093) ^ (Y * 19349663) ^ (Z * 83492791) ^ (Salt * 2654435761u));
		UInstancedStaticMeshComponent* PaletteMesh = (Hash % 11u == 0u)
			? StoneEdgeVoxels.Get()
			: (Hash % 4u == 0u ? StoneMidVoxels.Get() : StoneDarkVoxels.Get());
		AddVoxel(PaletteMesh, X, Y, Z);
	};

	// A squat, chipped portal assembled exclusively from 4 cm cells. The arch is
	// broad enough to read at the gameplay camera without becoming a bright wall.
	for (int32 Side : {-1, 1})
	{
		const int32 InnerY = Side < 0 ? -17 : 11;
		const int32 OuterY = Side < 0 ? -11 : 17;
		for (int32 X = -2; X <= 2; ++X)
		{
			for (int32 Y = InnerY; Y <= OuterY; ++Y)
			{
				for (int32 Z = 3; Z <= 31; ++Z)
				{
					const bool bShell = X == -2 || X == 2 || Y == InnerY || Y == OuterY;
					const bool bCoursedFace = Z % 5 == 0;
					if (bShell || bCoursedFace)
					{
						AddStone(X, Y, Z, Side);
					}
				}
			}
		}

		AddSolidBox(StoneDarkVoxels, -3, 3, Side < 0 ? -19 : 9, Side < 0 ? -9 : 19, 0, 2);
		AddSolidBox(StoneMidVoxels, -3, 3, Side < 0 ? -18 : 10, Side < 0 ? -10 : 18, 30, 32);
	}

	for (int32 X = -2; X <= 2; ++X)
	{
		for (int32 Y = -17; Y <= 17; ++Y)
		{
			for (int32 Z = 32; Z <= 37; ++Z)
			{
				const bool bShell = X == -2 || X == 2 || Z == 32 || Z == 37;
				if (bShell || Y % 6 == 0)
				{
					AddStone(X, Y, Z, 7);
				}
			}
		}
	}

	// Broken capstones and offset side teeth keep the silhouette handcrafted.
	for (int32 Y = -16; Y <= 16; Y += 8)
	{
		AddSolidBox(StoneEdgeVoxels, -2, 2, Y, Y + 2, 38, 39);
	}
	for (int32 Z = 7; Z <= 27; Z += 10)
	{
		AddStone(-3, -18, Z, 13);
		AddStone(3, 18, Z + 2, 17);
	}

	// The effect itself is a sparse rune-ring rather than a luminous rectangle.
	// Glow is an allowed lattice exception, but these marks still use the same cell.
	for (int32 Z = 5; Z <= 28; ++Z)
	{
		const float NormalizedZ = (static_cast<float>(Z) - 16.5f) / 11.5f;
		const int32 RadiusY = FMath::RoundToInt(8.5f * FMath::Sqrt(FMath::Max(0.0f, 1.0f - NormalizedZ * NormalizedZ)));
		AddVoxel(PortalCoreVoxels, 0, -RadiusY, Z);
		if (RadiusY > 0)
		{
			AddVoxel(PortalCoreVoxels, 0, RadiusY, Z);
		}
	}
	for (int32 Z = 9; Z <= 24; Z += 5)
	{
		AddVoxel(PortalSparkVoxels, -1, 0, Z);
	}
	for (const FIntVector& Cell : {
		FIntVector(-1, -5, 4), FIntVector(1, 5, 7), FIntVector(0, -8, 12),
		FIntVector(1, 7, 20), FIntVector(-1, -4, 25), FIntVector(0, 3, 29)})
	{
		AddVoxel(PortalSparkVoxels, Cell.X, Cell.Y, Cell.Z);
	}

	PortalLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PortalLight"));
	PortalLight->SetupAttachment(PortalGlowRoot);
	PortalLight->SetRelativeLocation(FVector(0.0f, 0.0f, 72.0f));
	PortalLight->SetIntensity(PortalLightBaseIntensity);
	PortalLight->SetAttenuationRadius(250.0f);
	PortalLight->SetCastShadows(false);
	PortalLight->SetVolumetricScatteringIntensity(0.18f);
}

void AEmberdeepPortal::BeginPlay()
{
	Super::BeginPlay();
	ApplyPortalVisuals();
}

void AEmberdeepPortal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	PortalVisualTime = FMath::Fmod(PortalVisualTime + DeltaSeconds, 1000.0f);
	const float Pulse = 0.90f + 0.10f * FMath::Sin(PortalVisualTime * 2.7f);
	const float Drift = 0.65f * FMath::Sin(PortalVisualTime * 1.35f);
	const float UniformScale = 0.995f + Pulse * 0.008f;

	if (PortalGlowRoot)
	{
		PortalGlowRoot->SetRelativeLocation(FVector(Drift, 0.0f, 0.0f));
		PortalGlowRoot->SetRelativeScale3D(FVector(UniformScale));
	}
	if (PortalLight)
	{
		PortalLight->SetIntensity(PortalLightBaseIntensity * Pulse);
	}
	if (PortalCoreMaterial)
	{
		PortalCoreMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), 0.85f + Pulse * 0.45f);
	}
	if (PortalSparkMaterial)
	{
		PortalSparkMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), 1.35f + Pulse * 0.75f);
	}
}

void AEmberdeepPortal::Configure(EEmberdeepRunStage NewDestination, const FString& NewLabel)
{
	if (!HasAuthority())
	{
		return;
	}
	Destination = NewDestination;
	InteractionLabel = NewLabel;
	ApplyPortalVisuals();
}

FString AEmberdeepPortal::GetInteractionPrompt(const AEmberdeepCharacter* Character) const
{
	return InteractionLabel;
}

void AEmberdeepPortal::Interact(AEmberdeepCharacter* Character, AEmberdeepPlayerController* Controller)
{
	if (HasAuthority())
	{
		if (AEmberdeepGameMode* GameMode = GetWorld()->GetAuthGameMode<AEmberdeepGameMode>())
		{
			GameMode->EnterRunStage(Destination);
		}
	}
}

void AEmberdeepPortal::OnRep_PortalState()
{
	ApplyPortalVisuals();
}

void AEmberdeepPortal::ApplyPortalVisuals()
{
	using namespace EmberdeepPortalVisuals;
	SetVoxelMaterial(StoneDarkVoxels, FLinearColor(0.018f, 0.014f, 0.018f));
	SetVoxelMaterial(StoneMidVoxels, FLinearColor(0.042f, 0.033f, 0.041f));
	SetVoxelMaterial(StoneEdgeVoxels, FLinearColor(0.075f, 0.060f, 0.068f));

	const FLinearColor GlowColor = Destination == EEmberdeepRunStage::RewardRoom
		? FLinearColor(0.42f, 0.035f, 0.17f)
		: Destination == EEmberdeepRunStage::Camp
			? FLinearColor(0.075f, 0.13f, 0.31f)
			: FLinearColor(0.25f, 0.018f, 0.40f);
	const FLinearColor SparkColor = Destination == EEmberdeepRunStage::RewardRoom
		? FLinearColor(0.68f, 0.075f, 0.22f)
		: Destination == EEmberdeepRunStage::Camp
			? FLinearColor(0.12f, 0.27f, 0.58f)
			: FLinearColor(0.48f, 0.055f, 0.72f);

	PortalLightBaseIntensity = Destination == EEmberdeepRunStage::RewardRoom
		? 1050.0f
		: Destination == EEmberdeepRunStage::Camp ? 720.0f : 860.0f;
	PortalCoreMaterial = SetVoxelMaterial(PortalCoreVoxels, GlowColor, 1.2f);
	PortalSparkMaterial = SetVoxelMaterial(PortalSparkVoxels, SparkColor, 2.0f);
	if (PortalLight)
	{
		PortalLight->SetLightColor(GlowColor);
		PortalLight->SetIntensity(PortalLightBaseIntensity);
	}
}

void AEmberdeepPortal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEmberdeepPortal, Destination);
	DOREPLIFETIME(AEmberdeepPortal, InteractionLabel);
}
