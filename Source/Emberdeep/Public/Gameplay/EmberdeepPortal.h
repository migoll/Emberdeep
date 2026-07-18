#pragma once

#include "CoreMinimal.h"
#include "Gameplay/EmberdeepInteractable.h"
#include "Gameplay/EmberdeepRunTypes.h"
#include "EmberdeepPortal.generated.h"

class UMaterialInstanceDynamic;
class UInstancedStaticMeshComponent;
class UPointLightComponent;
class USceneComponent;

UCLASS()
class EMBERDEEP_API AEmberdeepPortal : public AEmberdeepInteractable
{
	GENERATED_BODY()

public:
	AEmberdeepPortal();
	virtual void Tick(float DeltaSeconds) override;
	void Configure(EEmberdeepRunStage NewDestination, const FString& NewLabel);
	virtual FString GetInteractionPrompt(const AEmberdeepCharacter* Character) const override;
	virtual void Interact(AEmberdeepCharacter* Character, AEmberdeepPlayerController* Controller) override;

	EEmberdeepRunStage GetDestination() const { return Destination; }

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_PortalState();

	void ApplyPortalVisuals();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> PortalVisualRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> PortalGlowRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> StoneDarkVoxels;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> StoneMidVoxels;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> StoneEdgeVoxels;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> PortalCoreVoxels;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> PortalSparkVoxels;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> PortalLight;

	UPROPERTY(ReplicatedUsing = OnRep_PortalState)
	EEmberdeepRunStage Destination = EEmberdeepRunStage::Dungeon;

	UPROPERTY(ReplicatedUsing = OnRep_PortalState)
	FString InteractionLabel = TEXT("Enter the Ashen Crypt");

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> PortalCoreMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> PortalSparkMaterial;

	float PortalVisualTime = 0.0f;
	float PortalLightBaseIntensity = 900.0f;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
