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
#include "Gameplay/EmberdeepGameMode.h"
#include "Gameplay/EmberdeepPlayerState.h"
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
		Bone,
		Hide,
		Fur,
		Skin,
		Wood,
		Cloth
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

	const FThorgrimPartDefinition GThorgrimPartDefinitions[] =
	{
		{ EThorgrimPart::Body, TEXT("Body") },
		{ EThorgrimPart::Axe, TEXT("Axe") },
		{ EThorgrimPart::Shield, TEXT("Shield") }
	};

	const FThorgrimPaletteDefinition GThorgrimPaletteDefinitions[] =
	{
		{ EThorgrimPalette::Night, TEXT("Night"), {
			FLinearColor::FromSRGBColor(FColor(18, 25, 34)), FLinearColor::FromSRGBColor(FColor(24, 32, 43)), FLinearColor::FromSRGBColor(FColor(31, 42, 54)) } },
		{ EThorgrimPalette::Steel, TEXT("Steel"), {
			FLinearColor::FromSRGBColor(FColor(68, 73, 79)), FLinearColor::FromSRGBColor(FColor(85, 90, 96)), FLinearColor::FromSRGBColor(FColor(112, 119, 126)) } },
		{ EThorgrimPalette::Ash, TEXT("Ash"), {
			FLinearColor::FromSRGBColor(FColor(96, 93, 89)), FLinearColor::FromSRGBColor(FColor(119, 117, 114)), FLinearColor::FromSRGBColor(FColor(143, 139, 132)) } },
		{ EThorgrimPalette::Bone, TEXT("Bone"), {
			FLinearColor::FromSRGBColor(FColor(174, 156, 124)), FLinearColor::FromSRGBColor(FColor(210, 194, 162)), FLinearColor::FromSRGBColor(FColor(228, 213, 182)) } },
		{ EThorgrimPalette::Hide, TEXT("Hide"), {
			FLinearColor::FromSRGBColor(FColor(105, 70, 45)), FLinearColor::FromSRGBColor(FColor(138, 95, 60)), FLinearColor::FromSRGBColor(FColor(160, 112, 69)) } },
		{ EThorgrimPalette::Fur, TEXT("Fur"), {
			FLinearColor::FromSRGBColor(FColor(54, 39, 32)), FLinearColor::FromSRGBColor(FColor(73, 55, 45)), FLinearColor::FromSRGBColor(FColor(94, 68, 50)) } },
		{ EThorgrimPalette::Skin, TEXT("Skin"), {
			FLinearColor::FromSRGBColor(FColor(155, 90, 66)), FLinearColor::FromSRGBColor(FColor(184, 116, 84)), FLinearColor::FromSRGBColor(FColor(204, 137, 102)) } },
		{ EThorgrimPalette::Wood, TEXT("Wood"), {
			FLinearColor::FromSRGBColor(FColor(60, 39, 29)), FLinearColor::FromSRGBColor(FColor(78, 53, 37)), FLinearColor::FromSRGBColor(FColor(96, 65, 42)) } },
		{ EThorgrimPalette::Cloth, TEXT("Cloth"), {
			FLinearColor::FromSRGBColor(FColor(28, 40, 51)), FLinearColor::FromSRGBColor(FColor(36, 52, 66)), FLinearColor::FromSRGBColor(FColor(46, 65, 78)) } }
	};

	int32 PositiveModulo(int32 Value, int32 Divisor)
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
			return ClusterHash % 13u == 12u || PositiveModulo(Voxel.Y + Voxel.Z, 11) == 0 ? 2 : 1;

		case EThorgrimPalette::Hide:
			if (PositiveModulo(Voxel.X + Voxel.Z * 2, 13) == 0)
			{
				return 2;
			}
			return ClusterHash % 9u < 2u ? 0 : 1;

		case EThorgrimPalette::Wood:
			if (PositiveModulo(Voxel.Y + Voxel.Z * 2, 9) == 0)
			{
				return 2;
			}
			return PositiveModulo(Voxel.Y + Voxel.Z * 2, 9) <= 2 ? 0 : 1;

		case EThorgrimPalette::Steel:
		case EThorgrimPalette::Ash:
			if (PositiveModulo(Voxel.X + Voxel.Y * 2 + Voxel.Z * 3, 17) == 0)
			{
				return 2;
			}
			return CellHash % 12u < 2u ? 0 : 1;

		case EThorgrimPalette::Bone:
			if (PositiveModulo(Voxel.X - Voxel.Y + Voxel.Z * 2, 19) == 0)
			{
				return 2;
			}
			return ClusterHash % 10u < 2u ? 0 : 1;

		case EThorgrimPalette::Night:
		case EThorgrimPalette::Cloth:
			if (PositiveModulo(Voxel.X + Voxel.Y + Voxel.Z, 15) == 0)
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

#include "ThorgrimVoxelData.inl"
	static_assert(GThorgrimVoxelUnitCm == EmberdeepVoxelStyle::UnitCm);
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
	IsometricCamera->PostProcessSettings.AutoExposureBias = 0.15f;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> VoxelMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	ThorgrimAxeRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ThorgrimAxeRoot"));
	ThorgrimAxeRoot->SetupAttachment(RootComponent);
	ThorgrimAxeRoot->SetRelativeLocation(GThorgrimAxePivot);

	ThorgrimShieldRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ThorgrimShieldRoot"));
	ThorgrimShieldRoot->SetupAttachment(RootComponent);
	ThorgrimShieldRoot->SetRelativeLocation(GThorgrimShieldPivot);

	TMap<int32, UInstancedStaticMeshComponent*> PaletteMeshes;
	TArray<TArray<FTransform>> VoxelTransformsByMesh;
	VoxelTransformsByMesh.SetNum(
		UE_ARRAY_COUNT(GThorgrimPartDefinitions)
		* UE_ARRAY_COUNT(GThorgrimPaletteDefinitions)
		* EmberdeepVoxelStyle::ShadeCount);
	for (const FThorgrimPartDefinition& PartDefinition : GThorgrimPartDefinitions)
	{
		USceneComponent* PartRoot = RootComponent;
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
				PaletteMesh->SetStaticMesh(CubeMesh.Object);
				PaletteMesh->SetMaterial(0, VoxelMaterial.Object);
				ThorgrimPaletteMeshes.Add(PaletteMesh);
				PaletteMeshes.Add(
					GetThorgrimMeshKey(PartDefinition.Part, PaletteDefinition.Palette, ShadeIndex),
					PaletteMesh);
			}
		}
	}

	for (const FThorgrimVoxelCell& Voxel : GThorgrimVoxelCells)
	{
		const int32 ShadeIndex = SelectThorgrimMaterialShade(Voxel);
		const int32 MeshKey = GetThorgrimMeshKey(Voxel.Part, Voxel.Palette, ShadeIndex);
		VoxelTransformsByMesh[MeshKey].Add(FTransform(
			FQuat::Identity,
			EmberdeepVoxelStyle::CellCenter(Voxel.X, Voxel.Y, Voxel.Z),
			EmberdeepVoxelStyle::InstanceScale()));
	}
	for (const TPair<int32, UInstancedStaticMeshComponent*>& PaletteMesh : PaletteMeshes)
	{
		PaletteMesh.Value->AddInstances(VoxelTransformsByMesh[PaletteMesh.Key], false, false, false);
	}

	HealthComponent = CreateDefaultSubobject<UEmberdeepHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->SetMaxHealth(190.0f);
}

void AEmberdeepCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocallyControlled() && !IsDead())
	{
		UpdateMouseAim();
		IsometricCamera->OrthoWidth = FMath::FInterpTo(
			IsometricCamera->OrthoWidth,
			TargetOrthoWidth,
			DeltaSeconds,
			ZoomInterpolationSpeed);
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
				Material->SetVectorParameterValue(
					TEXT("Color"),
					GThorgrimPaletteDefinitions[PaletteIndex].Shades[ShadeIndex]);
				++ThorgrimMaterialCount;
			}
		}
	}
	UE_LOG(
		LogEmberdeep,
		Display,
		TEXT("EMBERDEEP_VISUAL ThorgrimGenerated Instances=%d Materials=%d"),
		ThorgrimInstanceCount,
		ThorgrimMaterialCount);

	ThorgrimAxeRestingRotation = ThorgrimAxeRoot->GetRelativeRotation();
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
	PlayerInputComponent->BindAction(TEXT("HeavyAttack"), IE_Pressed, this, &AEmberdeepCharacter::HeavyAttack);
	PlayerInputComponent->BindAction(TEXT("Dodge"), IE_Pressed, this, &AEmberdeepCharacter::Dodge);
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
	RequestAttack(false);
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
	LaunchCharacter(SafeDirection * 920.0f, true, false);
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
	if (!HasAuthority() || IsDead() || GetWorld()->GetTimeSeconds() < NextAttackTime)
	{
		return;
	}

	const float EquipmentDamage = GetEquipmentDamageBonus();
	const float Damage = bHeavyAttack ? 62.0f + EquipmentDamage * 1.4f : 34.0f + EquipmentDamage;
	const float Radius = bHeavyAttack ? 155.0f : 105.0f;
	const float Reach = bHeavyAttack ? 125.0f : 105.0f;
	const float KnockbackStrength = bHeavyAttack ? 620.0f : 260.0f;
	const float Cooldown = bHeavyAttack ? HeavyAttackCooldown : BasicAttackCooldown;
	const FVector SafeAttackDirection = AttackDirection.GetSafeNormal2D();
	if (SafeAttackDirection.IsNearlyZero())
	{
		return;
	}

	NextAttackTime = GetWorld()->GetTimeSeconds() + Cooldown;
	ReplicatedAimDirection = SafeAttackDirection;
	SetActorRotation(SafeAttackDirection.Rotation());
	MulticastPlayAttackVisual(bHeavyAttack);

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
		AActor* Target = Overlap.GetActor();
		if (!Target || Target == this || DamagedActors.Contains(Target))
		{
			continue;
		}

		DamagedActors.Add(Target);
		FPointDamageEvent DamageEvent;
		DamageEvent.Damage = Damage;
		DamageEvent.ShotDirection = SafeAttackDirection * KnockbackStrength;
		Target->TakeDamage(Damage, DamageEvent, GetController(), this);
	}

	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_COMBAT FighterAttack Type=%s Damage=%.0f Hits=%d"),
		bHeavyAttack ? TEXT("Heavy") : TEXT("Basic"), Damage, DamagedActors.Num());
}

void AEmberdeepCharacter::PlayAttackVisual(bool bHeavyAttack)
{
	const float SwingDegrees = bHeavyAttack ? 125.0f : 85.0f;
	const float SwingDuration = bHeavyAttack ? 0.20f : 0.13f;
	ThorgrimAxeRoot->SetRelativeRotation(ThorgrimAxeRestingRotation + FRotator(0.0f, 0.0f, SwingDegrees));
	FTimerHandle VisualTimer;
	GetWorldTimerManager().SetTimer(VisualTimer, this, &AEmberdeepCharacter::ResetAttackVisual, SwingDuration, false);
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

void AEmberdeepCharacter::MulticastPlayAttackVisual_Implementation(bool bHeavyAttack)
{
	PlayAttackVisual(bHeavyAttack);
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
	ThorgrimAxeRoot->SetRelativeRotation(ThorgrimAxeRestingRotation);
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
	return HealthComponent->ApplyDamage(MitigatedDamage);
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
		HealthComponent->SetMaxHealth(190.0f + RunState->GetTotalMaxHealthBonus(), true);
		UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_EQUIPMENT Applied Damage=%.1f Health=%.1f Armor=%.1f"),
			RunState->GetTotalDamageBonus(), RunState->GetTotalMaxHealthBonus(), RunState->GetTotalArmorBonus());
	}
}

float AEmberdeepCharacter::GetDodgeCooldownNormalized() const
{
	if (!GetWorld() || DodgeCooldown <= 0.0f)
	{
		return 1.0f;
	}

	return 1.0f - FMath::Clamp((NextDodgeTime - GetWorld()->GetTimeSeconds()) / DodgeCooldown, 0.0f, 1.0f);
}

bool AEmberdeepCharacter::IsDead() const
{
	return HealthComponent && HealthComponent->IsDead();
}

void AEmberdeepCharacter::HandleDeath()
{
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
}
