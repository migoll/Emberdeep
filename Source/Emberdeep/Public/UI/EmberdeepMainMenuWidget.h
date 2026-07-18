#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EmberdeepMainMenuWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;

UCLASS()
class EMBERDEEP_API UEmberdeepMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UFUNCTION()
	void HostGame();

	UFUNCTION()
	void JoinGame();

	void LeaveMenuMode();

	UPROPERTY()
	TObjectPtr<UEditableTextBox> AddressField;

	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;
};
