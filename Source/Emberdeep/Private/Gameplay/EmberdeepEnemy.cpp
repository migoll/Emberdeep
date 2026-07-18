#include "Gameplay/EmberdeepEnemy.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Gameplay/EmberdeepCharacter.h"
#include "Gameplay/EmberdeepGameMode.h"
#include "Gameplay/EmberdeepGoldPickup.h"
#include "Gameplay/EmberdeepHealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AEmberdeepEnemy::AEmberdeepEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	GetCapsuleComponent()->InitCapsuleSize(30.0f, 68.0f);
	GetCharacterMovement()->MaxWalkSpeed = 265.0f;
	GetCharacterMovement()->GroundFriction = 8.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 1800.0f;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	bUseControllerRotationYaw = false;

	HealthComponent = CreateDefaultSubobject<UEmberdeepHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->SetMaxHealth(92.0f);

	CreateBoneBlock(TEXT("Ribcage"), FVector(0.0f, 0.0f, 15.0f), FVector(0.42f, 0.28f, 0.55f));
	CreateBoneBlock(TEXT("Skull"), FVector(0.0f, 0.0f, 75.0f), FVector(0.43f, 0.43f, 0.43f));
	CreateBoneBlock(TEXT("LeftArm"), FVector(0.0f, -36.0f, 18.0f), FVector(0.15f, 0.16f, 0.62f), FRotator(0.0f, 0.0f, -12.0f));
	CreateBoneBlock(TEXT("RightArm"), FVector(0.0f, 36.0f, 18.0f), FVector(0.15f, 0.16f, 0.62f), FRotator(0.0f, 0.0f, 12.0f));
	CreateBoneBlock(TEXT("LeftLeg"), FVector(0.0f, -16.0f, -40.0f), FVector(0.17f, 0.17f, 0.58f));
	CreateBoneBlock(TEXT("RightLeg"), FVector(0.0f, 16.0f, -40.0f), FVector(0.17f, 0.17f, 0.58f));
}

UStaticMeshComponent* AEmberdeepEnemy::CreateBoneBlock(
	FName Name,
	const FVector& Location,
	const FVector& Scale,
	const FRotator& Rotation)
{
	UStaticMeshComponent* Bone = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Bone->SetupAttachment(RootComponent);
	Bone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bone->SetRelativeLocation(Location);
	Bone->SetRelativeScale3D(Scale);
	Bone->SetRelativeRotation(Rotation);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Bone->SetStaticMesh(CubeMesh.Object);
	BoneBlocks.Add(Bone);
	return Bone;
}

void AEmberdeepEnemy::BeginPlay()
{
	Super::BeginPlay();
	HealthComponent->OnDeath.AddUObject(this, &AEmberdeepEnemy::HandleDeath);

	for (UStaticMeshComponent* Bone : BoneBlocks)
	{
		if (UMaterialInstanceDynamic* Material = Bone->CreateDynamicMaterialInstance(0))
		{
			Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.48f, 0.42f, 0.31f));
			BoneMaterials.Add(Material);
		}
	}
}

void AEmberdeepEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority() && !HealthComponent->IsDead())
	{
		UpdateServerAI();
	}
}

void AEmberdeepEnemy::UpdateServerAI()
{
	AEmberdeepCharacter* Target = Cast<AEmberdeepCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!Target || Target->IsDead())
	{
		return;
	}

	const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	const float Distance = ToTarget.Size2D();
	if (!bHasAggro && Distance <= AggroRange)
	{
		bHasAggro = true;
	}

	if (!bHasAggro)
	{
		return;
	}

	if (Distance > AttackRange)
	{
		AddMovementInput(ToTarget.GetSafeNormal2D(), 1.0f);
	}
	else
	{
		AttackTarget(Target);
	}
}

void AEmberdeepEnemy::AttackTarget(ACharacter* TargetCharacter)
{
	if (GetWorld()->GetTimeSeconds() < NextAttackTime)
	{
		return;
	}

	NextAttackTime = GetWorld()->GetTimeSeconds() + AttackCooldown;
	UGameplayStatics::ApplyDamage(TargetCharacter, AttackDamage, GetController(), this, UDamageType::StaticClass());
}

float AEmberdeepEnemy::TakeDamage(
	float DamageAmount,
	const FDamageEvent& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (!HasAuthority() || HealthComponent->IsDead())
	{
		return 0.0f;
	}

	bHasAggro = true;
	const float AppliedDamage = HealthComponent->ApplyDamage(DamageAmount);

	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent* PointDamage = static_cast<const FPointDamageEvent*>(&DamageEvent);
		LaunchCharacter(PointDamage->ShotDirection, true, false);
	}

	for (UMaterialInstanceDynamic* Material : BoneMaterials)
	{
		Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.95f, 0.055f, 0.02f));
	}
	FTimerHandle FlashTimer;
	GetWorldTimerManager().SetTimer(FlashTimer, this, &AEmberdeepEnemy::ResetHitFlash, 0.10f, false);
	return AppliedDamage;
}

void AEmberdeepEnemy::ResetHitFlash()
{
	for (UMaterialInstanceDynamic* Material : BoneMaterials)
	{
		Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.48f, 0.42f, 0.31f));
	}
}

void AEmberdeepEnemy::HandleDeath()
{
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorScale3D(FVector(1.15f, 1.15f, 0.20f));

	if (HasAuthority())
	{
		const FVector DropLocation = GetActorLocation() + FVector(0.0f, 0.0f, 55.0f);
		if (AEmberdeepGoldPickup* Pickup = GetWorld()->SpawnActor<AEmberdeepGoldPickup>(DropLocation, FRotator::ZeroRotator))
		{
			Pickup->SetGoldValue(7);
		}

		if (AEmberdeepGameMode* GameMode = GetWorld()->GetAuthGameMode<AEmberdeepGameMode>())
		{
			GameMode->NotifyEnemyDefeated();
		}
	}

	SetLifeSpan(0.35f);
}
