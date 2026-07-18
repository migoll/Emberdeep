#include "Gameplay/EmberdeepEnemy.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Gameplay/EmberdeepCharacter.h"
#include "Gameplay/EmberdeepCombatFeedback.h"
#include "Gameplay/EmberdeepGameMode.h"
#include "Gameplay/EmberdeepGoldPickup.h"
#include "Gameplay/EmberdeepHealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
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

	EnemyVisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("EnemyVisualRoot"));
	EnemyVisualRoot->SetupAttachment(RootComponent);
	EnemyVisualRoot->SetUsingAbsoluteRotation(true);

	HealthComponent = CreateDefaultSubobject<UEmberdeepHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->SetMaxHealth(92.0f);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ProjectVoxelMaterial(
		TEXT("/Game/Emberdeep/Materials/M_VoxelSurface.M_VoxelSurface"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FallbackVoxelMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	UMaterialInterface* VoxelMaterial = ProjectVoxelMaterial.Succeeded()
		? ProjectVoxelMaterial.Object
		: FallbackVoxelMaterial.Object;
	for (int32 ShadeIndex = 0; ShadeIndex < EmberdeepVoxelStyle::ShadeCount; ++ShadeIndex)
	{
		const FName MeshName(*FString::Printf(TEXT("BoneVoxels_Shade%d"), ShadeIndex));
		UInstancedStaticMeshComponent* BoneMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(MeshName);
		BoneMesh->SetupAttachment(EnemyVisualRoot);
		BoneMesh->SetStaticMesh(CubeMesh.Object);
		BoneMesh->SetMaterial(0, VoxelMaterial);
		BoneMesh->SetMobility(EComponentMobility::Movable);
		BoneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BoneMesh->SetGenerateOverlapEvents(false);
		BoneMesh->SetCanEverAffectNavigation(false);
		BoneVoxelMeshes.Add(BoneMesh);
	}

	TSet<FIntVector> OccupiedCells;
	const auto AddCell = [&OccupiedCells](int32 X, int32 Y, int32 Z)
	{
		OccupiedCells.Add(FIntVector(X, Y, Z));
	};
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
	const auto AddBoneLine = [&AddBox](const FIntVector& Start, const FIntVector& End, int32 Steps)
	{
		for (int32 Step = 0; Step <= Steps; ++Step)
		{
			const float Alpha = static_cast<float>(Step) / static_cast<float>(FMath::Max(Steps, 1));
			const FIntVector Point(
				FMath::RoundToInt(FMath::Lerp(static_cast<float>(Start.X), static_cast<float>(End.X), Alpha)),
				FMath::RoundToInt(FMath::Lerp(static_cast<float>(Start.Y), static_cast<float>(End.Y), Alpha)),
				FMath::RoundToInt(FMath::Lerp(static_cast<float>(Start.Z), static_cast<float>(End.Z), Alpha)));
			AddBox(Point + FIntVector(-1, -1, -1), Point);
		}
	};

	// Sparse anatomy is the silhouette: separated shins, a ladder-like rib cage,
	// long arms, oversized hands and a large skull. The old filled cuboids read as
	// brown statues; these clusters remain unmistakably skeletal at isometric scale.
	AddBox(FIntVector(-2, -6, -17), FIntVector(4, -2, -16));
	AddBox(FIntVector(-2, 2, -17), FIntVector(4, 6, -16));
	AddBoneLine(FIntVector(0, -4, -15), FIntVector(0, -4, -5), 10);
	AddBoneLine(FIntVector(0, 4, -15), FIntVector(0, 4, -5), 10);
	AddBox(FIntVector(-2, -6, -5), FIntVector(2, 6, -3));
	AddBox(FIntVector(-1, -5, -2), FIntVector(1, -3, -1));
	AddBox(FIntVector(-1, 3, -2), FIntVector(1, 5, -1));
	AddBoneLine(FIntVector(0, 0, -2), FIntVector(0, 0, 8), 10);

	for (int32 RibZ : {1, 4, 7})
	{
		const int32 HalfWidth = RibZ == 7 ? 7 : (RibZ == 4 ? 6 : 5);
		AddBox(FIntVector(-1, -HalfWidth, RibZ), FIntVector(1, HalfWidth, RibZ + 1));
		// Short front prongs give the chest depth without filling the cavity.
		AddBox(FIntVector(2, -HalfWidth, RibZ), FIntVector(3, -HalfWidth + 1, RibZ + 1));
		AddBox(FIntVector(2, HalfWidth - 1, RibZ), FIntVector(3, HalfWidth, RibZ + 1));
	}
	AddBox(FIntVector(-1, -8, 8), FIntVector(1, 8, 9));
	for (const int32 Side : {-1, 1})
	{
		AddBoneLine(FIntVector(0, Side * 8, 8), FIntVector(0, Side * 11, 2), 7);
		AddBoneLine(FIntVector(0, Side * 11, 2), FIntVector(1, Side * 13, -4), 7);
		const int32 HandMinY = Side < 0 ? -15 : 12;
		const int32 HandMaxY = Side < 0 ? -12 : 15;
		AddBox(FIntVector(-1, HandMinY, -6), FIntVector(2, HandMaxY, -3));
	}

	AddBox(FIntVector(-1, -2, 9), FIntVector(1, 2, 10));
	AddBox(FIntVector(-1, -3, 9), FIntVector(3, 3, 11)); // jaw
	for (int32 Z = 11; Z <= 18; ++Z)
	{
		for (int32 Y = -4; Y <= 4; ++Y)
		{
			for (int32 X = -3; X <= 3; ++X)
			{
				const bool bSurface = X == -3 || X == 3 || Y == -4 || Y == 4 || Z == 11 || Z == 18;
				if (bSurface)
				{
					AddCell(X, Y, Z);
				}
			}
		}
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

	// A readable weapon is more important than anatomical micro-detail at the
	// gameplay camera. The sword is built from the same 4 cm cells and moves as a
	// rigid cluster, giving every skeleton a clear attack side and silhouette.
	WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
	WeaponRoot->SetupAttachment(EnemyVisualRoot);
	WeaponRoot->SetRelativeLocation(FVector(4.0f, 48.0f, -18.0f));
	WeaponRoot->SetRelativeRotation(FRotator(0.0f, 0.0f, 150.0f));

	WeaponSteelVoxels = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WeaponSteelVoxels"));
	WeaponSteelVoxels->SetupAttachment(WeaponRoot);
	WeaponSteelVoxels->SetStaticMesh(CubeMesh.Object);
	WeaponSteelVoxels->SetMaterial(0, VoxelMaterial);
	WeaponSteelVoxels->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponSteelVoxels->SetGenerateOverlapEvents(false);
	WeaponSteelVoxels->SetCanEverAffectNavigation(false);

	WeaponGripVoxels = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WeaponGripVoxels"));
	WeaponGripVoxels->SetupAttachment(WeaponRoot);
	WeaponGripVoxels->SetStaticMesh(CubeMesh.Object);
	WeaponGripVoxels->SetMaterial(0, VoxelMaterial);
	WeaponGripVoxels->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponGripVoxels->SetGenerateOverlapEvents(false);
	WeaponGripVoxels->SetCanEverAffectNavigation(false);

	const auto AddWeaponCell = [](UInstancedStaticMeshComponent* TargetMesh, int32 X, int32 Y, int32 Z)
	{
		TargetMesh->AddInstance(FTransform(
			FQuat::Identity,
			EmberdeepVoxelStyle::CellCenter(X, Y, Z),
			EmberdeepVoxelStyle::InstanceScale()));
	};
	for (int32 Z = -4; Z <= 3; ++Z)
	{
		for (int32 X = -1; X <= 0; ++X)
		{
			AddWeaponCell(WeaponGripVoxels, X, 0, Z);
		}
	}
	for (int32 Y = -5; Y <= 5; ++Y)
	{
		AddWeaponCell(WeaponSteelVoxels, 0, Y, 4);
	}
	for (int32 Z = 5; Z <= 22; ++Z)
	{
		AddWeaponCell(WeaponSteelVoxels, 0, 0, Z);
		if (Z <= 20)
		{
			AddWeaponCell(WeaponSteelVoxels, 1, 0, Z);
			AddWeaponCell(WeaponSteelVoxels, 0, 1, Z);
		}
	}
	AddWeaponCell(WeaponSteelVoxels, 0, 0, 23);

	AccentVoxels = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("EnemyAccentVoxels"));
	AccentVoxels->SetupAttachment(EnemyVisualRoot);
	AccentVoxels->SetStaticMesh(CubeMesh.Object);
	AccentVoxels->SetMaterial(0, VoxelMaterial);
	AccentVoxels->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AccentVoxels->SetGenerateOverlapEvents(false);
	AccentVoxels->SetCanEverAffectNavigation(false);
	// Recessed dark eye sockets stop the pale skull from becoming a featureless box.
	for (int32 EyeY : {-2, 2})
	{
		AddWeaponCell(AccentVoxels, 4, EyeY, 15);
		AddWeaponCell(AccentVoxels, 4, EyeY, 16);
	}
	AddWeaponCell(AccentVoxels, 4, 0, 13);

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

	VisualRestingLocation = EnemyVisualRoot->GetRelativeLocation();
	VisualRestingRotation = EnemyVisualRoot->GetRelativeRotation();
	VisualFacingYaw = GetActorRotation().Yaw;
	WeaponRestingLocation = WeaponRoot->GetRelativeLocation();
	WeaponRestingRotation = WeaponRoot->GetRelativeRotation();

	const FLinearColor BoneColor(0.72f, 0.54f, 0.29f);
	for (int32 ShadeIndex = 0; ShadeIndex < BoneVoxelMeshes.Num(); ++ShadeIndex)
	{
		if (UMaterialInstanceDynamic* Material = BoneVoxelMeshes[ShadeIndex]->CreateDynamicMaterialInstance(0))
		{
			Material->SetVectorParameterValue(TEXT("Color"), EmberdeepVoxelStyle::ShadeColor(BoneColor, ShadeIndex));
			Material->SetScalarParameterValue(TEXT("EmissiveStrength"), 0.07f);
			BoneVoxelMeshes[ShadeIndex]->SetMaterial(0, Material);
			BoneMaterials.Add(Material);
		}
	}
	WeaponSteelMaterial = WeaponSteelVoxels->CreateDynamicMaterialInstance(0);
	if (WeaponSteelMaterial)
	{
		WeaponSteelMaterial->SetVectorParameterValue(
			TEXT("Color"), FLinearColor(0.28f, 0.35f, 0.43f));
		WeaponSteelMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), 0.05f);
		WeaponSteelVoxels->SetMaterial(0, WeaponSteelMaterial);
	}
	WeaponGripMaterial = WeaponGripVoxels->CreateDynamicMaterialInstance(0);
	if (WeaponGripMaterial)
	{
		WeaponGripMaterial->SetVectorParameterValue(
			TEXT("Color"), FLinearColor(0.13f, 0.045f, 0.012f));
		WeaponGripMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), 0.03f);
		WeaponGripVoxels->SetMaterial(0, WeaponGripMaterial);
	}
	AccentMaterial = AccentVoxels->CreateDynamicMaterialInstance(0);
	if (AccentMaterial)
	{
		AccentMaterial->SetVectorParameterValue(
			TEXT("Color"), FLinearColor::FromSRGBColor(FColor(13, 10, 9)));
		AccentMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), 0.05f);
		AccentVoxels->SetMaterial(0, AccentMaterial);
	}

	if (UMaterialInstanceDynamic* TelegraphMaterial = AttackTelegraph->CreateDynamicMaterialInstance(0))
	{
		TelegraphMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.9f, 0.015f, 0.005f));
		TelegraphMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), 3.0f);
	}
	ApplyEnemyStyle();
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
	ApplyEnemyStyle();
}

void AEmberdeepEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateVisualPresentation(DeltaSeconds);
	if (HasAuthority() && !HealthComponent->IsDead())
	{
		UpdateServerAI();
	}
}

void AEmberdeepEnemy::UpdateVisualPresentation(float DeltaSeconds)
{
	(void)DeltaSeconds;
	ApplySteppedVisualPose(false);
}

void AEmberdeepEnemy::ApplySteppedVisualPose(bool bForce)
{
	if (!EnemyVisualRoot || !WeaponRoot || !GetWorld())
	{
		return;
	}

	constexpr float VisualPoseRate = 12.0f;
	const float Now = GetWorld()->GetTimeSeconds();
	const int32 PoseStep = FMath::FloorToInt(Now * VisualPoseRate);
	if (!bForce && LastVisualPoseStep == PoseStep)
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
	const float Gait = FMath::Sin(PoseTime * UE_PI * 5.2f);
	const float IdleRattle = FMath::Sin(PoseTime * UE_PI * 1.9f);

	FVector VisualOffset(0.0f, 0.0f, FMath::Lerp(IdleRattle * 0.7f, FMath::Abs(Gait) * 3.6f, MoveAlpha));
	FRotator VisualRotation(
		-LocalVelocity.X * 4.5f * MoveAlpha,
		IdleRattle * 1.2f * (1.0f - MoveAlpha),
		LocalVelocity.Y * 4.8f * MoveAlpha + Gait * 2.1f * MoveAlpha);
	FVector WeaponOffset = WeaponRestingLocation + FVector(0.0f, 0.0f, -Gait * 1.8f * MoveAlpha);
	FRotator WeaponRotation = WeaponRestingRotation + FRotator(0.0f, Gait * 2.0f * MoveAlpha, Gait * 5.0f * MoveAlpha);

	if (bAttackWindingUp)
	{
		const float WindupDuration = bIsElite ? 0.72f : 0.30f;
		const float WindupAlpha = FMath::Clamp((Now - VisualWindupStartTime) / WindupDuration, 0.0f, 1.0f);
		const float WindupEase = FMath::InterpEaseIn(0.0f, 1.0f, WindupAlpha, 2.0f);
		VisualOffset.Z -= 3.5f * WindupEase;
		VisualRotation.Pitch += 7.0f * WindupEase;
		VisualRotation.Yaw -= 8.0f * WindupEase;
		WeaponRotation = WeaponRestingRotation + FRotator(-10.0f * WindupEase, -8.0f * WindupEase, 76.0f * WindupEase);
		WeaponOffset = WeaponRestingLocation + FVector(-4.0f, 0.0f, 8.0f) * WindupEase;

		const float Pulse = 1.0f + 0.09f * (0.5f + 0.5f * FMath::Sin(PoseTime * UE_PI * 9.0f));
		const FVector TelegraphBase = bIsElite
			? FVector(3.2f, 3.2f, 0.025f)
			: FVector(2.05f, 2.05f, 0.020f);
		AttackTelegraph->SetRelativeScale3D(FVector(TelegraphBase.X * Pulse, TelegraphBase.Y * Pulse, TelegraphBase.Z));
	}
	else
	{
		const float RecoveryElapsed = Now - VisualAttackRecoveryStartTime;
		if (RecoveryElapsed >= 0.0f && RecoveryElapsed < 0.24f)
		{
			const float RecoveryEnvelope = 1.0f - RecoveryElapsed / 0.24f;
			VisualRotation.Pitch -= 10.0f * RecoveryEnvelope;
			VisualRotation.Yaw += 6.0f * RecoveryEnvelope;
			WeaponRotation = WeaponRestingRotation + FRotator(8.0f, 5.0f, -118.0f * RecoveryEnvelope);
			WeaponOffset = WeaponRestingLocation + FVector(8.0f, 0.0f, -4.0f) * RecoveryEnvelope;
		}
	}

	const float HitElapsed = Now - VisualHitStartTime;
	if (HitElapsed >= 0.0f && HitElapsed < 0.16f)
	{
		const float HitEnvelope = 1.0f - HitElapsed / 0.16f;
		VisualRotation.Pitch += 11.0f * HitEnvelope;
		VisualRotation.Roll -= 5.0f * HitEnvelope;
		VisualOffset.X -= 4.0f * HitEnvelope;
	}

	if (bVisualDead)
	{
		const float DeathAlpha = FMath::Clamp((Now - VisualDeathStartTime) / 0.62f, 0.0f, 1.0f);
		const float DeathEase = FMath::InterpEaseOut(0.0f, 1.0f, DeathAlpha, 2.0f);
		VisualOffset = FVector(0.0f, 0.0f, -38.0f * DeathEase);
		VisualRotation = FRotator(6.0f * DeathEase, 8.0f * DeathEase, -82.0f * DeathEase);
		WeaponRotation = WeaponRestingRotation + FRotator(0.0f, 12.0f * DeathEase, 105.0f * DeathEase);
		WeaponOffset = WeaponRestingLocation + FVector(0.0f, 8.0f, -28.0f) * DeathEase;
	}

	EnemyVisualRoot->SetRelativeLocation(VisualRestingLocation + VisualOffset);
	EnemyVisualRoot->SetWorldRotation(FRotator(
		VisualRestingRotation.Pitch + VisualRotation.Pitch,
		VisualFacingYaw + VisualRestingRotation.Yaw + VisualRotation.Yaw,
		VisualRestingRotation.Roll + VisualRotation.Roll));
	WeaponRoot->SetRelativeLocation(WeaponOffset);
	WeaponRoot->SetRelativeRotation(WeaponRotation);
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
	bAttackWindingUp = true;
	PendingAttackTarget = TargetCharacter;
	MulticastSetAttackTelegraph(true, bIsElite);
	GetCharacterMovement()->StopMovementImmediately();
	FTimerHandle WindupTimer;
	GetWorldTimerManager().SetTimer(
		WindupTimer,
		this,
		&AEmberdeepEnemy::ResolveEliteAttack,
		bIsElite ? 0.72f : 0.30f,
		false);
}

void AEmberdeepEnemy::ResolveEliteAttack()
{
	bAttackWindingUp = false;
	MulticastSetAttackTelegraph(false, bIsElite);

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
	const bool bFatal = HealthComponent->IsDead();
	if (HealthComponent->IsDead())
	{
		if (AEmberdeepCharacter* Killer = Cast<AEmberdeepCharacter>(DamageCauser))
		{
			Killer->AddExperience((bIsElite ? 50 : 20) + (RunTier - 1) * 10);
		}
	}

	FVector HitDirection = DamageCauser
		? (GetActorLocation() - DamageCauser->GetActorLocation()).GetSafeNormal2D()
		: FVector::ForwardVector;
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent* PointDamage = static_cast<const FPointDamageEvent*>(&DamageEvent);
		const FVector RequestedDirection = FVector(PointDamage->ShotDirection).GetSafeNormal2D();
		if (!RequestedDirection.IsNearlyZero())
		{
			HitDirection = RequestedDirection;
		}
	}
	const float KnockbackStrength = DamageAmount >= 55.0f ? 620.0f : 275.0f;
	LaunchCharacter(HitDirection * KnockbackStrength + FVector(0.0f, 0.0f, bFatal ? 165.0f : 70.0f), true, false);
	MulticastPlayHitFeedback(AppliedDamage, HitDirection, bFatal);
	return AppliedDamage;
}

void AEmberdeepEnemy::MulticastPlayHitFeedback_Implementation(
	float Damage,
	FVector_NetQuantizeNormal HitDirection,
	bool bFatal)
{
	ApplyBoneColor(bFatal
		? FLinearColor::FromSRGBColor(FColor(255, 82, 10))
		: FLinearColor::FromSRGBColor(FColor(255, 48, 12)));
	VisualHitStartTime = GetWorld()->GetTimeSeconds();
	ApplySteppedVisualPose(true);
	AEmberdeepCombatFeedback::SpawnHit(
		GetWorld(),
		GetActorLocation(),
		FVector(HitDirection),
		Damage,
		bFatal);
	if (!bFatal)
	{
		FTimerHandle FlashTimer;
		GetWorldTimerManager().SetTimer(FlashTimer, this, &AEmberdeepEnemy::ResetHitFlash, 0.10f, false);
	}
}

void AEmberdeepEnemy::MulticastSetAttackTelegraph_Implementation(bool bVisible, bool bEliteAttack)
{
	bAttackWindingUp = bVisible;
	if (bVisible)
	{
		VisualWindupStartTime = GetWorld()->GetTimeSeconds();
	}
	else
	{
		VisualAttackRecoveryStartTime = GetWorld()->GetTimeSeconds();
	}
	AttackTelegraph->SetRelativeScale3D(bEliteAttack
		? FVector(3.2f, 3.2f, 0.025f)
		: FVector(2.05f, 2.05f, 0.020f));
	AttackTelegraph->SetHiddenInGame(!bVisible);
	ApplySteppedVisualPose(true);
}

void AEmberdeepEnemy::ResetHitFlash()
{
	ApplyBoneColor(bIsElite
		? FLinearColor(0.62f, 0.10f, 0.025f)
		: FLinearColor(0.72f, 0.54f, 0.29f));
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

void AEmberdeepEnemy::ApplyEnemyStyle()
{
	// Never resize the actor to make an elite: that would also resize every 4 cm
	// cell and break the common world lattice. Palette and weapon contrast carry it.
	SetActorScale3D(FVector::OneVector);
	ApplyBoneColor(bIsElite
		? FLinearColor(0.62f, 0.10f, 0.025f)
		: FLinearColor(0.72f, 0.54f, 0.29f));
	if (WeaponSteelMaterial)
	{
		WeaponSteelMaterial->SetVectorParameterValue(
			TEXT("Color"),
			bIsElite
				? FLinearColor(0.78f, 0.11f, 0.025f)
				: FLinearColor(0.28f, 0.35f, 0.43f));
	}
}

void AEmberdeepEnemy::OnRep_EnemyStyle()
{
	ApplyEnemyStyle();
}

void AEmberdeepEnemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEmberdeepEnemy, bIsElite);
}

void AEmberdeepEnemy::HandleDeath()
{
	bVisualDead = true;
	VisualDeathStartTime = GetWorld()->GetTimeSeconds();
	ApplySteppedVisualPose(true);
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MulticastSetAttackTelegraph(false, bIsElite);

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

	SetLifeSpan(0.78f);
}
