#include "UI/EmberdeepIndexedButton.h"

UEmberdeepIndexedButton::UEmberdeepIndexedButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BindInternalClick();
}

void UEmberdeepIndexedButton::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	BindInternalClick();
}

void UEmberdeepIndexedButton::BindInternalClick()
{
	OnClicked.AddUniqueDynamic(this, &UEmberdeepIndexedButton::ForwardClick);
}

void UEmberdeepIndexedButton::ForwardClick()
{
	IndexedClicked.Broadcast(EntryId);
}

