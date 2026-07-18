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
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

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
		FLinearColor Color;
	};

	struct FThorgrimVoxelBlock
	{
		EThorgrimPart Part;
		EThorgrimPalette Palette;
		FVector Location;
		FVector Size;
		FRotator Rotation;
	};

	const FThorgrimPartDefinition GThorgrimPartDefinitions[] =
	{
		{ EThorgrimPart::Body, TEXT("Body") },
		{ EThorgrimPart::Axe, TEXT("Axe") },
		{ EThorgrimPart::Shield, TEXT("Shield") }
	};

	const FThorgrimPaletteDefinition GThorgrimPaletteDefinitions[] =
	{
		{ EThorgrimPalette::Night, TEXT("Night"), FLinearColor::FromSRGBColor(FColor(24, 32, 43)) },
		{ EThorgrimPalette::Steel, TEXT("Steel"), FLinearColor::FromSRGBColor(FColor(85, 90, 96)) },
		{ EThorgrimPalette::Ash, TEXT("Ash"), FLinearColor::FromSRGBColor(FColor(119, 117, 114)) },
		{ EThorgrimPalette::Bone, TEXT("Bone"), FLinearColor::FromSRGBColor(FColor(210, 194, 162)) },
		{ EThorgrimPalette::Hide, TEXT("Hide"), FLinearColor::FromSRGBColor(FColor(138, 95, 60)) },
		{ EThorgrimPalette::Fur, TEXT("Fur"), FLinearColor::FromSRGBColor(FColor(73, 55, 45)) },
		{ EThorgrimPalette::Skin, TEXT("Skin"), FLinearColor::FromSRGBColor(FColor(184, 116, 84)) },
		{ EThorgrimPalette::Wood, TEXT("Wood"), FLinearColor::FromSRGBColor(FColor(78, 53, 37)) },
		{ EThorgrimPalette::Cloth, TEXT("Cloth"), FLinearColor::FromSRGBColor(FColor(36, 52, 66)) }
	};

	int32 GetThorgrimMeshKey(EThorgrimPart Part, EThorgrimPalette Palette)
	{
		return static_cast<int32>(Part) * UE_ARRAY_COUNT(GThorgrimPaletteDefinitions) + static_cast<int32>(Palette);
	}

#include "ThorgrimVoxelData.inl"
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
	IsometricCamera->PostProcessSettings.AutoExposureBias = 0.5f;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	ThorgrimAxeRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ThorgrimAxeRoot"));
	ThorgrimAxeRoot->SetupAttachment(RootComponent);
	ThorgrimAxeRoot->SetRelativeLocation(GThorgrimAxePivot);

	ThorgrimShieldRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ThorgrimShieldRoot"));
	ThorgrimShieldRoot->SetupAttachment(RootComponent);
	ThorgrimShieldRoot->SetRelativeLocation(GThorgrimShieldPivot);

	TMap<int32, UInstancedStaticMeshComponent*> PaletteMeshes;
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
			const FName ComponentName(*FString::Printf(
				TEXT("Thorgrim%s%s"),
				PartDefinition.Name,
				PaletteDefinition.Name));
			UInstancedStaticMeshComponent* PaletteMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(ComponentName);
			PaletteMesh->SetupAttachment(PartRoot);
			PaletteMesh->SetMobility(EComponentMobility::Movable);
			PaletteMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			PaletteMesh->SetGenerateOverlapEvents(false);
			PaletteMesh->SetCanEverAffectNavigation(false);
			PaletteMesh->SetStaticMesh(CubeMesh.Object);
			ThorgrimPaletteMeshes.Add(PaletteMesh);
			PaletteMeshes.Add(GetThorgrimMeshKey(PartDefinition.Part, PaletteDefinition.Palette), PaletteMesh);
		}
	}

	for (const FThorgrimVoxelBlock& Block : GThorgrimVoxelBlocks)
	{
		if (UInstancedStaticMeshComponent* const* PaletteMesh = PaletteMeshes.Find(GetThorgrimMeshKey(Block.Part, Block.Palette)))
		{
			const FVector InstanceScale = Block.Size / 100.0f;
			(*PaletteMesh)->AddInstance(FTransform(Block.Rotation, Block.Location, InstanceScale));
		}
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
	}
}

void AEmberdeepCharacter::BeginPlay()
{
	Super::BeginPlay();

	int32 ThorgrimInstanceCount = 0;
	for (int32 MeshIndex = 0; MeshIndex < ThorgrimPaletteMeshes.Num(); ++MeshIndex)
	{
		UInstancedStaticMeshComponent* PaletteMesh = ThorgrimPaletteMeshes[MeshIndex];
		if (PaletteMesh)
		{
			ThorgrimInstanceCount += PaletteMesh->GetInstanceCount();
			const int32 PaletteIndex = MeshIndex % UE_ARRAY_COUNT(GThorgrimPaletteDefinitions);
			if (UMaterialInstanceDynamic* Material = PaletteMesh->CreateDynamicMaterialInstance(0))
			{
				Material->SetVectorParameterValue(TEXT("Color"), GThorgrimPaletteDefinitions[PaletteIndex].Color);
			}
		}
	}
	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_VISUAL ThorgrimGenerated Instances=%d"), ThorgrimInstanceCount);

	ThorgrimAxeRestingRotation = ThorgrimAxeRoot->GetRelativeRotation();
	HealthComponent->OnDeath.AddUObject(this, &AEmberdeepCharacter::HandleDeath);

	if (HasAuthority() && GetActorLocation().Z < 50.0f)
	{
		SetActorLocation(FVector(0.0f, 0.0f, 75.0f));
	}
}

void AEmberdeepCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AEmberdeepCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AEmberdeepCharacter::MoveRight);
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
		SetActorRotation(AimDirection.Rotation());
	}
}

void AEmberdeepCharacter::BasicAttack()
{
	PerformAttack(34.0f, 105.0f, 105.0f, 260.0f, BasicAttackCooldown);
}

void AEmberdeepCharacter::HeavyAttack()
{
	PerformAttack(62.0f, 155.0f, 125.0f, 620.0f, HeavyAttackCooldown);
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

	NextDodgeTime = GetWorld()->GetTimeSeconds() + DodgeCooldown;
	bInvulnerable = true;
	LaunchCharacter(Direction * 920.0f, true, false);
	FTimerHandle DodgeTimer;
	GetWorldTimerManager().SetTimer(DodgeTimer, this, &AEmberdeepCharacter::EndDodge, 0.24f, false);
	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_INPUT Dodge"));
}

void AEmberdeepCharacter::PerformAttack(
	float Damage,
	float Radius,
	float Reach,
	float KnockbackStrength,
	float Cooldown)
{
	if (IsDead() || GetWorld()->GetTimeSeconds() < NextAttackTime)
	{
		return;
	}

	NextAttackTime = GetWorld()->GetTimeSeconds() + Cooldown;
	ThorgrimAxeRoot->SetRelativeRotation(ThorgrimAxeRestingRotation + FRotator(0.0f, 0.0f, 85.0f));
	FTimerHandle VisualTimer;
	GetWorldTimerManager().SetTimer(VisualTimer, this, &AEmberdeepCharacter::ResetAttackVisual, 0.13f, false);

	if (!HasAuthority())
	{
		return;
	}

	const FVector AttackDirection = GetActorForwardVector().GetSafeNormal2D();
	const FVector AttackCenter = GetActorLocation() + AttackDirection * Reach;
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
		DamageEvent.ShotDirection = AttackDirection * KnockbackStrength;
		Target->TakeDamage(Damage, DamageEvent, GetController(), this);
	}

	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_COMBAT FighterAttack Damage=%.0f Hits=%d"), Damage, DamagedActors.Num());
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

	return HealthComponent->ApplyDamage(DamageAmount);
}

void AEmberdeepCharacter::AddGold(int32 Amount)
{
	if (HasAuthority())
	{
		Gold += FMath::Max(0, Amount);
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
	DisableInput(Cast<APlayerController>(GetController()));
	FTimerHandle RestartTimer;
	GetWorldTimerManager().SetTimer(RestartTimer, this, &AEmberdeepCharacter::RestartEncounter, 2.0f, false);
}

void AEmberdeepCharacter::RestartEncounter()
{
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		PlayerController->ConsoleCommand(TEXT("RestartLevel"));
	}
}

void AEmberdeepCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEmberdeepCharacter, Gold);
}
