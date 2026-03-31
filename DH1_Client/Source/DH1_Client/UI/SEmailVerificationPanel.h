#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

DECLARE_DELEGATE_TwoParams(FOnVerificationGoToLogin, const FString& /*StatusText*/, const FString& /*Email*/);

class SButton;
class SEditableTextBox;
class STextBlock;

class SEmailVerificationPanel : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SEmailVerificationPanel) {}
		SLATE_EVENT(FOnVerificationGoToLogin, OnGoToLogin)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** @param bSkipAutoSendVerifyCode true면 send-verify-code 호출 안 함(회원가입 직후 등 서버가 이미 발송한 경우). */
	void ResetPanel(const FString& InStatusText, const FString& Email, bool bSkipAutoSendVerifyCode = false);

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

private:

	FReply OnVerifyClicked();
	FReply OnResendClicked();
	FReply OnBackToLoginClicked();

	/** bManualResend false = 화면 진입 시 자동 발송. PendingStatus 비어 있으면 기본 문구 사용. */
	void StartSendVerifyCode(bool bManualResend, const FString& PendingStatus = FString());

	void OnVerifyResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnResendResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	void SetStatus(const FString& Text, const FLinearColor& Color);
	void SetVerifyLoading(bool bLoading);
	void SetResendLoading(bool bLoading);

	TSharedPtr<STextBlock> EmailText;
	TSharedPtr<SEditableTextBox> VerifyCodeInput;
	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<SButton> VerifyButton;
	TSharedPtr<SButton> ResendButton;

	bool bVerifyRequestInFlight = false;
	bool bResendRequestInFlight = false;

	FOnVerificationGoToLogin OnGoToLogin;
};
