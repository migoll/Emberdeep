#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EmberdeepRewardRoomEnvironment.generated.h"

class UBoxComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UPointLightComponent;
class USceneComponent;

/**
 * Replicated end-of-run reward chamber with a clear central chest approach.
 * Gold is intentionally isolated to this room's reward accents.
 */
UCLASS()
class EMBERDEEP_API AEmberdeepRewardRoomEnvironment : public AActor
{
	GENERATED_BODY()

public:
	AEmberdeepRewardRoomEnvironment();

	UFUNCTION(BlueprintPure, Category = "Reward Room")
	int32 GetPaletteMeshCount() const { return PaletteMeshes.Num(); }

	UFUNCTION(BlueprintPure, Category = "Reward Room")
	int32 GetVisualBlockCount() const;

	UFUNCTION(BlueprintPure, Category = "Reward Room")
	int32 GetCollisionProxyCount() const { return CollisionProxies.Num(); }

protected:
	virtual void BeginPlay() override;

private:
	void ApplyPaletteMaterials();

	UPROPERTY(VisibleAnywhere, Category = "Reward Room")
	TObjectPtr<USceneComponent> RewardRoomRoot;

	UPROPERTY(VisibleAnywhere, Category = "Reward Room")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> PaletteMeshes;

	UPROPERTY(VisibleAnywhere, Category = "Reward Room")
	TArray<TObjectPtr<UBoxComponent>> CollisionProxies;

	UPROPERTY(VisibleAnywhere, Category = "Reward Room|Lighting")
	TArray<TObjectPtr<UPointLightComponent>> LocalLights;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseBlockMaterial;
};
