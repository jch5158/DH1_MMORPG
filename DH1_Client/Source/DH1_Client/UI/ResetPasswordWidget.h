#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/IHttpRequest.h"
#include "ResetPasswordWidget.generated.h"

class UEditableTextBox;
class UButton;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGoToLoginDelegate, const FString&, const FString&);

UCLASS()
class DH1_CLIENT_API UResetPasswordWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    FOnGoToLoginDelegate OnGoToLogin;

    void ResetResetPasswordWidget(const FString& StatusText, const FString& Email) const;
    void SetStatusTextMessage(const FString& StatusText) const;

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* StatusTextMessage;

    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* EmailInputText;

    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* VerifyCodeInput;

    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* PasswordInput;

    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* PasswordConfirmInput;

	UPROPERTY(meta = (BindWidget))
	UButton* SendVerifyCodeButton;

    UPROPERTY(meta = (BindWidget))
    UButton* PasswordConfirmButton;

    UPROPERTY(meta = (BindWidget))
    UButton* BackToLoginButton;

    UFUNCTION()
    void OnSendVerifyCodeButtonClicked();

    UFUNCTION()
    void OnPasswordConfirmButtonClicked();

    UFUNCTION()
    void OnBackToLoginButtonClicked();

    void OnSendCodeResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful);
    void OnResetPasswordResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful);
};