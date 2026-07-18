#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Gameplay/EmberdeepCharacter.h"
#include "Gameplay/EmberdeepGameMode.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEmberdeepFoundationClassesTest,
	"Emberdeep.Foundation.GameplayClassesExist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEmberdeepFoundationClassesTest::RunTest(const FString& Parameters)
{
	TestNotNull(TEXT("The project game mode must exist"), AEmberdeepGameMode::StaticClass());
	TestNotNull(TEXT("The project character must exist"), AEmberdeepCharacter::StaticClass());
	return true;
}

#endif

