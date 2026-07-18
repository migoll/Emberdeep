#include "UI/EmberdeepInventoryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Gameplay/EmberdeepItemTypes.h"
#include "Gameplay/EmberdeepPlayerController.h"
#include "Gameplay/EmberdeepPlayerState.h"
#include "UI/EmberdeepIndexedButton.h"

namespace EmberdeepInventoryUI
{
	const FLinearColor Gold(0.78f, 0.48f, 0.12f, 1.0f);
	const FLinearColor Parchment(0.86f, 0.79f, 0.66f, 1.0f);
	const FLinearColor Muted(0.53f, 0.50f, 0.46f, 1.0f);

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
			Bonuses.Add(FString::Printf(TEXT("+%.0f DMG"), Item.DamageBonus));
		}
		if (!FMath::IsNearlyZero(Item.MaxHealthBonus))
		{
			Bonuses.Add(FString::Printf(TEXT("+%.0f HP"), Item.MaxHealthBonus));
		}
		if (!FMath::IsNearlyZero(Item.ArmorBonus))
		{
			Bonuses.Add(FString::Printf(TEXT("+%.0f ARMOR"), Item.ArmorBonus));
		}
		return Bonuses.IsEmpty() ? TEXT("NO BONUSES") : FString::Join(Bonuses, TEXT("   "));
	}
}

TSharedRef<SWidget> UEmberdeepInventoryWidget::RebuildWidget()
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
		// Keep the complete frame inside a 1280x720 viewport while still leaving
		// the local player visible to its left.
		FrameSlot->SetAnchors(FAnchors(0.66f, 0.48f));
		FrameSlot->SetAlignment(FVector2D(0.5f));
		FrameSlot->SetSize(FVector2D(760.0f, 630.0f));
	}

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(FLinearColor(0.018f, 0.014f, 0.016f, 0.97f));
	Panel->SetPadding(FMargin(22.0f, 16.0f));
	OuterFrame->SetContent(Panel);

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Content);

	UTextBlock* Heading = EmberdeepInventoryUI::MakeText(
		WidgetTree, TEXT("INVENTORY & EQUIPMENT"), 25, EmberdeepInventoryUI::Gold, ETextJustify::Center);
	if (UVerticalBoxSlot* AddedSlot = Content->AddChildToVerticalBox(Heading))
	{
		AddedSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	UBorder* EquipmentPanel = WidgetTree->ConstructWidget<UBorder>();
	EquipmentPanel->SetBrushColor(FLinearColor(0.055f, 0.042f, 0.035f, 1.0f));
	EquipmentPanel->SetPadding(FMargin(12.0f, 9.0f));
	if (UVerticalBoxSlot* AddedSlot = Content->AddChildToVerticalBox(EquipmentPanel))
	{
		AddedSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}
	UVerticalBox* EquipmentContent = WidgetTree->ConstructWidget<UVerticalBox>();
	EquipmentPanel->SetContent(EquipmentContent);
	EquippedSummaryText = EmberdeepInventoryUI::MakeText(
		WidgetTree, TEXT("WEAPON: -   |   ARMOR: -   |   TRINKET: -"), 14, EmberdeepInventoryUI::Parchment);
	EquipmentContent->AddChildToVerticalBox(EquippedSummaryText);
	StatSummaryText = EmberdeepInventoryUI::MakeText(
		WidgetTree, TEXT("EQUIPMENT BONUSES   DAMAGE +0   HEALTH +0   ARMOR +0"), 13, EmberdeepInventoryUI::Gold);
	EquipmentContent->AddChildToVerticalBox(StatSummaryText);

	UHorizontalBox* ListHeading = WidgetTree->ConstructWidget<UHorizontalBox>();
	if (UVerticalBoxSlot* AddedSlot = Content->AddChildToVerticalBox(ListHeading))
	{
		AddedSlot->SetPadding(FMargin(10.0f, 6.0f));
	}
	UTextBlock* ItemLabel = EmberdeepInventoryUI::MakeText(WidgetTree, TEXT("ITEM"), 12, EmberdeepInventoryUI::Muted);
	if (UHorizontalBoxSlot* AddedSlot = ListHeading->AddChildToHorizontalBox(ItemLabel))
	{
		AddedSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	UTextBlock* EquipLabel = EmberdeepInventoryUI::MakeText(
		WidgetTree, TEXT("CLICK TO EQUIP"), 12, EmberdeepInventoryUI::Muted, ETextJustify::Right);
	ListHeading->AddChildToHorizontalBox(EquipLabel);

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	ItemRows = WidgetTree->ConstructWidget<UVerticalBox>();
	Scroll->AddChild(ItemRows);
	if (UVerticalBoxSlot* AddedSlot = Content->AddChildToVerticalBox(Scroll))
	{
		AddedSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	EmptyText = EmberdeepInventoryUI::MakeText(
		WidgetTree, TEXT("Your pack is empty."), 16, EmberdeepInventoryUI::Muted, ETextJustify::Center);
	if (UVerticalBoxSlot* AddedSlot = Content->AddChildToVerticalBox(EmptyText))
	{
		AddedSlot->SetPadding(FMargin(0.0f, 10.0f));
	}

	UButton* CloseButton = WidgetTree->ConstructWidget<UButton>();
	CloseButton->SetBackgroundColor(FLinearColor(0.24f, 0.11f, 0.025f, 1.0f));
	CloseButton->OnClicked.AddDynamic(this, &UEmberdeepInventoryWidget::CloseWindow);
	CloseButton->AddChild(EmberdeepInventoryUI::MakeText(
		WidgetTree, TEXT("CLOSE"), 17, FLinearColor(1.0f, 0.68f, 0.22f), ETextJustify::Center));
	if (UVerticalBoxSlot* AddedSlot = Content->AddChildToVerticalBox(CloseButton))
	{
		AddedSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
	}

	RebuildInventoryRows();
	return Super::RebuildWidget();
}

void UEmberdeepInventoryWidget::ShowInventory()
{
	if (!IsInViewport())
	{
		AddToViewport(60);
	}
	SetVisibility(ESlateVisibility::Visible);
	RefreshInventory();
	EnterWindowInputMode();
}

void UEmberdeepInventoryWidget::RefreshInventory()
{
	RebuildInventoryRows();
}

void UEmberdeepInventoryWidget::RebuildInventoryRows()
{
	if (!ItemRows)
	{
		return;
	}

	ItemRows->ClearChildren();
	APlayerController* PlayerController = GetOwningPlayer();
	AEmberdeepPlayerState* PlayerState = PlayerController ? PlayerController->GetPlayerState<AEmberdeepPlayerState>() : nullptr;
	if (!PlayerState)
	{
		if (EmptyText)
		{
			EmptyText->SetText(FText::FromString(TEXT("Waiting for player inventory...")));
			EmptyText->SetVisibility(ESlateVisibility::Visible);
		}
		return;
	}

	const TArray<FEmberdeepItemInstance>& Inventory = PlayerState->GetInventory();
	if (EmptyText)
	{
		EmptyText->SetText(FText::FromString(TEXT("Your pack is empty.")));
		EmptyText->SetVisibility(Inventory.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	FString WeaponName = TEXT("-");
	FString ArmorName = TEXT("-");
	FString TrinketName = TEXT("-");
	for (const FEmberdeepItemInstance& Item : Inventory)
	{
		const bool bEquipped = PlayerState->IsItemEquipped(Item.InstanceId);
		if (bEquipped)
		{
			switch (Item.Slot)
			{
			case EEmberdeepItemSlot::Weapon: WeaponName = Item.DisplayName; break;
			case EEmberdeepItemSlot::Armor: ArmorName = Item.DisplayName; break;
			case EEmberdeepItemSlot::Trinket: TrinketName = Item.DisplayName; break;
			default: break;
			}
		}

		UEmberdeepIndexedButton* RowButton = WidgetTree->ConstructWidget<UEmberdeepIndexedButton>();
		RowButton->SetEntryId(Item.InstanceId);
		const FLinearColor RarityColor = FEmberdeepItemCatalog::GetRarityColor(Item.Rarity);
		RowButton->SetBackgroundColor(bEquipped
			? FLinearColor(0.22f, 0.14f, 0.035f, 1.0f)
			: RarityColor * 0.20f + FLinearColor(0.025f, 0.022f, 0.024f, 0.80f));
		RowButton->OnIndexedClicked().AddUObject(this, &UEmberdeepInventoryWidget::HandleEquipItem);

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		RowButton->AddChild(Row);

		UVerticalBox* Identity = WidgetTree->ConstructWidget<UVerticalBox>();
		if (UHorizontalBoxSlot* AddedSlot = Row->AddChildToHorizontalBox(Identity))
		{
			AddedSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			AddedSlot->SetPadding(FMargin(12.0f, 7.0f));
		}
		const FString DisplayName = bEquipped ? FString::Printf(TEXT("[EQUIPPED]  %s"), *Item.DisplayName) : Item.DisplayName;
		Identity->AddChildToVerticalBox(EmberdeepInventoryUI::MakeText(WidgetTree, DisplayName, 17, RarityColor));
		Identity->AddChildToVerticalBox(EmberdeepInventoryUI::MakeText(
			WidgetTree, EmberdeepInventoryUI::GetBonusSummary(Item), 12, EmberdeepInventoryUI::Parchment));

		const FString TypeLine = FString::Printf(
			TEXT("%s\n%s  |  iLVL %d"),
			*EmberdeepInventoryUI::GetSlotName(Item.Slot).ToUpper(),
			*FEmberdeepItemCatalog::GetRarityDisplayName(Item.Rarity).ToUpper(),
			Item.ItemLevel);
		UTextBlock* TypeText = EmberdeepInventoryUI::MakeText(
			WidgetTree, TypeLine, 11, bEquipped ? EmberdeepInventoryUI::Gold : EmberdeepInventoryUI::Muted, ETextJustify::Right);
		if (UHorizontalBoxSlot* AddedSlot = Row->AddChildToHorizontalBox(TypeText))
		{
			AddedSlot->SetVerticalAlignment(VAlign_Center);
			AddedSlot->SetPadding(FMargin(8.0f, 7.0f, 12.0f, 7.0f));
		}

		if (UVerticalBoxSlot* AddedSlot = ItemRows->AddChildToVerticalBox(RowButton))
		{
			AddedSlot->SetPadding(FMargin(0.0f, 3.0f));
		}
	}

	if (EquippedSummaryText)
	{
		EquippedSummaryText->SetText(FText::FromString(FString::Printf(
			TEXT("WEAPON: %s   |   ARMOR: %s   |   TRINKET: %s"),
			*WeaponName, *ArmorName, *TrinketName)));
	}
	if (StatSummaryText)
	{
		StatSummaryText->SetText(FText::FromString(FString::Printf(
			TEXT("EQUIPMENT BONUSES   DAMAGE +%.0f   HEALTH +%.0f   ARMOR +%.0f"),
			PlayerState->GetTotalDamageBonus(),
			PlayerState->GetTotalMaxHealthBonus(),
			PlayerState->GetTotalArmorBonus())));
	}
}

void UEmberdeepInventoryWidget::HandleEquipItem(int32 InstanceId)
{
	if (AEmberdeepPlayerController* PlayerController = Cast<AEmberdeepPlayerController>(GetOwningPlayer()))
	{
		PlayerController->RequestEquipItem(InstanceId);
	}
}

void UEmberdeepInventoryWidget::CloseWindow()
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

void UEmberdeepInventoryWidget::EnterWindowInputMode()
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
