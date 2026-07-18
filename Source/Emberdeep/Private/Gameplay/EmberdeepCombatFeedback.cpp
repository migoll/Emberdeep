#include "Gameplay/EmberdeepCombatFeedback.h"

#include "Components/PointLightComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AEmberdeepCombatFeedback::AEmberdeepCombatFeedback()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	SetActorEnableCollision(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ProjectVoxelMaterial(
		TEXT("/Game/Emberdeep/Materials/M_VoxelSurface.M_VoxelSurface"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FallbackVoxelMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	UMaterialInterface* VoxelMaterial = ProjectVoxelMaterial.Succeeded()
		? ProjectVoxelMaterial.Object
		: FallbackVoxelMaterial.Object;

	Shockwave = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shockwave"));
	Shockwave->SetupAttachment(SceneRoot);
	Shockwave->SetStaticMesh(CylinderMesh.Object);
	Shockwave->SetMaterial(0, VoxelMaterial);
	Shockwave->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Shockwave->SetCastShadow(false);
	Shockwave->SetHiddenInGame(true);

	ArcSegments = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ArcSegments"));
	ArcSegments->SetupAttachment(SceneRoot);
	ArcSegments->SetStaticMesh(CubeMesh.Object);
	ArcSegments->SetMaterial(0, VoxelMaterial);
	ArcSegments->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ArcSegments->SetGenerateOverlapEvents(false);
	ArcSegments->SetCastShadow(false);
	ArcSegments->SetHiddenInGame(true);

	ImpactRing = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ImpactRing"));
	ImpactRing->SetupAttachment(SceneRoot);
	ImpactRing->SetStaticMesh(CubeMesh.Object);
	ImpactRing->SetMaterial(0, VoxelMaterial);
	ImpactRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ImpactRing->SetGenerateOverlapEvents(false);
	ImpactRing->SetCastShadow(false);
	ImpactRing->SetHiddenInGame(true);

	DamageText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DamageText"));
	DamageText->SetupAttachment(SceneRoot);
	DamageText->SetHorizontalAlignment(EHTA_Center);
	DamageText->SetVerticalAlignment(EVRTA_TextCenter);
	DamageText->SetWorldSize(42.0f);
	DamageText->SetTextRenderColor(FColor::White);
	DamageText->SetTranslucentSortPriority(20);
	DamageText->SetHiddenInGame(true);

	FlashLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FlashLight"));
	FlashLight->SetupAttachment(SceneRoot);
	FlashLight->SetAttenuationRadius(310.0f);
	FlashLight->SetCastShadows(false);
	FlashLight->SetIntensity(0.0f);

	constexpr int32 ShardCount = 12;
	for (int32 Index = 0; Index < ShardCount; ++Index)
	{
		UStaticMeshComponent* Shard = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("ImpactShard_%02d"), Index));
		Shard->SetupAttachment(SceneRoot);
		Shard->SetStaticMesh(CubeMesh.Object);
		Shard->SetMaterial(0, VoxelMaterial);
		Shard->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Shard->SetCastShadow(false);
		Shard->SetHiddenInGame(true);
		Shards.Add(Shard);
	}
}

AEmberdeepCombatFeedback* AEmberdeepCombatFeedback::SpawnFeedback(UWorld* World, const FVector& Location)
{
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return nullptr;
	}

	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Parameters.ObjectFlags |= RF_Transient;
	return World->SpawnActor<AEmberdeepCombatFeedback>(
		AEmberdeepCombatFeedback::StaticClass(), Location, FRotator::ZeroRotator, Parameters);
}

void AEmberdeepCombatFeedback::SpawnSwing(
	UWorld* World,
	const FVector& Location,
	const FVector& Direction,
	bool bHeavyAttack,
	bool bConnected)
{
	if (AEmberdeepCombatFeedback* Feedback = SpawnFeedback(World, Location))
	{
		Feedback->ConfigureSwing(Direction, bHeavyAttack, bConnected);
	}
}

void AEmberdeepCombatFeedback::SpawnHit(
	UWorld* World,
	const FVector& Location,
	const FVector& Direction,
	float Damage,
	bool bFatal)
{
	if (AEmberdeepCombatFeedback* Feedback = SpawnFeedback(World, Location))
	{
		Feedback->ConfigureHit(Direction, Damage, bFatal, false);
	}
}

void AEmberdeepCombatFeedback::SpawnPlayerHurt(UWorld* World, const FVector& Location, float Damage, bool bFatal)
{
	if (AEmberdeepCombatFeedback* Feedback = SpawnFeedback(World, Location))
	{
		Feedback->ConfigureHit(FVector::ZeroVector, Damage, bFatal, true);
	}
}

void AEmberdeepCombatFeedback::SpawnDodge(
	UWorld* World,
	const FVector& Location,
	const FVector& Direction)
{
	if (AEmberdeepCombatFeedback* Feedback = SpawnFeedback(World, Location))
	{
		Feedback->ConfigureDodge(Direction);
	}
}

void AEmberdeepCombatFeedback::SetComponentColor(UStaticMeshComponent* Component, const FLinearColor& Color)
{
	if (Component)
	{
		if (UMaterialInstanceDynamic* Material = Component->CreateDynamicMaterialInstance(0))
		{
			Material->SetVectorParameterValue(TEXT("Color"), Color);
			Material->SetScalarParameterValue(TEXT("Roughness"), 0.72f);
			Material->SetScalarParameterValue(TEXT("Specular"), 0.12f);
			Material->SetScalarParameterValue(TEXT("EmissiveStrength"), 4.0f);
		}
	}
}

void AEmberdeepCombatFeedback::ConfigureSwing(const FVector& Direction, bool bHeavyAttack, bool bConnected)
{
	Lifetime = bHeavyAttack ? 0.34f : 0.22f;
	TextRiseSpeed = 0.0f;
	const FLinearColor Color = bHeavyAttack
		? FLinearColor(1.0f, 0.34f, 0.025f)
		: bConnected
			? FLinearColor(1.0f, 0.78f, 0.30f)
			: FLinearColor(0.52f, 0.66f, 0.82f);

	const FVector SafeDirection = Direction.GetSafeNormal2D();
	const FVector Side(-SafeDirection.Y, SafeDirection.X, 0.0f);
	ArcSegments->ClearInstances();
	ArcSegments->SetHiddenInGame(false);
	ArcSegments->SetRelativeLocation(FVector(0.0f, 0.0f, -12.0f));
	ArcSegments->SetRelativeScale3D(FVector::OneVector);
	SetComponentColor(ArcSegments, Color);
	const int32 ArcCount = bHeavyAttack ? 19 : 15;
	const float ArcRadius = bHeavyAttack ? 148.0f : 116.0f;
	const float ArcHalfAngle = bHeavyAttack ? 73.0f : 61.0f;
	for (int32 SegmentIndex = 0; SegmentIndex < ArcCount; ++SegmentIndex)
	{
		const float SegmentAlpha = ArcCount > 1
			? static_cast<float>(SegmentIndex) / static_cast<float>(ArcCount - 1)
			: 0.5f;
		const float AngleRadians = FMath::DegreesToRadians(FMath::Lerp(-ArcHalfAngle, ArcHalfAngle, SegmentAlpha));
		const FVector Radial = SafeDirection * FMath::Cos(AngleRadians) + Side * FMath::Sin(AngleRadians);
		const FVector SegmentLocation = Radial * ArcRadius;
		const float TangentYaw = Radial.Rotation().Yaw + 90.0f;
		const float EdgeFade = 0.45f + FMath::Sin(SegmentAlpha * UE_PI) * 0.55f;
		ArcSegments->AddInstance(FTransform(
			FRotator(0.0f, TangentYaw, 0.0f),
			SegmentLocation,
			FVector((bHeavyAttack ? 0.34f : 0.27f) * EdgeFade, 0.075f * EdgeFade, 0.045f)));
	}

	ShardVelocities.SetNum(Shards.Num());
	ShardSpin.SetNum(Shards.Num());
	for (int32 Index = 0; Index < Shards.Num(); ++Index)
	{
		const float Across = static_cast<float>(Index) - static_cast<float>(Shards.Num() - 1) * 0.5f;
		UStaticMeshComponent* Shard = Shards[Index];
		Shard->SetHiddenInGame(false);
		Shard->SetRelativeLocation(Side * Across * 11.0f + SafeDirection * FMath::Abs(Across) * -4.0f);
		Shard->SetRelativeScale3D(FVector(0.06f, bHeavyAttack ? 0.17f : 0.11f, 0.035f));
		SetComponentColor(Shard, Color);
		ShardVelocities[Index] = SafeDirection * (100.0f + Index * 4.0f) + Side * Across * 13.0f + FVector(0.0f, 0.0f, 35.0f);
		ShardSpin[Index] = FRotator(180.0f + Index * 9.0f, 220.0f, 140.0f);
	}

	InitialLightIntensity = bHeavyAttack ? 5200.0f : (bConnected ? 2600.0f : 900.0f);
	FlashLight->SetLightColor(Color);
	FlashLight->SetIntensity(InitialLightIntensity);
}

void AEmberdeepCombatFeedback::ConfigureHit(
	const FVector& Direction,
	float Damage,
	bool bFatal,
	bool bPlayerHurt)
{
	Lifetime = bFatal ? 0.72f : 0.52f;
	const FLinearColor Color = bPlayerHurt
		? FLinearColor(0.90f, 0.025f, 0.012f)
		: bFatal
			? FLinearColor(1.0f, 0.48f, 0.025f)
			: FLinearColor(1.0f, 0.86f, 0.46f);

	DamageText->SetHiddenInGame(false);
	DamageText->SetRelativeLocation(FVector(0.0f, 0.0f, bPlayerHurt ? 145.0f : 115.0f));
	DamageText->SetWorldSize(bFatal ? 58.0f : 43.0f);
	DamageText->SetText(FText::FromString(bFatal
		? FString::Printf(TEXT("%d!"), FMath::RoundToInt(Damage))
		: FString::Printf(TEXT("%d"), FMath::RoundToInt(Damage))));
	DamageText->SetTextRenderColor(Color.ToFColorSRGB());

	ImpactRing->ClearInstances();
	ImpactRing->SetHiddenInGame(false);
	ImpactRing->SetRelativeLocation(FVector(0.0f, 0.0f, -78.0f));
	InitialRingScale = FVector(bFatal ? 0.85f : 0.68f);
	FinalRingScale = FVector(bFatal ? 3.25f : 1.85f);
	ImpactRing->SetRelativeScale3D(InitialRingScale);
	SetComponentColor(ImpactRing, Color);
	const int32 RingCount = bFatal ? 18 : 14;
	for (int32 SegmentIndex = 0; SegmentIndex < RingCount; ++SegmentIndex)
	{
		const float Angle = 360.0f * static_cast<float>(SegmentIndex) / static_cast<float>(RingCount);
		const FVector Radial = FRotator(0.0f, Angle, 0.0f).Vector();
		ImpactRing->AddInstance(FTransform(
			FRotator(0.0f, Angle + 90.0f, 0.0f),
			Radial * 34.0f,
			FVector(bFatal ? 0.22f : 0.16f, 0.045f, 0.025f)));
	}

	const FVector SafeDirection = Direction.IsNearlyZero() ? FVector::ForwardVector : Direction.GetSafeNormal2D();
	const FVector Side(-SafeDirection.Y, SafeDirection.X, 0.0f);
	const int32 VisibleShardCount = bFatal ? Shards.Num() : 7;
	ShardVelocities.SetNum(Shards.Num());
	ShardSpin.SetNum(Shards.Num());
	for (int32 Index = 0; Index < Shards.Num(); ++Index)
	{
		if (Index >= VisibleShardCount)
		{
			continue;
		}
		const float AlternatingSide = Index % 2 == 0 ? 1.0f : -1.0f;
		const float Spread = (20.0f + Index * 10.0f) * AlternatingSide;
		UStaticMeshComponent* Shard = Shards[Index];
		Shard->SetHiddenInGame(false);
		Shard->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f + (Index % 3) * 14.0f));
		Shard->SetRelativeScale3D(FVector(bFatal ? 0.075f : 0.045f));
		SetComponentColor(Shard, Index % 3 == 0 ? FLinearColor(0.55f, 0.035f, 0.012f) : Color);
		ShardVelocities[Index] = SafeDirection * (90.0f + Index * 10.0f) + Side * Spread + FVector(0.0f, 0.0f, 105.0f + Index * 9.0f);
		ShardSpin[Index] = FRotator(260.0f + Index * 12.0f, 190.0f + Index * 8.0f, 230.0f);
	}

	InitialLightIntensity = bFatal ? 8200.0f : 4200.0f;
	FlashLight->SetLightColor(Color);
	FlashLight->SetIntensity(InitialLightIntensity);
}

void AEmberdeepCombatFeedback::ConfigureDodge(const FVector& Direction)
{
	Lifetime = 0.34f;
	TextRiseSpeed = 0.0f;
	const FLinearColor Color(0.18f, 0.48f, 0.88f);
	ImpactRing->ClearInstances();
	ImpactRing->SetHiddenInGame(false);
	ImpactRing->SetRelativeLocation(FVector(0.0f, 0.0f, -78.0f));
	InitialRingScale = FVector(0.75f);
	FinalRingScale = FVector(1.85f);
	ImpactRing->SetRelativeScale3D(InitialRingScale);
	SetComponentColor(ImpactRing, Color);
	for (int32 SegmentIndex = 0; SegmentIndex < 12; ++SegmentIndex)
	{
		const float Angle = 360.0f * static_cast<float>(SegmentIndex) / 12.0f;
		const FVector Radial = FRotator(0.0f, Angle, 0.0f).Vector();
		ImpactRing->AddInstance(FTransform(
			FRotator(0.0f, Angle + 90.0f, 0.0f),
			Radial * 29.0f,
			FVector(0.14f, 0.04f, 0.02f)));
	}

	const FVector SafeDirection = Direction.GetSafeNormal2D();
	const FVector Backward = -SafeDirection;
	const FVector Side(-SafeDirection.Y, SafeDirection.X, 0.0f);
	ShardVelocities.SetNum(Shards.Num());
	ShardSpin.SetNum(Shards.Num());
	for (int32 Index = 0; Index < 8; ++Index)
	{
		const float SideAmount = (static_cast<float>(Index) - 3.5f) * 18.0f;
		UStaticMeshComponent* Shard = Shards[Index];
		Shard->SetHiddenInGame(false);
		Shard->SetRelativeScale3D(FVector(0.035f, 0.09f, 0.025f));
		SetComponentColor(Shard, Index % 2 == 0 ? Color : FLinearColor(0.28f, 0.31f, 0.36f));
		ShardVelocities[Index] = Backward * (120.0f + Index * 9.0f) + Side * SideAmount + FVector(0.0f, 0.0f, 45.0f);
		ShardSpin[Index] = FRotator(170.0f, 210.0f + Index * 8.0f, 120.0f);
	}
	InitialLightIntensity = 1800.0f;
	FlashLight->SetLightColor(Color);
	FlashLight->SetIntensity(InitialLightIntensity);
}

void AEmberdeepCombatFeedback::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	const float Alpha = FMath::Clamp(Age / FMath::Max(Lifetime, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	const float Remaining = 1.0f - Alpha;

	if (!Shockwave->bHiddenInGame)
	{
		Shockwave->SetRelativeScale3D(FMath::Lerp(
			InitialShockwaveScale,
			FinalShockwaveScale,
			FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.2f)));
	}
	if (!ArcSegments->bHiddenInGame)
	{
		const float ArcExpansion = FMath::InterpEaseOut(1.0f, 1.12f, Alpha, 2.5f);
		ArcSegments->SetRelativeScale3D(FVector(ArcExpansion));
	}
	if (!ImpactRing->bHiddenInGame)
	{
		ImpactRing->SetRelativeScale3D(FMath::Lerp(
			InitialRingScale,
			FinalRingScale,
			FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.4f)));
	}
	FlashLight->SetIntensity(InitialLightIntensity * Remaining * Remaining);

	if (!DamageText->bHiddenInGame)
	{
		DamageText->AddRelativeLocation(FVector(0.0f, 0.0f, TextRiseSpeed * DeltaSeconds));
		if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
		{
			if (PlayerController->PlayerCameraManager)
			{
				const FVector ToCamera = PlayerController->PlayerCameraManager->GetCameraLocation() - DamageText->GetComponentLocation();
				DamageText->SetWorldRotation(ToCamera.Rotation());
			}
		}
	}

	for (int32 Index = 0; Index < Shards.Num(); ++Index)
	{
		UStaticMeshComponent* Shard = Shards[Index];
		if (!Shard || Shard->bHiddenInGame || !ShardVelocities.IsValidIndex(Index))
		{
			continue;
		}
		Shard->AddRelativeLocation(ShardVelocities[Index] * DeltaSeconds);
		ShardVelocities[Index].Z -= 360.0f * DeltaSeconds;
		if (ShardSpin.IsValidIndex(Index))
		{
			Shard->AddRelativeRotation(ShardSpin[Index] * DeltaSeconds);
		}
		Shard->SetRelativeScale3D(Shard->GetRelativeScale3D() * FMath::Pow(Remaining, DeltaSeconds * 2.5f));
	}

	if (Age >= Lifetime)
	{
		Destroy();
	}
}
