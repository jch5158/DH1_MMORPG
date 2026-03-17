#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/IHttpRequest.h"
#include "LoginWidget.generated.h"

class UEditableTextBox;
class UButton;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE(FOnGoToSignUpDelegate);
DECLARE_MULTICAST_DELEGATE(FOnLoginSuccessDelegate);
DECLARE_MULTICAST_DELEGATE(FOnGoToResetPasswordDelegate);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGoToEmailVerificationDelegate, const FString&, const FString&);

UCLASS()
class DH1_CLIENT_API ULoginWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    FOnGoToSignUpDelegate OnGoToSignUp;
    FOnLoginSuccessDelegate OnLoginSuccess;
    FOnGoToResetPasswordDelegate OnGoToResetPassword;
    FOnGoToEmailVerificationDelegate OnGoToEmailVerification;

    void ResetLoginWidget(const FString& StatusText, const FString& Email) const;
    void SetStatusTextMessage(const FString& StatusText) const;

protected:
    virtual void NativeConstruct() override;

    // 에러 메시지 출력용 텍스트 블록
    UPROPERTY(meta = (BindWidget))
    UTextBlock* StatusTextMessage;

    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* EmailInput;

    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* PasswordInput;

    UPROPERTY(meta = (BindWidget))
    UButton* LoginButton;

    UPROPERTY(meta = (BindWidget))
    UButton* SignUpButton;

    UPROPERTY(meta = (BindWidget))
    UButton* ResetPasswordButton;

    UFUNCTION()
    void OnLoginButtonClicked();

    UFUNCTION()
    void OnSignUpButtonClicked();

    UFUNCTION()
    void OnResetPasswordButtonClicked();

    void OnLoginResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful);
};