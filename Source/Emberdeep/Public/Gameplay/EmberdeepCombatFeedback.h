#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EmberdeepCombatFeedback.generated.h"

class UPointLightComponent;
class UInstancedStaticMeshComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Short-lived, client-only combat presentation assembled from engine primitives.
 * Authoritative damage never depends on this actor; every peer may render it locally.
 */
UCLASS(NotBlueprintable, Transient)
class EMBERDEEP_API AEmberdeepCombatFeedback : public AActor
{
	GENERATED_BODY()

public:
	AEmberdeepCombatFeedback();
	virtual void Tick(float DeltaSeconds) override;

	static void SpawnSwing(UWorld* World, const FVector& Location, const FVector& Direction, bool bHeavyAttack, bool bConnected);
	static void SpawnHit(UWorld* World, const FVector& Location, const FVector& Direction, float Damage, bool bFatal);
	static void SpawnPlayerHurt(UWorld* World, const FVector& Location, float Damage, bool bFatal);
	static void SpawnDodge(UWorld* World, const FVector& Location, const FVector& Direction);

private:
	static AEmberdeepCombatFeedback* SpawnFeedback(UWorld* World, const FVector& Location);
	void ConfigureSwing(const FVector& Direction, bool bHeavyAttack, bool bConnected);
	void ConfigureHit(const FVector& Direction, float Damage, bool bFatal, bool bPlayerHurt);
	void ConfigureDodge(const FVector& Direction);
	void SetComponentColor(UStaticMeshComponent* Component, const FLinearColor& Color);

	UPROPERTY()
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Shockwave;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> ArcSegments;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> ImpactRing;

	UPROPERTY()
	TObjectPtr<UTextRenderComponent> DamageText;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> FlashLight;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Shards;

	TArray<FVector> ShardVelocities;
	TArray<FRotator> ShardSpin;
	FVector InitialShockwaveScale = FVector::OneVector;
	FVector FinalShockwaveScale = FVector::OneVector;
	FVector InitialRingScale = FVector::OneVector;
	FVector FinalRingScale = FVector::OneVector;
	float Age = 0.0f;
	float Lifetime = 0.45f;
	float InitialLightIntensity = 0.0f;
	float TextRiseSpeed = 115.0f;
};
