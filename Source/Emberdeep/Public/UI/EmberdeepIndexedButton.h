#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "EmberdeepIndexedButton.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FEmberdeepIndexedButtonClicked, int32);

/**
 * Small UButton extension used by programmatic list widgets. It keeps the
 * replicated gameplay identifier separate from the row label and forwards it
 * through a native delegate when clicked.
 */
UCLASS()
class EMBERDEEP_API UEmberdeepIndexedButton : public UButton
{
	GENERATED_BODY()

public:
	UEmberdeepIndexedButton(const FObjectInitializer& ObjectInitializer);

	void SetEntryId(int32 InEntryId) { EntryId = InEntryId; }
	int32 GetEntryId() const { return EntryId; }
	FEmberdeepIndexedButtonClicked& OnIndexedClicked() { return IndexedClicked; }

protected:
	virtual void SynchronizeProperties() override;

private:
	void BindInternalClick();

	UFUNCTION()
	void ForwardClick();

	UPROPERTY()
	int32 EntryId = INDEX_NONE;

	FEmberdeepIndexedButtonClicked IndexedClicked;
};

