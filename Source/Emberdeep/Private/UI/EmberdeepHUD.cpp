#include "UI/EmberdeepHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Gameplay/EmberdeepCharacter.h"
#include "Gameplay/EmberdeepEnemy.h"
#include "Gameplay/EmberdeepGameMode.h"
#include "Gameplay/EmberdeepHealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AEmberdeepHUD::AEmberdeepHUD()
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> OrnamentTexture(
		TEXT("/Game/Emberdeep/UI/T_HUDOrnaments.T_HUDOrnaments"));
	HudOrnaments = OrnamentTexture.Object;

	static ConstructorHelpers::FObjectFinder<UTexture2D> ConnectedHudTexture(
		TEXT("/Game/Emberdeep/UI/T_ConnectedCombatHUD.T_ConnectedCombatHUD"));
	ConnectedCombatHud = ConnectedHudTexture.Object;
}

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

	DrawPartyRoster();
	DrawMinimapAndObjective();
	DrawActionBar(Character);

	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(this, AEmberdeepEnemy::StaticClass(), Enemies);
	for (AActor* EnemyActor : Enemies)
	{
		const AEmberdeepEnemy* Enemy = Cast<AEmberdeepEnemy>(EnemyActor);
		if (!Enemy || Enemy->GetHealthComponent()->IsDead())
		{
			continue;
		}

		FVector2D ScreenPosition;
		if (PlayerOwner && PlayerOwner->ProjectWorldLocationToScreen(
			Enemy->GetActorLocation() + FVector(0.0f, 0.0f, Enemy->IsElite() ? 155.0f : 115.0f),
			ScreenPosition))
		{
			const float EnemyBarWidth = Enemy->IsElite() ? 112.0f : 76.0f;
			DrawBar(
				ScreenPosition.X - EnemyBarWidth * 0.5f,
				ScreenPosition.Y,
				EnemyBarWidth,
				Enemy->IsElite() ? 10.0f : 7.0f,
				Enemy->GetHealthComponent()->GetHealthNormalized(),
				Enemy->IsAttackWindingUp()
					? FLinearColor(1.0f, 0.34f, 0.015f)
					: FLinearColor(0.68f, 0.018f, 0.012f));

			if (Enemy->IsElite())
			{
				DrawCenteredText(TEXT("BONE WARDEN"), FLinearColor(0.92f, 0.55f, 0.18f), ScreenPosition.X, ScreenPosition.Y - 18.0f, GEngine->GetSmallFont(), 0.85f);
			}
		}
	}

	if (Character->IsDead())
	{
		DrawCenteredText(TEXT("YOU FELL - RESTARTING"), FLinearColor(0.85f, 0.03f, 0.02f), Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f, GEngine->GetLargeFont(), 1.35f);
	}
}

void AEmberdeepHUD::DrawPartyRoster()
{
	const AGameStateBase* GameState = GetWorld()->GetGameState<AGameStateBase>();
	if (!GameState)
	{
		return;
	}

	const float StartX = 20.0f;
	const float StartY = 18.0f;
	const float RowHeight = 58.0f;
	for (int32 PlayerIndex = 0; PlayerIndex < GameState->PlayerArray.Num() && PlayerIndex < 5; ++PlayerIndex)
	{
		const APlayerState* PlayerState = GameState->PlayerArray[PlayerIndex];
		const AEmberdeepCharacter* PartyCharacter = PlayerState
			? Cast<AEmberdeepCharacter>(PlayerState->GetPawn())
			: nullptr;
		if (!PartyCharacter)
		{
			continue;
		}

		const float Y = StartY + PlayerIndex * RowHeight;
		DrawOrnamentSection(StartX, Y, 50.0f, 50.0f, 580.0f, 170.0f, 280.0f, 270.0f);
		DrawPanel(StartX + 48.0f, Y + 5.0f, 142.0f, 40.0f);
		DrawText(
			FString::Printf(TEXT("P%d  FIGHTER"), PlayerIndex + 1),
			PlayerIndex == 0 ? FLinearColor(0.95f, 0.62f, 0.24f) : FLinearColor(0.82f, 0.82f, 0.78f),
			StartX + 56.0f,
			Y + 6.0f,
			GEngine->GetSmallFont(),
			0.95f,
			false);

		const UEmberdeepHealthComponent* PartyHealth = PartyCharacter->GetHealthComponent();
		DrawBar(StartX + 56.0f, Y + 27.0f, 125.0f, 11.0f, PartyHealth->GetHealthNormalized(), FLinearColor(0.66f, 0.025f, 0.018f));
		DrawCenteredText(
			FString::Printf(TEXT("%.0f/%.0f"), PartyHealth->GetCurrentHealth(), PartyHealth->GetMaxHealth()),
			FLinearColor::White, StartX + 118.5f, Y + 25.0f, GEngine->GetSmallFont(), 0.72f);
	}
}

void AEmberdeepHUD::DrawMinimapAndObjective()
{
	const float MapWidth = 195.0f;
	const float MapHeight = 138.0f;
	const float MapX = Canvas->ClipX - MapWidth - 26.0f;
	const float MapY = 20.0f;
	DrawOrnamentSection(MapX - 10.0f, MapY - 8.0f, MapWidth + 20.0f, MapHeight + 19.0f, 990.0f, 25.0f, 520.0f, 505.0f);
	DrawRect(FLinearColor(0.035f, 0.035f, 0.038f, 0.92f), MapX + 11.0f, MapY + 16.0f, MapWidth - 22.0f, MapHeight - 32.0f);
	DrawRect(FLinearColor(0.16f, 0.16f, 0.17f, 0.95f), MapX + 24.0f, MapY + 31.0f, MapWidth - 48.0f, MapHeight - 62.0f);

	const auto WorldToMap = [MapX, MapY, MapWidth, MapHeight](const FVector& WorldLocation)
	{
		return FVector2D(
			MapX + 24.0f + FMath::Clamp((WorldLocation.X + 900.0f) / 1800.0f, 0.0f, 1.0f) * (MapWidth - 48.0f),
			MapY + 31.0f + FMath::Clamp((WorldLocation.Y + 700.0f) / 1400.0f, 0.0f, 1.0f) * (MapHeight - 62.0f));
	};

	if (const AGameStateBase* GameState = GetWorld()->GetGameState<AGameStateBase>())
	{
		for (const APlayerState* PlayerState : GameState->PlayerArray)
		{
			if (const APawn* Pawn = PlayerState ? PlayerState->GetPawn() : nullptr)
			{
				const FVector2D Marker = WorldToMap(Pawn->GetActorLocation());
				DrawRect(FLinearColor(0.08f, 0.58f, 1.0f), Marker.X - 3.0f, Marker.Y - 3.0f, 6.0f, 6.0f);
			}
		}
	}

	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(this, AEmberdeepEnemy::StaticClass(), Enemies);
	for (const AActor* Enemy : Enemies)
	{
		const FVector2D Marker = WorldToMap(Enemy->GetActorLocation());
		DrawRect(FLinearColor(0.88f, 0.055f, 0.025f), Marker.X - 2.0f, Marker.Y - 2.0f, 4.0f, 4.0f);
	}

	DrawText(TEXT("THE BONE WARDEN"), FLinearColor(0.95f, 0.62f, 0.20f), MapX, MapY + MapHeight + 16.0f, GEngine->GetSmallFont(), 1.0f, false);
	DrawText(TEXT("Defeat the arena skeletons"), FLinearColor(0.82f, 0.80f, 0.74f), MapX, MapY + MapHeight + 39.0f, GEngine->GetSmallFont(), 0.9f, false);

	const AEmberdeepGameMode* GameMode = GetWorld()->GetAuthGameMode<AEmberdeepGameMode>();
	const bool bComplete = GameMode && GameMode->IsEncounterComplete();
	DrawRect(bComplete ? FLinearColor(0.82f, 0.55f, 0.12f) : FLinearColor(0.14f, 0.13f, 0.12f), MapX, MapY + MapHeight + 65.0f, 12.0f, 12.0f);
	DrawText(
		bComplete ? TEXT("Arena cleared") : TEXT("Bone Warden remains"),
		FLinearColor(0.76f, 0.74f, 0.68f),
		MapX + 21.0f,
		MapY + MapHeight + 62.0f,
		GEngine->GetSmallFont(),
		0.85f,
		false);
}

void AEmberdeepHUD::DrawActionBar(const AEmberdeepCharacter* Character)
{
	constexpr float DesignWidth = 1744.0f;
	constexpr float DesignHeight = 362.0f;
	const float TargetWidth = FMath::Clamp(Canvas->ClipX * 0.72f, 620.0f, 980.0f);
	const float Scale = TargetWidth / DesignWidth;
	const float HudWidth = DesignWidth * Scale;
	const float HudHeight = DesignHeight * Scale;
	const float HudX = (Canvas->ClipX - HudWidth) * 0.5f;
	const float HudY = Canvas->ClipY - HudHeight - 4.0f;
	const auto DesignPoint = [HudX, HudY, Scale](float X, float Y)
	{
		return FVector2D(HudX + X * Scale, HudY + Y * Scale);
	};

	// Live liquids and icons are authored in the frame's native 1744x362
	// coordinate system. The single Scale value keeps every layer attached.
	const FVector2D HealthCenter = DesignPoint(228.0f, 161.5f);
	const FVector2D ResourceCenter = DesignPoint(1512.0f, 161.5f);

	const FVector4 SlotBounds[] =
	{
		FVector4(417.0f, 123.0f, 161.0f, 153.0f),
		FVector4(611.0f, 123.0f, 160.0f, 153.0f),
		FVector4(802.0f, 123.0f, 156.0f, 153.0f),
		FVector4(992.0f, 123.0f, 150.0f, 153.0f),
		FVector4(1172.0f, 123.0f, 138.0f, 153.0f)
	};
	const FLinearColor SlotColors[] =
	{
		FLinearColor(0.92f, 0.26f, 0.015f),
		FLinearColor(0.68f, 0.075f, 0.012f),
		FLinearColor(0.10f, 0.32f, 0.75f),
		FLinearColor(0.10f, 0.085f, 0.13f),
		FLinearColor(0.42f, 0.025f, 0.018f)
	};
	if (ConnectedCombatHud)
	{
		DrawTexture(
			ConnectedCombatHud,
			HudX,
			HudY,
			HudWidth,
			HudHeight,
			0.0f,
			0.0f,
			1.0f,
			1.0f,
			FLinearColor::White,
			BLEND_Translucent);
	}

	// The delivered aperture pixels are opaque black rather than transparent.
	// Draw content afterward, strictly inside the measured openings, so the
	// authored metalwork stays intact while live values remain visible.
	DrawOrb(HealthCenter, 97.0f * Scale, Character->GetHealthComponent()->GetHealthNormalized(), FLinearColor(0.70f, 0.015f, 0.01f));
	DrawOrb(ResourceCenter, 97.0f * Scale, Character->GetDodgeCooldownNormalized(), FLinearColor(0.025f, 0.24f, 0.78f));
	for (int32 SlotIndex = 0; SlotIndex < UE_ARRAY_COUNT(SlotBounds); ++SlotIndex)
	{
		const FVector4& Slot = SlotBounds[SlotIndex];
		const float Inset = 12.0f * Scale;
		DrawRect(
			SlotColors[SlotIndex],
			HudX + Slot.X * Scale + Inset,
			HudY + Slot.Y * Scale + Inset,
			Slot.Z * Scale - Inset * 2.0f,
			Slot.W * Scale - Inset * 2.0f);
	}

	const TCHAR* AbilityKeys[] = {TEXT("LMB"), TEXT("RMB"), TEXT("SHIFT"), TEXT("Q"), TEXT("3")};
	for (int32 SlotIndex = 0; SlotIndex < UE_ARRAY_COUNT(SlotBounds); ++SlotIndex)
	{
		const FVector4& Slot = SlotBounds[SlotIndex];
		DrawCenteredText(
			AbilityKeys[SlotIndex],
			FLinearColor(0.96f, 0.80f, 0.47f),
			HudX + (Slot.X + Slot.Z * 0.5f) * Scale,
			HudY + 279.0f * Scale,
			GEngine->GetSmallFont(),
			FMath::Clamp(Scale * 1.7f, 0.62f, 0.92f));
	}

	DrawCenteredText(
		FString::Printf(TEXT("%.0f / %.0f"), Character->GetHealthComponent()->GetCurrentHealth(), Character->GetHealthComponent()->GetMaxHealth()),
		FLinearColor::White,
		HealthCenter.X,
		HudY + 292.0f * Scale,
		GEngine->GetSmallFont(),
		FMath::Clamp(Scale * 1.55f, 0.62f, 0.86f));
	DrawCenteredText(
		FString::Printf(TEXT("GOLD  %d"), Character->GetGold()),
		FLinearColor(1.0f, 0.66f, 0.16f),
		Canvas->ClipX * 0.5f,
		HudY + 326.0f * Scale,
		GEngine->GetSmallFont(),
		FMath::Clamp(Scale * 1.55f, 0.62f, 0.86f));
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

void AEmberdeepHUD::DrawPanel(float X, float Y, float Width, float Height)
{
	DrawRect(FLinearColor(0.035f, 0.026f, 0.022f, 0.97f), X - 4.0f, Y - 4.0f, Width + 8.0f, Height + 8.0f);
	DrawRect(FLinearColor(0.42f, 0.29f, 0.15f, 0.96f), X - 2.0f, Y - 2.0f, Width + 4.0f, Height + 4.0f);
	DrawRect(FLinearColor(0.025f, 0.022f, 0.024f, 0.97f), X, Y, Width, Height);
}

void AEmberdeepHUD::DrawCenteredText(
	const FString& Text,
	const FLinearColor& Color,
	float CenterX,
	float Y,
	UFont* Font,
	float Scale)
{
	float TextWidth = 0.0f;
	float TextHeight = 0.0f;
	GetTextSize(Text, TextWidth, TextHeight, Font, Scale);
	DrawText(Text, Color, CenterX - TextWidth * 0.5f, Y, Font, Scale, false);
}

void AEmberdeepHUD::DrawOrb(
	const FVector2D& Center,
	float Radius,
	float NormalizedValue,
	const FLinearColor& FillColor)
{
	const float ClampedValue = FMath::Clamp(NormalizedValue, 0.0f, 1.0f);
	const float FillTop = Center.Y + Radius - Radius * 2.0f * ClampedValue;
	for (int32 OffsetY = -FMath::FloorToInt(Radius); OffsetY <= FMath::FloorToInt(Radius); OffsetY += 2)
	{
		const float HalfWidth = FMath::Sqrt(FMath::Max(0.0f, Radius * Radius - OffsetY * OffsetY));
		const float RowY = Center.Y + OffsetY;
		DrawRect(
			RowY >= FillTop ? FillColor : FLinearColor(0.035f, 0.025f, 0.03f, 0.98f),
			Center.X - HalfWidth,
			RowY,
			HalfWidth * 2.0f,
			2.0f);
	}
}

void AEmberdeepHUD::DrawOrnamentSection(
	float X,
	float Y,
	float Width,
	float Height,
	float SourceX,
	float SourceY,
	float SourceWidth,
	float SourceHeight)
{
	if (!HudOrnaments)
	{
		return;
	}

	DrawTexture(
		HudOrnaments,
		X,
		Y,
		Width,
		Height,
		SourceX / 1536.0f,
		SourceY / 1024.0f,
		SourceWidth / 1536.0f,
		SourceHeight / 1024.0f,
		FLinearColor::White,
		BLEND_Translucent);
}
