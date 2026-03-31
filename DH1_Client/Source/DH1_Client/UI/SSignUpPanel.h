#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

DECLARE_DELEGATE_TwoParams(FOnSignUpGoToLogin, const FString& /*StatusText*/, const FString& /*Email*/);
DECLARE_DELEGATE_ThreeParams(FOnSignUpGoToEmailVerification, const FString& /*StatusText*/, const FString& /*Email*/, bool /*bSkipAutoSendVerifyCode*/);

class SSignUpPanel : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSignUpPanel) {}
		SLATE_EVENT(FOnSignUpGoToLogin, OnGoToLogin)
		SLATE_EVENT(FOnSignUpGoToEmailVerification, OnGoToEmailVerification)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void ResetPanel();

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

private:

	FReply OnSignUpClicked();
	FReply OnBackLoginClicked();

	void OnSignUpResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	void SetStatus(const FString& Text, const FLinearColor& Color);
	void SetLoading(bool bLoading);

	TSharedPtr<SEditableTextBox> EmailInput;
	TSharedPtr<SEditableTextBox> PasswordInput;
	TSharedPtr<SEditableTextBox> PasswordConfirmInput;
	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<SButton> SignUpButton;

	FOnSignUpGoToLogin OnGoToLogin;
	FOnSignUpGoToEmailVerification OnGoToEmailVerification;
};
