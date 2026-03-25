#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoginWidget.generated.h"

class UEditableTextBox;
class UButton;
class UTextBlock;
class UWidget;
class UWidgetSwitcher;

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
    virtual void NativeDestruct() override;

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

    UPROPERTY(meta = (BindWidgetOptional))
    UWidget* LoadingOverlay;

    UFUNCTION()
    void OnLoginButtonClicked();

    UFUNCTION()
    void OnSignUpButtonClicked();

    UFUNCTION()
    void OnResetPasswordButtonClicked();

    UFUNCTION()
    void OnEmailInputCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void OnPasswordInputCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    void HandleGatewayLoginResult(int32 Result);
    void HandleHttpLoginError(int32 StatusCode, const FString& Message);
    void HandleEmailVerificationRequired(const FString& Message, const FString& Email);

    void SetLoading(bool bLoading);
    void SetStatusColor(const FLinearColor& Color) const;
};
