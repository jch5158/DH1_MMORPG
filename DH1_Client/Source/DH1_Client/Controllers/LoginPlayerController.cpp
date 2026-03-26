#include "Controllers/LoginPlayerController.h"
#include "UI/SAuthMasterWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

void ALoginPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (GEngine && GEngine->GameViewport)
	{
		AuthWidget = SNew(SAuthMasterWidget);
		GEngine->GameViewport->AddViewportWidgetContent(AuthWidget.ToSharedRef());

		FInputModeUIOnly InputModeData;
		InputModeData.SetWidgetToFocus(AuthWidget);
		SetInputMode(InputModeData);
		bShowMouseCursor = true;
	}
}

void ALoginPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AuthWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(AuthWidget.ToSharedRef());
		AuthWidget.Reset();
	}

	Super::EndPlay(EndPlayReason);
}
