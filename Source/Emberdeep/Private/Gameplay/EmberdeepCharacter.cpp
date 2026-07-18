#include "Gameplay/EmberdeepCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Emberdeep.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Gameplay/EmberdeepHealthComponent.h"
#include "Gameplay/EmberdeepCombatFeedback.h"
#include "Gameplay/EmberdeepEnemy.h"
#include "Gameplay/EmberdeepGameMode.h"
#include "Gameplay/EmberdeepPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Visual/EmberdeepVoxelStyle.h"

namespace
{
	enum class EThorgrimPart : uint8
	{
		Body,
		Axe,
		Shield
	};

	enum class EThorgrimPalette : uint8
	{
		Night,
		Steel,
		Ash,
		Brass,
		Bone,
		Hide,
		Fur,
		Skin,
		Wood,
		Cloth,
		Crimson
	};

	struct FThorgrimPartDefinition
	{
		EThorgrimPart Part;
		const TCHAR* Name;
	};

	struct FThorgrimPaletteDefinition
	{
		EThorgrimPalette Palette;
		const TCHAR* Name;
		FLinearColor Shades[EmberdeepVoxelStyle::ShadeCount];
	};

	struct FThorgrimVoxelCell
	{
		EThorgrimPart Part;
		EThorgrimPalette Palette;
		int16 X;
		int16 Y;
		int16 Z;
	};

	enum class EThorgrimRigidBone : uint8
	{
		Torso,
		Head,
		LeftArm,
		RightArm,
		LeftLeg,
		RightLeg,
		Count
	};

	struct FPlayableVoxelBonePose
	{
		FQuat Rotation;
		FVector Translation;
	};

	struct FPlayableVoxelAnimationClip
	{
		const TCHAR* Name;
		bool bLoop;
		float Duration;
		float SamplesPerSecond;
		int32 FrameCount;
		const FPlayableVoxelBonePose* Frames;
	};

	struct FPlayableEquipmentVisual
	{
		const TCHAR* DefinitionId;
		const FThorgrimVoxelCell* Cells;
		int32 CellCount;
	};

	static constexpr int32 GPlayableAnimationBoneCount = 8;

	const FThorgrimPartDefinition GThorgrimPartDefinitions[] =
	{
		{ EThorgrimPart::Body, TEXT("Body") },
		{ EThorgrimPart::Axe, TEXT("Axe") },
		{ EThorgrimPart::Shield, TEXT("Shield") }
	};

	const FThorgrimPaletteDefinition GThorgrimPaletteDefinitions[] =
	{
		{ EThorgrimPalette::Night, TEXT("Night"), {
			FLinearColor(0.006f, 0.010f, 0.016f), FLinearColor(0.014f, 0.022f, 0.034f), FLinearColor(0.025f, 0.040f, 0.060f) } },
		{ EThorgrimPalette::Steel, TEXT("Steel"), {
			FLinearColor(0.085f, 0.115f, 0.150f), FLinearColor(0.16f, 0.215f, 0.275f), FLinearColor(0.30f, 0.39f, 0.48f) } },
		{ EThorgrimPalette::Ash, TEXT("Ash"), {
			FLinearColor(0.22f, 0.25f, 0.28f), FLinearColor(0.36f, 0.40f, 0.44f), FLinearColor(0.56f, 0.61f, 0.64f) } },
		{ EThorgrimPalette::Brass, TEXT("Brass"), {
			FLinearColor(0.16f, 0.065f, 0.010f), FLinearColor(0.34f, 0.15f, 0.025f), FLinearColor(0.58f, 0.32f, 0.060f) } },
		{ EThorgrimPalette::Bone, TEXT("Bone"), {
			FLinearColor(0.42f, 0.34f, 0.23f), FLinearColor(0.63f, 0.52f, 0.36f), FLinearColor(0.80f, 0.70f, 0.52f) } },
		{ EThorgrimPalette::Hide, TEXT("Hide"), {
			FLinearColor(0.12f, 0.045f, 0.018f), FLinearColor(0.25f, 0.10f, 0.035f), FLinearColor(0.39f, 0.18f, 0.065f) } },
		{ EThorgrimPalette::Fur, TEXT("Fur"), {
			FLinearColor(0.028f, 0.014f, 0.010f), FLinearColor(0.060f, 0.032f, 0.021f), FLinearColor(0.11f, 0.058f, 0.033f) } },
		{ EThorgrimPalette::Skin, TEXT("Skin"), {
			FLinearColor(0.25f, 0.075f, 0.040f), FLinearColor(0.43f, 0.15f, 0.075f), FLinearColor(0.62f, 0.26f, 0.13f) } },
		{ EThorgrimPalette::Wood, TEXT("Wood"), {
			FLinearColor(0.032f, 0.013f, 0.008f), FLinearColor(0.075f, 0.030f, 0.014f), FLinearColor(0.14f, 0.060f, 0.025f) } },
		{ EThorgrimPalette::Cloth, TEXT("Cloth"), {
			FLinearColor(0.012f, 0.027f, 0.050f), FLinearColor(0.025f, 0.060f, 0.11f), FLinearColor(0.055f, 0.12f, 0.20f) } },
		{ EThorgrimPalette::Crimson, TEXT("Crimson"), {
			FLinearColor(0.16f, 0.003f, 0.006f), FLinearColor(0.40f, 0.010f, 0.014f), FLinearColor(0.72f, 0.026f, 0.020f) } }
	};

	int32 PositiveModuloCharacter(int32 Value, int32 Divisor)
	{
		const int32 Result = Value % Divisor;
		return Result < 0 ? Result + Divisor : Result;
	}

	uint32 HashThorgrimCell(int32 X, int32 Y, int32 Z, int32 Seed)
	{
		uint32 Hash = 2166136261u;
		Hash = (Hash ^ static_cast<uint32>(X)) * 16777619u;
		Hash = (Hash ^ static_cast<uint32>(Y)) * 16777619u;
		Hash = (Hash ^ static_cast<uint32>(Z)) * 16777619u;
		return (Hash ^ static_cast<uint32>(Seed)) * 16777619u;
	}

	int32 SelectThorgrimMaterialShade(const FThorgrimVoxelCell& Voxel)
	{
		const int32 PaletteSeed = static_cast<int32>(Voxel.Palette) + static_cast<int32>(Voxel.Part) * 17;
		const int32 ClusterX = FMath::FloorToInt(static_cast<float>(Voxel.X) / 2.0f);
		const int32 ClusterY = FMath::FloorToInt(static_cast<float>(Voxel.Y) / 2.0f);
		const int32 ClusterZ = FMath::FloorToInt(static_cast<float>(Voxel.Z) / 2.0f);
		const uint32 CellHash = HashThorgrimCell(Voxel.X, Voxel.Y, Voxel.Z, PaletteSeed);
		const uint32 ClusterHash = HashThorgrimCell(ClusterX, ClusterY, ClusterZ, PaletteSeed);

		switch (Voxel.Palette)
		{
		case EThorgrimPalette::Fur:
			// Two-cell patches read as matted fur clumps instead of random noise.
			if (ClusterHash % 11u < 4u)
			{
				return 0;
			}
			return ClusterHash % 13u == 12u || PositiveModuloCharacter(Voxel.Y + Voxel.Z, 11) == 0 ? 2 : 1;

		case EThorgrimPalette::Hide:
			if (PositiveModuloCharacter(Voxel.X + Voxel.Z * 2, 13) == 0)
			{
				return 2;
			}
			return ClusterHash % 9u < 2u ? 0 : 1;

		case EThorgrimPalette::Wood:
			if (PositiveModuloCharacter(Voxel.Y + Voxel.Z * 2, 9) == 0)
			{
				return 2;
			}
			return PositiveModuloCharacter(Voxel.Y + Voxel.Z * 2, 9) <= 2 ? 0 : 1;

		case EThorgrimPalette::Steel:
		case EThorgrimPalette::Ash:
		case EThorgrimPalette::Brass:
		case EThorgrimPalette::Bone:
			if (PositiveModuloCharacter(Voxel.X + Voxel.Y * 2 + Voxel.Z * 3, 17) == 0)
			{
				return 2;
			}
			return CellHash % 12u < 2u ? 0 : 1;

		case EThorgrimPalette::Night:
		case EThorgrimPalette::Cloth:
		case EThorgrimPalette::Crimson:
			if (PositiveModuloCharacter(Voxel.X + Voxel.Y + Voxel.Z, 15) == 0)
			{
				return 2;
			}
			return ClusterHash % 13u < 2u ? 0 : 1;

		case EThorgrimPalette::Skin:
			return CellHash % 16u == 0u ? 0 : (CellHash % 16u == 15u ? 2 : 1);

		default:
			return 1;
		}
	}

	int32 GetThorgrimMeshKey(EThorgrimPart Part, EThorgrimPalette Palette, int32 ShadeIndex)
	{
		return (
			static_cast<int32>(Part) * UE_ARRAY_COUNT(GThorgrimPaletteDefinitions)
			+ static_cast<int32>(Palette))
			* EmberdeepVoxelStyle::ShadeCount
			+ ShadeIndex;
	}

	EThorgrimRigidBone SelectThorgrimRigidBone(const FThorgrimVoxelCell& Voxel)
	{
		if (Voxel.Part != EThorgrimPart::Body)
		{
			return EThorgrimRigidBone::Torso;
		}
		if (Voxel.Z >= 11 && FMath::Abs(static_cast<int32>(Voxel.Y)) <= 8)
		{
			return EThorgrimRigidBone::Head;
		}
		if (Voxel.Z <= -6)
		{
			return Voxel.Y < 0 ? EThorgrimRigidBone::LeftLeg : EThorgrimRigidBone::RightLeg;
		}
		if (Voxel.Z <= 5 && Voxel.Y <= -9)
		{
			return EThorgrimRigidBone::LeftArm;
		}
		if (Voxel.Z <= 5 && Voxel.Y >= 9)
		{
			return EThorgrimRigidBone::RightArm;
		}
		return EThorgrimRigidBone::Torso;
	}

	FVector GetThorgrimBonePivot(EThorgrimRigidBone Bone)
	{
		switch (Bone)
		{
		case EThorgrimRigidBone::Head:
			return FVector(8.0f, 0.0f, 46.0f);
		case EThorgrimRigidBone::LeftArm:
			return FVector(4.0f, -40.0f, 20.0f);
		case EThorgrimRigidBone::RightArm:
			return FVector(4.0f, 40.0f, 20.0f);
		case EThorgrimRigidBone::LeftLeg:
			return FVector(4.0f, -16.0f, -20.0f);
		case EThorgrimRigidBone::RightLeg:
			return FVector(4.0f, 16.0f, -20.0f);
		default:
			return FVector(4.0f, 0.0f, 0.0f);
		}
	}

	namespace FighterVoxelData
	{
#include "FighterVoxelData.inl"
#include "FighterAnimationData.inl"
	}

	namespace FighterEquipmentData
	{
#include "FighterEquipmentData.inl"

		const FPlayableEquipmentVisual& FindMainHandVisual(FName DefinitionId)
		{
			for (const FPlayableEquipmentVisual& Visual : GMainHandEquipmentVisuals)
			{
				if (DefinitionId == FName(Visual.DefinitionId))
				{
					return Visual;
				}
			}
			return GMainHandEquipmentVisuals[0];
		}
	}

	namespace ThorgrimVoxelData
	{
#include "ThorgrimVoxelData.inl"
		static constexpr const TCHAR* GPlayableVoxelCharacterName = TEXT("Thorgrim");
		static const FQuat GPlayableAxeRestingRotation = FQuat::Identity;
		static const FQuat GPlayableShieldRestingRotation = FQuat::Identity;
	}

	static_assert(FighterVoxelData::GThorgrimVoxelUnitCm == EmberdeepVoxelStyle::UnitCm);
	static_assert(ThorgrimVoxelData::GThorgrimVoxelUnitCm == EmberdeepVoxelStyle::UnitCm);
}

AEmberdeepCharacter::AEmberdeepCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 80.0f);
	GetCharacterMovement()->MaxWalkSpeed = 500.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2200.0f;
	GetCharacterMovement()->GroundFriction = 9.0f;
	// Movement and aim are intentionally independent. WASD controls velocity;
	// the mouse controls the character's full 360-degree facing direction.
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 900.0f, 0.0f);
	bUseControllerRotationYaw = false;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1150.0f;
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->SetRelativeRotation(FRotator(-55.0f, -45.0f, 0.0f));
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = false;

	IsometricCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("IsometricCamera"));
	IsometricCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	IsometricCamera->ProjectionMode = ECameraProjectionMode::Orthographic;
	IsometricCamera->OrthoWidth = 1100.0f;
	IsometricCamera->PostProcessBlendWeight = 1.0f;
	IsometricCamera->PostProcessSettings.bOverride_AutoExposureMethod = true;
	IsometricCamera->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	IsometricCamera->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	IsometricCamera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = 0;
	IsometricCamera->PostProcessSettings.bOverride_AutoExposureBias = true;
	IsometricCamera->PostProcessSettings.AutoExposureBias = 1.60f;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ProjectVoxelMaterial(
		TEXT("/Game/Emberdeep/Materials/M_VoxelSurface.M_VoxelSurface"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FallbackVoxelMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	UMaterialInterface* VoxelMaterial = ProjectVoxelMaterial.Succeeded()
		? ProjectVoxelMaterial.Object
		: FallbackVoxelMaterial.Object;

	// Gameplay rotation and collision remain on the replicated Character root. All
	// authored geometry lives below a cosmetic root so every peer can derive the
	// same deliberately stepped miniature animation from replicated movement.
	ThorgrimVisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ThorgrimVisualRoot"));
	ThorgrimVisualRoot->SetupAttachment(RootComponent);
	ThorgrimVisualRoot->SetUsingAbsoluteRotation(true);

	ThorgrimBodyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ThorgrimBodyRoot"));
	ThorgrimBodyRoot->SetupAttachment(ThorgrimVisualRoot);

	ThorgrimAxeRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ThorgrimAxeRoot"));
	ThorgrimAxeRoot->SetupAttachment(ThorgrimVisualRoot);
	ThorgrimAxeRoot->SetRelativeLocation(FighterVoxelData::GThorgrimAxePivot);
	ThorgrimAxeRoot->SetRelativeRotation(FighterVoxelData::GPlayableAxeRestingRotation.Rotator());

	ThorgrimShieldRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ThorgrimShieldRoot"));
	ThorgrimShieldRoot->SetupAttachment(ThorgrimVisualRoot);
	ThorgrimShieldRoot->SetRelativeLocation(FighterVoxelData::GThorgrimShieldPivot);
	ThorgrimShieldRoot->SetRelativeRotation(FighterVoxelData::GPlayableShieldRestingRotation.Rotator());

	ThorgrimRestTransformsByMesh.SetNum(
		UE_ARRAY_COUNT(GThorgrimPartDefinitions)
		* UE_ARRAY_COUNT(GThorgrimPaletteDefinitions)
		* EmberdeepVoxelStyle::ShadeCount);
	ThorgrimRigidBonesByMesh.SetNum(ThorgrimRestTransformsByMesh.Num());
	for (const FThorgrimPartDefinition& PartDefinition : GThorgrimPartDefinitions)
	{
		USceneComponent* PartRoot = ThorgrimBodyRoot;
		if (PartDefinition.Part == EThorgrimPart::Axe)
		{
			PartRoot = ThorgrimAxeRoot;
		}
		else if (PartDefinition.Part == EThorgrimPart::Shield)
		{
			PartRoot = ThorgrimShieldRoot;
		}

		for (const FThorgrimPaletteDefinition& PaletteDefinition : GThorgrimPaletteDefinitions)
		{
			for (int32 ShadeIndex = 0; ShadeIndex < EmberdeepVoxelStyle::ShadeCount; ++ShadeIndex)
			{
				const FName ComponentName(*FString::Printf(
					TEXT("Thorgrim%s%sShade%d"),
					PartDefinition.Name,
					PaletteDefinition.Name,
					ShadeIndex));
				UInstancedStaticMeshComponent* PaletteMesh =
					CreateDefaultSubobject<UInstancedStaticMeshComponent>(ComponentName);
				PaletteMesh->SetupAttachment(PartRoot);
				PaletteMesh->SetMobility(EComponentMobility::Movable);
				PaletteMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				PaletteMesh->SetGenerateOverlapEvents(false);
				PaletteMesh->SetCanEverAffectNavigation(false);
				PaletteMesh->SetCastShadow(true);
				PaletteMesh->SetStaticMesh(CubeMesh.Object);
				PaletteMesh->SetMaterial(0, VoxelMaterial);
				ThorgrimPaletteMeshes.Add(PaletteMesh);
			}
		}
	}

	RebuildPlayableVoxelCharacter(false);

	HealthComponent = CreateDefaultSubobject<UEmberdeepHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->SetMaxHealth(190.0f);

}

void AEmberdeepCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateVisualPresentation(DeltaSeconds);
	UpdateThorgrimAnimation(DeltaSeconds);
	if (HasAuthority()
		&& (bBasicAttackQueued || bBasicAttackHeld)
		&& !IsDead()
		&& GetWorld()->GetTimeSeconds() >= NextAttackTime)
	{
		const FVector AttackDirection = bBasicAttackHeld
			? FVector(ReplicatedAimDirection).GetSafeNormal2D()
			: QueuedBasicAttackDirection;
		ExecuteAttack(false, AttackDirection);
	}

	if (IsLocallyControlled())
	{
		if (!IsDead())
		{
			UpdateMouseAim();
		}
		IsometricCamera->OrthoWidth = FMath::FInterpTo(
			IsometricCamera->OrthoWidth,
			TargetOrthoWidth,
			DeltaSeconds,
			ZoomInterpolationSpeed);
		UpdateLocalCameraShake(DeltaSeconds);
	}
}

void AEmberdeepCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	ApplyEquipmentStats();
}

void AEmberdeepCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	ApplyEquipmentStats();
}

void AEmberdeepCharacter::BeginPlay()
{
	Super::BeginPlay();

	int32 ThorgrimInstanceCount = 0;
	int32 ThorgrimMaterialCount = 0;
	for (int32 MeshIndex = 0; MeshIndex < ThorgrimPaletteMeshes.Num(); ++MeshIndex)
	{
		UInstancedStaticMeshComponent* PaletteMesh = ThorgrimPaletteMeshes[MeshIndex];
		if (PaletteMesh)
		{
			ThorgrimInstanceCount += PaletteMesh->GetInstanceCount();
			const int32 ShadeIndex = MeshIndex % EmberdeepVoxelStyle::ShadeCount;
			const int32 PaletteIndex =
				(MeshIndex / EmberdeepVoxelStyle::ShadeCount) % UE_ARRAY_COUNT(GThorgrimPaletteDefinitions);
			if (UMaterialInstanceDynamic* Material = PaletteMesh->CreateDynamicMaterialInstance(0))
			{
				const FLinearColor BaseColor = GThorgrimPaletteDefinitions[PaletteIndex].Shades[ShadeIndex];
				Material->SetVectorParameterValue(
					TEXT("Color"),
					BaseColor);
				// A small presentation lift keeps the miniature readable between torch
				// pools without making the armour self-lit.
				Material->SetScalarParameterValue(TEXT("EmissiveStrength"), 0.08f);
				PaletteMesh->SetMaterial(0, Material);
				++ThorgrimMaterialCount;
				ThorgrimMaterials.Add(Material);
				ThorgrimMaterialBaseColors.Add(BaseColor);
			}
		}
	}
	UE_LOG(
		LogEmberdeep,
		Display,
		TEXT("EMBERDEEP_VISUAL %sGenerated Instances=%d Materials=%d"),
		*PlayableVoxelCharacterName,
		ThorgrimInstanceCount,
		ThorgrimMaterialCount);

	ThorgrimVisualRestingLocation = ThorgrimVisualRoot->GetRelativeLocation();
	ThorgrimVisualRestingRotation = ThorgrimVisualRoot->GetRelativeRotation();
	VisualFacingYaw = GetActorRotation().Yaw;
	ThorgrimBodyRestingLocation = ThorgrimBodyRoot->GetRelativeLocation();
	ThorgrimAxeRestingRotation = ThorgrimAxeRoot->GetRelativeRotation();
	ThorgrimAxeRestingLocation = ThorgrimAxeRoot->GetRelativeLocation();
	ThorgrimShieldRestingRotation = ThorgrimShieldRoot->GetRelativeRotation();
	ThorgrimShieldRestingLocation = ThorgrimShieldRoot->GetRelativeLocation();
	HealthComponent->OnDeath.AddUObject(this, &AEmberdeepCharacter::HandleDeath);

	if (HasAuthority() && GetActorLocation().Z < 50.0f)
	{
		SetActorLocation(FVector(0.0f, -430.0f, 85.0f));
	}
	ApplyEquipmentStats();
}

void AEmberdeepCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AEmberdeepCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AEmberdeepCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("ZoomCamera"), this, &AEmberdeepCharacter::ZoomCamera);
	PlayerInputComponent->BindAction(TEXT("BasicAttack"), IE_Pressed, this, &AEmberdeepCharacter::BasicAttack);
	PlayerInputComponent->BindAction(TEXT("BasicAttack"), IE_Released, this, &AEmberdeepCharacter::StopBasicAttack);
	PlayerInputComponent->BindAction(TEXT("HeavyAttack"), IE_Pressed, this, &AEmberdeepCharacter::HeavyAttack);
	PlayerInputComponent->BindAction(TEXT("Dodge"), IE_Pressed, this, &AEmberdeepCharacter::Dodge);
	PlayerInputComponent->BindAction(TEXT("SwitchCharacter"), IE_Pressed, this, &AEmberdeepCharacter::TogglePlayableCharacter);
}

void AEmberdeepCharacter::TogglePlayableCharacter()
{
	RebuildPlayableVoxelCharacter(!bUsingThorgrimVisual);
}

void AEmberdeepCharacter::RebuildPlayableVoxelCharacter(bool bUseThorgrim)
{
	const FThorgrimVoxelCell* VoxelCells = bUseThorgrim
		? ThorgrimVoxelData::GThorgrimVoxelCells
		: FighterVoxelData::GThorgrimVoxelCells;
	const int32 VoxelCount = bUseThorgrim
		? ThorgrimVoxelData::GThorgrimVoxelCount
		: FighterVoxelData::GThorgrimVoxelCount;
	const FVector AxePivot = bUseThorgrim
		? ThorgrimVoxelData::GThorgrimAxePivot
		: FighterVoxelData::GThorgrimAxePivot;
	const FVector ShieldPivot = bUseThorgrim
		? ThorgrimVoxelData::GThorgrimShieldPivot
		: FighterVoxelData::GThorgrimShieldPivot;
	const FQuat AxeRotation = bUseThorgrim
		? ThorgrimVoxelData::GPlayableAxeRestingRotation
		: FighterVoxelData::GPlayableAxeRestingRotation;
	const FQuat ShieldRotation = bUseThorgrim
		? ThorgrimVoxelData::GPlayableShieldRestingRotation
		: FighterVoxelData::GPlayableShieldRestingRotation;

	for (TArray<FTransform>& RestTransforms : ThorgrimRestTransformsByMesh)
	{
		RestTransforms.Reset();
	}
	for (TArray<uint8>& RigidBones : ThorgrimRigidBonesByMesh)
	{
		RigidBones.Reset();
	}
	for (UInstancedStaticMeshComponent* PaletteMesh : ThorgrimPaletteMeshes)
	{
		if (PaletteMesh)
		{
			PaletteMesh->ClearInstances();
		}
	}

	const auto AddVoxel = [this](const FThorgrimVoxelCell& Voxel)
	{
		const int32 ShadeIndex = SelectThorgrimMaterialShade(Voxel);
		const int32 MeshKey = GetThorgrimMeshKey(Voxel.Part, Voxel.Palette, ShadeIndex);
		ThorgrimRestTransformsByMesh[MeshKey].Add(FTransform(
			FQuat::Identity,
			EmberdeepVoxelStyle::CellCenter(Voxel.X, Voxel.Y, Voxel.Z),
			EmberdeepVoxelStyle::InstanceScale()));
		ThorgrimRigidBonesByMesh[MeshKey].Add(static_cast<uint8>(SelectThorgrimRigidBone(Voxel)));
	};
	for (int32 VoxelIndex = 0; VoxelIndex < VoxelCount; ++VoxelIndex)
	{
		AddVoxel(VoxelCells[VoxelIndex]);
	}

	int32 EquipmentVoxelCount = 0;
	if (!bUseThorgrim)
	{
		const FPlayableEquipmentVisual& MainHand =
			FighterEquipmentData::FindMainHandVisual(EquippedWeaponVisualId);
		for (int32 VoxelIndex = 0; VoxelIndex < MainHand.CellCount; ++VoxelIndex)
		{
			AddVoxel(MainHand.Cells[VoxelIndex]);
		}
		for (int32 VoxelIndex = 0; VoxelIndex < FighterEquipmentData::GDefaultOffHandVisual.CellCount; ++VoxelIndex)
		{
			AddVoxel(FighterEquipmentData::GDefaultOffHandVisual.Cells[VoxelIndex]);
		}
		EquipmentVoxelCount = MainHand.CellCount + FighterEquipmentData::GDefaultOffHandVisual.CellCount;
	}
	for (int32 MeshIndex = 0; MeshIndex < ThorgrimPaletteMeshes.Num(); ++MeshIndex)
	{
		if (ThorgrimPaletteMeshes[MeshIndex] && ThorgrimRestTransformsByMesh.IsValidIndex(MeshIndex))
		{
			ThorgrimPaletteMeshes[MeshIndex]->AddInstances(
				ThorgrimRestTransformsByMesh[MeshIndex], false, false, false);
		}
	}

	bUsingThorgrimVisual = bUseThorgrim;
	PlayableVoxelCharacterName = bUseThorgrim
		? ThorgrimVoxelData::GPlayableVoxelCharacterName
		: FighterVoxelData::GPlayableVoxelCharacterName;
	PlayableAxePivot = AxePivot;
	PlayableShieldPivot = ShieldPivot;
	ThorgrimAxeRoot->SetRelativeLocation(PlayableAxePivot);
	ThorgrimAxeRoot->SetRelativeRotation(AxeRotation.Rotator());
	ThorgrimShieldRoot->SetRelativeLocation(PlayableShieldPivot);
	ThorgrimShieldRoot->SetRelativeRotation(ShieldRotation.Rotator());
	ThorgrimAxeRestingRotation = ThorgrimAxeRoot->GetRelativeRotation();
	ThorgrimShieldRestingRotation = ThorgrimShieldRoot->GetRelativeRotation();
	ThorgrimAttackTimeRemaining = 0.0f;
	ThorgrimAttackDuration = 0.0f;
	ThorgrimDodgeTimeRemaining = 0.0f;
	ThorgrimPoseAccumulator = 1.0f;

	UE_LOG(
		LogEmberdeep,
		Display,
		TEXT("EMBERDEEP_VISUAL SwitchedCharacter Name=%s Instances=%d"),
		*PlayableVoxelCharacterName,
		VoxelCount + EquipmentVoxelCount);
}

void AEmberdeepCharacter::MoveForward(float Value)
{
	if (!FMath::IsNearlyZero(Value))
	{
		FVector ScreenUp = IsometricCamera->GetUpVector();
		ScreenUp.Z = 0.0f;
		AddMovementInput(ScreenUp.GetSafeNormal(), Value);
	}
}

void AEmberdeepCharacter::MoveRight(float Value)
{
	if (!FMath::IsNearlyZero(Value))
	{
		FVector ScreenRight = IsometricCamera->GetRightVector();
		ScreenRight.Z = 0.0f;
		AddMovementInput(ScreenRight.GetSafeNormal(), Value);
	}
}

void AEmberdeepCharacter::ZoomCamera(float Value)
{
	if (!FMath::IsNearlyZero(Value))
	{
		// Positive wheel input zooms in; negative input zooms out.
		TargetOrthoWidth = FMath::Clamp(
			TargetOrthoWidth - Value * ZoomStep,
			MinimumOrthoWidth,
			MaximumOrthoWidth);
	}
}

void AEmberdeepCharacter::UpdateVisualPresentation(float DeltaSeconds)
{
	(void)DeltaSeconds;
	ApplySteppedVisualPose(false);
}

void AEmberdeepCharacter::ApplySteppedVisualPose(bool bForce)
{
	if (!ThorgrimVisualRoot || !ThorgrimBodyRoot || !ThorgrimAxeRoot || !ThorgrimShieldRoot || !GetWorld())
	{
		return;
	}

	// The simulation stays full-rate. Only the rigid visual clusters hold poses at
	// 12 fps, matching the low-resolution miniature presentation in the target.
	constexpr float VisualPoseRate = 12.0f;
	const float Now = GetWorld()->GetTimeSeconds();
	const int32 PoseStep = FMath::FloorToInt(Now * VisualPoseRate);
	if (!bForce && PoseStep == LastVisualPoseStep)
	{
		return;
	}
	LastVisualPoseStep = PoseStep;
	const float PoseTime = static_cast<float>(PoseStep) / VisualPoseRate;
	VisualFacingYaw = bForce
		? GetActorRotation().Yaw
		: FMath::FixedTurn(VisualFacingYaw, GetActorRotation().Yaw, 60.0f);

	const FVector PlanarVelocity(GetVelocity().X, GetVelocity().Y, 0.0f);
	const float MoveAlpha = FMath::Clamp(
		PlanarVelocity.Size() / FMath::Max(GetCharacterMovement()->MaxWalkSpeed, 1.0f),
		0.0f,
		1.0f);
	const FVector LocalVelocity = GetActorTransform().InverseTransformVectorNoScale(
		PlanarVelocity.GetSafeNormal());
	const float Gait = FMath::Sin(PoseTime * UE_PI * 6.0f);
	const float GaitLift = FMath::Abs(Gait);
	const float IdleBreath = FMath::Sin(PoseTime * UE_PI * 1.55f);

	FVector VisualOffset(0.0f, 0.0f, FMath::Lerp(IdleBreath * 1.25f, GaitLift * 3.25f, MoveAlpha));
	FRotator VisualRotation(
		-LocalVelocity.X * 3.8f * MoveAlpha,
		0.0f,
		LocalVelocity.Y * 4.2f * MoveAlpha + Gait * 1.2f * MoveAlpha);
	FVector BodyOffset(0.0f, 0.0f, IdleBreath * (1.0f - MoveAlpha) * 0.65f);
	FRotator AxeRotation = ThorgrimAxeRestingRotation
		+ FRotator(Gait * 1.5f * MoveAlpha, 0.0f, -Gait * 4.5f * MoveAlpha);
	FVector AxeOffset = ThorgrimAxeRestingLocation + FVector(0.0f, 0.0f, -Gait * 1.4f * MoveAlpha);
	FRotator ShieldRotation = ThorgrimShieldRestingRotation
		+ FRotator(-Gait * 1.0f * MoveAlpha, 0.0f, Gait * 3.0f * MoveAlpha);
	FVector ShieldOffset = ThorgrimShieldRestingLocation + FVector(0.0f, 0.0f, Gait * 1.0f * MoveAlpha);

	const float AttackElapsed = Now - VisualAttackStartTime;
	if (AttackElapsed >= 0.0f && AttackElapsed < VisualAttackDuration)
	{
		const float AttackAlpha = FMath::Clamp(AttackElapsed / FMath::Max(VisualAttackDuration, 0.01f), 0.0f, 1.0f);
		const float Recovery = FMath::InterpEaseOut(1.0f, 0.0f, AttackAlpha, bVisualHeavyAttack ? 2.8f : 3.8f);
		const bool bFinisher = !bVisualHeavyAttack && VisualComboStep == 2;
		const float SwingDirection = bVisualHeavyAttack || VisualComboStep % 2 == 1 ? 1.0f : -1.0f;
		const float SwingDegrees = bVisualHeavyAttack ? 132.0f : bFinisher ? 112.0f : 91.0f;
		AxeRotation = ThorgrimAxeRestingRotation
			+ FRotator(-12.0f * Recovery, SwingDirection * 8.0f * Recovery, SwingDegrees * SwingDirection * Recovery);
		AxeOffset = ThorgrimAxeRestingLocation + FVector(10.0f * Recovery, 0.0f, 5.0f * Recovery);
		ShieldRotation = ThorgrimShieldRestingRotation + FRotator(0.0f, -6.0f * SwingDirection * Recovery, -8.0f * SwingDirection * Recovery);
		VisualRotation.Pitch -= (bVisualHeavyAttack ? 9.0f : 5.5f) * Recovery;
		VisualRotation.Yaw -= 7.0f * SwingDirection * Recovery;
		VisualOffset.X += (bVisualHeavyAttack ? 9.0f : 5.0f) * Recovery;
	}

	const float DodgeElapsed = Now - VisualDodgeStartTime;
	if (DodgeElapsed >= 0.0f && DodgeElapsed < 0.30f)
	{
		const float DodgeAlpha = FMath::Clamp(DodgeElapsed / 0.30f, 0.0f, 1.0f);
		const float DodgeEnvelope = FMath::Sin(DodgeAlpha * UE_PI);
		const FVector LocalDodge = GetActorTransform().InverseTransformVectorNoScale(
			VisualDodgeDirection.GetSafeNormal2D());
		VisualRotation.Pitch -= LocalDodge.X * 18.0f * DodgeEnvelope;
		VisualRotation.Roll += LocalDodge.Y * 21.0f * DodgeEnvelope;
		VisualOffset.Z -= 4.0f * DodgeEnvelope;
		AxeRotation += FRotator(0.0f, 0.0f, -12.0f * LocalDodge.Y * DodgeEnvelope);
		ShieldRotation += FRotator(0.0f, 0.0f, 10.0f * LocalDodge.Y * DodgeEnvelope);
	}

	const float HurtElapsed = Now - VisualHurtStartTime;
	if (HurtElapsed >= 0.0f && HurtElapsed < 0.18f)
	{
		const float HurtEnvelope = 1.0f - HurtElapsed / 0.18f;
		VisualRotation.Pitch += 9.0f * HurtEnvelope;
		VisualRotation.Roll -= 4.0f * HurtEnvelope;
		VisualOffset.Z += 2.0f * HurtEnvelope;
	}

	if (bVisualDead)
	{
		const float DeathAlpha = FMath::Clamp((Now - VisualDeathStartTime) / 0.55f, 0.0f, 1.0f);
		const float DeathEase = FMath::InterpEaseOut(0.0f, 1.0f, DeathAlpha, 2.0f);
		VisualOffset = FVector(0.0f, 0.0f, -44.0f * DeathEase);
		VisualRotation = FRotator(8.0f * DeathEase, -9.0f * DeathEase, 74.0f * DeathEase);
		BodyOffset = FVector::ZeroVector;
	}

	ThorgrimVisualRoot->SetRelativeLocation(ThorgrimVisualRestingLocation + VisualOffset);
	ThorgrimVisualRoot->SetWorldRotation(FRotator(
		ThorgrimVisualRestingRotation.Pitch + VisualRotation.Pitch,
		VisualFacingYaw + ThorgrimVisualRestingRotation.Yaw + VisualRotation.Yaw,
		ThorgrimVisualRestingRotation.Roll + VisualRotation.Roll));
	ThorgrimBodyRoot->SetRelativeLocation(ThorgrimBodyRestingLocation + BodyOffset);
	ThorgrimAxeRoot->SetRelativeLocation(AxeOffset);
	ThorgrimAxeRoot->SetRelativeRotation(AxeRotation);
	ThorgrimShieldRoot->SetRelativeLocation(ShieldOffset);
	ThorgrimShieldRoot->SetRelativeRotation(ShieldRotation);
}

void AEmberdeepCharacter::UpdateMouseAim()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	FVector MouseWorldOrigin;
	FVector MouseWorldDirection;
	if (!PlayerController->DeprojectMousePositionToWorld(MouseWorldOrigin, MouseWorldDirection))
	{
		return;
	}

	// Intersect the mouse ray with a horizontal plane through the character.
	// This continues to work when the cursor is beyond the visible floor mesh.
	const FPlane AimPlane(GetActorLocation(), FVector::UpVector);
	const FVector AimPoint = FMath::LinePlaneIntersection(
		MouseWorldOrigin,
		MouseWorldOrigin + MouseWorldDirection * 100000.0f,
		AimPlane);
	FVector AimDirection = AimPoint - GetActorLocation();
	AimDirection.Z = 0.0f;
	if (!AimDirection.IsNearlyZero())
	{
		AimDirection.Normalize();
		SetActorRotation(AimDirection.Rotation());

		const float Now = GetWorld()->GetTimeSeconds();
		const bool bDirectionChanged = FVector::DotProduct(AimDirection, LastSentAimDirection) < 0.997f;
		if (HasAuthority())
		{
			ReplicatedAimDirection = AimDirection;
		}
		else if (bDirectionChanged && Now >= NextAimReplicationTime)
		{
			LastSentAimDirection = AimDirection;
			NextAimReplicationTime = Now + 0.05f;
			ServerSetAimDirection(AimDirection);
		}
	}
}

void AEmberdeepCharacter::BasicAttack()
{
	const float Now = GetWorld()->GetTimeSeconds();
	bBasicAttackHeld = true;
	BasicAttackFeedbackStartTime = Now;
	UE_LOG(
		LogEmberdeep,
		Display,
		TEXT("EMBERDEEP_INPUT BasicAttack Pressed Ready=%d RecoveryRemaining=%.3f Queued=%d"),
		Now >= NextAttackTime ? 1 : 0,
		FMath::Max(0.0f, NextAttackTime - Now),
		bBasicAttackQueued ? 1 : 0);
	const FVector AttackDirection = GetActorForwardVector().GetSafeNormal2D();
	if (HasAuthority())
	{
		ExecuteAttack(false, AttackDirection);
	}
	else
	{
		ServerSetBasicAttackHeld(true, AttackDirection);
	}
}

void AEmberdeepCharacter::StopBasicAttack()
{
	bBasicAttackHeld = false;
	if (!HasAuthority())
	{
		ServerSetBasicAttackHeld(false, GetActorForwardVector().GetSafeNormal2D());
	}
}

void AEmberdeepCharacter::HeavyAttack()
{
	RequestAttack(true);
}

void AEmberdeepCharacter::Dodge()
{
	if (IsDead() || GetWorld()->GetTimeSeconds() < NextDodgeTime)
	{
		return;
	}

	FVector Direction = GetLastMovementInputVector().GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		Direction = GetActorForwardVector();
	}

	// Predict the cooldown display locally, but the host owns movement and invulnerability.
	if (!HasAuthority())
	{
		NextDodgeTime = GetWorld()->GetTimeSeconds() + DodgeCooldown;
		ServerDodge(Direction);
		return;
	}

	ExecuteDodge(Direction);
}

void AEmberdeepCharacter::ExecuteDodge(const FVector& DodgeDirection)
{
	if (!HasAuthority() || IsDead() || GetWorld()->GetTimeSeconds() < NextDodgeTime)
	{
		return;
	}

	const FVector SafeDirection = DodgeDirection.GetSafeNormal2D();
	if (SafeDirection.IsNearlyZero())
	{
		return;
	}

	NextDodgeTime = GetWorld()->GetTimeSeconds() + DodgeCooldown;
	bInvulnerable = true;
	ThorgrimDodgeDuration = 0.28f;
	ThorgrimDodgeTimeRemaining = ThorgrimDodgeDuration;
	ThorgrimPoseAccumulator = 1.0f;
	LaunchCharacter(SafeDirection * 920.0f, true, false);
	MulticastPlayDodgeVisual(SafeDirection);
	FTimerHandle DodgeTimer;
	GetWorldTimerManager().SetTimer(DodgeTimer, this, &AEmberdeepCharacter::EndDodge, 0.24f, false);
	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_INPUT Dodge"));
}

void AEmberdeepCharacter::RequestAttack(bool bHeavyAttack)
{
	if (IsDead())
	{
		return;
	}

	const FVector AttackDirection = GetActorForwardVector().GetSafeNormal2D();
	if (HasAuthority())
	{
		ExecuteAttack(bHeavyAttack, AttackDirection);
	}
	else
	{
		ServerPerformAttack(bHeavyAttack, AttackDirection);
	}
}

void AEmberdeepCharacter::ExecuteAttack(bool bHeavyAttack, const FVector& AttackDirection)
{
	if (!HasAuthority() || IsDead())
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < NextAttackTime)
	{
		if (!bHeavyAttack)
		{
			bBasicAttackQueued = true;
			QueuedBasicAttackDirection = AttackDirection.GetSafeNormal2D();
		}
		return;
	}

	int32 ComboStep = INDEX_NONE;
	if (bHeavyAttack)
	{
		BasicComboStep = 0;
		LastBasicAttackTime = -10.0f;
	}
	else
	{
		BasicComboStep = Now - LastBasicAttackTime > 0.82f ? 0 : (BasicComboStep + 1) % 3;
		LastBasicAttackTime = Now;
		ComboStep = BasicComboStep;
	}
	const bool bComboFinisher = !bHeavyAttack && ComboStep == 2;
	const float EquipmentDamage = GetEquipmentDamageBonus();
	const float ComboDamageScale = ComboStep == 1 ? 1.08f : bComboFinisher ? 1.28f : 1.0f;
	const float Damage = bHeavyAttack
		? 62.0f + EquipmentDamage * 1.4f
		: (34.0f + EquipmentDamage) * ComboDamageScale;
	const float Radius = bHeavyAttack ? 155.0f : bComboFinisher ? 126.0f : 105.0f;
	const float Reach = bHeavyAttack ? 125.0f : bComboFinisher ? 120.0f : 105.0f;
	const float KnockbackStrength = bHeavyAttack ? 620.0f : bComboFinisher ? 410.0f : 260.0f;
	const float Cooldown = bHeavyAttack ? HeavyAttackCooldown : bComboFinisher ? 0.50f : BasicAttackCooldown;
	const FVector SafeAttackDirection = AttackDirection.GetSafeNormal2D();
	if (SafeAttackDirection.IsNearlyZero())
	{
		return;
	}

	if (!bHeavyAttack)
	{
		bBasicAttackQueued = false;
	}
	NextAttackTime = Now + Cooldown;
	ReplicatedAimDirection = SafeAttackDirection;
	SetActorRotation(SafeAttackDirection.Rotation());
	LaunchCharacter(SafeAttackDirection * (bHeavyAttack ? 145.0f : 85.0f), false, false);

	const FVector AttackCenter = GetActorLocation() + SafeAttackDirection * Reach;
	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EmberdeepMeleeAttack), false, this);
	TArray<FOverlapResult> Overlaps;

	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		AttackCenter,
		FQuat::Identity,
		ObjectQuery,
		FCollisionShape::MakeSphere(Radius),
		QueryParams);

	TSet<AActor*> DamagedActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AEmberdeepEnemy* Target = Cast<AEmberdeepEnemy>(Overlap.GetActor());
		if (!Target || DamagedActors.Contains(Target) || Target->GetHealthComponent()->IsDead())
		{
			continue;
		}

		DamagedActors.Add(Target);
		FPointDamageEvent DamageEvent;
		DamageEvent.Damage = Damage;
		DamageEvent.ShotDirection = SafeAttackDirection * KnockbackStrength;
		Target->TakeDamage(Damage, DamageEvent, GetController(), this);
	}
	MulticastPlayAttackVisual(bHeavyAttack, DamagedActors.Num(), ComboStep);

	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_COMBAT FighterAttack Type=%s Damage=%.0f Hits=%d"),
		bHeavyAttack ? TEXT("Heavy") : TEXT("Basic"), Damage, DamagedActors.Num());
}

void AEmberdeepCharacter::PlayAttackVisual(bool bHeavyAttack, int32 HitCount, int32 ComboStep)
{
	const bool bComboFinisher = !bHeavyAttack && ComboStep == 2;
	const float SwingDuration = bHeavyAttack ? 0.34f : bComboFinisher ? 0.28f : 0.22f;
	if (!bHeavyAttack)
	{
		BasicAttackCooldownVisualEndTime = GetWorld()->GetTimeSeconds() + GetBasicAttackInterval();
		BasicAttackFeedbackStartTime = GetWorld()->GetTimeSeconds();
	}
	VisualAttackStartTime = GetWorld()->GetTimeSeconds();
	VisualAttackDuration = SwingDuration;
	bVisualHeavyAttack = bHeavyAttack;
	VisualComboStep = ComboStep;
	++AttackVisualSequence;
	ApplySteppedVisualPose(true);

	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	AEmberdeepCombatFeedback::SpawnSwing(
		GetWorld(),
		GetActorLocation() + FVector(0.0f, 0.0f, 42.0f),
		Forward,
		bHeavyAttack || bComboFinisher,
		HitCount > 0);
	if (IsLocallyControlled() && HitCount > 0)
	{
		StartLocalCameraShake(bHeavyAttack ? 8.5f : 4.5f, bHeavyAttack ? 0.19f : 0.11f);
	}
	bThorgrimHeavyAttack = bHeavyAttack;
	ThorgrimAttackDuration = SwingDuration;
	ThorgrimAttackTimeRemaining = SwingDuration;
	ThorgrimPoseAccumulator = 1.0f;
	FTimerHandle VisualTimer;
	GetWorldTimerManager().SetTimer(VisualTimer, this, &AEmberdeepCharacter::ResetAttackVisual, SwingDuration, false);
}

void AEmberdeepCharacter::UpdateThorgrimAnimation(float DeltaSeconds)
{
	static constexpr float PoseUpdateRate = 24.0f;
	static constexpr float PoseInterval = 1.0f / PoseUpdateRate;
	ThorgrimAnimationTime += DeltaSeconds;
	ThorgrimPoseAccumulator += DeltaSeconds;
	ThorgrimAttackTimeRemaining = FMath::Max(0.0f, ThorgrimAttackTimeRemaining - DeltaSeconds);
	ThorgrimDodgeTimeRemaining = FMath::Max(0.0f, ThorgrimDodgeTimeRemaining - DeltaSeconds);
	if (ThorgrimPoseAccumulator < PoseInterval)
	{
		return;
	}
	ThorgrimPoseAccumulator = FMath::Fmod(ThorgrimPoseAccumulator, PoseInterval);

	const float Speed = GetVelocity().Size2D();
	const bool bMoving = Speed > 12.0f && !IsDead();
	const float CycleRate = bMoving ? FMath::GetMappedRangeValueClamped(
		FVector2D(0.0f, GetCharacterMovement()->MaxWalkSpeed),
		FVector2D(5.5f, 9.0f),
		Speed) : 1.65f;
	const float Cycle = ThorgrimAnimationTime * CycleRate;
	const float Step = FMath::Sin(Cycle);
	const float Breath = FMath::Sin(ThorgrimAnimationTime * 1.65f);
	const bool bAttacking = ThorgrimAttackTimeRemaining > 0.0f
		&& ThorgrimAttackDuration > KINDA_SMALL_NUMBER;
	const float AttackAlpha = bAttacking
		? FMath::Clamp(1.0f - ThorgrimAttackTimeRemaining / ThorgrimAttackDuration, 0.0f, 1.0f)
		: 0.0f;
	const auto SmoothStep = [](float Alpha)
	{
		const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		return ClampedAlpha * ClampedAlpha * (3.0f - 2.0f * ClampedAlpha);
	};
	const auto LerpRotator = [](const FRotator& Start, const FRotator& End, float Alpha)
	{
		return FRotator(
			FMath::Lerp(Start.Pitch, End.Pitch, Alpha),
			FMath::Lerp(Start.Yaw, End.Yaw, Alpha),
			FMath::Lerp(Start.Roll, End.Roll, Alpha));
	};
	// The strike travels forward and inward across Thorgrim's body at roughly
	// 45 degrees, then reserves almost half the animation for pulling back to guard.
	const float WindUpAlpha = SmoothStep(AttackAlpha / 0.20f);
	const float SlashAlpha = SmoothStep((AttackAlpha - 0.20f) / 0.35f);
	const float RecoveryAlpha = SmoothStep((AttackAlpha - 0.55f) / 0.45f);
	const float AttackCommit = !bAttacking ? 0.0f
		: AttackAlpha < 0.20f ? -0.25f * WindUpAlpha
		: AttackAlpha < 0.55f ? FMath::Lerp(-0.25f, 1.0f, SlashAlpha)
		: 1.0f - RecoveryAlpha;
	const float AttackHold = !bAttacking ? 0.0f
		: AttackAlpha < 0.20f ? WindUpAlpha
		: AttackAlpha < 0.55f ? 1.0f
		: 1.0f - RecoveryAlpha;

	const FRotator ArmWindUp(-28.0f, 12.0f, 9.0f);
	const FRotator ArmFollowThrough(
		bThorgrimHeavyAttack ? 56.0f : 48.0f,
		bThorgrimHeavyAttack ? -38.0f : -34.0f,
		bThorgrimHeavyAttack ? -22.0f : -18.0f);
	const FRotator AttackArmRotation = !bAttacking ? FRotator::ZeroRotator
		: AttackAlpha < 0.20f ? LerpRotator(FRotator::ZeroRotator, ArmWindUp, WindUpAlpha)
		: AttackAlpha < 0.55f ? LerpRotator(ArmWindUp, ArmFollowThrough, SlashAlpha)
		: LerpRotator(ArmFollowThrough, FRotator::ZeroRotator, RecoveryAlpha);

	const FRotator AxeWindUp(-27.0f, 14.0f, 15.0f);
	const FRotator AxeFollowThrough(
		bThorgrimHeavyAttack ? 86.0f : 72.0f,
		bThorgrimHeavyAttack ? -36.0f : -31.0f,
		bThorgrimHeavyAttack ? -46.0f : -40.0f);
	const FRotator AxeSlashRotation = !bAttacking ? FRotator::ZeroRotator
		: AttackAlpha < 0.20f ? LerpRotator(FRotator::ZeroRotator, AxeWindUp, WindUpAlpha)
		: AttackAlpha < 0.55f ? LerpRotator(AxeWindUp, AxeFollowThrough, SlashAlpha)
		: LerpRotator(AxeFollowThrough, FRotator::ZeroRotator, RecoveryAlpha);

	FRotator BoneRotations[static_cast<int32>(EThorgrimRigidBone::Count)] = {};
	FVector BoneTranslations[static_cast<int32>(EThorgrimRigidBone::Count)] = {};
	const float WalkSwing = bMoving ? Step : 0.0f;
	BoneRotations[static_cast<int32>(EThorgrimRigidBone::Head)] =
		FRotator(0.0f, bMoving ? WalkSwing * 1.5f : Breath * 2.0f, 0.0f);
	BoneRotations[static_cast<int32>(EThorgrimRigidBone::LeftArm)] =
		FRotator(WalkSwing * 11.0f, 0.0f, 0.0f);
	BoneRotations[static_cast<int32>(EThorgrimRigidBone::RightArm)] =
		FRotator(-WalkSwing * 11.0f, 0.0f, 0.0f) + AttackArmRotation;
	BoneTranslations[static_cast<int32>(EThorgrimRigidBone::RightArm)] =
		FVector(1.0f, -1.0f, -0.25f).GetSafeNormal()
		* AttackCommit
		* (bThorgrimHeavyAttack ? 11.0f : 8.0f);
	BoneRotations[static_cast<int32>(EThorgrimRigidBone::LeftLeg)] =
		FRotator(-WalkSwing * 18.0f, 0.0f, 0.0f);
	BoneRotations[static_cast<int32>(EThorgrimRigidBone::RightLeg)] =
		FRotator(WalkSwing * 18.0f, 0.0f, 0.0f);

	FVector BodyTranslation(0.0f, 0.0f, bMoving ? FMath::Abs(Step) * 1.8f : Breath * 0.65f);
	FRotator BodyRotation(
		AttackCommit * (bThorgrimHeavyAttack ? -7.0f : -5.0f),
		AttackCommit * (bThorgrimHeavyAttack ? 3.0f : 2.0f),
		bMoving ? Step * 1.4f : Breath * 0.35f);
	FQuat AxeAnimationQuat = AxeSlashRotation.Quaternion();
	FVector AxeAnimationTranslation = FVector::ZeroVector;
	FQuat ShieldAnimationQuat = FRotator(WalkSwing * 2.0f, -AttackHold * 8.0f, -AttackHold * 7.0f).Quaternion();
	FVector ShieldAnimationTranslation = FVector::ZeroVector;

	if (!bUsingThorgrimVisual)
	{
		int32 ClipIndex = FighterVoxelData::GPlayableIdleClipIndex;
		float ClipTime = FMath::Fmod(ThorgrimAnimationTime, FighterVoxelData::GPlayableAnimationClips[ClipIndex].Duration);
		if (ThorgrimDodgeTimeRemaining > 0.0f && ThorgrimDodgeDuration > KINDA_SMALL_NUMBER)
		{
			ClipIndex = FighterVoxelData::GPlayableDodgeClipIndex;
			ClipTime = (1.0f - ThorgrimDodgeTimeRemaining / ThorgrimDodgeDuration)
				* FighterVoxelData::GPlayableAnimationClips[ClipIndex].Duration;
		}
		else if (bAttacking)
		{
			ClipIndex = bThorgrimHeavyAttack
				? FighterVoxelData::GPlayableHeavyAttackClipIndex
				: FighterVoxelData::GPlayableBasicAttackClipIndex;
			ClipTime = AttackAlpha * FighterVoxelData::GPlayableAnimationClips[ClipIndex].Duration;
		}
		else if (bMoving)
		{
			ClipIndex = FighterVoxelData::GPlayableWalkClipIndex;
			const float SpeedScale = FMath::GetMappedRangeValueClamped(
				FVector2D(0.0f, GetCharacterMovement()->MaxWalkSpeed), FVector2D(0.65f, 1.5f), Speed);
			ClipTime = FMath::Fmod(
				ThorgrimAnimationTime * SpeedScale,
				FighterVoxelData::GPlayableAnimationClips[ClipIndex].Duration);
		}

		const FPlayableVoxelAnimationClip& Clip = FighterVoxelData::GPlayableAnimationClips[ClipIndex];
		const int32 FrameIndex = FMath::Clamp(
			FMath::FloorToInt(ClipTime * Clip.SamplesPerSecond), 0, Clip.FrameCount - 1);
		const FPlayableVoxelBonePose* Poses = Clip.Frames + FrameIndex * GPlayableAnimationBoneCount;
		BodyRotation = Poses[0].Rotation.Rotator();
		BodyTranslation = Poses[0].Translation;
		for (int32 BoneIndex = 1; BoneIndex < static_cast<int32>(EThorgrimRigidBone::Count); ++BoneIndex)
		{
			BoneRotations[BoneIndex] = Poses[BoneIndex].Rotation.Rotator();
			BoneTranslations[BoneIndex] = Poses[BoneIndex].Translation;
		}
		AxeAnimationQuat = Poses[6].Rotation;
		AxeAnimationTranslation = Poses[6].Translation;
		ShieldAnimationQuat = Poses[7].Rotation;
		ShieldAnimationTranslation = Poses[7].Translation;
	}
	const FQuat BodyQuat = BodyRotation.Quaternion();
	const FVector BodyPivot(4.0f, 0.0f, -4.0f);

	const int32 BodyMeshCount = UE_ARRAY_COUNT(GThorgrimPaletteDefinitions) * EmberdeepVoxelStyle::ShadeCount;
	for (int32 MeshIndex = 0; MeshIndex < BodyMeshCount; ++MeshIndex)
	{
		if (!ThorgrimPaletteMeshes.IsValidIndex(MeshIndex)
			|| !ThorgrimRestTransformsByMesh.IsValidIndex(MeshIndex)
			|| !ThorgrimRigidBonesByMesh.IsValidIndex(MeshIndex))
		{
			continue;
		}

		const TArray<FTransform>& RestTransforms = ThorgrimRestTransformsByMesh[MeshIndex];
		const TArray<uint8>& RigidBones = ThorgrimRigidBonesByMesh[MeshIndex];
		TArray<FTransform> AnimatedTransforms;
		AnimatedTransforms.Reserve(RestTransforms.Num());
		for (int32 InstanceIndex = 0; InstanceIndex < RestTransforms.Num(); ++InstanceIndex)
		{
			const FTransform& RestTransform = RestTransforms[InstanceIndex];
			const EThorgrimRigidBone Bone = RigidBones.IsValidIndex(InstanceIndex)
				? static_cast<EThorgrimRigidBone>(RigidBones[InstanceIndex])
				: EThorgrimRigidBone::Torso;
			const int32 BoneIndex = static_cast<int32>(Bone);
			const FQuat BoneQuat = BoneRotations[BoneIndex].Quaternion();
			const FVector BonePivot = GetThorgrimBonePivot(Bone);
			FVector AnimatedLocation = BonePivot
				+ BoneQuat.RotateVector(RestTransform.GetLocation() - BonePivot)
				+ BoneTranslations[BoneIndex];
			AnimatedLocation = BodyPivot + BodyQuat.RotateVector(AnimatedLocation - BodyPivot) + BodyTranslation;
			AnimatedTransforms.Emplace(
				BodyQuat * BoneQuat * RestTransform.GetRotation(),
				AnimatedLocation,
				RestTransform.GetScale3D());
		}
		ThorgrimPaletteMeshes[MeshIndex]->BatchUpdateInstancesTransforms(
			0,
			AnimatedTransforms,
			false,
			true,
			true);
	}

	const int32 RightArmIndex = static_cast<int32>(EThorgrimRigidBone::RightArm);
	const int32 LeftArmIndex = static_cast<int32>(EThorgrimRigidBone::LeftArm);
	const FVector RightArmPivot = GetThorgrimBonePivot(EThorgrimRigidBone::RightArm);
	const FVector LeftArmPivot = GetThorgrimBonePivot(EThorgrimRigidBone::LeftArm);
	const FQuat RightArmQuat = BoneRotations[RightArmIndex].Quaternion();
	const FQuat LeftArmQuat = BoneRotations[LeftArmIndex].Quaternion();
	FVector AxeLocation = RightArmPivot
		+ RightArmQuat.RotateVector(PlayableAxePivot - RightArmPivot)
		+ BoneTranslations[RightArmIndex]
		+ RightArmQuat.RotateVector(AxeAnimationTranslation);
	FVector ShieldLocation = LeftArmPivot
		+ LeftArmQuat.RotateVector(PlayableShieldPivot - LeftArmPivot)
		+ BoneTranslations[LeftArmIndex]
		+ LeftArmQuat.RotateVector(ShieldAnimationTranslation);
	AxeLocation = BodyPivot + BodyQuat.RotateVector(AxeLocation - BodyPivot) + BodyTranslation;
	ShieldLocation = BodyPivot + BodyQuat.RotateVector(ShieldLocation - BodyPivot) + BodyTranslation;

	ThorgrimAxeRoot->SetRelativeLocation(AxeLocation);
	ThorgrimAxeRoot->SetRelativeRotation((
		BodyQuat
		* RightArmQuat
		* AxeAnimationQuat
		* ThorgrimAxeRestingRotation.Quaternion()).Rotator());
	ThorgrimShieldRoot->SetRelativeLocation(ShieldLocation);
	ThorgrimShieldRoot->SetRelativeRotation((
		BodyQuat
		* LeftArmQuat
		* ShieldAnimationQuat
		* ThorgrimShieldRestingRotation.Quaternion()).Rotator());
}

void AEmberdeepCharacter::ServerSetAimDirection_Implementation(FVector_NetQuantizeNormal NewAimDirection)
{
	const FVector SafeDirection = FVector(NewAimDirection).GetSafeNormal2D();
	if (!SafeDirection.IsNearlyZero() && !IsDead())
	{
		ReplicatedAimDirection = SafeDirection;
		SetActorRotation(SafeDirection.Rotation());
	}
}

void AEmberdeepCharacter::ServerPerformAttack_Implementation(
	bool bHeavyAttack,
	FVector_NetQuantizeNormal RequestedAimDirection)
{
	ExecuteAttack(bHeavyAttack, FVector(RequestedAimDirection));
}

void AEmberdeepCharacter::ServerSetBasicAttackHeld_Implementation(
	bool bHeld,
	FVector_NetQuantizeNormal RequestedAimDirection)
{
	bBasicAttackHeld = bHeld;
	if (bHeld)
	{
		ExecuteAttack(false, FVector(RequestedAimDirection));
	}
}

void AEmberdeepCharacter::MulticastPlayAttackVisual_Implementation(
	bool bHeavyAttack,
	int32 HitCount,
	int32 ComboStep)
{
	PlayAttackVisual(bHeavyAttack, HitCount, ComboStep);
}

void AEmberdeepCharacter::MulticastPlayDodgeVisual_Implementation(FVector_NetQuantizeNormal DodgeDirection)
{
	VisualDodgeStartTime = GetWorld()->GetTimeSeconds();
	VisualDodgeDirection = FVector(DodgeDirection).GetSafeNormal2D();
	ApplySteppedVisualPose(true);
	AEmberdeepCombatFeedback::SpawnDodge(GetWorld(), GetActorLocation(), VisualDodgeDirection);
}

void AEmberdeepCharacter::ServerDodge_Implementation(FVector_NetQuantizeNormal RequestedDodgeDirection)
{
	ExecuteDodge(FVector(RequestedDodgeDirection));
}

void AEmberdeepCharacter::OnRep_AimDirection()
{
	if (!IsLocallyControlled() && !ReplicatedAimDirection.IsNearlyZero())
	{
		SetActorRotation(FVector(ReplicatedAimDirection).Rotation());
	}
}

void AEmberdeepCharacter::ResetAttackVisual()
{
	VisualAttackStartTime = -100.0f;
	VisualAttackDuration = 0.0f;
	ApplySteppedVisualPose(true);
	ThorgrimAttackTimeRemaining = 0.0f;
	ThorgrimAttackDuration = 0.0f;
	ThorgrimAxeRoot->SetRelativeLocation(PlayableAxePivot);
	ThorgrimAxeRoot->SetRelativeRotation(ThorgrimAxeRestingRotation);
	ThorgrimShieldRoot->SetRelativeLocation(PlayableShieldPivot);
	ThorgrimShieldRoot->SetRelativeRotation(ThorgrimShieldRestingRotation);
}

void AEmberdeepCharacter::EndDodge()
{
	bInvulnerable = false;
}

float AEmberdeepCharacter::TakeDamage(
	float DamageAmount,
	const FDamageEvent& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (bInvulnerable || IsDead() || !HasAuthority())
	{
		return 0.0f;
	}

	const float MitigatedDamage = FMath::Max(1.0f, DamageAmount - GetEquipmentArmorBonus());
	const float AppliedDamage = HealthComponent->ApplyDamage(MitigatedDamage);
	if (AppliedDamage > 0.0f)
	{
		MulticastPlayHurtVisual(AppliedDamage, IsDead());
	}
	return AppliedDamage;
}

void AEmberdeepCharacter::MulticastPlayHurtVisual_Implementation(float Damage, bool bFatal)
{
	PlayHurtVisual(Damage, bFatal);
}

void AEmberdeepCharacter::PlayHurtVisual(float Damage, bool bFatal)
{
	const FLinearColor HitTint = bFatal
		? FLinearColor::FromSRGBColor(FColor(68, 8, 3))
		: FLinearColor::FromSRGBColor(FColor(255, 54, 20));
	const int32 MaterialCount = FMath::Min(ThorgrimMaterials.Num(), ThorgrimMaterialBaseColors.Num());
	for (int32 Index = 0; Index < MaterialCount; ++Index)
	{
		if (ThorgrimMaterials[Index])
		{
			ThorgrimMaterials[Index]->SetVectorParameterValue(
				TEXT("Color"),
				FMath::Lerp(ThorgrimMaterialBaseColors[Index], HitTint, bFatal ? 0.80f : 0.62f));
		}
	}
	VisualHurtStartTime = GetWorld()->GetTimeSeconds();
	ApplySteppedVisualPose(true);
	AEmberdeepCombatFeedback::SpawnPlayerHurt(GetWorld(), GetActorLocation(), Damage, bFatal);
	if (IsLocallyControlled())
	{
		StartLocalCameraShake(bFatal ? 13.0f : 7.0f, bFatal ? 0.30f : 0.16f);
	}

	FTimerHandle HitVisualTimer;
	GetWorldTimerManager().SetTimer(HitVisualTimer, this, &AEmberdeepCharacter::ResetHitVisual, 0.11f, false);
}

void AEmberdeepCharacter::ResetHitVisual()
{
	const int32 MaterialCount = FMath::Min(ThorgrimMaterials.Num(), ThorgrimMaterialBaseColors.Num());
	for (int32 Index = 0; Index < MaterialCount; ++Index)
	{
		if (ThorgrimMaterials[Index])
		{
			ThorgrimMaterials[Index]->SetVectorParameterValue(TEXT("Color"), ThorgrimMaterialBaseColors[Index]);
		}
	}
}

void AEmberdeepCharacter::StartLocalCameraShake(float Strength, float Duration)
{
	CameraShakeStrength = FMath::Max(CameraShakeStrength, Strength);
	CameraShakeDuration = FMath::Max(CameraShakeDuration, Duration);
	CameraShakeRemaining = FMath::Max(CameraShakeRemaining, Duration);
}

void AEmberdeepCharacter::UpdateLocalCameraShake(float DeltaSeconds)
{
	if (!CameraBoom)
	{
		return;
	}
	if (CameraShakeRemaining <= 0.0f)
	{
		CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, FVector::ZeroVector, DeltaSeconds, 24.0f);
		return;
	}

	CameraShakeRemaining = FMath::Max(0.0f, CameraShakeRemaining - DeltaSeconds);
	const float StrengthAlpha = CameraShakeDuration > KINDA_SMALL_NUMBER
		? CameraShakeRemaining / CameraShakeDuration
		: 0.0f;
	const float Time = GetWorld()->GetTimeSeconds();
	CameraBoom->SocketOffset = FVector(
		0.0f,
		FMath::Sin(Time * 91.0f) * CameraShakeStrength * StrengthAlpha,
		FMath::Cos(Time * 117.0f) * CameraShakeStrength * 0.72f * StrengthAlpha);
	if (CameraShakeRemaining <= 0.0f)
	{
		CameraShakeStrength = 0.0f;
		CameraShakeDuration = 0.0f;
	}
}

void AEmberdeepCharacter::AddGold(int32 Amount)
{
	if (HasAuthority())
	{
		if (AEmberdeepPlayerState* RunState = GetPlayerState<AEmberdeepPlayerState>())
		{
			RunState->AddGold(FMath::Max(0, Amount));
		}
	}
}

void AEmberdeepCharacter::AddExperience(int32 Amount)
{
	if (!HasAuthority() || Amount <= 0)
	{
		return;
	}

	if (AEmberdeepPlayerState* RunState = GetPlayerState<AEmberdeepPlayerState>())
	{
		RunState->AddExperience(Amount);
		UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_PROGRESSION Experience Level=%d"), RunState->GetCharacterLevel());
	}
}

int32 AEmberdeepCharacter::GetGold() const
{
	const AEmberdeepPlayerState* RunState = GetPlayerState<AEmberdeepPlayerState>();
	return RunState ? RunState->GetGold() : 0;
}

int32 AEmberdeepCharacter::GetCharacterLevel() const
{
	const AEmberdeepPlayerState* RunState = GetPlayerState<AEmberdeepPlayerState>();
	return RunState ? RunState->GetCharacterLevel() : 1;
}

float AEmberdeepCharacter::GetExperienceNormalized() const
{
	const AEmberdeepPlayerState* RunState = GetPlayerState<AEmberdeepPlayerState>();
	return RunState ? RunState->GetExperienceNormalized() : 0.0f;
}

float AEmberdeepCharacter::GetEquipmentDamageBonus() const
{
	const AEmberdeepPlayerState* RunState = GetPlayerState<AEmberdeepPlayerState>();
	return RunState ? RunState->GetTotalDamageBonus() : 0.0f;
}

float AEmberdeepCharacter::GetEquipmentArmorBonus() const
{
	const AEmberdeepPlayerState* RunState = GetPlayerState<AEmberdeepPlayerState>();
	return RunState ? RunState->GetTotalArmorBonus() : 0.0f;
}

void AEmberdeepCharacter::ApplyEquipmentStats()
{
	if (!HasAuthority())
	{
		return;
	}
	if (AEmberdeepPlayerState* RunState = GetPlayerState<AEmberdeepPlayerState>())
	{
		const FEmberdeepItemInstance* Weapon = RunState->GetEquippedItem(EEmberdeepItemSlot::Weapon);
		const FName DesiredWeaponVisual = Weapon ? Weapon->DefinitionId : FName(TEXT("NotchedIronAxe"));
		if (EquippedWeaponVisualId != DesiredWeaponVisual)
		{
			EquippedWeaponVisualId = DesiredWeaponVisual;
			RebuildPlayableVoxelCharacter(bUsingThorgrimVisual);
			ForceNetUpdate();
		}
		HealthComponent->SetMaxHealth(190.0f + RunState->GetTotalMaxHealthBonus(), true);
		UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_EQUIPMENT Applied Damage=%.1f Health=%.1f Armor=%.1f"),
			RunState->GetTotalDamageBonus(), RunState->GetTotalMaxHealthBonus(), RunState->GetTotalArmorBonus());
	}
}

void AEmberdeepCharacter::OnRep_EquippedWeaponVisual()
{
	RebuildPlayableVoxelCharacter(bUsingThorgrimVisual);
}

float AEmberdeepCharacter::GetDodgeCooldownNormalized() const
{
	if (!GetWorld() || DodgeCooldown <= 0.0f)
	{
		return 1.0f;
	}

	return 1.0f - FMath::Clamp((NextDodgeTime - GetWorld()->GetTimeSeconds()) / DodgeCooldown, 0.0f, 1.0f);
}

float AEmberdeepCharacter::GetBasicAttackCooldownNormalized() const
{
	const float BasicAttackInterval = GetBasicAttackInterval();
	if (!GetWorld() || BasicAttackInterval <= 0.0f)
	{
		return 1.0f;
	}

	return 1.0f - FMath::Clamp(
		(BasicAttackCooldownVisualEndTime - GetWorld()->GetTimeSeconds()) / BasicAttackInterval,
		0.0f,
		1.0f);
}

float AEmberdeepCharacter::GetBasicAttacksPerSecond() const
{
	return BasicAttackCooldown > KINDA_SMALL_NUMBER ? 1.0f / BasicAttackCooldown : 0.0f;
}

bool AEmberdeepCharacter::IsBasicAttackQueued() const
{
	return bBasicAttackQueued || bBasicAttackHeld;
}

float AEmberdeepCharacter::GetBasicAttackInterval() const
{
	return FMath::Max(BasicAttackCooldown, KINDA_SMALL_NUMBER);
}

float AEmberdeepCharacter::GetBasicAttackFeedbackNormalized() const
{
	if (!GetWorld() || BasicAttackFeedbackStartTime < 0.0f)
	{
		return 0.0f;
	}

	static constexpr float FeedbackDuration = 0.14f;
	return 1.0f - FMath::Clamp(
		(GetWorld()->GetTimeSeconds() - BasicAttackFeedbackStartTime) / FeedbackDuration,
		0.0f,
		1.0f);
}

bool AEmberdeepCharacter::IsDead() const
{
	return HealthComponent && HealthComponent->IsDead();
}

void AEmberdeepCharacter::HandleDeath()
{
	bVisualDead = true;
	VisualDeathStartTime = GetWorld()->GetTimeSeconds();
	ApplySteppedVisualPose(true);
	bBasicAttackHeld = false;
	bBasicAttackQueued = false;
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (HasAuthority())
	{
		if (AEmberdeepGameMode* GameMode = GetWorld()->GetAuthGameMode<AEmberdeepGameMode>())
		{
			GameMode->SchedulePlayerRespawn(this);
		}
	}
}

void AEmberdeepCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEmberdeepCharacter, ReplicatedAimDirection);
	DOREPLIFETIME(AEmberdeepCharacter, EquippedWeaponVisualId);
}
