#include "Gameplay/EmberdeepEnemy.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
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
#include "Visual/EmberdeepVoxelStyle.h"

AEmberdeepEnemy::AEmberdeepEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();

	GetCapsuleComponent()->InitCapsuleSize(30.0f, 68.0f);
	GetCharacterMovement()->MaxWalkSpeed = 265.0f;
	GetCharacterMovement()->GroundFriction = 8.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 1800.0f;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	bUseControllerRotationYaw = false;

	HealthComponent = CreateDefaultSubobject<UEmberdeepHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->SetMaxHealth(92.0f);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	for (int32 ShadeIndex = 0; ShadeIndex < EmberdeepVoxelStyle::ShadeCount; ++ShadeIndex)
	{
		const FName MeshName(*FString::Printf(TEXT("BoneVoxels_Shade%d"), ShadeIndex));
		UInstancedStaticMeshComponent* BoneMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(MeshName);
		BoneMesh->SetupAttachment(RootComponent);
		BoneMesh->SetStaticMesh(CubeMesh.Object);
		BoneMesh->SetMobility(EComponentMobility::Movable);
		BoneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BoneMesh->SetGenerateOverlapEvents(false);
		BoneMesh->SetCanEverAffectNavigation(false);
		BoneVoxelMeshes.Add(BoneMesh);
	}

	TSet<FIntVector> OccupiedCells;
	const auto AddBox = [&OccupiedCells](const FIntVector& Min, const FIntVector& Max)
	{
		for (int32 Z = Min.Z; Z <= Max.Z; ++Z)
		{
			for (int32 Y = Min.Y; Y <= Max.Y; ++Y)
			{
				for (int32 X = Min.X; X <= Max.X; ++X)
				{
					OccupiedCells.Add(FIntVector(X, Y, Z));
				}
			}
		}
	};

	// One actor-local 4 cm lattice. The silhouette is assembled from cell clusters;
	// no individual voxel is scaled, stretched, or placed off-grid.
	AddBox(FIntVector(-2, -2, -3), FIntVector(2, 2, 10)); // spine
	for (int32 RibZ = 0; RibZ <= 9; RibZ += 3)
	{
		const int32 HalfWidth = RibZ >= 6 ? 6 : 5;
		AddBox(FIntVector(-2, -HalfWidth, RibZ), FIntVector(2, HalfWidth, RibZ + 1));
	}
	AddBox(FIntVector(-3, -5, -5), FIntVector(3, 5, -2)); // pelvis
	AddBox(FIntVector(-4, -4, 12), FIntVector(3, 4, 20)); // skull
	AddBox(FIntVector(-3, -3, 10), FIntVector(2, 3, 13)); // neck/jaw

	for (const int32 Side : {-1, 1})
	{
		for (int32 Step = 0; Step < 13; ++Step)
		{
			const int32 Y = Side * (7 + Step / 3);
			const int32 Z = 9 - Step;
			AddBox(FIntVector(-2, Y - 1, Z - 1), FIntVector(1, Y + 1, Z + 1));
		}
		for (int32 Step = 0; Step < 14; ++Step)
		{
			const int32 Y = Side * (3 + Step / 5);
			const int32 Z = -6 - Step;
			AddBox(FIntVector(-2, Y - 1, Z), FIntVector(1, Y + 1, Z + 1));
		}
		AddBox(FIntVector(-3, Side * 5 - 2, -21), FIntVector(3, Side * 5 + 2, -19));
	}

	TArray<FTransform> ShadeTransforms[EmberdeepVoxelStyle::ShadeCount];
	for (const FIntVector& Cell : OccupiedCells)
	{
		const int32 ShadeIndex = EmberdeepVoxelStyle::SelectShade(Cell.X, Cell.Y, Cell.Z, 31);
		ShadeTransforms[ShadeIndex].Emplace(
			FRotator::ZeroRotator,
			EmberdeepVoxelStyle::CellCenter(Cell.X, Cell.Y, Cell.Z),
			EmberdeepVoxelStyle::InstanceScale());
	}
	for (int32 ShadeIndex = 0; ShadeIndex < EmberdeepVoxelStyle::ShadeCount; ++ShadeIndex)
	{
		BoneVoxelMeshes[ShadeIndex]->AddInstances(ShadeTransforms[ShadeIndex], false, false, true);
	}

	AttackTelegraph = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttackTelegraph"));
	AttackTelegraph->SetupAttachment(RootComponent);
	AttackTelegraph->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttackTelegraph->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
	AttackTelegraph->SetRelativeScale3D(FVector(2.7f, 2.7f, 0.025f));
	AttackTelegraph->SetHiddenInGame(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	AttackTelegraph->SetStaticMesh(CylinderMesh.Object);
}

void AEmberdeepEnemy::BeginPlay()
{
	Super::BeginPlay();
	HealthComponent->OnDeath.AddUObject(this, &AEmberdeepEnemy::HandleDeath);

	const FLinearColor BoneColor(0.48f, 0.42f, 0.31f);
	for (int32 ShadeIndex = 0; ShadeIndex < BoneVoxelMeshes.Num(); ++ShadeIndex)
	{
		if (UMaterialInstanceDynamic* Material = BoneVoxelMeshes[ShadeIndex]->CreateDynamicMaterialInstance(0))
		{
			Material->SetVectorParameterValue(TEXT("Color"), EmberdeepVoxelStyle::ShadeColor(BoneColor, ShadeIndex));
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
	ConfigureForRun(1, true);
}

void AEmberdeepEnemy::ConfigureForRun(int32 Tier, bool bElite)
{
	RunTier = FMath::Max(1, Tier);
	bIsElite = bElite;
	const float TierScale = 1.0f + static_cast<float>(RunTier - 1) * 0.22f;
	HealthComponent->SetMaxHealth((bIsElite ? 280.0f : 92.0f) * TierScale);
	AttackDamage = (bIsElite ? 46.0f : 18.0f) * (1.0f + static_cast<float>(RunTier - 1) * 0.16f);
	AttackRange = bIsElite ? 165.0f : 125.0f;
	AttackCooldown = bIsElite ? 2.25f : 1.15f;
	AggroRange = bIsElite ? 680.0f : 520.0f;
	GoldDropValue = FMath::RoundToInt((bIsElite ? 30.0f : 7.0f) * TierScale);
	GetCharacterMovement()->MaxWalkSpeed = bIsElite ? 225.0f : 265.0f;
	SetActorScale3D(bIsElite ? FVector(1.25f) : FVector::OneVector);
	ApplyBoneColor(bIsElite ? FLinearColor(0.34f, 0.10f, 0.055f) : FLinearColor(0.48f, 0.42f, 0.31f));
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
			Killer->AddExperience((bIsElite ? 50 : 20) + (RunTier - 1) * 10);
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
	for (int32 ShadeIndex = 0; ShadeIndex < BoneMaterials.Num(); ++ShadeIndex)
	{
		BoneMaterials[ShadeIndex]->SetVectorParameterValue(
			TEXT("Color"),
			EmberdeepVoxelStyle::ShadeColor(Color, ShadeIndex));
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
			GameMode->SpawnLootForEnemy(DropLocation + FVector(0.0f, 75.0f, 0.0f), bIsElite);
			GameMode->NotifyEnemyDefeated();
		}
	}

	SetLifeSpan(0.35f);
}
