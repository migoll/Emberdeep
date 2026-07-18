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
 * Replicated, code-generated Ashen Crypt combat room.
 *
 * All visible solids share the project's fixed four-centimetre voxel lattice.
 * Palette batches remain non-colliding; the proven floor and perimeter proxies
 * preserve the open direct-steering combat space while dense wall-side dressing
 * supplies the dark-fantasy room silhouette.
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
