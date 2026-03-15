#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/IHttpRequest.h"
#include "LoginWidget.generated.h"

class UEditableTextBox;
class UButton;
class UTextBlock;

// 마스터 위젯으로 화면 전환 및 성공 이벤트를 전달할 델리게이트 선언
DECLARE_MULTICAST_DELEGATE(FOnGoToSignUpDelegate);
DECLARE_MULTICAST_DELEGATE(FOnLoginSuccessDelegate); // 필요 시 매개변수로 JWT 토큰(FString) 등을 넘길 수 있습니다.

UCLASS()
class DH1_CLIENT_API ULoginWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    FOnGoToSignUpDelegate OnGoToSignUp;
    FOnLoginSuccessDelegate OnLoginSuccess;

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

    UFUNCTION()
    void OnLoginButtonClicked();

    UFUNCTION()
    void OnSignUpButtonClicked();

    void OnLoginResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful);
};