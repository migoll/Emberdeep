#if WITH_DEV_AUTOMATION_TESTS

#include "Components/InstancedStaticMeshComponent.h"
#include "Misc/AutomationTest.h"
#include "Gameplay/EmberdeepCharacter.h"
#include "Gameplay/EmberdeepEnemy.h"
#include "Gameplay/EmberdeepGameMode.h"
#include "Gameplay/EmberdeepGoldPickup.h"
#include "Gameplay/EmberdeepHealthComponent.h"
#include "UI/EmberdeepHUD.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEmberdeepFoundationClassesTest,
	"Emberdeep.Foundation.GameplayClassesExist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEmberdeepFoundationClassesTest::RunTest(const FString& Parameters)
{
	TestNotNull(TEXT("The project game mode must exist"), AEmberdeepGameMode::StaticClass());
	TestNotNull(TEXT("The project character must exist"), AEmberdeepCharacter::StaticClass());
	TestNotNull(TEXT("The skeleton enemy must exist"), AEmberdeepEnemy::StaticClass());
	TestNotNull(TEXT("The gold pickup must exist"), AEmberdeepGoldPickup::StaticClass());
	TestNotNull(TEXT("The combat HUD must exist"), AEmberdeepHUD::StaticClass());

	const AEmberdeepCharacter* CharacterDefault = AEmberdeepCharacter::StaticClass()->GetDefaultObject<AEmberdeepCharacter>();
	TArray<UInstancedStaticMeshComponent*> ThorgrimPaletteMeshes;
	CharacterDefault->GetComponents<UInstancedStaticMeshComponent>(ThorgrimPaletteMeshes);
	TestEqual(TEXT("Thorgrim must provide body, axe, and shield palette groups"), ThorgrimPaletteMeshes.Num(), 27);

	int32 ThorgrimInstanceCount = 0;
	for (const UInstancedStaticMeshComponent* PaletteMesh : ThorgrimPaletteMeshes)
	{
		ThorgrimInstanceCount += PaletteMesh->GetInstanceCount();
	}
	TestTrue(TEXT("Thorgrim must retain the agreed voxel density"), ThorgrimInstanceCount >= 200);
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
