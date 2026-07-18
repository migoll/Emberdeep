#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EmberdeepCampEnvironment.generated.h"

class UBoxComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UPointLightComponent;
class USceneComponent;

/**
 * Lightweight, code-generated art pass for the Broken Caravan Camp.
 *
 * Decorative voxels are palette-batched and never participate in collision.
 * A small set of authored box proxies supplies the floor, perimeter, and major
 * prop collision so the camp remains cheap enough to iterate on in Phase 0A.
 */
UCLASS()
class EMBERDEEP_API AEmberdeepCampEnvironment : public AActor
{
	GENERATED_BODY()

public:
	AEmberdeepCampEnvironment();
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Camp")
	int32 GetVoxelInstanceCount() const;

	UFUNCTION(BlueprintPure, Category = "Camp")
	int32 GetPaletteMeshCount() const { return PaletteMeshes.Num(); }

	UFUNCTION(BlueprintPure, Category = "Camp")
	int32 GetCollisionProxyCount() const { return CollisionProxies.Num(); }

protected:
	virtual void BeginPlay() override;

private:
	void ApplyPaletteMaterials();
	void UpdateCampfireFlicker();

	UPROPERTY(VisibleAnywhere, Category = "Camp")
	TObjectPtr<USceneComponent> CampRoot;

	UPROPERTY(VisibleAnywhere, Category = "Camp")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> PaletteMeshes;

	UPROPERTY(VisibleAnywhere, Category = "Camp")
	TArray<TObjectPtr<UBoxComponent>> CollisionProxies;

	UPROPERTY(VisibleAnywhere, Category = "Camp|Lighting")
	TObjectPtr<UPointLightComponent> CampfireLight;

	UPROPERTY(VisibleAnywhere, Category = "Camp|Lighting")
	TObjectPtr<UPointLightComponent> WagonLanternLight;

	UPROPERTY(VisibleAnywhere, Category = "Camp|Lighting")
	TObjectPtr<UPointLightComponent> ShelterLanternLight;

	UPROPERTY(VisibleAnywhere, Category = "Camp|Lighting")
	TObjectPtr<UPointLightComponent> WorkstationLanternLight;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseBlockMaterial;

	int32 LastFlickerStep = INDEX_NONE;
};
