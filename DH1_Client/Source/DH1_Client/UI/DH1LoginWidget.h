#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/IHttpRequest.h"
#include "DH1LoginWidget.generated.h"

class UEditableTextBox;
class UButton;

UCLASS()
class DH1_CLIENT_API UDH1LoginWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* EmailInput;

    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* PasswordInput;

    UPROPERTY(meta = (BindWidget))
    UButton* LoginButton;

    UPROPERTY(meta = (BindWidget))
    UButton* SignUpButton;

    UFUNCTION()
    void OnLoginButtonClicked();

    UFUNCTION()
    void OnSignUpButtonClicked();

    void OnLoginResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful);
    void OnSignUpResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful);
};