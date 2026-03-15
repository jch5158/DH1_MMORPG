// Fill out your copyright notice in the Description page of Project Settings.


#include "Controllers/LoginPlayerController.h"
#include "Blueprint/UserWidget.h"

void ALoginPlayerController::BeginPlay()
{
	Super::BeginPlay();

    if (AuthMasterWidgetClass) // 에디터에서 WBP_AuthMaster 할당
    {
	    if (UUserWidget* AuthWidget = CreateWidget<UUserWidget>(this, AuthMasterWidgetClass))
        {
            AuthWidget->AddToViewport();

            // UI 입력 모드 설정
            FInputModeUIOnly InputModeData;
            InputModeData.SetWidgetToFocus(AuthWidget->TakeWidget());
            SetInputMode(InputModeData);
            bShowMouseCursor = true;
        }
    }
}
