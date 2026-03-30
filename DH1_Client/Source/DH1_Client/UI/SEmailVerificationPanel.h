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

	void ResetPanel(const FString& InStatusText, const FString& Email);

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

private:

	FReply OnVerifyClicked();
	FReply OnResendClicked();
	FReply OnBackToLoginClicked();

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
