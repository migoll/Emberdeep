#include "UI/EmberdeepMainMenuWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Emberdeep.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetTree.h"

namespace
{
	UTextBlock* AddMenuText(
		UWidgetTree* WidgetTree,
		UVerticalBox* Parent,
		const FString& Text,
		int32 FontSize,
		const FLinearColor& Color,
		ETextJustify::Type Justification = ETextJustify::Center)
	{
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>();
		TextBlock->SetText(FText::FromString(Text));
		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		TextBlock->SetJustification(Justification);
		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = FontSize;
		TextBlock->SetFont(Font);
		Parent->AddChildToVerticalBox(TextBlock);
		return TextBlock;
	}

	UButton* AddMenuButton(UWidgetTree* WidgetTree, UVerticalBox* Parent, const FString& Label)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>();
		Button->SetBackgroundColor(FLinearColor(0.25f, 0.12f, 0.035f, 1.0f));
		UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>();
		LabelText->SetText(FText::FromString(Label));
		LabelText->SetJustification(ETextJustify::Center);
		LabelText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.72f, 0.25f)));
		FSlateFontInfo Font = LabelText->GetFont();
		Font.Size = 24;
		LabelText->SetFont(Font);
		Button->AddChild(LabelText);
		if (UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Button))
		{
			Slot->SetPadding(FMargin(0.0f, 8.0f));
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		return Button;
	}
}

TSharedRef<SWidget> UEmberdeepMainMenuWidget::RebuildWidget()
{
	if (WidgetTree->RootWidget)
	{
		return Super::RebuildWidget();
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Root;

	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>();
	Backdrop->SetBrushColor(FLinearColor(0.008f, 0.006f, 0.008f, 0.90f));
	if (UCanvasPanelSlot* BackdropSlot = Root->AddChildToCanvas(Backdrop))
	{
		BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackdropSlot->SetOffsets(FMargin(0.0f));
	}

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(FLinearColor(0.035f, 0.025f, 0.025f, 0.98f));
	Panel->SetPadding(FMargin(42.0f, 32.0f));
	if (UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f));
		PanelSlot->SetPosition(FVector2D::ZeroVector);
		PanelSlot->SetSize(FVector2D(620.0f, 470.0f));
	}

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Content);
	AddMenuText(WidgetTree, Content, TEXT("EMBERDEEP"), 46, FLinearColor(0.95f, 0.52f, 0.12f));
	AddMenuText(WidgetTree, Content, TEXT("AUTHORITATIVE CO-OP COMBAT PROOF"), 15, FLinearColor(0.62f, 0.60f, 0.56f));

	USpacer* Spacer = WidgetTree->ConstructWidget<USpacer>();
	Spacer->SetSize(FVector2D(1.0f, 24.0f));
	Content->AddChildToVerticalBox(Spacer);

	UButton* HostButton = AddMenuButton(WidgetTree, Content, TEXT("HOST GAME"));
	HostButton->OnClicked.AddDynamic(this, &UEmberdeepMainMenuWidget::HostGame);
	AddMenuText(WidgetTree, Content, TEXT("JOIN BY DIRECT IP"), 16, FLinearColor(0.76f, 0.72f, 0.65f), ETextJustify::Left);

	AddressField = WidgetTree->ConstructWidget<UEditableTextBox>();
	AddressField->SetText(FText::FromString(TEXT("127.0.0.1:7777")));
	AddressField->SetHintText(FText::FromString(TEXT("IP address, optionally :port")));
	AddressField->SetForegroundColor(FLinearColor::White);
	if (UVerticalBoxSlot* AddressSlot = Content->AddChildToVerticalBox(AddressField))
	{
		AddressSlot->SetPadding(FMargin(0.0f, 6.0f));
	}

	UButton* JoinButton = AddMenuButton(WidgetTree, Content, TEXT("JOIN GAME"));
	JoinButton->OnClicked.AddDynamic(this, &UEmberdeepMainMenuWidget::JoinGame);
	StatusText = AddMenuText(WidgetTree, Content, TEXT("Host uses UDP port 7777. Up to five players."), 14, FLinearColor(0.54f, 0.52f, 0.49f));
	AddMenuText(WidgetTree, Content, TEXT("LAN and forwarded direct-IP connections only in Phase 0B."), 13, FLinearColor(0.42f, 0.40f, 0.38f));
	return Super::RebuildWidget();
}

void UEmberdeepMainMenuWidget::HostGame()
{
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		StatusText->SetText(FText::FromString(TEXT("Opening listen server...")));
		LeaveMenuMode();
		PlayerController->ConsoleCommand(TEXT("open /Engine/Maps/Entry?listen"));
	}
}

void UEmberdeepMainMenuWidget::JoinGame()
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController || !AddressField)
	{
		return;
	}

	FString Address = AddressField->GetText().ToString().TrimStartAndEnd();
	if (Address.IsEmpty())
	{
		Address = TEXT("127.0.0.1:7777");
	}
	StatusText->SetText(FText::FromString(FString::Printf(TEXT("Connecting to %s..."), *Address)));
	LeaveMenuMode();
	PlayerController->ClientTravel(Address, TRAVEL_Absolute);
}

void UEmberdeepMainMenuWidget::LeaveMenuMode()
{
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->SetPause(false);
		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(false);
		PlayerController->SetInputMode(InputMode);
	}
	RemoveFromParent();
}
