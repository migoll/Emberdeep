#include "Gameplay/EmberdeepPortal.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Gameplay/EmberdeepGameMode.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AEmberdeepPortal::AEmberdeepPortal()
{
	InteractionRange = 270.0f;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("PortalRoot"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* PortalCubeMesh = CubeMesh.Object;
	const auto CreateBlock = [this, PortalCubeMesh, Root](const TCHAR* Name, const FVector& Location, const FVector& Scale)
	{
		UStaticMeshComponent* Block = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Block->SetupAttachment(Root);
		Block->SetStaticMesh(PortalCubeMesh);
		Block->SetRelativeLocation(Location);
		Block->SetRelativeScale3D(Scale);
		Block->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Block;
	};

	LeftPillar = CreateBlock(TEXT("LeftPillar"), FVector(0.0f, -68.0f, 72.0f), FVector(0.30f, 0.30f, 1.45f));
	RightPillar = CreateBlock(TEXT("RightPillar"), FVector(0.0f, 68.0f, 72.0f), FVector(0.30f, 0.30f, 1.45f));
	Lintel = CreateBlock(TEXT("Lintel"), FVector(0.0f, 0.0f, 150.0f), FVector(0.34f, 1.65f, 0.26f));
	PortalGlow = CreateBlock(TEXT("PortalGlow"), FVector(0.0f, 0.0f, 74.0f), FVector(0.08f, 1.05f, 1.20f));

	PortalLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PortalLight"));
	PortalLight->SetupAttachment(Root);
	PortalLight->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	PortalLight->SetIntensity(10000.0f);
	PortalLight->SetAttenuationRadius(480.0f);
	PortalLight->SetCastShadows(false);
}

void AEmberdeepPortal::BeginPlay()
{
	Super::BeginPlay();
	ApplyPortalVisuals();
}

void AEmberdeepPortal::Configure(EEmberdeepRunStage NewDestination, const FString& NewLabel)
{
	if (!HasAuthority())
	{
		return;
	}
	Destination = NewDestination;
	InteractionLabel = NewLabel;
	ApplyPortalVisuals();
}

FString AEmberdeepPortal::GetInteractionPrompt(const AEmberdeepCharacter* Character) const
{
	return InteractionLabel;
}

void AEmberdeepPortal::Interact(AEmberdeepCharacter* Character, AEmberdeepPlayerController* Controller)
{
	if (HasAuthority())
	{
		if (AEmberdeepGameMode* GameMode = GetWorld()->GetAuthGameMode<AEmberdeepGameMode>())
		{
			GameMode->EnterRunStage(Destination);
		}
	}
}

void AEmberdeepPortal::OnRep_PortalState()
{
	ApplyPortalVisuals();
}

void AEmberdeepPortal::ApplyPortalVisuals()
{
	const FLinearColor StoneColor(0.12f, 0.105f, 0.11f);
	const FLinearColor GlowColor = Destination == EEmberdeepRunStage::RewardRoom
		? FLinearColor(0.78f, 0.22f, 0.035f)
		: Destination == EEmberdeepRunStage::Camp
			? FLinearColor(0.16f, 0.42f, 0.62f)
			: FLinearColor(0.36f, 0.055f, 0.62f);

	for (UStaticMeshComponent* Stone : {LeftPillar.Get(), RightPillar.Get(), Lintel.Get()})
	{
		if (Stone)
		{
			if (UMaterialInstanceDynamic* Material = Stone->CreateDynamicMaterialInstance(0))
			{
				Material->SetVectorParameterValue(TEXT("Color"), StoneColor);
			}
		}
	}
	if (PortalGlow)
	{
		if (UMaterialInstanceDynamic* Material = PortalGlow->CreateDynamicMaterialInstance(0))
		{
			Material->SetVectorParameterValue(TEXT("Color"), GlowColor);
		}
	}
	PortalLight->SetLightColor(GlowColor);
}

void AEmberdeepPortal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEmberdeepPortal, Destination);
	DOREPLIFETIME(AEmberdeepPortal, InteractionLabel);
}
