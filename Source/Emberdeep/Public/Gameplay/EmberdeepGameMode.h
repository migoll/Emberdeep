#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EmberdeepGameMode.generated.h"

class UMaterialInterface;
class UStaticMesh;

UCLASS()
class EMBERDEEP_API AEmberdeepGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AEmberdeepGameMode();
	virtual void StartPlay() override;

private:
	void SpawnBlockoutArena();
	void SpawnBlock(const FVector& Location, const FVector& Scale, const FRotator& Rotation = FRotator::ZeroRotator);

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BlockMaterial;
};

