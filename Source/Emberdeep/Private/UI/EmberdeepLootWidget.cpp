#include "UI/EmberdeepLootWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Gameplay/EmberdeepPlayerController.h"
#include "Styling/SlateBrush.h"
#include "UI/EmberdeepIndexedButton.h"

namespace EmberdeepLootUI
{
	const FLinearColor Gold(0.94f, 0.65f, 0.24f, 1.0f);
	const FLinearColor Parchment(0.88f, 0.80f, 0.65f, 1.0f);
	const FLinearColor Muted(0.60f, 0.55f, 0.48f, 1.0f);

	UTextBlock* MakeText(UWidgetTree* Tree, const FString& Value, int32 Size, const FLinearColor& Color,
		ETextJustify::Type Justification = ETextJustify::Left)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
		Text->SetText(FText::FromString(Value));
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetJustification(Justification);
		Text->SetShadowOffset(FVector2D(1.0f, 2.0f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.95f));
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Font.TypefaceFontName = TEXT("Bold");
		Text->SetFont(Font);
		return Text;
	}

	FString GetSlotName(EEmberdeepItemSlot Slot)
	{
		if (const UEnum* Enum = StaticEnum<EEmberdeepItemSlot>()) return Enum->GetDisplayNameTextByValue(static_cast<int64>(Slot)).ToString();
		return TEXT("Item");
	}

	FString GetBonusSummary(const FEmberdeepItemInstance& Item)
	{
		TArray<FString> Bonuses;
		if (!FMath::IsNearlyZero(Item.DamageBonus)) Bonuses.Add(FString::Printf(TEXT("+%.0f DAMAGE"), Item.DamageBonus));
		if (!FMath::IsNearlyZero(Item.MaxHealthBonus)) Bonuses.Add(FString::Printf(TEXT("+%.0f HEALTH"), Item.MaxHealthBonus));
		if (!FMath::IsNearlyZero(Item.ArmorBonus)) Bonuses.Add(FString::Printf(TEXT("+%.0f ARMOR"), Item.ArmorBonus));
		return Bonuses.IsEmpty() ? TEXT("NO BONUSES") : FString::Join(Bonuses, TEXT("   "));
	}

	FVector2D GetIconCell(const FEmberdeepItemInstance& Item)
	{
		if (Item.Slot == EEmberdeepItemSlot::Weapon)
		{
			if (Item.Rarity == EEmberdeepItemRarity::Legendary) return FVector2D(2.0f, 2.0f);
			return Item.DefinitionId.ToString().Contains(TEXT("Axe")) ? FVector2D(1.0f, 0.0f) : FVector2D(0.0f, 0.0f);
		}
		if (Item.Slot == EEmberdeepItemSlot::Armor) return FVector2D(3.0f, 0.0f);
		if (Item.Slot == EEmberdeepItemSlot::Trinket) return Item.DefinitionId.ToString().Contains(TEXT("Ring")) ? FVector2D(2.0f, 1.0f) : FVector2D(3.0f, 1.0f);
		return FVector2D(3.0f, 2.0f);
	}

	FSlateBrush MakeIconBrush(UTexture2D* Atlas, const FEmberdeepItemInstance& Item)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Atlas);
		Brush.ImageSize = FVector2D(70.0f);
		const FVector2D Cell = GetIconCell(Item);
		Brush.SetUVRegion(FBox2f(FVector2f(Cell.X / 4.0f, Cell.Y / 3.0f), FVector2f((Cell.X + 1.0f) / 4.0f, (Cell.Y + 1.0f) / 3.0f)));
		return Brush;
	}

	void Place(UCanvasPanel* Canvas, UWidget* Widget, const FVector2D& Position, const FVector2D& Size)
	{
		if (UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget))
		{
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
		}
	}
}

TSharedRef<SWidget> UEmberdeepLootWidget::RebuildWidget()
{
	if (WidgetTree->RootWidget) return Super::RebuildWidget();
	PanelTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Emberdeep/UI/T_LootPanel.T_LootPanel"));
	ItemIconAtlas = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Emberdeep/UI/T_ItemIconAtlas.T_ItemIconAtlas"));

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Root;

	UScaleBox* ResponsiveScale = WidgetTree->ConstructWidget<UScaleBox>();
	ResponsiveScale->SetStretch(EStretch::ScaleToFit);
	ResponsiveScale->SetStretchDirection(EStretchDirection::DownOnly);
	if (UOverlaySlot* AddedSlot = Root->AddChildToOverlay(ResponsiveScale))
	{
		AddedSlot->SetHorizontalAlignment(HAlign_Right);
		AddedSlot->SetVerticalAlignment(VAlign_Center);
		AddedSlot->SetPadding(FMargin(24.0f));
	}

	USizeBox* DesignSize = WidgetTree->ConstructWidget<USizeBox>();
	DesignSize->SetWidthOverride(600.0f);
	DesignSize->SetHeightOverride(750.0f);
	ResponsiveScale->SetContent(DesignSize);

	UOverlay* Panel = WidgetTree->ConstructWidget<UOverlay>();
	DesignSize->SetContent(Panel);
	UImage* Background = WidgetTree->ConstructWidget<UImage>();
	Background->SetBrushFromTexture(PanelTexture, true);
	Panel->AddChildToOverlay(Background);
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	Panel->AddChildToOverlay(Canvas);

	TitleText = EmberdeepLootUI::MakeText(WidgetTree, CurrentTitle.IsEmpty() ? TEXT("SPOILS") : CurrentTitle.ToUpper(), 24, EmberdeepLootUI::Gold, ETextJustify::Center);
	EmberdeepLootUI::Place(Canvas, TitleText, FVector2D(132.0f, 55.0f), FVector2D(336.0f, 42.0f));

	UButton* CloseX = WidgetTree->ConstructWidget<UButton>();
	CloseX->SetBackgroundColor(FLinearColor(0.18f, 0.05f, 0.025f, 0.22f));
	CloseX->OnClicked.AddDynamic(this, &UEmberdeepLootWidget::CloseWindow);
	CloseX->AddChild(EmberdeepLootUI::MakeText(WidgetTree, TEXT("X"), 17, EmberdeepLootUI::Parchment, ETextJustify::Center));
	EmberdeepLootUI::Place(Canvas, CloseX, FVector2D(517.0f, 57.0f), FVector2D(32.0f, 32.0f));

	ItemRows = WidgetTree->ConstructWidget<UVerticalBox>();
	EmberdeepLootUI::Place(Canvas, ItemRows, FVector2D(98.0f, 140.0f), FVector2D(398.0f, 467.0f));

	EmptyText = EmberdeepLootUI::MakeText(WidgetTree, TEXT("NOTHING REMAINS"), 16, EmberdeepLootUI::Muted, ETextJustify::Center);
	EmberdeepLootUI::Place(Canvas, EmptyText, FVector2D(150.0f, 350.0f), FVector2D(300.0f, 32.0f));

	UButton* TakeAllButton = WidgetTree->ConstructWidget<UButton>();
	TakeAllButton->SetBackgroundColor(FLinearColor(0.30f, 0.10f, 0.025f, 0.28f));
	TakeAllButton->OnClicked.AddDynamic(this, &UEmberdeepLootWidget::TakeAll);
	TakeAllButton->AddChild(EmberdeepLootUI::MakeText(WidgetTree, TEXT("TAKE ALL"), 17, EmberdeepLootUI::Gold, ETextJustify::Center));
	EmberdeepLootUI::Place(Canvas, TakeAllButton, FVector2D(100.0f, 638.0f), FVector2D(190.0f, 62.0f));

	UButton* CloseButton = WidgetTree->ConstructWidget<UButton>();
	CloseButton->SetBackgroundColor(FLinearColor(0.18f, 0.06f, 0.025f, 0.20f));
	CloseButton->OnClicked.AddDynamic(this, &UEmberdeepLootWidget::CloseWindow);
	CloseButton->AddChild(EmberdeepLootUI::MakeText(WidgetTree, TEXT("LEAVE"), 17, EmberdeepLootUI::Parchment, ETextJustify::Center));
	EmberdeepLootUI::Place(Canvas, CloseButton, FVector2D(310.0f, 638.0f), FVector2D(190.0f, 62.0f));

	RebuildLootRows();
	return Super::RebuildWidget();
}

void UEmberdeepLootWidget::ShowLoot(const FString& Title, const TArray<FEmberdeepItemInstance>& Items)
{
	RefreshLoot(Title, Items);
	if (!IsInViewport()) AddToViewport(70);
	SetVisibility(ESlateVisibility::Visible);
	EnterWindowInputMode();
}

void UEmberdeepLootWidget::RefreshLoot(const FString& Title, const TArray<FEmberdeepItemInstance>& Items)
{
	CurrentTitle = Title;
	CurrentItems = Items;
	if (TitleText) TitleText->SetText(FText::FromString(CurrentTitle.IsEmpty() ? TEXT("SPOILS") : CurrentTitle.ToUpper()));
	RebuildLootRows();
}

void UEmberdeepLootWidget::RebuildLootRows()
{
	if (!ItemRows) return;
	ItemRows->ClearChildren();
	if (EmptyText) EmptyText->SetVisibility(CurrentItems.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	for (int32 Index = 0; Index < 5; ++Index)
	{
		USizeBox* RowSize = WidgetTree->ConstructWidget<USizeBox>();
		RowSize->SetHeightOverride(88.0f);
		if (Index < CurrentItems.Num())
		{
			const FEmberdeepItemInstance& Item = CurrentItems[Index];
			UEmberdeepIndexedButton* RowButton = WidgetTree->ConstructWidget<UEmberdeepIndexedButton>();
			RowButton->SetEntryId(Item.InstanceId);
			RowButton->SetBackgroundColor(FLinearColor(0.02f, 0.015f, 0.012f, 0.08f));
			RowButton->SetToolTipText(FText::FromString(TEXT("Click to take this item")));
			RowButton->OnIndexedClicked().AddUObject(this, &UEmberdeepLootWidget::HandleTakeItem);
			UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
			RowButton->AddChild(Row);

			USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>();
			IconSize->SetWidthOverride(78.0f);
			IconSize->SetHeightOverride(78.0f);
			UImage* Icon = WidgetTree->ConstructWidget<UImage>();
			Icon->SetBrush(EmberdeepLootUI::MakeIconBrush(ItemIconAtlas, Item));
			IconSize->SetContent(Icon);
			if (UHorizontalBoxSlot* AddedSlot = Row->AddChildToHorizontalBox(IconSize)) AddedSlot->SetPadding(FMargin(5.0f));

			UVerticalBox* Identity = WidgetTree->ConstructWidget<UVerticalBox>();
			if (UHorizontalBoxSlot* AddedSlot = Row->AddChildToHorizontalBox(Identity))
			{
				AddedSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				AddedSlot->SetVerticalAlignment(VAlign_Center);
				AddedSlot->SetPadding(FMargin(10.0f, 8.0f, 6.0f, 5.0f));
			}
			Identity->AddChildToVerticalBox(EmberdeepLootUI::MakeText(WidgetTree, Item.DisplayName.ToUpper(), 16, FEmberdeepItemCatalog::GetRarityColor(Item.Rarity)));
			Identity->AddChildToVerticalBox(EmberdeepLootUI::MakeText(WidgetTree, EmberdeepLootUI::GetBonusSummary(Item), 11, EmberdeepLootUI::Parchment));
			Identity->AddChildToVerticalBox(EmberdeepLootUI::MakeText(WidgetTree, FString::Printf(TEXT("%s  ·  %s  ·  iLVL %d"),
				*FEmberdeepItemCatalog::GetRarityDisplayName(Item.Rarity).ToUpper(), *EmberdeepLootUI::GetSlotName(Item.Slot).ToUpper(), Item.ItemLevel), 10, EmberdeepLootUI::Muted));
			RowSize->SetContent(RowButton);
		}
		if (UVerticalBoxSlot* AddedSlot = ItemRows->AddChildToVerticalBox(RowSize)) AddedSlot->SetPadding(FMargin(0.0f, 1.8f));
	}
}

void UEmberdeepLootWidget::HandleTakeItem(int32 EntryId)
{
	if (AEmberdeepPlayerController* Controller = Cast<AEmberdeepPlayerController>(GetOwningPlayer())) Controller->RequestTakeLoot(EntryId);
}

void UEmberdeepLootWidget::TakeAll()
{
	AEmberdeepPlayerController* Controller = Cast<AEmberdeepPlayerController>(GetOwningPlayer());
	if (!Controller) return;
	TArray<int32> Ids;
	for (const FEmberdeepItemInstance& Item : CurrentItems) Ids.Add(Item.InstanceId);
	for (const int32 Id : Ids) Controller->RequestTakeLoot(Id);
}

void UEmberdeepLootWidget::CloseWindow()
{
	if (AEmberdeepPlayerController* Controller = Cast<AEmberdeepPlayerController>(GetOwningPlayer())) Controller->CloseGameplayWindows();
	else RemoveFromParent();
}

void UEmberdeepLootWidget::EnterWindowInputMode()
{
	if (APlayerController* Controller = GetOwningPlayer())
	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetWidgetToFocus(TakeWidget());
		Controller->SetInputMode(Mode);
		Controller->SetShowMouseCursor(true);
	}
}
