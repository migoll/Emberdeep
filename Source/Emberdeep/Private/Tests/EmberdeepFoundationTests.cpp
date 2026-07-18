#if WITH_DEV_AUTOMATION_TESTS

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Environment/EmberdeepCampEnvironment.h"
#include "Environment/EmberdeepDungeonEnvironment.h"
#include "Environment/EmberdeepRewardRoomEnvironment.h"
#include "Misc/AutomationTest.h"
#include "Gameplay/EmberdeepCharacter.h"
#include "Gameplay/EmberdeepCombatFeedback.h"
#include "Gameplay/EmberdeepEnemy.h"
#include "Gameplay/EmberdeepGameMode.h"
#include "Gameplay/EmberdeepGameState.h"
#include "Gameplay/EmberdeepGoldPickup.h"
#include "Gameplay/EmberdeepHealthComponent.h"
#include "Gameplay/EmberdeepItemTypes.h"
#include "Gameplay/EmberdeepLootContainer.h"
#include "Gameplay/EmberdeepPlayerController.h"
#include "Gameplay/EmberdeepPlayerState.h"
#include "Gameplay/EmberdeepPortal.h"
#include "UI/EmberdeepHUD.h"
#include "UI/EmberdeepInventoryWidget.h"
#include "UI/EmberdeepLootWidget.h"
#include "UI/EmberdeepMainMenuWidget.h"
#include "Visual/EmberdeepVoxelStyle.h"

namespace
{
	bool ValidateFixedVoxelMeshes(
		FAutomationTestBase& Test,
		const TCHAR* AssetName,
		const TArray<UInstancedStaticMeshComponent*>& Meshes)
	{
		bool bValid = true;
		const FVector ExpectedScale = EmberdeepVoxelStyle::InstanceScale();
		for (const UInstancedStaticMeshComponent* Mesh : Meshes)
		{
			for (int32 InstanceIndex = 0; InstanceIndex < Mesh->GetInstanceCount(); ++InstanceIndex)
			{
				FTransform Transform;
				if (!Mesh->GetInstanceTransform(InstanceIndex, Transform, false))
				{
					Test.AddError(FString::Printf(TEXT("%s has an unreadable voxel instance"), AssetName));
					bValid = false;
					continue;
				}

				if (!Transform.GetScale3D().Equals(ExpectedScale, KINDA_SMALL_NUMBER))
				{
					Test.AddError(FString::Printf(TEXT("%s contains a stretched voxel"), AssetName));
					return false;
				}

				const FVector Location = Transform.GetLocation();
				for (const float Coordinate : {Location.X, Location.Y, Location.Z})
				{
					const float CellCoordinate = Coordinate / EmberdeepVoxelStyle::UnitCm - 0.5f;
					if (!FMath::IsNearlyEqual(CellCoordinate, FMath::RoundToFloat(CellCoordinate), KINDA_SMALL_NUMBER))
					{
						Test.AddError(FString::Printf(TEXT("%s contains an off-grid voxel"), AssetName));
						return false;
					}
				}
			}
		}
		return bValid;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEmberdeepFoundationClassesTest,
	"Emberdeep.Foundation.GameplayClassesExist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEmberdeepFoundationClassesTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("Solid voxels must meet face-to-face so subpixel gaps cannot shimmer"),
		EmberdeepVoxelStyle::RenderFill,
		1.0f);
	TestNotNull(TEXT("The project game mode must exist"), AEmberdeepGameMode::StaticClass());
	TestNotNull(TEXT("Replicated encounter game state must exist"), AEmberdeepGameState::StaticClass());
	TestNotNull(TEXT("The project character must exist"), AEmberdeepCharacter::StaticClass());
	TestNotNull(TEXT("The skeleton enemy must exist"), AEmberdeepEnemy::StaticClass());
	TestNotNull(TEXT("Client-only combat feedback must exist"), AEmberdeepCombatFeedback::StaticClass());
	TestNotNull(TEXT("The gold pickup must exist"), AEmberdeepGoldPickup::StaticClass());
	TestNotNull(TEXT("The combat HUD must exist"), AEmberdeepHUD::StaticClass());
	TestNotNull(TEXT("The Broken Caravan Camp environment must exist"), AEmberdeepCampEnvironment::StaticClass());
	TestNotNull(TEXT("The Host/Join menu must exist"), UEmberdeepMainMenuWidget::StaticClass());
	TestNotNull(TEXT("The Ashen Crypt environment must exist"), AEmberdeepDungeonEnvironment::StaticClass());
	TestNotNull(TEXT("The Cinder Vault environment must exist"), AEmberdeepRewardRoomEnvironment::StaticClass());
	TestNotNull(TEXT("The portal interaction must exist"), AEmberdeepPortal::StaticClass());
	TestNotNull(TEXT("Shared loot containers must exist"), AEmberdeepLootContainer::StaticClass());
	TestNotNull(TEXT("Run-persistent player state must exist"), AEmberdeepPlayerState::StaticClass());
	TestNotNull(TEXT("The loot window must exist"), UEmberdeepLootWidget::StaticClass());
	TestNotNull(TEXT("The inventory window must exist"), UEmberdeepInventoryWidget::StaticClass());
	TestTrue(
		TEXT("The GameMode must use run-persistent PlayerState"),
		AEmberdeepGameMode::StaticClass()->GetDefaultObject<AEmberdeepGameMode>()->PlayerStateClass ==
			AEmberdeepPlayerState::StaticClass());
	TestNotNull(
		TEXT("Interaction must enter through an owned server RPC"),
		AEmberdeepPlayerController::StaticClass()->FindFunctionByName(TEXT("ServerInteract")));
	TestNotNull(
		TEXT("Loot claims must enter through an owned server RPC"),
		AEmberdeepPlayerController::StaticClass()->FindFunctionByName(TEXT("ServerTakeLoot")));
	TestNotNull(
		TEXT("Equipment changes must enter through an owned server RPC"),
		AEmberdeepPlayerController::StaticClass()->FindFunctionByName(TEXT("ServerEquipItem")));
	TestNotNull(
		TEXT("Attacks must enter through a server RPC"),
		AEmberdeepCharacter::StaticClass()->FindFunctionByName(TEXT("ServerPerformAttack")));
	TestNotNull(
		TEXT("Aim direction must be reported to the server"),
		AEmberdeepCharacter::StaticClass()->FindFunctionByName(TEXT("ServerSetAimDirection")));
	TestNotNull(
		TEXT("Dodge must enter through a server RPC"),
		AEmberdeepCharacter::StaticClass()->FindFunctionByName(TEXT("ServerDodge")));
	TestNotNull(
		TEXT("Attack presentation must multicast to the party"),
		AEmberdeepCharacter::StaticClass()->FindFunctionByName(TEXT("MulticastPlayAttackVisual")));
	TestNotNull(
		TEXT("Dodge presentation must multicast to the party"),
		AEmberdeepCharacter::StaticClass()->FindFunctionByName(TEXT("MulticastPlayDodgeVisual")));
	TestNotNull(
		TEXT("Player damage presentation must multicast to the party"),
		AEmberdeepCharacter::StaticClass()->FindFunctionByName(TEXT("MulticastPlayHurtVisual")));
	TestNotNull(
		TEXT("Enemy hit presentation must multicast to the party"),
		AEmberdeepEnemy::StaticClass()->FindFunctionByName(TEXT("MulticastPlayHitFeedback")));
	TestNotNull(
		TEXT("Enemy attack warnings must multicast to the party"),
		AEmberdeepEnemy::StaticClass()->FindFunctionByName(TEXT("MulticastSetAttackTelegraph")));

	const AEmberdeepCharacter* CharacterDefault = AEmberdeepCharacter::StaticClass()->GetDefaultObject<AEmberdeepCharacter>();
	TestEqual(TEXT("Characters begin at level one"), CharacterDefault->GetCharacterLevel(), 1);
	TestEqual(TEXT("Characters begin with an empty XP bar"), CharacterDefault->GetExperienceNormalized(), 0.0f);
	TArray<UInstancedStaticMeshComponent*> ThorgrimPaletteMeshes;
	CharacterDefault->GetComponents<UInstancedStaticMeshComponent>(ThorgrimPaletteMeshes);
	TestEqual(TEXT("Thorgrim must provide three shades for each part and palette"), ThorgrimPaletteMeshes.Num(), 81);

	int32 ThorgrimInstanceCount = 0;
	for (const UInstancedStaticMeshComponent* PaletteMesh : ThorgrimPaletteMeshes)
	{
		ThorgrimInstanceCount += PaletteMesh->GetInstanceCount();
	}
	TestEqual(TEXT("Thorgrim must retain the agreed fixed-grid voxel density"), ThorgrimInstanceCount, 8746);
	TestTrue(
		TEXT("Every Thorgrim cell must use the shared 4 cm lattice"),
		ValidateFixedVoxelMeshes(*this, TEXT("Thorgrim"), ThorgrimPaletteMeshes));

	const AEmberdeepCampEnvironment* CampDefault =
		AEmberdeepCampEnvironment::StaticClass()->GetDefaultObject<AEmberdeepCampEnvironment>();
	TestEqual(TEXT("The camp must provide three shades for each restricted palette color"), CampDefault->GetPaletteMeshCount(), 39);
	TestEqual(TEXT("The camp must retain its fixed-grid environment dressing"), CampDefault->GetVoxelInstanceCount(), 256394);
	TestEqual(
		TEXT("The camp must provide floor, perimeter, and prop collision proxies"),
		CampDefault->GetCollisionProxyCount(),
		10);

	TArray<UPointLightComponent*> CampLights;
	CampDefault->GetComponents<UPointLightComponent>(CampLights);
	TestEqual(TEXT("The camp must retain its fire and three lantern light pools"), CampLights.Num(), 4);

	TArray<UInstancedStaticMeshComponent*> CampPaletteMeshes;
	CampDefault->GetComponents<UInstancedStaticMeshComponent>(CampPaletteMeshes);
	TestTrue(
		TEXT("Every camp cell must use the shared 4 cm lattice"),
		ValidateFixedVoxelMeshes(*this, TEXT("Broken Caravan Camp"), CampPaletteMeshes));
	for (const UInstancedStaticMeshComponent* PaletteMesh : CampPaletteMeshes)
	{
		TestTrue(
			TEXT("Decorative camp voxels must not create per-instance collision"),
			PaletteMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	}

	TArray<UBoxComponent*> CampCollisionProxies;
	CampDefault->GetComponents<UBoxComponent>(CampCollisionProxies);
	for (const UBoxComponent* CollisionProxy : CampCollisionProxies)
	{
		TestEqual(
			TEXT("Camp collision proxies must provide physics collision"),
			CollisionProxy->GetCollisionEnabled(),
			ECollisionEnabled::QueryAndPhysics);
		TestEqual(
			TEXT("Camp collision proxies must block pawns"),
			CollisionProxy->GetCollisionResponseToChannel(ECC_Pawn),
			ECR_Block);
	}

	const AEmberdeepEnemy* EnemyDefault = AEmberdeepEnemy::StaticClass()->GetDefaultObject<AEmberdeepEnemy>();
	TArray<UInstancedStaticMeshComponent*> EnemyVoxelMeshes;
	EnemyDefault->GetComponents<UInstancedStaticMeshComponent>(EnemyVoxelMeshes);
	TestEqual(TEXT("The skeleton must use three fixed-grid shade batches"), EnemyVoxelMeshes.Num(), 3);
	TestTrue(
		TEXT("Every skeleton cell must use the shared 4 cm lattice"),
		ValidateFixedVoxelMeshes(*this, TEXT("Skeleton"), EnemyVoxelMeshes));

	const AEmberdeepGoldPickup* GoldDefault = AEmberdeepGoldPickup::StaticClass()->GetDefaultObject<AEmberdeepGoldPickup>();
	TArray<UInstancedStaticMeshComponent*> GoldVoxelMeshes;
	GoldDefault->GetComponents<UInstancedStaticMeshComponent>(GoldVoxelMeshes);
	TestEqual(TEXT("Gold pickups must use three fixed-grid shade batches"), GoldVoxelMeshes.Num(), 3);
	TestTrue(
		TEXT("Every gold cell must use the shared 4 cm lattice"),
		ValidateFixedVoxelMeshes(*this, TEXT("Gold pickup"), GoldVoxelMeshes));

	const AEmberdeepDungeonEnvironment* DungeonDefault =
		AEmberdeepDungeonEnvironment::StaticClass()->GetDefaultObject<AEmberdeepDungeonEnvironment>();
	TestEqual(TEXT("The dungeon uses a restricted palette"), DungeonDefault->GetPaletteMeshCount(), 8);
	TestTrue(TEXT("The dungeon retains readable dressing"), DungeonDefault->GetVisualBlockCount() >= 400);
	TestEqual(TEXT("The dungeon uses only floor/perimeter collision"), DungeonDefault->GetCollisionProxyCount(), 5);

	const AEmberdeepRewardRoomEnvironment* RewardDefault =
		AEmberdeepRewardRoomEnvironment::StaticClass()->GetDefaultObject<AEmberdeepRewardRoomEnvironment>();
	TestEqual(TEXT("The reward room uses a restricted palette"), RewardDefault->GetPaletteMeshCount(), 9);
	TestTrue(TEXT("The reward room retains readable dressing"), RewardDefault->GetVisualBlockCount() >= 200);
	TestEqual(TEXT("The reward room uses only floor/perimeter collision"), RewardDefault->GetCollisionProxyCount(), 5);

	const AEmberdeepPlayerState* RunStateDefault =
		AEmberdeepPlayerState::StaticClass()->GetDefaultObject<AEmberdeepPlayerState>();
	TestEqual(TEXT("The bounded prototype inventory has twelve slots"), RunStateDefault->GetInventoryCapacity(), 12);
	const FEmberdeepItemInstance Legendary = FEmberdeepItemCatalog::MakeLegendaryReward(99, 3);
	TestTrue(TEXT("The final chest reward is a valid item"), Legendary.IsValid());
	TestEqual(TEXT("The final chest reward is legendary"), Legendary.Rarity, EEmberdeepItemRarity::Legendary);
	TestTrue(TEXT("The legendary reward materially increases damage"), Legendary.DamageBonus >= 30.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEmberdeepHealthComponentTest,
	"Emberdeep.Foundation.HealthComponentDamageAndDeath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEmberdeepHealthComponentTest::RunTest(const FString& Parameters)
{
	UEmberdeepHealthComponent* Health = NewObject<UEmberdeepHealthComponent>();
	Health->SetMaxHealth(100.0f);
	bool bDeathBroadcast = false;
	Health->OnDeath.AddLambda([&bDeathBroadcast]() { bDeathBroadcast = true; });

	TestEqual(TEXT("Damage is applied"), Health->ApplyDamage(35.0f), 35.0f);
	TestEqual(TEXT("Health is reduced"), Health->GetCurrentHealth(), 65.0f);
	TestEqual(TEXT("Overkill damage clamps to remaining health"), Health->ApplyDamage(100.0f), 65.0f);
	TestTrue(TEXT("Component reports death"), Health->IsDead());
	TestTrue(TEXT("Death event is broadcast"), bDeathBroadcast);
	return true;
}

#endif
