#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AuthMasterWidget.generated.h"

class UResetPasswordWidget;
class UWidgetSwitcher;
class ULoginWidget;
class USignUpWidget;
class UEmailVerificationWidget; // 이메일 인증 위젯 포워드 선언

enum class EAuthWidgetType : uint8
{
	Login,
	SignUp,
	EmailVerification,
	ResetPassword
};

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

	UPROPERTY(meta = (BindWidget))
	UResetPasswordWidget* ResetPasswordWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	FName LobbyLevelName = TEXT("/Game/Levels/L_Lobby");

private:
	void HandleGoToSignUp() const;
	void HandleLoginSuccess();
	void HandleGoToResetPassword() const;

	void HandleGoToLogin(const FString& StatusText, const FString& Email) const;
	void HandleGoToEmailVerification(const FString& StatusText, const FString& Email) const;

	void SwitchWidget(const EAuthWidgetType WidgetType, const FString& StatusText, const FString& Email) const;

	TSharedPtr<SWidget> WorldSelectWidgetRef;
};