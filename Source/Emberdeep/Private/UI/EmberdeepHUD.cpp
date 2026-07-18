#include "UI/EmberdeepHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Gameplay/EmberdeepCharacter.h"
#include "Gameplay/EmberdeepGameMode.h"
#include "Gameplay/EmberdeepHealthComponent.h"

void AEmberdeepHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}

	const AEmberdeepCharacter* Character = Cast<AEmberdeepCharacter>(GetOwningPawn());
	if (!Character)
	{
		return;
	}

	const float Margin = 28.0f;
	const float BarWidth = 270.0f;
	const float Bottom = Canvas->ClipY - 72.0f;
	const UEmberdeepHealthComponent* Health = Character->GetHealthComponent();

	DrawBar(Margin, Bottom, BarWidth, 24.0f, Health->GetHealthNormalized(), FLinearColor(0.62f, 0.025f, 0.018f));
	DrawText(
		FString::Printf(TEXT("FIGHTER   %.0f / %.0f"), Health->GetCurrentHealth(), Health->GetMaxHealth()),
		FLinearColor::White,
		Margin + 10.0f,
		Bottom + 2.0f,
		GEngine->GetSmallFont(),
		1.0f,
		false);

	DrawBar(Margin, Bottom + 32.0f, BarWidth, 10.0f, Character->GetDodgeCooldownNormalized(), FLinearColor(0.12f, 0.48f, 0.78f));
	DrawText(
		FString::Printf(TEXT("GOLD  %d"), Character->GetGold()),
		FLinearColor(1.0f, 0.62f, 0.12f),
		Canvas->ClipX - 150.0f,
		Margin,
		GEngine->GetSmallFont(),
		1.2f,
		false);

	const AEmberdeepGameMode* GameMode = GetWorld()->GetAuthGameMode<AEmberdeepGameMode>();
	if (GameMode)
	{
		DrawText(
			FString::Printf(TEXT("SKELETONS  %d"), GameMode->GetRemainingEnemies()),
			FLinearColor(0.82f, 0.78f, 0.67f),
			Canvas->ClipX - 190.0f,
			Margin + 28.0f,
			GEngine->GetSmallFont(),
			1.0f,
			false);

		if (GameMode->IsEncounterComplete())
		{
			DrawText(
				TEXT("ARENA CLEARED"),
				FLinearColor(1.0f, 0.58f, 0.10f),
				Canvas->ClipX * 0.5f,
				80.0f,
				GEngine->GetLargeFont(),
				1.25f,
				true);
		}
	}

	DrawText(
		TEXT("LMB / SPACE  Attack     RMB / E  Heavy     SHIFT  Dodge"),
		FLinearColor(0.72f, 0.72f, 0.72f),
		Canvas->ClipX * 0.5f,
		Canvas->ClipY - 32.0f,
		GEngine->GetSmallFont(),
		1.0f,
		true);

	if (Character->IsDead())
	{
		DrawText(
			TEXT("YOU FELL - RESTARTING"),
			FLinearColor(0.85f, 0.03f, 0.02f),
			Canvas->ClipX * 0.5f,
			Canvas->ClipY * 0.5f,
			GEngine->GetLargeFont(),
			1.35f,
			true);
	}
}

void AEmberdeepHUD::DrawBar(
	float X,
	float Y,
	float Width,
	float Height,
	float NormalizedValue,
	const FLinearColor& FillColor)
{
	DrawRect(FLinearColor(0.015f, 0.012f, 0.012f, 0.92f), X - 3.0f, Y - 3.0f, Width + 6.0f, Height + 6.0f);
	DrawRect(FLinearColor(0.08f, 0.06f, 0.055f, 0.95f), X, Y, Width, Height);
	DrawRect(FillColor, X, Y, Width * FMath::Clamp(NormalizedValue, 0.0f, 1.0f), Height);
}
