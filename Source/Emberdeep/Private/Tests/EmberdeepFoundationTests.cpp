#if WITH_DEV_AUTOMATION_TESTS

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Environment/EmberdeepCampEnvironment.h"
#include "Misc/AutomationTest.h"
#include "Gameplay/EmberdeepCharacter.h"
#include "Gameplay/EmberdeepEnemy.h"
#include "Gameplay/EmberdeepGameMode.h"
#include "Gameplay/EmberdeepGameState.h"
#include "Gameplay/EmberdeepGoldPickup.h"
#include "Gameplay/EmberdeepHealthComponent.h"
#include "UI/EmberdeepHUD.h"
#include "UI/EmberdeepMainMenuWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEmberdeepFoundationClassesTest,
	"Emberdeep.Foundation.GameplayClassesExist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEmberdeepFoundationClassesTest::RunTest(const FString& Parameters)
{
	TestNotNull(TEXT("The project game mode must exist"), AEmberdeepGameMode::StaticClass());
	TestNotNull(TEXT("Replicated encounter game state must exist"), AEmberdeepGameState::StaticClass());
	TestNotNull(TEXT("The project character must exist"), AEmberdeepCharacter::StaticClass());
	TestNotNull(TEXT("The skeleton enemy must exist"), AEmberdeepEnemy::StaticClass());
	TestNotNull(TEXT("The gold pickup must exist"), AEmberdeepGoldPickup::StaticClass());
	TestNotNull(TEXT("The combat HUD must exist"), AEmberdeepHUD::StaticClass());
	TestNotNull(TEXT("The Broken Caravan Camp environment must exist"), AEmberdeepCampEnvironment::StaticClass());
	TestNotNull(TEXT("The Host/Join menu must exist"), UEmberdeepMainMenuWidget::StaticClass());
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

	const AEmberdeepCharacter* CharacterDefault = AEmberdeepCharacter::StaticClass()->GetDefaultObject<AEmberdeepCharacter>();
	TestEqual(TEXT("Characters begin at level one"), CharacterDefault->GetCharacterLevel(), 1);
	TestEqual(TEXT("Characters begin with an empty XP bar"), CharacterDefault->GetExperienceNormalized(), 0.0f);
	TArray<UInstancedStaticMeshComponent*> ThorgrimPaletteMeshes;
	CharacterDefault->GetComponents<UInstancedStaticMeshComponent>(ThorgrimPaletteMeshes);
	TestEqual(TEXT("Thorgrim must provide body, axe, and shield palette groups"), ThorgrimPaletteMeshes.Num(), 27);

	int32 ThorgrimInstanceCount = 0;
	for (const UInstancedStaticMeshComponent* PaletteMesh : ThorgrimPaletteMeshes)
	{
		ThorgrimInstanceCount += PaletteMesh->GetInstanceCount();
	}
	TestTrue(TEXT("Thorgrim must retain the agreed voxel density"), ThorgrimInstanceCount >= 200);

	const AEmberdeepCampEnvironment* CampDefault =
		AEmberdeepCampEnvironment::StaticClass()->GetDefaultObject<AEmberdeepCampEnvironment>();
	TestEqual(TEXT("The camp must batch its restricted palette"), CampDefault->GetPaletteMeshCount(), 13);
	TestTrue(TEXT("The camp must retain its environment dressing"), CampDefault->GetVoxelInstanceCount() >= 1200);
	TestEqual(
		TEXT("The camp must provide floor, perimeter, and prop collision proxies"),
		CampDefault->GetCollisionProxyCount(),
		10);

	TArray<UPointLightComponent*> CampLights;
	CampDefault->GetComponents<UPointLightComponent>(CampLights);
	TestEqual(TEXT("The camp must retain its fire and three lantern light pools"), CampLights.Num(), 4);

	TArray<UInstancedStaticMeshComponent*> CampPaletteMeshes;
	CampDefault->GetComponents<UInstancedStaticMeshComponent>(CampPaletteMeshes);
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
