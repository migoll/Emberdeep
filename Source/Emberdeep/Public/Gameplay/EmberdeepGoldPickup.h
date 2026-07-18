#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EmberdeepGoldPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;

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
	TObjectPtr<UStaticMeshComponent> GoldMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	int32 GoldValue = 5;

	float RunningTime = 0.0f;
	float BaseHeight = 0.0f;
};
