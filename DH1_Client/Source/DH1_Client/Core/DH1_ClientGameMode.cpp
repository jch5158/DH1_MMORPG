// Copyright Epic Games, Inc. All Rights Reserved.

#include "DH1_ClientGameMode.h"
#include "Controllers/LoginPlayerController.h"

ADH1_ClientGameMode::ADH1_ClientGameMode()
{
	// L_Login: LoginPlayerController가 Slate Auth UI를 생성
	DefaultPawnClass = nullptr;
	PlayerControllerClass = ALoginPlayerController::StaticClass();
}