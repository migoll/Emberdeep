#include "Gameplay/EmberdeepGoldPickup.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Gameplay/EmberdeepCharacter.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Visual/EmberdeepVoxelStyle.h"

AEmberdeepGoldPickup::AEmberdeepGoldPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	SetRootComponent(PickupSphere);
	PickupSphere->InitSphereRadius(55.0f);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	for (int32 ShadeIndex = 0; ShadeIndex < EmberdeepVoxelStyle::ShadeCount; ++ShadeIndex)
	{
		const FName MeshName(*FString::Printf(TEXT("GoldVoxels_Shade%d"), ShadeIndex));
		UInstancedStaticMeshComponent* GoldMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(MeshName);
		GoldMesh->SetupAttachment(PickupSphere);
		GoldMesh->SetStaticMesh(CubeMesh.Object);
		GoldMesh->SetMobility(EComponentMobility::Movable);
		GoldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GoldMesh->SetGenerateOverlapEvents(false);
		GoldMesh->SetCanEverAffectNavigation(false);
		GoldVoxelMeshes.Add(GoldMesh);
	}

	TArray<FTransform> ShadeTransforms[EmberdeepVoxelStyle::ShadeCount];
	for (int32 Z = -2; Z <= 2; ++Z)
	{
		const int32 Radius = 3 - FMath::Abs(Z);
		for (int32 Y = -Radius; Y <= Radius; ++Y)
		{
			for (int32 X = -Radius; X <= Radius; ++X)
			{
				if (FMath::Abs(X) + FMath::Abs(Y) > Radius + 1)
				{
					continue;
				}
				const int32 ShadeIndex = EmberdeepVoxelStyle::SelectShade(X, Y, Z, 73);
				ShadeTransforms[ShadeIndex].Emplace(
					FRotator::ZeroRotator,
					EmberdeepVoxelStyle::CellCenter(X, Y, Z),
					EmberdeepVoxelStyle::InstanceScale());
			}
		}
	}
	for (int32 ShadeIndex = 0; ShadeIndex < EmberdeepVoxelStyle::ShadeCount; ++ShadeIndex)
	{
		GoldVoxelMeshes[ShadeIndex]->AddInstances(ShadeTransforms[ShadeIndex], false, false, true);
	}

	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AEmberdeepGoldPickup::HandleOverlap);
	SetLifeSpan(30.0f);
}

void AEmberdeepGoldPickup::BeginPlay()
{
	Super::BeginPlay();
	BaseHeight = GetActorLocation().Z;

	const FLinearColor GoldColor(0.95f, 0.48f, 0.035f);
	for (int32 ShadeIndex = 0; ShadeIndex < GoldVoxelMeshes.Num(); ++ShadeIndex)
	{
		if (UMaterialInstanceDynamic* Material = GoldVoxelMeshes[ShadeIndex]->CreateDynamicMaterialInstance(0))
		{
			Material->SetVectorParameterValue(TEXT("Color"), EmberdeepVoxelStyle::ShadeColor(GoldColor, ShadeIndex));
		}
	}
}

void AEmberdeepGoldPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RunningTime += DeltaSeconds;
	AddActorWorldRotation(FRotator(0.0f, 100.0f * DeltaSeconds, 0.0f));

	FVector Location = GetActorLocation();
	Location.Z = BaseHeight + FMath::Sin(RunningTime * 3.5f) * 12.0f;
	SetActorLocation(Location);
}

void AEmberdeepGoldPickup::HandleOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	if (AEmberdeepCharacter* Character = Cast<AEmberdeepCharacter>(OtherActor))
	{
		Character->AddGold(GoldValue);
		Destroy();
	}
}
