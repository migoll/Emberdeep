#pragma once

#include "CoreMinimal.h"
#include "Gameplay/EmberdeepInteractable.h"
#include "Gameplay/EmberdeepRunTypes.h"
#include "EmberdeepPortal.generated.h"

class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;

UCLASS()
class EMBERDEEP_API AEmberdeepPortal : public AEmberdeepInteractable
{
	GENERATED_BODY()

public:
	AEmberdeepPortal();
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
	TObjectPtr<UStaticMeshComponent> LeftPillar;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> RightPillar;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Lintel;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> PortalGlow;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> PortalLight;

	UPROPERTY(ReplicatedUsing = OnRep_PortalState)
	EEmberdeepRunStage Destination = EEmberdeepRunStage::Dungeon;

	UPROPERTY(ReplicatedUsing = OnRep_PortalState)
	FString InteractionLabel = TEXT("Enter the Ashen Crypt");

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> PortalMaterials;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
