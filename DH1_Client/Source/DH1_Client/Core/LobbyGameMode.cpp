#include "LobbyGameMode.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/DefaultPawn.h"

ALobbyGameMode::ALobbyGameMode()
{
	DefaultPawnClass = ADefaultPawn::StaticClass();
	PlayerControllerClass = APlayerController::StaticClass();
}
