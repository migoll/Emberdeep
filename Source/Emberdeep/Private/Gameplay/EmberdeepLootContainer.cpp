#include "Gameplay/EmberdeepLootContainer.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Emberdeep.h"
#include "Gameplay/EmberdeepPlayerController.h"
#include "Gameplay/EmberdeepPlayerState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AEmberdeepLootContainer::AEmberdeepLootContainer()
{
	InteractionRange = 250.0f;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("LootRoot"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	ContainerBase = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ContainerBase"));
	ContainerBase->SetupAttachment(Root);
	ContainerBase->SetStaticMesh(CubeMesh.Object);
	ContainerBase->SetRelativeScale3D(FVector(0.62f, 0.82f, 0.35f));
	ContainerBase->SetRelativeLocation(FVector(0.0f, 0.0f, 30.0f));
	ContainerBase->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ContainerLid = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ContainerLid"));
	ContainerLid->SetupAttachment(Root);
	ContainerLid->SetStaticMesh(CubeMesh.Object);
	ContainerLid->SetRelativeScale3D(FVector(0.68f, 0.88f, 0.15f));
	ContainerLid->SetRelativeLocation(FVector(0.0f, 0.0f, 58.0f));
	ContainerLid->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	LootGlow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LootGlow"));
	LootGlow->SetupAttachment(Root);
	LootGlow->SetStaticMesh(CubeMesh.Object);
	LootGlow->SetRelativeScale3D(FVector(0.12f, 0.56f, 0.08f));
	LootGlow->SetRelativeLocation(FVector(-62.0f, 0.0f, 42.0f));
	LootGlow->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	LootLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LootLight"));
	LootLight->SetupAttachment(Root);
	LootLight->SetRelativeLocation(FVector(0.0f, 0.0f, 75.0f));
	LootLight->SetIntensity(5000.0f);
	LootLight->SetAttenuationRadius(300.0f);
	LootLight->SetCastShadows(false);
}

void AEmberdeepLootContainer::BeginPlay()
{
	Super::BeginPlay();
	ApplyVisualState();
}

void AEmberdeepLootContainer::ConfigureLoot(
	const FString& NewTitle,
	const TArray<FEmberdeepItemInstance>& NewEntries,
	bool bAsChest,
	bool bLegendary)
{
	if (!HasAuthority())
	{
		return;
	}
	ContainerTitle = NewTitle;
	LootEntries = NewEntries;
	bChest = bAsChest;
	bLegendaryChest = bLegendary;
	bDepleted = LootEntries.IsEmpty();
	ApplyVisualState();
}

FString AEmberdeepLootContainer::GetInteractionPrompt(const AEmberdeepCharacter* Character) const
{
	return bDepleted
		? FString::Printf(TEXT("%s (empty)"), *ContainerTitle)
		: FString::Printf(TEXT("Open %s"), *ContainerTitle);
}

void AEmberdeepLootContainer::Interact(AEmberdeepCharacter* Character, AEmberdeepPlayerController* Controller)
{
	if (HasAuthority() && Controller && !bDepleted)
	{
		AddViewer(Controller);
		Controller->ShowLootWindowFromServer(this, ContainerTitle, LootEntries);
	}
}

bool AEmberdeepLootContainer::TryTakeEntry(AEmberdeepPlayerController* Controller, int32 EntryId)
{
	if (!HasAuthority() || !Controller || bDepleted)
	{
		return false;
	}

	const int32 EntryIndex = LootEntries.IndexOfByPredicate([EntryId](const FEmberdeepItemInstance& Entry)
	{
		return Entry.InstanceId == EntryId;
	});
	if (!LootEntries.IsValidIndex(EntryIndex))
	{
		return false;
	}

	AEmberdeepPlayerState* RunState = Controller->GetPlayerState<AEmberdeepPlayerState>();
	if (!RunState || !RunState->AddItem(LootEntries[EntryIndex]))
	{
		Controller->NotifyInventoryFull();
		return false;
	}

	UE_LOG(LogEmberdeep, Display, TEXT("EMBERDEEP_LOOT Claimed Player=%d Item=%s"),
		RunState->GetPlayerId(), *LootEntries[EntryIndex].DisplayName);
	LootEntries.RemoveAt(EntryIndex);
	bDepleted = LootEntries.IsEmpty();
	ApplyVisualState();
	Controller->NotifyInventoryChanged();
	RefreshViewers();
	return true;
}

void AEmberdeepLootContainer::AddViewer(AEmberdeepPlayerController* Controller)
{
	if (Controller)
	{
		Viewers.AddUnique(Controller);
	}
}

void AEmberdeepLootContainer::RemoveViewer(AEmberdeepPlayerController* Controller)
{
	Viewers.Remove(Controller);
}

void AEmberdeepLootContainer::RefreshViewers()
{
	for (int32 ViewerIndex = Viewers.Num() - 1; ViewerIndex >= 0; --ViewerIndex)
	{
		AEmberdeepPlayerController* Viewer = Viewers[ViewerIndex].Get();
		if (!Viewer)
		{
			Viewers.RemoveAtSwap(ViewerIndex);
			continue;
		}
		Viewer->ShowLootWindowFromServer(this, ContainerTitle, LootEntries);
	}
}

void AEmberdeepLootContainer::OnRep_VisualState()
{
	ApplyVisualState();
}

void AEmberdeepLootContainer::ApplyVisualState()
{
	const FLinearColor Wood = bChest ? FLinearColor(0.20f, 0.10f, 0.035f) : FLinearColor(0.08f, 0.07f, 0.08f);
	const FLinearColor Metal = bLegendaryChest ? FLinearColor(0.78f, 0.38f, 0.035f) : FLinearColor(0.30f, 0.27f, 0.24f);
	const FLinearColor Glow = bDepleted
		? FLinearColor(0.025f, 0.025f, 0.025f)
		: bLegendaryChest ? FLinearColor(0.98f, 0.50f, 0.035f) : FLinearColor(0.25f, 0.12f, 0.55f);

	if (UMaterialInstanceDynamic* Material = ContainerBase->CreateDynamicMaterialInstance(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), Wood);
	}
	if (UMaterialInstanceDynamic* Material = ContainerLid->CreateDynamicMaterialInstance(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), Metal);
	}
	if (UMaterialInstanceDynamic* Material = LootGlow->CreateDynamicMaterialInstance(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), Glow);
	}
	LootGlow->SetHiddenInGame(bDepleted);
	LootLight->SetVisibility(!bDepleted);
	LootLight->SetLightColor(Glow);
	ContainerLid->SetRelativeRotation(bDepleted ? FRotator(0.0f, 0.0f, -32.0f) : FRotator::ZeroRotator);
}

void AEmberdeepLootContainer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AEmberdeepLootContainer, ContainerTitle);
	DOREPLIFETIME(AEmberdeepLootContainer, bChest);
	DOREPLIFETIME(AEmberdeepLootContainer, bLegendaryChest);
	DOREPLIFETIME(AEmberdeepLootContainer, bDepleted);
}
