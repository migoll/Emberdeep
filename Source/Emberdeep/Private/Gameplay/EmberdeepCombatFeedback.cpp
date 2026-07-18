#include "Gameplay/EmberdeepCombatFeedback.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
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

	Shockwave = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shockwave"));
	Shockwave->SetupAttachment(SceneRoot);
	Shockwave->SetStaticMesh(CylinderMesh.Object);
	Shockwave->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Shockwave->SetCastShadow(false);
	Shockwave->SetHiddenInGame(true);

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

	Shockwave->SetHiddenInGame(false);
	Shockwave->SetRelativeLocation(FVector(0.0f, 0.0f, -58.0f));
	InitialShockwaveScale = bHeavyAttack ? FVector(0.55f, 0.55f, 0.018f) : FVector(0.30f, 0.30f, 0.012f);
	FinalShockwaveScale = bHeavyAttack ? FVector(2.25f, 2.25f, 0.010f) : FVector(1.25f, 1.25f, 0.006f);
	Shockwave->SetRelativeScale3D(InitialShockwaveScale);
	SetComponentColor(Shockwave, Color);

	const FVector SafeDirection = Direction.GetSafeNormal2D();
	const FVector Side(-SafeDirection.Y, SafeDirection.X, 0.0f);
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

	Shockwave->SetHiddenInGame(false);
	Shockwave->SetRelativeLocation(FVector(0.0f, 0.0f, -55.0f));
	InitialShockwaveScale = FVector(0.24f, 0.24f, 0.012f);
	FinalShockwaveScale = bFatal ? FVector(2.8f, 2.8f, 0.006f) : FVector(1.35f, 1.35f, 0.005f);
	Shockwave->SetRelativeScale3D(InitialShockwaveScale);
	SetComponentColor(Shockwave, Color);

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
	Shockwave->SetHiddenInGame(false);
	Shockwave->SetRelativeLocation(FVector(0.0f, 0.0f, -58.0f));
	InitialShockwaveScale = FVector(0.42f, 0.42f, 0.010f);
	FinalShockwaveScale = FVector(1.55f, 1.55f, 0.004f);
	Shockwave->SetRelativeScale3D(InitialShockwaveScale);
	SetComponentColor(Shockwave, Color);

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

	Shockwave->SetRelativeScale3D(FMath::Lerp(InitialShockwaveScale, FinalShockwaveScale, FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.2f)));
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
