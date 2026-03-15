#include "AuthMasterWidget.h"
#include "Components/WidgetSwitcher.h"
#include "LoginWidget.h"
#include "SignUpWidget.h"
#include "EmailVerificationWidget.h"

void UAuthMasterWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 1. 로그인 위젯 이벤트 바인딩
	if (LoginWidget)
	{
		LoginWidget->OnGoToSignUp.AddUObject(this, &UAuthMasterWidget::HandleGoToSignUp);
		LoginWidget->OnLoginSuccess.AddUObject(this, &UAuthMasterWidget::HandleLoginSuccess);
	}

	// 2. 회원가입 위젯 이벤트 바인딩
	if (SignUpWidget)
	{
		SignUpWidget->OnGoToLogin.AddUObject(this, &UAuthMasterWidget::HandleGoToLogin);
		SignUpWidget->OnSignUpSuccess.AddUObject(this, &UAuthMasterWidget::HandleSignUpSuccess);
	}

	if (EmailVerificationWidget)
	{
		EmailVerificationWidget->OnVerificationSuccess.AddUObject(this, &UAuthMasterWidget::HandleVerificationSuccess);
	}
}

void UAuthMasterWidget::HandleGoToLogin()
{
	if (MainSwitcher)
	{
		// 인덱스 0: 로그인 화면
		MainSwitcher->SetActiveWidgetIndex(0);
	}
}

void UAuthMasterWidget::HandleGoToSignUp()
{
	if (MainSwitcher)
	{
		// 인덱스 1: 회원가입 화면
		MainSwitcher->SetActiveWidgetIndex(1);
	}
}

void UAuthMasterWidget::HandleSignUpSuccess(const FString& RegisteredEmail)
{
	if (MainSwitcher && EmailVerificationWidget)
	{
		// 대상 이메일 세팅 후 화면 전환
		EmailVerificationWidget->SetVerificationEmail(RegisteredEmail);

		// 인덱스 2: 이메일 인증 화면
		MainSwitcher->SetActiveWidgetIndex(2);
	}
}

void UAuthMasterWidget::HandleVerificationSuccess(const FString& LoginEmail)
{
	if (MainSwitcher)
	{
		// 대상 이메일 세팅 후 화면 전환
		LoginWidget->SetEmail(LoginEmail);

		// 인덱스 2: 이메일 인증 화면
		MainSwitcher->SetActiveWidgetIndex(0);
	}
}

void UAuthMasterWidget::HandleLoginSuccess()
{
	// 로그인 성공 시 처리 로직
	// 예: 마스터 위젯을 화면에서 제거하고(RemoveFromParent), 
	// 로비 맵으로 Level을 이동하거나 메인 메뉴 위젯을 띄우는 로직을 구현합니다.

	UE_LOG(LogTemp, Log, TEXT("로그인 완료! 로비로 이동합니다."));
	// this->RemoveFromParent();
}
