#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "EmberdeepHUD.generated.h"

UCLASS()
class EMBERDEEP_API AEmberdeepHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawBar(float X, float Y, float Width, float Height, float NormalizedValue, const FLinearColor& FillColor);
};
