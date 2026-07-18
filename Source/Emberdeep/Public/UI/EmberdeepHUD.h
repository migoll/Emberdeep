#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "EmberdeepHUD.generated.h"

class AEmberdeepCharacter;
class UTexture2D;

UCLASS()
class EMBERDEEP_API AEmberdeepHUD : public AHUD
{
	GENERATED_BODY()

public:
	AEmberdeepHUD();
	virtual void DrawHUD() override;

private:
	void DrawBar(float X, float Y, float Width, float Height, float NormalizedValue, const FLinearColor& FillColor);
	void DrawOrb(const FVector2D& Center, float Radius, float NormalizedValue, const FLinearColor& FillColor);
	void DrawOrnamentSection(
		float X,
		float Y,
		float Width,
		float Height,
		float SourceX,
		float SourceY,
		float SourceWidth,
		float SourceHeight);
	void DrawPartyRoster();
	void DrawMinimapAndObjective();
	void DrawActionBar(const AEmberdeepCharacter* Character);

	UPROPERTY()
	TObjectPtr<UTexture2D> HudOrnaments;
};
