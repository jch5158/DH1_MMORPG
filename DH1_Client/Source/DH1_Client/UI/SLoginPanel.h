#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/SCompoundWidget.h"

class SOverlay;

DECLARE_DELEGATE(FOnGoToSignUp);
DECLARE_DELEGATE(FOnGoToResetPassword);
DECLARE_DELEGATE(FOnLoginSuccess);
DECLARE_DELEGATE_ThreeParams(FOnGoToEmailVerification, const FString& /*StatusText*/, const FString& /*Email*/, bool /*bSkipAutoSendVerifyCode*/);

class SLoginPanel : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SLoginPanel) {}
		SLATE_EVENT(FOnGoToSignUp, OnGoToSignUp)
		SLATE_EVENT(FOnGoToResetPassword, OnGoToResetPassword)
		SLATE_EVENT(FOnLoginSuccess, OnLoginSuccess)
		SLATE_EVENT(FOnGoToEmailVerification, OnGoToEmailVerification)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void ResetPanel(const FString& StatusText, const FString& Email);

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

	// SAuthMasterWidget에서 delegate 바인딩으로 호출
	void HandleGatewayLoginResult(int32 Result);
	void HandleHttpLoginError(int32 StatusCode, const FString& Message);
	void HandleEmailVerificationRequired(const FString& Message, const FString& Email);

private:

	FReply OnLoginClicked();
	FReply OnSignUpClicked();
	FReply OnResetPasswordClicked();
	void OnRememberEmailCheckChanged(ECheckBoxState NewState);
	void InvalidateRememberEmailToggleVisual();

	void SetStatus(const FString& Text, const FLinearColor& Color);
	void SetLoading(bool bLoading);

	TSharedPtr<SEditableTextBox> EmailInput;
	TSharedPtr<SEditableTextBox> PasswordInput;
	TSharedPtr<SCheckBox> RememberEmailCheckBox;
	TSharedPtr<SOverlay> RememberEmailToggleRoot;
	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<SButton> LoginButton;

	bool bRememberEmail = false;
	FString LastSubmittedLoginEmail;

	FOnGoToSignUp OnGoToSignUp;
	FOnGoToResetPassword OnGoToResetPassword;
	FOnLoginSuccess OnLoginSuccess;
	FOnGoToEmailVerification OnGoToEmailVerification;
};
