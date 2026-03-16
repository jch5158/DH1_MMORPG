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
DECLARE_MULTICAST_DELEGATE(FOnGoToForgotPasswordDelegate);

UCLASS()
class DH1_CLIENT_API ULoginWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    FOnGoToSignUpDelegate OnGoToSignUp;
    FOnLoginSuccessDelegate OnLoginSuccess;
    FOnGoToForgotPasswordDelegate OnGoToForgotPassword;

    void SetEmail(const FString& EmailToSet);

protected:
    virtual void NativeConstruct() override;

    // [팩트 체크] 마스터 위젯이 화면 전환을 담당하므로 SignUpWidgetClass는 삭제되었습니다.

    // 에러 메시지 출력용 텍스트 블록
    UPROPERTY(meta = (BindWidget))
    UTextBlock* ErrorTextMessage;

    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* EmailInput;

    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* PasswordInput;

    UPROPERTY(meta = (BindWidget))
    UButton* LoginButton;

    UPROPERTY(meta = (BindWidget))
    UButton* SignUpButton;

    UPROPERTY(meta = (BindWidget))
    UButton* ForgotPasswordButton;

    UFUNCTION()
    void OnLoginButtonClicked();

    UFUNCTION()
    void OnSignUpButtonClicked();

    UFUNCTION()
    void OnForgotPasswordButtonClicked();

    void OnLoginResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful);
};