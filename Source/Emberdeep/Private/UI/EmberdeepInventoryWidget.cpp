#include "UI/EmberdeepInventoryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Engine/Texture2D.h"
#include "Gameplay/EmberdeepItemTypes.h"
#include "Gameplay/EmberdeepPlayerController.h"
#include "Gameplay/EmberdeepPlayerState.h"
#include "Styling/SlateBrush.h"
#include "UI/EmberdeepIndexedButton.h"

namespace EmberdeepInventoryUI
{
	constexpr float PanelWidth = 560.0f;
	constexpr float PanelHeight = 840.0f;
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

	FString GetBonusSummary(const FEmberdeepItemInstance& Item)
	{
		TArray<FString> Bonuses;
		if (!FMath::IsNearlyZero(Item.DamageBonus)) Bonuses.Add(FString::Printf(TEXT("+%.0f DAMAGE"), Item.DamageBonus));
		if (!FMath::IsNearlyZero(Item.MaxHealthBonus)) Bonuses.Add(FString::Printf(TEXT("+%.0f HEALTH"), Item.MaxHealthBonus));
		if (!FMath::IsNearlyZero(Item.ArmorBonus)) Bonuses.Add(FString::Printf(TEXT("+%.0f ARMOR"), Item.ArmorBonus));
		return Bonuses.IsEmpty() ? TEXT("NO BONUSES") : FString::Join(Bonuses, TEXT("  |  "));
	}

	FVector2D GetIconCell(const FEmberdeepItemInstance& Item)
	{
		switch (Item.Slot)
		{
		case EEmberdeepItemSlot::Weapon:
			return Item.Rarity == EEmberdeepItemRarity::Legendary ? FVector2D(2.0f, 2.0f)
				: (Item.DefinitionId.ToString().Contains(TEXT("Axe")) ? FVector2D(1.0f, 0.0f) : FVector2D(0.0f, 0.0f));
		case EEmberdeepItemSlot::Armor: return FVector2D(3.0f, 0.0f);
		case EEmberdeepItemSlot::Trinket:
			return Item.DefinitionId.ToString().Contains(TEXT("Ring")) ? FVector2D(2.0f, 1.0f) : FVector2D(3.0f, 1.0f);
		default: return FVector2D(3.0f, 2.0f);
		}
	}

	FSlateBrush MakeIconBrush(UTexture2D* Atlas, const FEmberdeepItemInstance& Item, float Size)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Atlas);
		Brush.ImageSize = FVector2D(Size);
		const FVector2D Cell = GetIconCell(Item);
		const FVector2f Min(Cell.X / 4.0f, Cell.Y / 3.0f);
		const FVector2f Max((Cell.X + 1.0f) / 4.0f, (Cell.Y + 1.0f) / 3.0f);
		Brush.SetUVRegion(FBox2f(Min, Max));
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

TSharedRef<SWidget> UEmberdeepInventoryWidget::RebuildWidget()
{
	if (WidgetTree->RootWidget) return Super::RebuildWidget();

	PanelTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Emberdeep/UI/T_InventoryPanel.T_InventoryPanel"));
	ItemIconAtlas = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Emberdeep/UI/T_ItemIconAtlas.T_ItemIconAtlas"));

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Root;

	UScaleBox* ResponsiveScale = WidgetTree->ConstructWidget<UScaleBox>();
	ResponsiveScale->SetStretch(EStretch::ScaleToFit);
	ResponsiveScale->SetStretchDirection(EStretchDirection::DownOnly);
	if (UOverlaySlot* RootSlot = Root->AddChildToOverlay(ResponsiveScale))
	{
		RootSlot->SetHorizontalAlignment(HAlign_Right);
		RootSlot->SetVerticalAlignment(VAlign_Center);
		RootSlot->SetPadding(FMargin(24.0f));
	}

	USizeBox* DesignSize = WidgetTree->ConstructWidget<USizeBox>();
	DesignSize->SetWidthOverride(EmberdeepInventoryUI::PanelWidth);
	DesignSize->SetHeightOverride(EmberdeepInventoryUI::PanelHeight);
	ResponsiveScale->SetContent(DesignSize);

	UOverlay* Panel = WidgetTree->ConstructWidget<UOverlay>();
	DesignSize->SetContent(Panel);
	UImage* Background = WidgetTree->ConstructWidget<UImage>();
	Background->SetBrushFromTexture(PanelTexture, true);
	Panel->AddChildToOverlay(Background);

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	Panel->AddChildToOverlay(Canvas);

	UTextBlock* Heading = EmberdeepInventoryUI::MakeText(WidgetTree, TEXT("INVENTORY"), 24, EmberdeepInventoryUI::Gold, ETextJustify::Center);
	EmberdeepInventoryUI::Place(Canvas, Heading, FVector2D(153.0f, 48.0f), FVector2D(255.0f, 40.0f));

	UButton* CloseButton = WidgetTree->ConstructWidget<UButton>();
	CloseButton->SetBackgroundColor(FLinearColor(0.32f, 0.05f, 0.025f, 0.42f));
	CloseButton->OnClicked.AddDynamic(this, &UEmberdeepInventoryWidget::CloseWindow);
	CloseButton->AddChild(EmberdeepInventoryUI::MakeText(WidgetTree, TEXT("X"), 18, EmberdeepInventoryUI::Parchment, ETextJustify::Center));
	EmberdeepInventoryUI::Place(Canvas, CloseButton, FVector2D(500.0f, 43.0f), FVector2D(34.0f, 34.0f));

	EquippedWeaponIcon = WidgetTree->ConstructWidget<UImage>();
	EquippedArmorIcon = WidgetTree->ConstructWidget<UImage>();
	EquippedTrinketIcon = WidgetTree->ConstructWidget<UImage>();
	EquippedWeaponIcon->SetVisibility(ESlateVisibility::Collapsed);
	EquippedArmorIcon->SetVisibility(ESlateVisibility::Collapsed);
	EquippedTrinketIcon->SetVisibility(ESlateVisibility::Collapsed);
	EmberdeepInventoryUI::Place(Canvas, EquippedWeaponIcon, FVector2D(68.0f, 344.0f), FVector2D(74.0f, 142.0f));
	EmberdeepInventoryUI::Place(Canvas, EquippedArmorIcon, FVector2D(250.0f, 210.0f), FVector2D(88.0f, 125.0f));
	EmberdeepInventoryUI::Place(Canvas, EquippedTrinketIcon, FVector2D(365.0f, 348.0f), FVector2D(67.0f, 67.0f));

	StatSummaryText = EmberdeepInventoryUI::MakeText(WidgetTree, TEXT("DAMAGE +0   HEALTH +0   ARMOR +0"), 12, EmberdeepInventoryUI::Gold, ETextJustify::Center);
	EmberdeepInventoryUI::Place(Canvas, StatSummaryText, FVector2D(160.0f, 522.0f), FVector2D(305.0f, 28.0f));

	ItemRows = WidgetTree->ConstructWidget<UUniformGridPanel>();
	ItemRows->SetSlotPadding(FMargin(3.0f));
	EmberdeepInventoryUI::Place(Canvas, ItemRows, FVector2D(54.0f, 565.0f), FVector2D(454.0f, 245.0f));

	EmptyText = EmberdeepInventoryUI::MakeText(WidgetTree, TEXT("YOUR PACK IS EMPTY"), 15, EmberdeepInventoryUI::Muted, ETextJustify::Center);
	EmberdeepInventoryUI::Place(Canvas, EmptyText, FVector2D(130.0f, 665.0f), FVector2D(300.0f, 30.0f));

	RebuildInventoryRows();
	return Super::RebuildWidget();
}

void UEmberdeepInventoryWidget::ShowInventory()
{
	if (!IsInViewport()) AddToViewport(60);
	SetVisibility(ESlateVisibility::Visible);
	RefreshInventory();
	EnterWindowInputMode();
}

void UEmberdeepInventoryWidget::RefreshInventory() { RebuildInventoryRows(); }

void UEmberdeepInventoryWidget::RebuildInventoryRows()
{
	if (!ItemRows) return;
	ItemRows->ClearChildren();

	APlayerController* Controller = GetOwningPlayer();
	AEmberdeepPlayerState* PlayerState = Controller ? Controller->GetPlayerState<AEmberdeepPlayerState>() : nullptr;
	if (!PlayerState)
	{
		if (EmptyText)
		{
			EmptyText->SetText(FText::FromString(TEXT("WAITING FOR INVENTORY...")));
			EmptyText->SetVisibility(ESlateVisibility::Visible);
		}
		return;
	}

	const TArray<FEmberdeepItemInstance>& Inventory = PlayerState->GetInventory();
	if (EmptyText)
	{
		EmptyText->SetText(FText::FromString(TEXT("YOUR PACK IS EMPTY")));
		EmptyText->SetVisibility(Inventory.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	EquippedWeaponIcon->SetVisibility(ESlateVisibility::Collapsed);
	EquippedArmorIcon->SetVisibility(ESlateVisibility::Collapsed);
	EquippedTrinketIcon->SetVisibility(ESlateVisibility::Collapsed);

	for (int32 CellIndex = 0; CellIndex < 15; ++CellIndex)
	{
		USizeBox* CellSize = WidgetTree->ConstructWidget<USizeBox>();
		CellSize->SetWidthOverride(85.0f);
		CellSize->SetHeightOverride(75.0f);
		if (CellIndex < Inventory.Num())
		{
			const FEmberdeepItemInstance& Item = Inventory[CellIndex];
			const bool bEquipped = PlayerState->IsItemEquipped(Item.InstanceId);
			UEmberdeepIndexedButton* Button = WidgetTree->ConstructWidget<UEmberdeepIndexedButton>();
			Button->SetEntryId(Item.InstanceId);
			Button->SetBackgroundColor(bEquipped ? FLinearColor(0.55f, 0.32f, 0.05f, 0.42f) : FLinearColor(0.02f, 0.015f, 0.012f, 0.12f));
			Button->SetToolTipText(FText::FromString(FString::Printf(TEXT("%s\n%s · ITEM LEVEL %d\n%s\nClick to equip"),
				*Item.DisplayName, *FEmberdeepItemCatalog::GetRarityDisplayName(Item.Rarity).ToUpper(), Item.ItemLevel,
				*EmberdeepInventoryUI::GetBonusSummary(Item))));
			Button->OnIndexedClicked().AddUObject(this, &UEmberdeepInventoryWidget::HandleEquipItem);
			UImage* Icon = WidgetTree->ConstructWidget<UImage>();
			Icon->SetBrush(EmberdeepInventoryUI::MakeIconBrush(ItemIconAtlas, Item, 70.0f));
			Icon->SetColorAndOpacity(bEquipped ? FLinearColor(1.0f, 0.86f, 0.48f, 1.0f) : FLinearColor::White);
			Button->AddChild(Icon);
			CellSize->SetContent(Button);

			if (bEquipped)
			{
				UImage* EquippedIcon = Item.Slot == EEmberdeepItemSlot::Weapon ? EquippedWeaponIcon
					: (Item.Slot == EEmberdeepItemSlot::Armor ? EquippedArmorIcon : EquippedTrinketIcon);
				EquippedIcon->SetBrush(EmberdeepInventoryUI::MakeIconBrush(ItemIconAtlas, Item, 86.0f));
				EquippedIcon->SetToolTipText(FText::FromString(Item.DisplayName));
				EquippedIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		}
		else
		{
			UBorder* EmptyCell = WidgetTree->ConstructWidget<UBorder>();
			EmptyCell->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.04f));
			CellSize->SetContent(EmptyCell);
		}
		ItemRows->AddChildToUniformGrid(CellSize, CellIndex / 5, CellIndex % 5);
	}

	if (StatSummaryText)
	{
		StatSummaryText->SetText(FText::FromString(FString::Printf(TEXT("DAMAGE +%.0f   HEALTH +%.0f   ARMOR +%.0f"),
			PlayerState->GetTotalDamageBonus(), PlayerState->GetTotalMaxHealthBonus(), PlayerState->GetTotalArmorBonus())));
	}
}

void UEmberdeepInventoryWidget::HandleEquipItem(int32 InstanceId)
{
	if (AEmberdeepPlayerController* Controller = Cast<AEmberdeepPlayerController>(GetOwningPlayer())) Controller->RequestEquipItem(InstanceId);
}

void UEmberdeepInventoryWidget::CloseWindow()
{
	if (AEmberdeepPlayerController* Controller = Cast<AEmberdeepPlayerController>(GetOwningPlayer())) Controller->CloseGameplayWindows();
	else RemoveFromParent();
}

void UEmberdeepInventoryWidget::EnterWindowInputMode()
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
