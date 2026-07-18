#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EmberdeepGoldPickup.generated.h"

class USphereComponent;
class UInstancedStaticMeshComponent;

UCLASS()
class EMBERDEEP_API AEmberdeepGoldPickup : public AActor
{
	GENERATED_BODY()

public:
	AEmberdeepGoldPickup();
	virtual void Tick(float DeltaSeconds) override;

	void SetGoldValue(int32 NewValue) { GoldValue = FMath::Max(1, NewValue); }

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> PickupSphere;

	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> GoldVoxelMeshes;

	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	int32 GoldValue = 5;

	float RunningTime = 0.0f;
	float BaseHeight = 0.0f;
};
