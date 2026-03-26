#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

DECLARE_DELEGATE_TwoParams(FOnVerificationGoToLogin, const FString& /*StatusText*/, const FString& /*Email*/);

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

	TSharedPtr<STextBlock> EmailText;
	TSharedPtr<SEditableTextBox> VerifyCodeInput;
	TSharedPtr<STextBlock> StatusText;

	FOnVerificationGoToLogin OnGoToLogin;
};
