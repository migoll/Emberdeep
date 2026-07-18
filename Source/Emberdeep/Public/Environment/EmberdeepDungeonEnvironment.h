#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EmberdeepDungeonEnvironment.generated.h"

class UBoxComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UPointLightComponent;
class USceneComponent;

/**
 * Replicated, code-generated open dungeon room used by the short-run prototype.
 *
 * Decorative blocks are palette-batched and have no collision. Only the floor
 * and perimeter use simple collision proxies, which keeps direct-steering enemy
 * AI from becoming trapped on scenery while the navigation layer is still small.
 */
UCLASS()
class EMBERDEEP_API AEmberdeepDungeonEnvironment : public AActor
{
	GENERATED_BODY()

public:
	AEmberdeepDungeonEnvironment();

	UFUNCTION(BlueprintPure, Category = "Dungeon")
	int32 GetPaletteMeshCount() const { return PaletteMeshes.Num(); }

	UFUNCTION(BlueprintPure, Category = "Dungeon")
	int32 GetVisualBlockCount() const;

	UFUNCTION(BlueprintPure, Category = "Dungeon")
	int32 GetCollisionProxyCount() const { return CollisionProxies.Num(); }

protected:
	virtual void BeginPlay() override;

private:
	void ApplyPaletteMaterials();

	UPROPERTY(VisibleAnywhere, Category = "Dungeon")
	TObjectPtr<USceneComponent> DungeonRoot;

	UPROPERTY(VisibleAnywhere, Category = "Dungeon")
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> PaletteMeshes;

	UPROPERTY(VisibleAnywhere, Category = "Dungeon")
	TArray<TObjectPtr<UBoxComponent>> CollisionProxies;

	UPROPERTY(VisibleAnywhere, Category = "Dungeon|Lighting")
	TArray<TObjectPtr<UPointLightComponent>> LocalLights;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseBlockMaterial;
};
