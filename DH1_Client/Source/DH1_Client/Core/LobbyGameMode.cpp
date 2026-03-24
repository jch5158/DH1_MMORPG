#include "LobbyGameMode.h"
#include "Controllers/DH1_ClientCharacter.h"
#include "Controllers/DH1_ClientPlayerController.h"

ALobbyGameMode::ALobbyGameMode()
{
	DefaultPawnClass = ADH1_ClientCharacter::StaticClass();
	PlayerControllerClass = ADH1_ClientPlayerController::StaticClass();
}
