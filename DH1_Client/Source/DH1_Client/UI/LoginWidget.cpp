#include "LoginWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/Widget.h"

#include "Network/Subsystem/ClientNetSubsystem.h"

void ULoginWidget::ResetLoginWidget(const FString& StatusText, const FString& Email) const
{
	if (StatusTextMessage)
	{
		StatusTextMessage->SetText(FText::FromString(StatusText));
	}

	if (EmailInput)
	{
		EmailInput->SetText(FText::FromString(Email));
	}

	if (PasswordInput)
	{
		PasswordInput->SetText(FText::GetEmpty());
	}
}

void ULoginWidget::SetStatusTextMessage(const FString& StatusText) const
{
	if (StatusTextMessage)
	{
		StatusTextMessage->SetText(FText::FromString(StatusText));
	}
}

void ULoginWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LoginButton)
	{
		LoginButton->OnClicked.AddDynamic(this, &ULoginWidget::OnLoginButtonClicked);
	}

	if (SignUpButton)
	{
		SignUpButton->OnClicked.AddDynamic(this, &ULoginWidget::OnSignUpButtonClicked);
	}

	if (ResetPasswordButton)
	{
		ResetPasswordButton->OnClicked.AddDynamic(this, &ULoginWidget::OnResetPasswordButtonClicked);
	}

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (UClientNetSubsystem* NetSubsystem = GameInstance->GetSubsystem<UClientNetSubsystem>())
		{
			NetSubsystem->OnGatewayLoginResult.AddUObject(this, &ULoginWidget::HandleGatewayLoginResult);
			NetSubsystem->OnHttpLoginError.AddUObject(this, &ULoginWidget::HandleHttpLoginError);
			NetSubsystem->OnEmailVerificationRequired.AddUObject(this, &ULoginWidget::HandleEmailVerificationRequired);
		}
	}

	SetLoading(false);
}

void ULoginWidget::NativeDestruct()
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (UClientNetSubsystem* NetSubsystem = GameInstance->GetSubsystem<UClientNetSubsystem>())
		{
			NetSubsystem->OnGatewayLoginResult.RemoveAll(this);
			NetSubsystem->OnHttpLoginError.RemoveAll(this);
			NetSubsystem->OnEmailVerificationRequired.RemoveAll(this);
		}
	}

	Super::NativeDestruct();
}

void ULoginWidget::OnLoginButtonClicked()
{
	const FString Email = EmailInput->GetText().ToString();
	const FString Password = PasswordInput->GetText().ToString();

	if (Email.IsEmpty() || Password.IsEmpty())
	{
		SetStatusTextMessage(TEXT("이메일과 비밀번호를 모두 입력해주세요."));
		return;
	}

	if (!Email.Contains(TEXT("@")) || !Email.Contains(TEXT(".")))
	{
		SetStatusTextMessage(TEXT("유효한 이메일 형식이 아닙니다."));
		return;
	}

	SetStatusTextMessage(TEXT(""));
	SetLoading(true);

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (UClientNetSubsystem* NetSubsystem = GameInstance->GetSubsystem<UClientNetSubsystem>())
		{
			NetSubsystem->RequestLogin(Email, Password);
		}
	}
}

void ULoginWidget::OnSignUpButtonClicked()
{
	if (OnGoToSignUp.IsBound())
	{
		OnGoToSignUp.Broadcast();
	}
}

void ULoginWidget::OnResetPasswordButtonClicked()
{
	if (OnGoToResetPassword.IsBound())
	{
		OnGoToResetPassword.Broadcast();
	}
}

void ULoginWidget::HandleGatewayLoginResult(const int32 Result)
{
	SetLoading(false);

	// eLoginResult: 0=SUCCESS, 1=INVALID_TICKET, 2=SERVER_FULL, 3=MAINTENANCE, 4=INTERNAL_ERROR
	if (Result == 0)
	{
		if (OnLoginSuccess.IsBound())
		{
			OnLoginSuccess.Broadcast();
		}
	}
	else
	{
		switch (Result)
		{
		case 1:
			SetStatusTextMessage(TEXT("인증 정보가 만료되었습니다. 다시 로그인해주세요."));
			break;
		case 2:
			SetStatusTextMessage(TEXT("서버가 가득 찼습니다. 잠시 후 다시 시도해주세요."));
			break;
		case 3:
			SetStatusTextMessage(TEXT("서버 점검 중입니다. 잠시 후 다시 시도해주세요."));
			break;
		default:
			SetStatusTextMessage(TEXT("서버 오류가 발생했습니다. 잠시 후 다시 시도해주세요."));
			break;
		}
	}
}

void ULoginWidget::HandleHttpLoginError(const int32 StatusCode, const FString& Message)
{
	SetLoading(false);
	SetStatusTextMessage(Message);
}

void ULoginWidget::HandleEmailVerificationRequired(const FString& Message, const FString& Email)
{
	SetLoading(false);
	if (OnGoToEmailVerification.IsBound())
	{
		OnGoToEmailVerification.Broadcast(Message, Email);
	}
}

void ULoginWidget::SetLoading(const bool bLoading)
{
	if (LoginButton)
	{
		LoginButton->SetIsEnabled(!bLoading);
	}

	if (LoadingOverlay)
	{
		LoadingOverlay->SetVisibility(bLoading ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}
