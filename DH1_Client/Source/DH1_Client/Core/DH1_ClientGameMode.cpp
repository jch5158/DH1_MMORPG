// Copyright Epic Games, Inc. All Rights Reserved.

#include "DH1_ClientGameMode.h"
#include "Controllers/DH1_ClientCharacter.h"
#include "GameFramework/PlayerController.h"

ADH1_ClientGameMode::ADH1_ClientGameMode()
{
	// L_Login에서는 캐릭터 스폰 불필요 (UI만 사용)
	DefaultPawnClass = nullptr;
	PlayerControllerClass = APlayerController::StaticClass();
}