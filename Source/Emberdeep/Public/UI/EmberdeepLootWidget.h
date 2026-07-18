#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Gameplay/EmberdeepItemTypes.h"
#include "EmberdeepLootWidget.generated.h"

/** Responsive multiplayer-safe corpse/chest loot window. This widget never pauses play. */
UCLASS()
class EMBERDEEP_API UEmberdeepLootWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowLoot(const FString& Title, const TArray<FEmberdeepItemInstance>& Items);
	void RefreshLoot(const FString& Title, const TArray<FEmberdeepItemInstance>& Items);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void RebuildLootRows();
	void HandleTakeItem(int32 EntryId);
	void EnterWindowInputMode();

	UFUNCTION() void TakeAll();
	UFUNCTION() void CloseWindow();

	UPROPERTY() TObjectPtr<class UTextBlock> TitleText;
	UPROPERTY() TObjectPtr<class UTextBlock> EmptyText;
	UPROPERTY() TObjectPtr<class UVerticalBox> ItemRows;
	UPROPERTY() TObjectPtr<class UTexture2D> PanelTexture;
	UPROPERTY() TObjectPtr<class UTexture2D> ItemIconAtlas;

	FString CurrentTitle;
	TArray<FEmberdeepItemInstance> CurrentItems;
};
