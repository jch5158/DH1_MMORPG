#include "SLoginPanel.h"
#include "AuthLocalPreferences.h"
#include "AuthWidgetStyle.h"
#include "AuthErrorMapper.h"
#include "Network/Subsystem/ClientNetSubsystem.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/CoreStyle.h"

namespace SLoginPanel_Local
{
	FString NormalizeEmail(const FString& InEmail)
	{
		return InEmail.TrimStartAndEnd().ToLower();
	}

	bool IsLikelyValidEmail(const FString& Email)
	{
		const FString Normalized = NormalizeEmail(Email);
		int32 AtIndex = INDEX_NONE;
		if (!Normalized.FindChar(TEXT('@'), AtIndex))
		{
			return false;
		}

		if (AtIndex <= 0 || AtIndex >= Normalized.Len() - 3)
		{
			return false;
		}

		const int32 DotIndex = Normalized.Find(TEXT("."), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		return DotIndex > AtIndex + 1 && DotIndex < Normalized.Len() - 1;
	}
}

void SLoginPanel::Construct(const FArguments& InArgs)
{
	OnGoToSignUp = InArgs._OnGoToSignUp;
	OnGoToResetPassword = InArgs._OnGoToResetPassword;
	OnLoginSuccess = InArgs._OnLoginSuccess;
	OnGoToEmailVerification = InArgs._OnGoToEmailVerification;

	FString SavedEmailFromDisk;
	AuthLocalPreferences::LoadLoginRemember(bRememberEmail, SavedEmailFromDisk);

	ChildSlot
	[
		// Blood red outer border (1px glow/border effect)
		SNew(SBorder)
		.BorderImage(AuthStyle::GlassCardBorderBrush())
		.BorderBackgroundColor(AuthStyle::C::CardBorder)
		.Padding(FMargin(1.0f))
		[
			SNew(SBox)
			.WidthOverride(720.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SBorder)
				.BorderImage(AuthStyle::GlassCardFillBrush())
				.BorderBackgroundColor(AuthStyle::C::CardBg)
				.Padding(FMargin(64.0f, 34.0f, 64.0f, 64.0f))
				[
					SNew(SVerticalBox)

					// Game title removed by request
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 0)
					[
						SNew(SSpacer)
						.Size(FVector2D(1.0f, 0.0f))
					]

					// Status text
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
					[
						SAssignNew(StatusText, STextBlock)
						.Font(AuthStyle::SmallFont())
						.ColorAndOpacity(AuthStyle::C::Dim)
						.Justification(ETextJustify::Center)
					]

					// Email label
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("이메일")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
						.ColorAndOpacity(AuthStyle::C::Crimson)
					]

					// Email input
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 24)
					[
						AuthStyle::MakeInput(EmailInput, FText::FromString(TEXT("example@mail.com")), false,
							[this](const FText&, ETextCommit::Type CommitType)
							{
								if (CommitType == ETextCommit::OnEnter && PasswordInput.IsValid())
								{
									FSlateApplication::Get().SetKeyboardFocus(PasswordInput);
								}
							})
					]

					// Password label
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("비밀번호")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
						.ColorAndOpacity(AuthStyle::C::Crimson)
					]

					// Password input
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
					[
						AuthStyle::MakeInput(PasswordInput, FText::GetEmpty(), true,
							[this](const FText&, ETextCommit::Type CommitType)
							{
								if (CommitType == ETextCommit::OnEnter)
								{
									OnLoginClicked();
								}
							})
					]

					// Remember email — 오른쪽 정렬: [체크박스] 이메일 저장
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 20)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Right).VAlign(VAlign_Center)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								SNew(SBox)
								.WidthOverride(22.0f)
								.HeightOverride(22.0f)
								[
									SAssignNew(RememberEmailCheckBox, SCheckBox)
									.Style(&AuthStyle::RememberEmailCheckBoxStyle())
									.IsChecked(bRememberEmail ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
									.OnCheckStateChanged(FOnCheckStateChanged::CreateSP(this, &SLoginPanel::OnRememberEmailCheckChanged))
								]
							]
							+ SHorizontalBox::Slot().AutoWidth().Padding(10.0f, 0.0f, 0.0f, 0.0f).VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("이메일 저장")))
								.Font(AuthStyle::SmallFont())
								.ColorAndOpacity(AuthStyle::C::Body)
							]
						]
					]

					// Login Button
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
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

					// Bottom crimson separator
					+ SVerticalBox::Slot().AutoHeight().MaxHeight(1.0f).Padding(0, 4, 0, 18)
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(AuthStyle::C::CrimsonDim)
					]

					// SignUp Button
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
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
		]
	];

	if (EmailInput.IsValid() && bRememberEmail && !SavedEmailFromDisk.IsEmpty())
	{
		EmailInput->SetText(FText::FromString(SavedEmailFromDisk));
	}

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
	SetLoading(false);

	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(InStatusText));
	}

	if (Email.IsEmpty())
	{
		bool bRememberFromDisk = false;
		FString SavedEmailFromDisk;
		AuthLocalPreferences::LoadLoginRemember(bRememberFromDisk, SavedEmailFromDisk);
		bRememberEmail = bRememberFromDisk;
		if (RememberEmailCheckBox.IsValid())
		{
			RememberEmailCheckBox->SetIsChecked(bRememberEmail ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
		}
		if (EmailInput.IsValid())
		{
			const FString EmailToShow =
				(bRememberEmail && !SavedEmailFromDisk.IsEmpty()) ? SavedEmailFromDisk : FString();
			EmailInput->SetText(FText::FromString(EmailToShow));
		}
	}
	else if (EmailInput.IsValid())
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
		SetStatus(TEXT("로그인 UI 초기화에 실패했습니다. 다시 실행해주세요."), AuthStyle::C::Error);
		return FReply::Handled();
	}
	if (LoginButton.IsValid() && !LoginButton->IsEnabled())
	{
		return FReply::Handled();
	}

	const FString Email = SLoginPanel_Local::NormalizeEmail(EmailInput->GetText().ToString());
	const FString Password = PasswordInput->GetText().ToString();

	if (Email.IsEmpty() || Password.IsEmpty())
	{
		SetStatus(TEXT("이메일과 비밀번호를 모두 입력해주세요."), AuthStyle::C::Error);
		return FReply::Handled();
	}

	if (!SLoginPanel_Local::IsLikelyValidEmail(Email))
	{
		SetStatus(TEXT("유효한 이메일 형식이 아닙니다."), AuthStyle::C::Error);
		return FReply::Handled();
	}
	if (Password.Len() < 8 || Password.Len() > 128)
	{
		SetStatus(TEXT("비밀번호는 8자 이상 128자 이하여야 합니다."), AuthStyle::C::Error);
		return FReply::Handled();
	}

	SetStatus(TEXT("로그인 중..."), AuthStyle::C::Body);
	SetLoading(true);
	LastSubmittedLoginEmail = Email;

	if (GEngine && GEngine->GameViewport)
	{
		if (const UWorld* World = GEngine->GameViewport->GetWorld())
		{
			if (const UGameInstance* GI = World->GetGameInstance())
			{
				if (UClientNetSubsystem* Net = GI->GetSubsystem<UClientNetSubsystem>())
				{
					Net->RequestLogin(Email, Password);
					return FReply::Handled();
				}
			}
		}
	}

	SetLoading(false);
	SetStatus(TEXT("네트워크 초기화에 실패했습니다. 잠시 후 다시 시도해주세요."), AuthStyle::C::Error);
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

void SLoginPanel::OnRememberEmailCheckChanged(const ECheckBoxState NewState)
{
	bRememberEmail = (NewState == ECheckBoxState::Checked);
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
	if (RememberEmailCheckBox.IsValid())
	{
		RememberEmailCheckBox->SetEnabled(!bLoading);
	}
}

void SLoginPanel::HandleGatewayLoginResult(const int32 Result)
{
	SetLoading(false);

	if (Result == 0)
	{
		AuthLocalPreferences::SaveLoginRemember(bRememberEmail, LastSubmittedLoginEmail);
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
	const FString DisplayMessage = AuthErrorMapper::ResolveMessage(StatusCode, TEXT(""), Message);
	SetStatus(DisplayMessage, AuthStyle::C::Error);
}

void SLoginPanel::HandleEmailVerificationRequired(const FString& Message, const FString& Email)
{
	SetLoading(false);
	OnGoToEmailVerification.ExecuteIfBound(Message, Email, false);
}
