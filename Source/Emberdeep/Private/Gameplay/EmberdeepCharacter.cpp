#include "Gameplay/EmberdeepCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Emberdeep.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AEmberdeepCharacter::AEmberdeepCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	GetCapsuleComponent()->InitCapsuleSize(34.0f, 72.0f);
	GetCharacterMovement()->MaxWalkSpeed = 500.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2200.0f;
	GetCharacterMovement()->GroundFriction = 9.0f;
	GetCharacterMovement()->bOrientRotationToMovement = true;
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
	IsometricCamera->OrthoWidth = 1350.0f;
	IsometricCamera->PostProcessBlendWeight = 1.0f;
	IsometricCamera->PostProcessSettings.bOverride_AutoExposureMethod = true;
	IsometricCamera->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	IsometricCamera->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	IsometricCamera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = 0;
	IsometricCamera->PostProcessSettings.bOverride_AutoExposureBias = true;
	IsometricCamera->PostProcessSettings.AutoExposureBias = 0.0f;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	BodyBlock = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyBlock"));
	BodyBlock->SetupAttachment(RootComponent);
	BodyBlock->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyBlock->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
	BodyBlock->SetRelativeScale3D(FVector(0.55f, 0.40f, 0.85f));
	BodyBlock->SetStaticMesh(CubeMesh.Object);

	HeadBlock = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadBlock"));
	HeadBlock->SetupAttachment(RootComponent);
	HeadBlock->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeadBlock->SetRelativeLocation(FVector(0.0f, 0.0f, 85.0f));
	HeadBlock->SetRelativeScale3D(FVector(0.46f, 0.46f, 0.46f));
	HeadBlock->SetStaticMesh(CubeMesh.Object);

	SwordBlock = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwordBlock"));
	SwordBlock->SetupAttachment(RootComponent);
	SwordBlock->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SwordBlock->SetRelativeLocation(FVector(12.0f, 48.0f, 8.0f));
	SwordBlock->SetRelativeRotation(FRotator(0.0f, 0.0f, -28.0f));
	SwordBlock->SetRelativeScale3D(FVector(0.11f, 0.11f, 0.95f));
	SwordBlock->SetStaticMesh(CubeMesh.Object);

	ShieldBlock = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldBlock"));
	ShieldBlock->SetupAttachment(RootComponent);
	ShieldBlock->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShieldBlock->SetRelativeLocation(FVector(10.0f, -44.0f, 15.0f));
	ShieldBlock->SetRelativeScale3D(FVector(0.16f, 0.52f, 0.62f));
	ShieldBlock->SetStaticMesh(CubeMesh.Object);
}

void AEmberdeepCharacter::BeginPlay()
{
	Super::BeginPlay();

	const auto SetBlockColor = [](UStaticMeshComponent* BlockMesh, const FLinearColor& Color)
	{
		if (UMaterialInstanceDynamic* Material = BlockMesh->CreateDynamicMaterialInstance(0))
		{
			Material->SetVectorParameterValue(TEXT("Color"), Color);
		}
	};

	SetBlockColor(BodyBlock, FLinearColor(0.30f, 0.055f, 0.035f));
	SetBlockColor(HeadBlock, FLinearColor(0.42f, 0.30f, 0.20f));
	SetBlockColor(SwordBlock, FLinearColor(0.52f, 0.60f, 0.68f));
	SetBlockColor(ShieldBlock, FLinearColor(0.42f, 0.07f, 0.035f));

	if (HasAuthority() && GetActorLocation().Z < 50.0f)
	{
		SetActorLocation(FVector(0.0f, 0.0f, 110.0f));
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

void AEmberdeepCharacter::BasicAttack()
{
	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_INPUT BasicAttack"));
}

void AEmberdeepCharacter::HeavyAttack()
{
	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_INPUT HeavyAttack"));
}

void AEmberdeepCharacter::Dodge()
{
	const FVector Direction = GetLastMovementInputVector().GetSafeNormal();
	if (!Direction.IsNearlyZero())
	{
		LaunchCharacter(Direction * 700.0f, true, false);
	}
	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_INPUT Dodge"));
}
