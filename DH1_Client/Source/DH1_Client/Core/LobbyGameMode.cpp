#include "LobbyGameMode.h"
#include "UObject/ConstructorHelpers.h"

ALobbyGameMode::ALobbyGameMode()
{
	static ConstructorHelpers::FClassFinder<APawn> CharacterBP(TEXT("/Game/TopDown/Blueprints/BP_TopDownCharacter"));
	if (CharacterBP.Succeeded())
	{
		DefaultPawnClass = CharacterBP.Class;
	}

	static ConstructorHelpers::FClassFinder<APlayerController> ControllerBP(TEXT("/Game/TopDown/Blueprints/BP_TopDownController"));
	if (ControllerBP.Succeeded())
	{
		PlayerControllerClass = ControllerBP.Class;
	}
}
