#include "Gameplay/EmberdeepEnemy.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
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

	AttackTelegraph = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttackTelegraph"));
	AttackTelegraph->SetupAttachment(RootComponent);
	AttackTelegraph->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttackTelegraph->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
	AttackTelegraph->SetRelativeScale3D(FVector(2.7f, 2.7f, 0.025f));
	AttackTelegraph->SetHiddenInGame(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	AttackTelegraph->SetStaticMesh(CylinderMesh.Object);
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

	if (UMaterialInstanceDynamic* TelegraphMaterial = AttackTelegraph->CreateDynamicMaterialInstance(0))
	{
		TelegraphMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.9f, 0.015f, 0.005f));
	}
}

void AEmberdeepEnemy::ConfigureAsElite()
{
	bIsElite = true;
	HealthComponent->SetMaxHealth(280.0f);
	AttackDamage = 46.0f;
	AttackRange = 165.0f;
	AttackCooldown = 2.25f;
	AggroRange = 680.0f;
	GoldDropValue = 30;
	GetCharacterMovement()->MaxWalkSpeed = 225.0f;
	SetActorScale3D(FVector(1.25f));
	ApplyBoneColor(FLinearColor(0.34f, 0.10f, 0.055f));
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
	AEmberdeepCharacter* Target = nullptr;
	float NearestDistanceSquared = TNumericLimits<float>::Max();
	for (TActorIterator<AEmberdeepCharacter> CharacterIt(GetWorld()); CharacterIt; ++CharacterIt)
	{
		AEmberdeepCharacter* Candidate = *CharacterIt;
		if (!Candidate || Candidate->IsDead())
		{
			continue;
		}

		const float CandidateDistanceSquared = FVector::DistSquared2D(GetActorLocation(), Candidate->GetActorLocation());
		if (CandidateDistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = CandidateDistanceSquared;
			Target = Candidate;
		}
	}

	if (!Target)
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
	if (bAttackWindingUp)
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
	if (bAttackWindingUp || GetWorld()->GetTimeSeconds() < NextAttackTime)
	{
		return;
	}

	NextAttackTime = GetWorld()->GetTimeSeconds() + AttackCooldown;
	if (bIsElite)
	{
		bAttackWindingUp = true;
		PendingAttackTarget = TargetCharacter;
		AttackTelegraph->SetHiddenInGame(false);
		GetCharacterMovement()->StopMovementImmediately();
		FTimerHandle WindupTimer;
		GetWorldTimerManager().SetTimer(WindupTimer, this, &AEmberdeepEnemy::ResolveEliteAttack, 0.72f, false);
		return;
	}

	UGameplayStatics::ApplyDamage(TargetCharacter, AttackDamage, GetController(), this, UDamageType::StaticClass());
}

void AEmberdeepEnemy::ResolveEliteAttack()
{
	bAttackWindingUp = false;
	AttackTelegraph->SetHiddenInGame(true);

	ACharacter* TargetCharacter = PendingAttackTarget.Get();
	PendingAttackTarget.Reset();
	if (!TargetCharacter || HealthComponent->IsDead())
	{
		return;
	}

	const float Distance = FVector::Dist2D(GetActorLocation(), TargetCharacter->GetActorLocation());
	if (Distance <= AttackRange + 35.0f)
	{
		UGameplayStatics::ApplyDamage(TargetCharacter, AttackDamage, GetController(), this, UDamageType::StaticClass());
	}
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
	if (HealthComponent->IsDead())
	{
		if (AEmberdeepCharacter* Killer = Cast<AEmberdeepCharacter>(DamageCauser))
		{
			Killer->AddExperience(bIsElite ? 50 : 20);
		}
	}

	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent* PointDamage = static_cast<const FPointDamageEvent*>(&DamageEvent);
		LaunchCharacter(PointDamage->ShotDirection, true, false);
	}

	ApplyBoneColor(FLinearColor(0.95f, 0.055f, 0.02f));
	FTimerHandle FlashTimer;
	GetWorldTimerManager().SetTimer(FlashTimer, this, &AEmberdeepEnemy::ResetHitFlash, 0.10f, false);
	return AppliedDamage;
}

void AEmberdeepEnemy::ResetHitFlash()
{
	ApplyBoneColor(bIsElite
		? FLinearColor(0.34f, 0.10f, 0.055f)
		: FLinearColor(0.48f, 0.42f, 0.31f));
}

void AEmberdeepEnemy::ApplyBoneColor(const FLinearColor& Color)
{
	for (UMaterialInstanceDynamic* Material : BoneMaterials)
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

void AEmberdeepEnemy::HandleDeath()
{
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttackTelegraph->SetHiddenInGame(true);
	SetActorScale3D(FVector(1.15f, 1.15f, 0.20f));

	if (HasAuthority())
	{
		const FVector DropLocation = GetActorLocation() + FVector(0.0f, 0.0f, 55.0f);
		if (AEmberdeepGoldPickup* Pickup = GetWorld()->SpawnActor<AEmberdeepGoldPickup>(DropLocation, FRotator::ZeroRotator))
		{
			Pickup->SetGoldValue(GoldDropValue);
		}

		if (AEmberdeepGameMode* GameMode = GetWorld()->GetAuthGameMode<AEmberdeepGameMode>())
		{
			GameMode->NotifyEnemyDefeated();
		}
	}

	SetLifeSpan(0.35f);
}
