#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SLoginPanel;
class SSignUpPanel;
class SEmailVerificationPanel;
class SResetPasswordPanel;
class SRealmSelectWidget;
class SWidgetSwitcher;
class UClientNetSubsystem;

enum class EAuthPanelType : uint8
{
	Login,
	SignUp,
	EmailVerification,
	ResetPassword
};

class SAuthMasterWidget : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SAuthMasterWidget) {}
	SLATE_END_ARGS()

	~SAuthMasterWidget();

	void Construct(const FArguments& InArgs);

private:

	// Panel switching
	void SwitchToLogin(const FString& StatusText, const FString& Email);
	void SwitchToSignUp();
	void SwitchToEmailVerification(const FString& StatusText, const FString& Email, bool bSkipAutoSendVerifyCode = false);
	void SwitchToResetPassword();

	// Login success flow
	void HandleLoginSuccess();

	// ClientNetSubsystem delegate handlers
	void HandleGatewayLoginResult(int32 Result);
	void HandleHttpLoginError(int32 StatusCode, const FString& Message);
	void HandleEmailVerificationRequired(const FString& Message, const FString& Email);
	void HandleRealmListReceived(const TArray<struct FRealmServerInfo>& RealmList);
	void HandleRealmSelectResult(int32 Result);

	UClientNetSubsystem* GetNetSubsystem() const;

	TSharedPtr<SWidgetSwitcher> PanelSwitcher;
	TSharedPtr<SLoginPanel> LoginPanel;
	TSharedPtr<SSignUpPanel> SignUpPanel;
	TSharedPtr<SEmailVerificationPanel> EmailVerificationPanel;
	TSharedPtr<SResetPasswordPanel> ResetPasswordPanel;
	TSharedPtr<SWidget> RealmSelectWidgetRef;
};
