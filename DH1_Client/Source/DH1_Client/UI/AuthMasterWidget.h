#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AuthMasterWidget.generated.h"

class UWidgetSwitcher;
class ULoginWidget;
class USignUpWidget;
class UEmailVerificationWidget; // 이메일 인증 위젯 포워드 선언

UCLASS()
class DH1_CLIENT_API UAuthMasterWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// 언리얼 에디터(UMG)의 계층 구조에 있는 위젯 이름과 정확히 일치해야 합니다.
	UPROPERTY(meta = (BindWidget))
	UWidgetSwitcher* MainSwitcher;

	UPROPERTY(meta = (BindWidget))
	ULoginWidget* LoginWidget;

	UPROPERTY(meta = (BindWidget))
	USignUpWidget* SignUpWidget;

	UPROPERTY(meta = (BindWidget))
	UEmailVerificationWidget* EmailVerificationWidget;

private:
	// 자식 위젯들에서 Broadcast된 이벤트를 수신할 콜백 함수들
	void HandleGoToSignUp();
	void HandleGoToLogin();
	void HandleSignUpSuccess(const FString& RegisteredEmail);
	void HandleVerificationSuccess(const FString& LoginEmail);
	void HandleLoginSuccess();
};