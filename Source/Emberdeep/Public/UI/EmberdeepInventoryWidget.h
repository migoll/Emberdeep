#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EmberdeepInventoryWidget.generated.h"

/** Compact equipment/inventory window. Equipping remains server-authoritative. */
UCLASS()
class EMBERDEEP_API UEmberdeepInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowInventory();
	void RefreshInventory();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void RebuildInventoryRows();
	void HandleEquipItem(int32 InstanceId);
	void EnterWindowInputMode();

	UFUNCTION()
	void CloseWindow();

	UPROPERTY()
	TObjectPtr<class UVerticalBox> ItemRows;

	UPROPERTY()
	TObjectPtr<class UTextBlock> EmptyText;

	UPROPERTY()
	TObjectPtr<class UTextBlock> EquippedSummaryText;

	UPROPERTY()
	TObjectPtr<class UTextBlock> StatSummaryText;
};

