#include "UI/EmberdeepHUD.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Gameplay/EmberdeepCharacter.h"
#include "Gameplay/EmberdeepEnemy.h"
#include "Gameplay/EmberdeepGameMode.h"
#include "Gameplay/EmberdeepGameState.h"
#include "Gameplay/EmberdeepHealthComponent.h"
#include "Gameplay/EmberdeepPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	float GetPresentationUiScale(const UCanvas* Canvas)
	{
		return Canvas ? FMath::Clamp(Canvas->ClipY / 1024.0f, 0.80f, 1.45f) : 1.0f;
	}
}

AEmberdeepHUD::AEmberdeepHUD()
{
	// AHUD is created locally for every owning player, including remote clients.
	// Housing the visual baseline here avoids server-only GameMode lighting and
	// guarantees that every view receives the same grade and global fill.
	SetHidden(false);
	USceneComponent* PresentationRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PresentationRoot"));
	PresentationRoot->SetMobility(EComponentMobility::Movable);
	SetRootComponent(PresentationRoot);

	UDirectionalLightComponent* MoonKeyLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("MoonKeyLight"));
	MoonKeyLight->SetupAttachment(PresentationRoot);
	MoonKeyLight->SetRelativeRotation(FRotator(-52.0f, -38.0f, 0.0f));
	MoonKeyLight->SetMobility(EComponentMobility::Movable);
	// Keep only a faint, shadow-casting moon baseline. Local environment lights
	// must remain the dominant source so torches produce readable pools instead
	// of the entire room receiving the same flat value.
	MoonKeyLight->SetIntensity(1.60f);
	MoonKeyLight->SetLightColor(FLinearColor::FromSRGBColor(FColor(121, 127, 137)));
	MoonKeyLight->SetCastShadows(true);
	MoonKeyLight->SetLightSourceAngle(1.5f);

	UPointLightComponent* AmbientFillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("AmbientFillLight"));
	AmbientFillLight->SetupAttachment(PresentationRoot);
	AmbientFillLight->SetRelativeLocation(FVector(0.0f, 0.0f, 900.0f));
	AmbientFillLight->SetMobility(EComponentMobility::Movable);
	AmbientFillLight->SetIntensity(0.0f);
	AmbientFillLight->SetLightColor(FLinearColor::FromSRGBColor(FColor(135, 123, 110)));
	AmbientFillLight->SetAttenuationRadius(3600.0f);
	AmbientFillLight->SetSourceRadius(500.0f);
	AmbientFillLight->SetCastShadows(false);

	UPostProcessComponent* PresentationPostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PresentationPostProcess"));
	PresentationPostProcess->SetupAttachment(PresentationRoot);
	PresentationPostProcess->bUnbound = true;
	PresentationPostProcess->BlendWeight = 1.0f;
	FPostProcessSettings& Settings = PresentationPostProcess->Settings;
	Settings.bOverride_AutoExposureMethod = true;
	Settings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	Settings.AutoExposureApplyPhysicalCameraExposure = 0;
	Settings.bOverride_AutoExposureBias = true;
	Settings.AutoExposureBias = 1.60f;
	Settings.bOverride_ColorSaturation = true;
	Settings.ColorSaturation = FVector4(0.93f, 0.91f, 0.87f, 1.0f);
	Settings.bOverride_ColorContrast = true;
	Settings.ColorContrast = FVector4(1.06f, 1.05f, 1.04f, 1.0f);
	Settings.bOverride_ColorGain = true;
	Settings.ColorGain = FVector4(0.98f, 0.99f, 1.0f, 1.0f);
	Settings.bOverride_ColorOffset = true;
	Settings.ColorOffset = FVector4(0.004f, 0.004f, 0.006f, 0.0f);
	Settings.bOverride_VignetteIntensity = true;
	Settings.VignetteIntensity = 0.28f;
	Settings.bOverride_AmbientOcclusionIntensity = true;
	Settings.AmbientOcclusionIntensity = 0.95f;
	Settings.bOverride_AmbientOcclusionRadius = true;
	Settings.AmbientOcclusionRadius = 95.0f;
	Settings.bOverride_BloomIntensity = true;
	Settings.BloomIntensity = 0.22f;
	Settings.bOverride_BloomThreshold = true;
	Settings.BloomThreshold = 2.8f;
	Settings.bOverride_MotionBlurAmount = true;
	Settings.MotionBlurAmount = 0.0f;
	Settings.bOverride_FilmGrainIntensity = true;
	Settings.FilmGrainIntensity = 0.0f;
	Settings.bOverride_SceneFringeIntensity = true;
	Settings.SceneFringeIntensity = 0.0f;

	static ConstructorHelpers::FObjectFinder<UTexture2D> OrnamentTexture(
		TEXT("/Game/Emberdeep/UI/T_HUDOrnaments.T_HUDOrnaments"));
	HudOrnaments = OrnamentTexture.Object;

	static ConstructorHelpers::FObjectFinder<UTexture2D> ConnectedHudTexture(
		TEXT("/Game/Emberdeep/UI/T_CombatHUD_TargetMatched.T_CombatHUD_TargetMatched"));
	ConnectedCombatHud = ConnectedHudTexture.Object;

	static ConstructorHelpers::FObjectFinder<UTexture2D> PartyRowTexture(
		TEXT("/Game/Emberdeep/UI/T_PartyRow_TargetMatched.T_PartyRow_TargetMatched"));
	PartyRowFrame = PartyRowTexture.Object;

	static ConstructorHelpers::FObjectFinder<UTexture2D> MinimapFrameTexture(
		TEXT("/Game/Emberdeep/UI/T_MinimapFrame_TargetMatched.T_MinimapFrame_TargetMatched"));
	MinimapFrame = MinimapFrameTexture.Object;

	static ConstructorHelpers::FObjectFinder<UTexture2D> AbilityAtlasTexture(
		TEXT("/Game/Emberdeep/UI/T_FighterAbilityAtlas.T_FighterAbilityAtlas"));
	FighterAbilityAtlas = AbilityAtlasTexture.Object;
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

	const float HealthNormalized = Character->GetHealthComponent()->GetHealthNormalized();
	if (HealthNormalized > 0.0f && HealthNormalized < 0.32f)
	{
		const float Severity = 1.0f - HealthNormalized / 0.32f;
		const float Pulse = 0.5f + 0.5f * FMath::Sin(GetWorld()->GetTimeSeconds() * 5.5f);
		const float EdgeDepth = (44.0f + Severity * 52.0f) * GetPresentationUiScale(Canvas);
		for (int32 LayerIndex = 0; LayerIndex < 10; ++LayerIndex)
		{
			const float LayerAlpha = (1.0f - LayerIndex / 10.0f) * (0.026f + Pulse * 0.026f) * Severity;
			const float LayerInset = LayerIndex * EdgeDepth / 10.0f;
			const float LayerThickness = EdgeDepth / 10.0f + 1.0f;
			const FLinearColor WarningColor(0.62f, 0.004f, 0.002f, LayerAlpha);
			DrawRect(WarningColor, LayerInset, LayerInset, Canvas->ClipX - LayerInset * 2.0f, LayerThickness);
			DrawRect(WarningColor, LayerInset, Canvas->ClipY - LayerInset - LayerThickness, Canvas->ClipX - LayerInset * 2.0f, LayerThickness);
			DrawRect(WarningColor, LayerInset, LayerInset, LayerThickness, Canvas->ClipY - LayerInset * 2.0f);
			DrawRect(WarningColor, Canvas->ClipX - LayerInset - LayerThickness, LayerInset, LayerThickness, Canvas->ClipY - LayerInset * 2.0f);
		}
	}

	FString InteractionPrompt;
	if (const AEmberdeepPlayerController* EmberdeepController = Cast<AEmberdeepPlayerController>(PlayerOwner);
		EmberdeepController && EmberdeepController->GetClosestInteractionPrompt(InteractionPrompt))
	{
		const float UiScale = GetPresentationUiScale(Canvas);
		float PromptWidth = 0.0f;
		float PromptHeight = 0.0f;
		const FString Prompt = FString::Printf(TEXT("[F]  %s"), *InteractionPrompt);
		GetTextSize(Prompt, PromptWidth, PromptHeight, GEngine->GetSmallFont(), UiScale);
		const float PromptY = Canvas->ClipY * 0.72f;
		DrawPanel(Canvas->ClipX * 0.5f - PromptWidth * 0.5f - 16.0f * UiScale, PromptY - 8.0f * UiScale, PromptWidth + 32.0f * UiScale, 30.0f * UiScale);
		DrawCenteredText(Prompt, FLinearColor(1.0f, 0.70f, 0.25f), Canvas->ClipX * 0.5f, PromptY, GEngine->GetSmallFont(), UiScale);
	}

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
			const float UiScale = FMath::Min(GetPresentationUiScale(Canvas), 1.25f);
			const float EnemyBarWidth = (Enemy->IsElite() ? 112.0f : 76.0f) * UiScale;
			DrawBar(
				ScreenPosition.X - EnemyBarWidth * 0.5f,
				ScreenPosition.Y,
				EnemyBarWidth,
				(Enemy->IsElite() ? 10.0f : 7.0f) * UiScale,
				Enemy->GetHealthComponent()->GetHealthNormalized(),
				Enemy->IsAttackWindingUp()
					? FLinearColor(1.0f, 0.34f, 0.015f)
					: FLinearColor(0.68f, 0.018f, 0.012f));

			if (Enemy->IsElite())
			{
				DrawCenteredText(TEXT("BONE WARDEN"), FLinearColor(0.92f, 0.55f, 0.18f), ScreenPosition.X, ScreenPosition.Y - 18.0f * UiScale, GEngine->GetSmallFont(), 0.85f * UiScale);
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

	const float UiScale = GetPresentationUiScale(Canvas);
	const float StartX = 18.0f * UiScale;
	const float StartY = 16.0f * UiScale;
	const float FrameWidth = 215.0f * UiScale;
	const float FrameHeight = 58.0f * UiScale;
	const float RowHeight = 62.0f * UiScale;
	const FLinearColor PartyColors[] =
	{
		FLinearColor(0.96f, 0.63f, 0.23f),
		FLinearColor(0.58f, 0.78f, 0.28f),
		FLinearColor(0.25f, 0.62f, 0.95f),
		FLinearColor(0.90f, 0.82f, 0.56f),
		FLinearColor(0.73f, 0.42f, 0.86f)
	};
	int32 VisiblePlayerIndex = 0;
	for (int32 PlayerIndex = 0; PlayerIndex < GameState->PlayerArray.Num() && VisiblePlayerIndex < 5; ++PlayerIndex)
	{
		const APlayerState* PlayerState = GameState->PlayerArray[PlayerIndex];
		const AEmberdeepCharacter* PartyCharacter = PlayerState
			? Cast<AEmberdeepCharacter>(PlayerState->GetPawn())
			: nullptr;
		if (!PartyCharacter)
		{
			continue;
		}

		const float Y = StartY + VisiblePlayerIndex * RowHeight;
		const FLinearColor PlayerColor = PartyColors[VisiblePlayerIndex];

		// Live portrait and health sit behind the authored apertures. The chunky
		// portrait is deliberately a tiny in-game silhouette, not an empty box.
		DrawRect(FLinearColor(0.020f, 0.016f, 0.015f, 0.98f), StartX + 7.0f * UiScale, Y + 7.0f * UiScale, 43.0f * UiScale, 42.0f * UiScale);
		DrawRect(FLinearColor(0.12f, 0.105f, 0.095f), StartX + 13.0f * UiScale, Y + 33.0f * UiScale, 31.0f * UiScale, 12.0f * UiScale);
		DrawRect(FLinearColor(0.31f, 0.30f, 0.29f), StartX + 20.0f * UiScale, Y + 15.0f * UiScale, 18.0f * UiScale, 20.0f * UiScale);
		DrawRect(FLinearColor(0.15f, 0.15f, 0.16f), StartX + 17.0f * UiScale, Y + 12.0f * UiScale, 24.0f * UiScale, 8.0f * UiScale);
		DrawRect(PlayerColor, StartX + 21.0f * UiScale, Y + 25.0f * UiScale, 16.0f * UiScale, 3.0f * UiScale);

		const UEmberdeepHealthComponent* PartyHealth = PartyCharacter->GetHealthComponent();
		DrawBar(
			StartX + 62.0f * UiScale,
			Y + 38.0f * UiScale,
			143.0f * UiScale,
			10.0f * UiScale,
			PartyHealth->GetHealthNormalized(),
			FLinearColor(0.69f, 0.025f, 0.016f));

		if (PartyRowFrame)
		{
			DrawTexture(PartyRowFrame, StartX, Y, FrameWidth, FrameHeight, 0.0f, 0.0f, 1.0f, 1.0f, FLinearColor::White, BLEND_Translucent);
		}

		DrawText(
			FString::Printf(TEXT("P%d  FIGHTER"), VisiblePlayerIndex + 1),
			PlayerColor,
			StartX + 59.0f * UiScale,
			Y + 10.0f * UiScale,
			GEngine->GetSmallFont(),
			0.86f * UiScale,
			false);
		DrawCenteredText(
			FString::Printf(TEXT("%.0f/%.0f"), PartyHealth->GetCurrentHealth(), PartyHealth->GetMaxHealth()),
			FLinearColor::White,
			StartX + 133.5f * UiScale,
			Y + 35.5f * UiScale,
			GEngine->GetSmallFont(),
			0.68f * UiScale);

		++VisiblePlayerIndex;
	}
}

void AEmberdeepHUD::DrawMinimapAndObjective()
{
	const float UiScale = GetPresentationUiScale(Canvas);
	const float MapWidth = 242.0f * UiScale;
	const float MapHeight = 184.0f * UiScale;
	const float MapX = Canvas->ClipX - MapWidth - 18.0f * UiScale;
	const float MapY = 18.0f * UiScale;

	const float InnerX = MapX + 8.0f * UiScale;
	const float InnerY = MapY + 8.0f * UiScale;
	const float InnerWidth = MapWidth - 16.0f * UiScale;
	const float InnerHeight = MapHeight - 16.0f * UiScale;
	DrawRect(FLinearColor(0.012f, 0.013f, 0.015f, 0.98f), InnerX, InnerY, InnerWidth, InnerHeight);

	const AEmberdeepGameState* EmberdeepGameState = GetWorld()->GetGameState<AEmberdeepGameState>();
	const EEmberdeepRunStage Stage = EmberdeepGameState ? EmberdeepGameState->GetRunStage() : EEmberdeepRunStage::Camp;
	const float StageInset = Stage == EEmberdeepRunStage::RewardRoom ? 22.0f * UiScale : 10.0f * UiScale;
	const float FloorX = InnerX + StageInset;
	const float FloorY = InnerY + StageInset * 0.72f;
	const float FloorWidth = InnerWidth - StageInset * 2.0f;
	const float FloorHeight = InnerHeight - StageInset * 1.44f;
	DrawRect(FLinearColor(0.26f, 0.25f, 0.24f, 0.96f), FloorX - 3.0f * UiScale, FloorY - 3.0f * UiScale, FloorWidth + 6.0f * UiScale, FloorHeight + 6.0f * UiScale);
	DrawRect(FLinearColor(0.075f, 0.075f, 0.078f, 0.98f), FloorX, FloorY, FloorWidth, FloorHeight);
	// Subtle room seams read as an authored floor plan instead of a debug slab.
	for (int32 GridIndex = 1; GridIndex < 4; ++GridIndex)
	{
		DrawRect(FLinearColor(0.13f, 0.125f, 0.12f, 0.62f), FloorX + FloorWidth * GridIndex / 4.0f, FloorY, 1.0f * UiScale, FloorHeight);
	}
	for (int32 GridIndex = 1; GridIndex < 3; ++GridIndex)
	{
		DrawRect(FLinearColor(0.13f, 0.125f, 0.12f, 0.62f), FloorX, FloorY + FloorHeight * GridIndex / 3.0f, FloorWidth, 1.0f * UiScale);
	}
	if (Stage == EEmberdeepRunStage::Dungeon)
	{
		const float SealSize = 16.0f * UiScale;
		DrawRect(FLinearColor(0.34f, 0.12f, 0.09f, 0.75f), FloorX + FloorWidth * 0.5f - SealSize * 0.5f, FloorY + FloorHeight * 0.5f - 2.0f * UiScale, SealSize, 4.0f * UiScale);
		DrawRect(FLinearColor(0.24f, 0.09f, 0.31f, 0.75f), FloorX + FloorWidth * 0.5f - 2.0f * UiScale, FloorY + FloorHeight * 0.5f - SealSize * 0.5f, 4.0f * UiScale, SealSize);
	}

	const float WorldHalfWidth = Stage == EEmberdeepRunStage::RewardRoom ? 600.0f : 900.0f;
	const float WorldHalfHeight = Stage == EEmberdeepRunStage::RewardRoom ? 450.0f : 700.0f;
	const auto WorldToMap = [FloorX, FloorY, FloorWidth, FloorHeight, WorldHalfWidth, WorldHalfHeight](const FVector& WorldLocation)
	{
		return FVector2D(
			FloorX + FMath::Clamp((WorldLocation.X + WorldHalfWidth) / (WorldHalfWidth * 2.0f), 0.0f, 1.0f) * FloorWidth,
			FloorY + FMath::Clamp((WorldLocation.Y + WorldHalfHeight) / (WorldHalfHeight * 2.0f), 0.0f, 1.0f) * FloorHeight);
	};

	if (const AGameStateBase* GameState = GetWorld()->GetGameState<AGameStateBase>())
	{
		for (const APlayerState* PlayerState : GameState->PlayerArray)
		{
			if (const APawn* Pawn = PlayerState ? PlayerState->GetPawn() : nullptr)
			{
				const FVector2D Marker = WorldToMap(Pawn->GetActorLocation());
				const float MarkerSize = 7.0f * UiScale;
				DrawRect(FLinearColor(0.02f, 0.24f, 0.36f), Marker.X - MarkerSize * 0.65f, Marker.Y - MarkerSize * 0.65f, MarkerSize * 1.3f, MarkerSize * 1.3f);
				DrawRect(FLinearColor(0.10f, 0.72f, 1.0f), Marker.X - MarkerSize * 0.5f, Marker.Y - MarkerSize * 0.5f, MarkerSize, MarkerSize);
			}
		}
	}

	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(this, AEmberdeepEnemy::StaticClass(), Enemies);
	for (const AActor* EnemyActor : Enemies)
	{
		const AEmberdeepEnemy* Enemy = Cast<AEmberdeepEnemy>(EnemyActor);
		if (!Enemy || Enemy->GetHealthComponent()->IsDead())
		{
			continue;
		}
		const FVector2D Marker = WorldToMap(Enemy->GetActorLocation());
		const float MarkerSize = Enemy->IsElite() ? 6.0f * UiScale : 4.0f * UiScale;
		DrawRect(FLinearColor(0.78f, 0.035f, 0.018f), Marker.X - MarkerSize * 0.5f, Marker.Y - MarkerSize * 0.5f, MarkerSize, MarkerSize);
	}

	if (MinimapFrame)
	{
		DrawTexture(MinimapFrame, MapX, MapY, MapWidth, MapHeight, 0.0f, 0.0f, 1.0f, 1.0f, FLinearColor::White, BLEND_Translucent);
	}

	FString ObjectiveTitle;
	FString ObjectiveText;
	FString StatusText;
	bool bObjectiveComplete = false;
	if (Stage == EEmberdeepRunStage::Camp)
	{
		ObjectiveTitle = TEXT("DESCEND INTO EMBERDEEP");
		ObjectiveText = TEXT("Enter the Ashen Crypt");
		StatusText = TEXT("F: portal   I: inventory");
	}
	else if (Stage == EEmberdeepRunStage::RewardRoom)
	{
		ObjectiveTitle = TEXT("THE CINDER VAULT");
		ObjectiveText = TEXT("Claim your legendary reward");
		StatusText = TEXT("Return to camp when ready");
		bObjectiveComplete = true;
	}
	else
	{
		ObjectiveTitle = FString::Printf(TEXT("ASHEN CRYPT  -  TIER %d"), EmberdeepGameState ? EmberdeepGameState->GetRunTier() : 1);
		ObjectiveText = TEXT("Defeat the crypt guardians");
		bObjectiveComplete = EmberdeepGameState && EmberdeepGameState->IsEncounterComplete();
		StatusText = bObjectiveComplete ? TEXT("Cinder Vault unlocked") : FString::Printf(TEXT("Enemies remaining: %d"), EmberdeepGameState ? EmberdeepGameState->GetRemainingEnemies() : 0);
	}

	const float QuestY = MapY + MapHeight + 14.0f * UiScale;
	DrawRect(FLinearColor(0.42f, 0.30f, 0.16f, 0.88f), MapX - 1.0f * UiScale, QuestY - 7.0f * UiScale, MapWidth + 2.0f * UiScale, 82.0f * UiScale);
	DrawRect(FLinearColor(0.008f, 0.008f, 0.009f, 0.86f), MapX + 1.0f * UiScale, QuestY - 5.0f * UiScale, MapWidth - 2.0f * UiScale, 78.0f * UiScale);
	DrawText(ObjectiveTitle, FLinearColor(0.96f, 0.63f, 0.22f), MapX, QuestY, GEngine->GetSmallFont(), 1.0f * UiScale, false);
	DrawText(ObjectiveText, FLinearColor(0.86f, 0.84f, 0.78f), MapX, QuestY + 23.0f * UiScale, GEngine->GetSmallFont(), 0.88f * UiScale, false);
	const float CheckX = MapX;
	const float CheckY = QuestY + 51.0f * UiScale;
	const float CheckSize = 13.0f * UiScale;
	DrawRect(FLinearColor(0.54f, 0.42f, 0.24f), CheckX, CheckY, CheckSize, CheckSize);
	DrawRect(FLinearColor(0.035f, 0.032f, 0.029f), CheckX + 2.0f * UiScale, CheckY + 2.0f * UiScale, CheckSize - 4.0f * UiScale, CheckSize - 4.0f * UiScale);
	if (bObjectiveComplete)
	{
		DrawRect(FLinearColor(0.96f, 0.66f, 0.18f), CheckX + 3.0f * UiScale, CheckY + 6.0f * UiScale, 4.0f * UiScale, 3.0f * UiScale);
		DrawRect(FLinearColor(0.96f, 0.66f, 0.18f), CheckX + 6.0f * UiScale, CheckY + 4.0f * UiScale, 5.0f * UiScale, 3.0f * UiScale);
	}
	DrawText(
		StatusText,
		FLinearColor(0.78f, 0.76f, 0.70f),
		MapX + 21.0f * UiScale,
		QuestY + 49.0f * UiScale,
		GEngine->GetSmallFont(),
		0.82f * UiScale,
		false);
}

void AEmberdeepHUD::DrawActionBar(const AEmberdeepCharacter* Character)
{
	constexpr float DesignWidth = 2157.0f;
	constexpr float DesignHeight = 389.0f;
	// The reference occupies about 58% of a 1536x1024 frame. Deriving width
	// primarily from viewport height keeps that same physical weight at 720p,
	// 1080p and 1440p while the width clamps protect narrow and ultrawide views.
	const float MaximumWidth = Canvas->ClipX * 0.63f;
	const float MinimumWidth = FMath::Min(520.0f, MaximumWidth);
	const float TargetWidth = FMath::Clamp(Canvas->ClipY * 0.88f, MinimumWidth, MaximumWidth);
	const float Scale = TargetWidth / DesignWidth;
	const float HudWidth = DesignWidth * Scale;
	const float HudHeight = DesignHeight * Scale;
	const float HudX = (Canvas->ClipX - HudWidth) * 0.5f;
	// Keep the authored feet and lower bevel inside the viewport instead of
	// landing their antialiased edge exactly on the final scanline. The whole
	// connected assembly moves together so apertures and runtime layers remain
	// registered at every supported resolution.
	const float BottomSafePadding = FMath::Clamp(Canvas->ClipY * 0.014f, 10.0f, 18.0f);
	const float HudY = Canvas->ClipY - HudHeight - BottomSafePadding;
	const auto DesignPoint = [HudX, HudY, Scale](float X, float Y)
	{
		return FVector2D(HudX + X * Scale, HudY + Y * Scale);
	};

	// All live layers use the exact native coordinates of the new project-bound
	// texture. The authored frame is drawn last so its rims mask every fill.
	const FVector2D HealthCenter = DesignPoint(267.0f, 176.0f);
	const FVector2D ResourceCenter = DesignPoint(1888.0f, 176.0f);

	const FVector4 SlotBounds[] =
	{
		FVector4(560.0f, 132.0f, 202.0f, 156.0f),
		FVector4(817.0f, 132.0f, 188.0f, 156.0f),
		FVector4(1057.0f, 133.0f, 189.0f, 155.0f),
		FVector4(1297.0f, 132.0f, 187.0f, 156.0f),
		FVector4(1536.0f, 133.0f, 72.0f, 155.0f)
	};
	const FLinearColor SlotColors[] =
	{
		FLinearColor(0.92f, 0.26f, 0.015f),
		FLinearColor(0.68f, 0.075f, 0.012f),
		FLinearColor(0.10f, 0.32f, 0.75f),
		FLinearColor(0.10f, 0.085f, 0.13f),
		FLinearColor(0.42f, 0.025f, 0.018f)
	};
	DrawOrb(HealthCenter, 124.0f * Scale, Character->GetHealthComponent()->GetHealthNormalized(), FLinearColor(0.70f, 0.015f, 0.008f));
	DrawOrb(ResourceCenter, 124.0f * Scale, Character->GetDodgeCooldownNormalized(), FLinearColor(0.018f, 0.20f, 0.74f));

	// This recessed channel is permanently reserved for experience progress.
	const float ExperienceTrackX = HudX + 509.0f * Scale;
	const float ExperienceTrackY = HudY + 344.0f * Scale;
	const float ExperienceTrackWidth = 1138.0f * Scale;
	const float ExperienceTrackHeight = 14.0f * Scale;
	DrawRect(FLinearColor(0.055f, 0.035f, 0.012f, 0.98f), ExperienceTrackX, ExperienceTrackY, ExperienceTrackWidth, ExperienceTrackHeight);
	DrawRect(
		FLinearColor(0.95f, 0.50f, 0.045f, 0.98f),
		ExperienceTrackX,
		ExperienceTrackY,
		ExperienceTrackWidth * Character->GetExperienceNormalized(),
		ExperienceTrackHeight);
	for (int32 SlotIndex = 0; SlotIndex < UE_ARRAY_COUNT(SlotBounds); ++SlotIndex)
	{
		const FVector4& Slot = SlotBounds[SlotIndex];
		const float Inset = 5.0f * Scale;
		const float IconX = HudX + Slot.X * Scale + Inset;
		const float IconY = HudY + Slot.Y * Scale + Inset;
		const float IconWidth = Slot.Z * Scale - Inset * 2.0f;
		const float IconHeight = Slot.W * Scale - Inset * 2.0f;
		if (FighterAbilityAtlas)
		{
			DrawTexture(
				FighterAbilityAtlas,
				IconX,
				IconY,
				IconWidth,
				IconHeight,
				(35.0f + SlotIndex * 390.0f) / 1983.0f,
				200.0f / 793.0f,
				351.0f / 1983.0f,
				378.0f / 793.0f,
				FLinearColor::White,
				BLEND_Translucent);
		}
		else
		{
			DrawRect(SlotColors[SlotIndex], IconX, IconY, IconWidth, IconHeight);
		}

		if (SlotIndex == 2)
		{
			const float CooldownRemaining = 1.0f - Character->GetDodgeCooldownNormalized();
			DrawRect(
				FLinearColor(0.0f, 0.0f, 0.0f, 0.72f),
				IconX,
				IconY,
				IconWidth,
				IconHeight * CooldownRemaining);
		}
		else if (SlotIndex == 0)
		{
			const float CooldownRemaining = 1.0f - Character->GetBasicAttackCooldownNormalized();
			DrawRadialCooldown(
				FVector2D(IconX + IconWidth * 0.5f, IconY + IconHeight * 0.5f),
				FVector2D(IconWidth * 0.5f, IconHeight * 0.5f),
				CooldownRemaining);

			const float Feedback = Character->GetBasicAttackFeedbackNormalized();
			const bool bQueued = Character->IsBasicAttackQueued() && CooldownRemaining > 0.0f;
			if (Feedback > 0.0f || bQueued)
			{
				const float Border = FMath::Max(2.0f, 7.0f * Scale);
				const FLinearColor FlashColor(
					1.0f,
					0.62f,
					0.12f,
					bQueued ? FMath::Max(0.72f, Feedback) : 0.35f + Feedback * 0.65f);
				DrawRect(FlashColor, IconX, IconY, IconWidth, Border);
				DrawRect(FlashColor, IconX, IconY + IconHeight - Border, IconWidth, Border);
				DrawRect(FlashColor, IconX, IconY, Border, IconHeight);
				DrawRect(FlashColor, IconX + IconWidth - Border, IconY, Border, IconHeight);
			}
		}
	}

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

	const TCHAR* AbilityKeys[] = {TEXT("LMB"), TEXT("RMB"), TEXT("SHIFT"), TEXT("Q"), TEXT("3")};
	for (int32 SlotIndex = 0; SlotIndex < UE_ARRAY_COUNT(SlotBounds); ++SlotIndex)
	{
		const FVector4& Slot = SlotBounds[SlotIndex];
		const float KeyCenterX = HudX + (Slot.X + Slot.Z * 0.5f) * Scale;
		const float KeyY = HudY + 264.0f * Scale;
		const float BadgeWidth = FMath::Min(54.0f, Slot.Z - 10.0f) * Scale;
		DrawRect(
			FLinearColor(0.015f, 0.012f, 0.012f, 0.82f),
			KeyCenterX - BadgeWidth * 0.5f,
			KeyY - 2.0f * Scale,
			BadgeWidth,
			20.0f * Scale);
		DrawCenteredText(
			AbilityKeys[SlotIndex],
			FLinearColor(0.91f, 0.78f, 0.55f),
			KeyCenterX,
			KeyY,
			GEngine->GetSmallFont(),
			FMath::Clamp(Scale * 2.0f, 0.58f, 0.90f));
	}
	DrawCenteredText(
		FString::Printf(TEXT("%.0f / %.0f"), Character->GetHealthComponent()->GetCurrentHealth(), Character->GetHealthComponent()->GetMaxHealth()),
		FLinearColor::White,
		HealthCenter.X,
		HudY + 323.0f * Scale,
		GEngine->GetSmallFont(),
		FMath::Clamp(Scale * 1.9f, 0.58f, 0.84f));
	DrawCenteredText(
		TEXT("100 / 100"),
		FLinearColor::White,
		ResourceCenter.X,
		HudY + 323.0f * Scale,
		GEngine->GetSmallFont(),
		FMath::Clamp(Scale * 1.9f, 0.58f, 0.84f));
	DrawCenteredText(
		FString::Printf(TEXT("LEVEL %d          GOLD %d"),
			Character->GetCharacterLevel(), Character->GetGold()),
		FLinearColor(1.0f, 0.66f, 0.16f),
		Canvas->ClipX * 0.5f,
		HudY + 309.0f * Scale,
		GEngine->GetSmallFont(),
		FMath::Clamp(Scale * 1.75f, 0.56f, 0.82f));
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
		const float EdgeAlpha = FMath::Clamp(HalfWidth / FMath::Max(Radius, 1.0f), 0.0f, 1.0f);
		const float VerticalAlpha = FMath::Clamp((OffsetY + Radius) / FMath::Max(Radius * 2.0f, 1.0f), 0.0f, 1.0f);
		FLinearColor LiquidColor = FillColor * (0.52f + VerticalAlpha * 0.58f);
		LiquidColor = FMath::Lerp(LiquidColor * 0.58f, LiquidColor, EdgeAlpha);
		LiquidColor.A = 1.0f;
		DrawRect(
			RowY >= FillTop ? LiquidColor : FLinearColor(0.025f, 0.018f, 0.024f, 0.99f),
			Center.X - HalfWidth,
			RowY,
			HalfWidth * 2.0f,
			2.0f);
	}
	if (ClampedValue > 0.02f)
	{
		const float SurfaceHalfWidth = FMath::Sqrt(FMath::Max(0.0f, Radius * Radius - FMath::Square(FillTop - Center.Y)));
		DrawRect(
			FLinearColor(1.0f, 0.70f, 0.44f, 0.28f),
			Center.X - SurfaceHalfWidth * 0.72f,
			FillTop,
			SurfaceHalfWidth * 1.44f,
			FMath::Max(2.0f, Radius * 0.025f));
	}
	// Small stepped specular glint keeps the liquid dimensional without smooth
	// gradients that fight the low-resolution presentation.
	const FVector2D GlintCenter = Center + FVector2D(-Radius * 0.28f, -Radius * 0.30f);
	for (int32 GlintRow = 0; GlintRow < 4; ++GlintRow)
	{
		const float GlintY = GlintCenter.Y + GlintRow * 2.0f;
		if (GlintY >= FillTop)
		{
			DrawRect(FLinearColor(1.0f, 0.92f, 0.82f, 0.10f + GlintRow * 0.025f), GlintCenter.X - (4 - GlintRow) * 2.0f, GlintY, (8 - GlintRow * 2.0f) * 2.0f, 2.0f);
		}
	}
}

void AEmberdeepHUD::DrawRadialCooldown(
	const FVector2D& Center,
	const FVector2D& HalfSize,
	float NormalizedRemaining)
{
	FTexture* CooldownTexture = Canvas && Canvas->DefaultTexture
		? Canvas->DefaultTexture->GetResource()
		: nullptr;
	if (!CooldownTexture)
	{
		return;
	}

	const float Sweep = FMath::Clamp(NormalizedRemaining, 0.0f, 1.0f) * UE_TWO_PI;
	if (Sweep <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	static constexpr int32 SegmentCount = 32;
	static constexpr float StartAngle = -UE_HALF_PI;
	static constexpr float SegmentAngle = UE_TWO_PI / static_cast<float>(SegmentCount);
	const auto RectangleEdgePoint = [&Center, &HalfSize](float Angle)
	{
		const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
		const float HorizontalDistance = FMath::Abs(Direction.X) > KINDA_SMALL_NUMBER
			? HalfSize.X / FMath::Abs(Direction.X)
			: MAX_flt;
		const float VerticalDistance = FMath::Abs(Direction.Y) > KINDA_SMALL_NUMBER
			? HalfSize.Y / FMath::Abs(Direction.Y)
			: MAX_flt;
		return Center + Direction * FMath::Min(HorizontalDistance, VerticalDistance);
	};

	for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
	{
		const float SegmentStart = SegmentIndex * SegmentAngle;
		if (SegmentStart >= Sweep)
		{
			break;
		}

		const float SegmentEnd = FMath::Min(SegmentStart + SegmentAngle, Sweep);
		FCanvasTriangleItem Triangle(
			Center,
			RectangleEdgePoint(StartAngle + SegmentStart),
			RectangleEdgePoint(StartAngle + SegmentEnd),
			CooldownTexture);
		Triangle.SetColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.72f));
		Triangle.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Triangle);
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
