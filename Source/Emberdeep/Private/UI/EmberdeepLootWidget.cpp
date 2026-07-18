#include "UI/EmberdeepLootWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Gameplay/EmberdeepPlayerController.h"
#include "UI/EmberdeepIndexedButton.h"

namespace EmberdeepLootUI
{
	const FLinearColor Gold(0.78f, 0.48f, 0.12f, 1.0f);
	const FLinearColor Parchment(0.86f, 0.79f, 0.66f, 1.0f);
	const FLinearColor Muted(0.53f, 0.50f, 0.46f, 1.0f);
	const FLinearColor Panel(0.018f, 0.014f, 0.016f, 0.97f);

	UTextBlock* MakeText(
		UWidgetTree* WidgetTree,
		const FString& Value,
		int32 Size,
		const FLinearColor& Color,
		ETextJustify::Type Justification = ETextJustify::Left)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
		Text->SetText(FText::FromString(Value));
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetJustification(Justification);
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
		return Text;
	}

	FString GetSlotName(EEmberdeepItemSlot Slot)
	{
		if (const UEnum* Enum = StaticEnum<EEmberdeepItemSlot>())
		{
			return Enum->GetDisplayNameTextByValue(static_cast<int64>(Slot)).ToString();
		}
		return TEXT("Item");
	}

	FString GetBonusSummary(const FEmberdeepItemInstance& Item)
	{
		TArray<FString> Bonuses;
		if (!FMath::IsNearlyZero(Item.DamageBonus))
		{
			Bonuses.Add(FString::Printf(TEXT("+%.0f damage"), Item.DamageBonus));
		}
		if (!FMath::IsNearlyZero(Item.MaxHealthBonus))
		{
			Bonuses.Add(FString::Printf(TEXT("+%.0f health"), Item.MaxHealthBonus));
		}
		if (!FMath::IsNearlyZero(Item.ArmorBonus))
		{
			Bonuses.Add(FString::Printf(TEXT("+%.0f armor"), Item.ArmorBonus));
		}
		return Bonuses.IsEmpty() ? TEXT("No bonuses") : FString::Join(Bonuses, TEXT("  |  "));
	}

	UButton* MakeFooterButton(UWidgetTree* WidgetTree, const FString& Label)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>();
		Button->SetBackgroundColor(FLinearColor(0.24f, 0.11f, 0.025f, 1.0f));
		UTextBlock* Text = MakeText(WidgetTree, Label, 17, FLinearColor(1.0f, 0.68f, 0.22f), ETextJustify::Center);
		Button->AddChild(Text);
		return Button;
	}
}

TSharedRef<SWidget> UEmberdeepLootWidget::RebuildWidget()
{
	if (WidgetTree->RootWidget)
	{
		return Super::RebuildWidget();
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Root;

	UBorder* OuterFrame = WidgetTree->ConstructWidget<UBorder>();
	OuterFrame->SetBrushColor(FLinearColor(0.31f, 0.20f, 0.08f, 0.98f));
	OuterFrame->SetPadding(FMargin(2.0f));
	if (UCanvasPanelSlot* FrameSlot = Root->AddChildToCanvas(OuterFrame))
	{
		FrameSlot->SetAnchors(FAnchors(0.66f, 0.48f));
		FrameSlot->SetAlignment(FVector2D(0.5f));
		FrameSlot->SetSize(FVector2D(570.0f, 500.0f));
	}

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(EmberdeepLootUI::Panel);
	Panel->SetPadding(FMargin(20.0f, 16.0f));
	OuterFrame->SetContent(Panel);

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Content);

	TitleText = EmberdeepLootUI::MakeText(
		WidgetTree,
		CurrentTitle.IsEmpty() ? TEXT("LOOT") : CurrentTitle.ToUpper(),
		25,
		EmberdeepLootUI::Gold,
		ETextJustify::Center);
	if (UVerticalBoxSlot* AddedSlot = Content->AddChildToVerticalBox(TitleText))
	{
		AddedSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}

	UTextBlock* Hint = EmberdeepLootUI::MakeText(
		WidgetTree,
		TEXT("Click an item to take it. Combat continues while this window is open."),
		12,
		EmberdeepLootUI::Muted,
		ETextJustify::Center);
	if (UVerticalBoxSlot* AddedSlot = Content->AddChildToVerticalBox(Hint))
	{
		AddedSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	ItemRows = WidgetTree->ConstructWidget<UVerticalBox>();
	Scroll->AddChild(ItemRows);
	if (UVerticalBoxSlot* AddedSlot = Content->AddChildToVerticalBox(Scroll))
	{
		AddedSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	EmptyText = EmberdeepLootUI::MakeText(
		WidgetTree,
		TEXT("The container is empty."),
		16,
		EmberdeepLootUI::Muted,
		ETextJustify::Center);
	if (UVerticalBoxSlot* AddedSlot = Content->AddChildToVerticalBox(EmptyText))
	{
		AddedSlot->SetPadding(FMargin(0.0f, 10.0f));
	}

	UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>();
	if (UVerticalBoxSlot* AddedSlot = Content->AddChildToVerticalBox(Footer))
	{
		AddedSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
	}

	UButton* TakeAllButton = EmberdeepLootUI::MakeFooterButton(WidgetTree, TEXT("TAKE ALL"));
	TakeAllButton->OnClicked.AddDynamic(this, &UEmberdeepLootWidget::TakeAll);
	if (UHorizontalBoxSlot* AddedSlot = Footer->AddChildToHorizontalBox(TakeAllButton))
	{
		AddedSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		AddedSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
	}

	UButton* CloseButton = EmberdeepLootUI::MakeFooterButton(WidgetTree, TEXT("CLOSE"));
	CloseButton->OnClicked.AddDynamic(this, &UEmberdeepLootWidget::CloseWindow);
	if (UHorizontalBoxSlot* AddedSlot = Footer->AddChildToHorizontalBox(CloseButton))
	{
		AddedSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		AddedSlot->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));
	}

	RebuildLootRows();
	return Super::RebuildWidget();
}

void UEmberdeepLootWidget::ShowLoot(const FString& Title, const TArray<FEmberdeepItemInstance>& Items)
{
	RefreshLoot(Title, Items);
	if (!IsInViewport())
	{
		AddToViewport(70);
	}
	SetVisibility(ESlateVisibility::Visible);
	EnterWindowInputMode();
}

void UEmberdeepLootWidget::RefreshLoot(const FString& Title, const TArray<FEmberdeepItemInstance>& Items)
{
	CurrentTitle = Title;
	CurrentItems = Items;
	if (TitleText)
	{
		TitleText->SetText(FText::FromString(CurrentTitle.IsEmpty() ? TEXT("LOOT") : CurrentTitle.ToUpper()));
	}
	RebuildLootRows();
}

void UEmberdeepLootWidget::RebuildLootRows()
{
	if (!ItemRows)
	{
		return;
	}

	ItemRows->ClearChildren();
	if (EmptyText)
	{
		EmptyText->SetVisibility(CurrentItems.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	for (const FEmberdeepItemInstance& Item : CurrentItems)
	{
		UEmberdeepIndexedButton* RowButton = WidgetTree->ConstructWidget<UEmberdeepIndexedButton>();
		RowButton->SetEntryId(Item.InstanceId);
		RowButton->SetBackgroundColor(FEmberdeepItemCatalog::GetRarityColor(Item.Rarity) * 0.24f + FLinearColor(0.035f, 0.028f, 0.026f, 0.76f));
		RowButton->OnIndexedClicked().AddUObject(this, &UEmberdeepLootWidget::HandleTakeItem);

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		RowButton->AddChild(Row);

		UVerticalBox* Identity = WidgetTree->ConstructWidget<UVerticalBox>();
		if (UHorizontalBoxSlot* AddedSlot = Row->AddChildToHorizontalBox(Identity))
		{
			AddedSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			AddedSlot->SetPadding(FMargin(12.0f, 8.0f));
		}
		UTextBlock* NameText = EmberdeepLootUI::MakeText(
			WidgetTree,
			Item.DisplayName,
			18,
			FEmberdeepItemCatalog::GetRarityColor(Item.Rarity));
		Identity->AddChildToVerticalBox(NameText);
		UTextBlock* BonusText = EmberdeepLootUI::MakeText(
			WidgetTree,
			EmberdeepLootUI::GetBonusSummary(Item),
			12,
			EmberdeepLootUI::Parchment);
		Identity->AddChildToVerticalBox(BonusText);

		const FString TypeLine = FString::Printf(
			TEXT("%s  |  %s  |  ITEM LEVEL %d"),
			*FEmberdeepItemCatalog::GetRarityDisplayName(Item.Rarity).ToUpper(),
			*EmberdeepLootUI::GetSlotName(Item.Slot).ToUpper(),
			Item.ItemLevel);
		UTextBlock* TypeText = EmberdeepLootUI::MakeText(
			WidgetTree,
			TypeLine,
			11,
			EmberdeepLootUI::Muted,
			ETextJustify::Right);
		if (UHorizontalBoxSlot* AddedSlot = Row->AddChildToHorizontalBox(TypeText))
		{
			AddedSlot->SetVerticalAlignment(VAlign_Center);
			AddedSlot->SetPadding(FMargin(8.0f, 8.0f, 12.0f, 8.0f));
		}

		if (UVerticalBoxSlot* AddedSlot = ItemRows->AddChildToVerticalBox(RowButton))
		{
			AddedSlot->SetPadding(FMargin(0.0f, 3.0f));
		}
	}
}

void UEmberdeepLootWidget::HandleTakeItem(int32 EntryId)
{
	if (AEmberdeepPlayerController* PlayerController = Cast<AEmberdeepPlayerController>(GetOwningPlayer()))
	{
		PlayerController->RequestTakeLoot(EntryId);
	}
}

void UEmberdeepLootWidget::TakeAll()
{
	AEmberdeepPlayerController* PlayerController = Cast<AEmberdeepPlayerController>(GetOwningPlayer());
	if (!PlayerController)
	{
		return;
	}

	TArray<int32> EntryIds;
	EntryIds.Reserve(CurrentItems.Num());
	for (const FEmberdeepItemInstance& Item : CurrentItems)
	{
		EntryIds.Add(Item.InstanceId);
	}
	for (const int32 EntryId : EntryIds)
	{
		PlayerController->RequestTakeLoot(EntryId);
	}
}

void UEmberdeepLootWidget::CloseWindow()
{
	if (AEmberdeepPlayerController* PlayerController = Cast<AEmberdeepPlayerController>(GetOwningPlayer()))
	{
		PlayerController->CloseGameplayWindows();
	}
	else
	{
		RemoveFromParent();
	}
}

void UEmberdeepLootWidget::EnterWindowInputMode()
{
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetWidgetToFocus(TakeWidget());
		PlayerController->SetInputMode(InputMode);
		PlayerController->SetShowMouseCursor(true);
	}
}
