#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/IHttpRequest.h" // 필수 헤더 추가
#include "EmailVerificationWidget.generated.h"

class UEditableTextBox;
class UTextBlock;
class UButton;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGoToLoginDelegate, const FString&, const FString&);

UCLASS()
class DH1_CLIENT_API UEmailVerificationWidget : public UUserWidget
{
    GENERATED_BODY()

public:

    FOnGoToLoginDelegate OnGoToLogin;

    void ResetVerificationWidget(const FString& StatusText, const FString& Email) const;
    void SetStatusTextMessage(const FString& StatusText) const;

protected:
    virtual void NativeConstruct() override;

    // 에러 메시지 출력용
    UPROPERTY(meta = (BindWidget))
    UTextBlock* StatusTextMessage;

    // 현재 인증을 진행 중인 이메일을 보여줄 텍스트
    UPROPERTY(meta = (BindWidget))
    UTextBlock* EmailText;

    // 인증번호 입력 칸
    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* VerifyCodeInput;

    // 인증 확인 버튼
    UPROPERTY(meta = (BindWidget))
    UButton* VerifyButton;

    // 인증번호 재전송 버튼 (옵션)
    UPROPERTY(meta = (BindWidget))
    UButton* ResendButton;

    UPROPERTY(meta = (BindWidget))
	UButton* BackToLoginButton;

    // --- 이벤트 함수 ---
    UFUNCTION()
    void OnVerifyButtonClicked();

    UFUNCTION()
    void OnResendButtonClicked();

    UFUNCTION()
    void OnBackToLoginButtonClicked();

    // --- HTTP 콜백 ---
    void OnVerifyResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void OnResendResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};