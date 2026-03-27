#include "SLoginPanel.h"
#include "AuthWidgetStyle.h"
#include "Network/Subsystem/ClientNetSubsystem.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"

void SLoginPanel::Construct(const FArguments& InArgs)
{
	OnGoToSignUp = InArgs._OnGoToSignUp;
	OnGoToResetPassword = InArgs._OnGoToResetPassword;
	OnLoginSuccess = InArgs._OnLoginSuccess;
	OnGoToEmailVerification = InArgs._OnGoToEmailVerification;

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(620.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderBackgroundColor(AuthStyle::C::CardBg)
			.Padding(FMargin(60.0f, 50.0f))
			[
				SNew(SVerticalBox)

				// Title
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 32)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("로그인")))
					.Font(AuthStyle::TitleFont())
					.ColorAndOpacity(AuthStyle::C::Title)
					.Justification(ETextJustify::Center)
				]

				// Status
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
				[
					SAssignNew(StatusText, STextBlock)
					.Font(AuthStyle::SmallFont())
					.ColorAndOpacity(AuthStyle::C::Dim)
					.Justification(ETextJustify::Center)
				]

				// Email
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 14)
				[
					AuthStyle::MakeInput(EmailInput, FText::FromString(TEXT("이메일")), false,
						[this](const FText&, ETextCommit::Type CommitType)
						{
							if (CommitType == ETextCommit::OnEnter && PasswordInput.IsValid())
							{
								FSlateApplication::Get().SetKeyboardFocus(PasswordInput);
							}
						})
				]

				// Password
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 28)
				[
					AuthStyle::MakeInput(PasswordInput, FText::FromString(TEXT("비밀번호")), true,
						[this](const FText&, ETextCommit::Type CommitType)
						{
							if (CommitType == ETextCommit::OnEnter)
							{
								OnLoginClicked();
							}
						})
				]

				// Login Button
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 14)
				[
					SAssignNew(LoginButton, SButton)
					.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
					.ButtonColorAndOpacity(AuthStyle::C::Primary)
					.OnClicked(FOnClicked::CreateSP(this, &SLoginPanel::OnLoginClicked))
					.ContentPadding(FMargin(0.0f, 14.0f))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("로그인")))
						.Font(AuthStyle::ButtonFont())
						.ColorAndOpacity(AuthStyle::C::BtnText)
						.Justification(ETextJustify::Center)
					]
				]

				// SignUp Button
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
				[
					AuthStyle::MakeSecondaryButton(
						FText::FromString(TEXT("회원가입")),
						FOnClicked::CreateSP(this, &SLoginPanel::OnSignUpClicked))
				]

				// Reset Password Button
				+ SVerticalBox::Slot().AutoHeight()
				[
					AuthStyle::MakeSecondaryButton(
						FText::FromString(TEXT("비밀번호 찾기")),
						FOnClicked::CreateSP(this, &SLoginPanel::OnResetPasswordClicked))
				]
			]
		]
	];

	// 이메일 입력란에 자동 포커스
	if (EmailInput.IsValid())
	{
		RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateLambda(
			[this](double, float) -> EActiveTimerReturnType
			{
				if (EmailInput.IsValid())
				{
					FSlateApplication::Get().SetKeyboardFocus(EmailInput);
				}
				return EActiveTimerReturnType::Stop;
			}));
	}
}

void SLoginPanel::ResetPanel(const FString& InStatusText, const FString& Email)
{
	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(InStatusText));
	}

	if (EmailInput.IsValid())
	{
		EmailInput->SetText(FText::FromString(Email));
	}

	if (PasswordInput.IsValid())
	{
		PasswordInput->SetText(FText::GetEmpty());
	}
}

FReply SLoginPanel::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Enter)
	{
		if (EmailInput.IsValid() && EmailInput->HasKeyboardFocus() && PasswordInput.IsValid())
		{
			FSlateApplication::Get().SetKeyboardFocus(PasswordInput);
			return FReply::Handled();
		}

		if (PasswordInput.IsValid() && PasswordInput->HasKeyboardFocus())
		{
			OnLoginClicked();
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

FReply SLoginPanel::OnLoginClicked()
{
	if (!EmailInput.IsValid() || !PasswordInput.IsValid())
	{
		return FReply::Handled();
	}

	const FString Email = EmailInput->GetText().ToString();
	const FString Password = PasswordInput->GetText().ToString();

	if (Email.IsEmpty() || Password.IsEmpty())
	{
		SetStatus(TEXT("이메일과 비밀번호를 모두 입력해주세요."), AuthStyle::C::Error);
		return FReply::Handled();
	}

	if (!Email.Contains(TEXT("@")) || !Email.Contains(TEXT(".")))
	{
		SetStatus(TEXT("유효한 이메일 형식이 아닙니다."), AuthStyle::C::Error);
		return FReply::Handled();
	}

	SetStatus(TEXT("로그인 중..."), AuthStyle::C::Body);
	SetLoading(true);

	if (GEngine && GEngine->GameViewport)
	{
		if (const UWorld* World = GEngine->GameViewport->GetWorld())
		{
			if (const UGameInstance* GI = World->GetGameInstance())
			{
				if (UClientNetSubsystem* Net = GI->GetSubsystem<UClientNetSubsystem>())
				{
					Net->RequestLogin(Email, Password);
				}
			}
		}
	}

	return FReply::Handled();
}

FReply SLoginPanel::OnSignUpClicked()
{
	OnGoToSignUp.ExecuteIfBound();
	return FReply::Handled();
}

FReply SLoginPanel::OnResetPasswordClicked()
{
	OnGoToResetPassword.ExecuteIfBound();
	return FReply::Handled();
}

void SLoginPanel::SetStatus(const FString& Text, const FLinearColor& Color)
{
	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(Text));
		StatusText->SetColorAndOpacity(FSlateColor(Color));
	}
}

void SLoginPanel::SetLoading(const bool bLoading)
{
	if (LoginButton.IsValid())
	{
		LoginButton->SetEnabled(!bLoading);
	}
}

void SLoginPanel::HandleGatewayLoginResult(const int32 Result)
{
	SetLoading(false);

	if (Result == 0)
	{
		OnLoginSuccess.ExecuteIfBound();
	}
	else
	{
		switch (Result)
		{
		case 1: SetStatus(TEXT("인증 정보가 만료되었습니다. 다시 로그인해주세요."), AuthStyle::C::Error); break;
		case 2: SetStatus(TEXT("서버가 가득 찼습니다. 잠시 후 다시 시도해주세요."), AuthStyle::C::Error); break;
		case 3: SetStatus(TEXT("서버 점검 중입니다. 잠시 후 다시 시도해주세요."), AuthStyle::C::Error); break;
		default: SetStatus(TEXT("서버 오류가 발생했습니다. 잠시 후 다시 시도해주세요."), AuthStyle::C::Error); break;
		}
	}
}

void SLoginPanel::HandleHttpLoginError(const int32 StatusCode, const FString& Message)
{
	SetLoading(false);
	SetStatus(Message, AuthStyle::C::Error);
}

void SLoginPanel::HandleEmailVerificationRequired(const FString& Message, const FString& Email)
{
	SetLoading(false);
	OnGoToEmailVerification.ExecuteIfBound(Message, Email);
}
