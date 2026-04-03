#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

DECLARE_DELEGATE_TwoParams(FOnResetPwGoToLogin, const FString& /*StatusText*/, const FString& /*Email*/);

class SButton;
class SEditableTextBox;
class STextBlock;

class SResetPasswordPanel : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SResetPasswordPanel) {}
		SLATE_EVENT(FOnResetPwGoToLogin, OnGoToLogin)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void ResetPanel(const FString& InStatusText, const FString& Email);

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return false; }

private:

	FReply OnSendVerifyCodeClicked();
	FReply OnPasswordConfirmClicked();
	FReply OnBackToLoginClicked();

	void OnSendCodeResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnResetPasswordResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	void SetStatus(const FString& Text, const FLinearColor& Color);
	void SetSendCodeLoading(bool bLoading);
	void SetResetLoading(bool bLoading);

	TSharedPtr<SEditableTextBox> EmailInputText;
	TSharedPtr<SEditableTextBox> VerifyCodeInput;
	TSharedPtr<SEditableTextBox> PasswordInput;
	TSharedPtr<SEditableTextBox> PasswordConfirmInput;
	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<SButton> SendVerifyCodeButton;
	TSharedPtr<SButton> ResetPasswordButton;

	bool bSendCodeInFlight = false;
	bool bResetInFlight = false;

	FOnResetPwGoToLogin OnGoToLogin;
};
