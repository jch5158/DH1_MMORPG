#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/IHttpRequest.h" // 필수 헤더 추가
#include "EmailVerificationWidget.generated.h"

class UEditableTextBox;
class UTextBlock;
class UButton;

// 화면 전환을 위한 델리게이트 선언
DECLARE_MULTICAST_DELEGATE_OneParam(FOnVerificationSuccessDelegate, const FString&);

UCLASS()
class DH1_CLIENT_API UEmailVerificationWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 델리게이트 변수 선언
    FOnVerificationSuccessDelegate OnVerificationSuccess;

    // 이전 화면(SignUpWidget)에서 호출하여 이메일을 세팅해 줄 함수
    void SetVerificationEmail(const FString& Email);

protected:
    virtual void NativeConstruct() override;

    // 현재 인증을 진행 중인 이메일을 보여줄 텍스트
    UPROPERTY(meta = (BindWidget))
    UTextBlock* TargetEmailText;

    // 에러 메시지 출력용
    UPROPERTY(meta = (BindWidget))
    UTextBlock* ErrorTextMessage;

    // 인증번호 입력 칸
    UPROPERTY(meta = (BindWidget))
    UEditableTextBox* VerificationCodeInput;

    // 인증 확인 버튼
    UPROPERTY(meta = (BindWidget))
    UButton* VerifyButton;

    // 인증번호 재전송 버튼 (옵션)
    UPROPERTY(meta = (BindWidget))
    UButton* ResendButton;

    // (옵션) 뒤로가기/취소 버튼이 있다면 추가
    // UPROPERTY(meta = (BindWidget))
    // UButton* CancelButton;

    // --- 이벤트 함수 ---
    UFUNCTION()
    void OnVerifyButtonClicked();

    UFUNCTION()
    void OnResendButtonClicked();

    // --- HTTP 콜백 ---
    void OnVerifyResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void OnResendResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

private:
    // 현재 인증 중인 이메일을 보관할 변수
    FString CurrentEmail;
};