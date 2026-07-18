#include "Gameplay/EmberdeepGoldPickup.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Gameplay/EmberdeepCharacter.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

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

	GoldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GoldMesh"));
	GoldMesh->SetupAttachment(PickupSphere);
	GoldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GoldMesh->SetRelativeScale3D(FVector(0.22f, 0.22f, 0.22f));
	GoldMesh->SetRelativeRotation(FRotator(0.0f, 45.0f, 45.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	GoldMesh->SetStaticMesh(CubeMesh.Object);

	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AEmberdeepGoldPickup::HandleOverlap);
	SetLifeSpan(30.0f);
}

void AEmberdeepGoldPickup::BeginPlay()
{
	Super::BeginPlay();
	BaseHeight = GetActorLocation().Z;

	if (UMaterialInstanceDynamic* Material = GoldMesh->CreateDynamicMaterialInstance(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.95f, 0.48f, 0.035f));
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
