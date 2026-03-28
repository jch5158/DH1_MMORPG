#include "SSignUpPanel.h"
#include "AuthWidgetStyle.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"

void SSignUpPanel::Construct(const FArguments& InArgs)
{
	OnGoToLogin = InArgs._OnGoToLogin;
	OnGoToEmailVerification = InArgs._OnGoToEmailVerification;

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(AuthStyle::C::CardBorder)
		.Padding(FMargin(1.0f))
		[
			SNew(SBox)
			.WidthOverride(720.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(AuthStyle::C::CardBg)
				.Padding(FMargin(64.0f, 64.0f))
				[
					SNew(SVerticalBox)

					// Title
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 32)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("회원가입")))
						.Font(AuthStyle::TitleFont())
						.ColorAndOpacity(AuthStyle::C::Title)
						.Justification(ETextJustify::Center)
					]

					// Status
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 18)
					[
						SAssignNew(StatusText, STextBlock)
						.Font(AuthStyle::SmallFont())
						.ColorAndOpacity(AuthStyle::C::Dim)
						.Justification(ETextJustify::Center)
					]

					// Email
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
					[
						AuthStyle::MakeInput(EmailInput, FText::FromString(TEXT("이메일")))
					]

					// Password
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
					[
						AuthStyle::MakeInput(PasswordInput, FText::FromString(TEXT("비밀번호")), true)
					]

					// Password Confirm
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 28)
					[
						AuthStyle::MakeInput(PasswordConfirmInput, FText::FromString(TEXT("비밀번호 확인")), true)
					]

					// SignUp Button
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
					[
						SAssignNew(SignUpButton, SButton)
						.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
						.ButtonColorAndOpacity(AuthStyle::C::Primary)
						.OnClicked(FOnClicked::CreateSP(this, &SSignUpPanel::OnSignUpClicked))
						.ContentPadding(FMargin(0.0f, 14.0f))
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("회원가입")))
							.Font(AuthStyle::ButtonFont())
							.ColorAndOpacity(AuthStyle::C::BtnText)
							.Justification(ETextJustify::Center)
						]
					]

					// Back to Login Button
					+ SVerticalBox::Slot().AutoHeight()
					[
						AuthStyle::MakeSecondaryButton(
							FText::FromString(TEXT("로그인으로 돌아가기")),
							FOnClicked::CreateSP(this, &SSignUpPanel::OnBackLoginClicked))
					]
				]
			]
		]
	];
}

void SSignUpPanel::ResetPanel()
{
	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::GetEmpty());
	}

	if (EmailInput.IsValid())
	{
		EmailInput->SetText(FText::GetEmpty());
	}

	if (PasswordInput.IsValid())
	{
		PasswordInput->SetText(FText::GetEmpty());
	}

	if (PasswordConfirmInput.IsValid())
	{
		PasswordConfirmInput->SetText(FText::GetEmpty());
	}
}

FReply SSignUpPanel::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Enter)
	{
		if (EmailInput.IsValid() && EmailInput->HasKeyboardFocus() && PasswordInput.IsValid())
		{
			FSlateApplication::Get().SetKeyboardFocus(PasswordInput);
			return FReply::Handled();
		}

		if (PasswordInput.IsValid() && PasswordInput->HasKeyboardFocus() && PasswordConfirmInput.IsValid())
		{
			FSlateApplication::Get().SetKeyboardFocus(PasswordConfirmInput);
			return FReply::Handled();
		}

		if (PasswordConfirmInput.IsValid() && PasswordConfirmInput->HasKeyboardFocus())
		{
			OnSignUpClicked();
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

FReply SSignUpPanel::OnSignUpClicked()
{
	if (!EmailInput.IsValid() || !PasswordInput.IsValid() || !PasswordConfirmInput.IsValid())
	{
		return FReply::Handled();
	}

	const FString Email = EmailInput->GetText().ToString();
	const FString Password = PasswordInput->GetText().ToString();
	const FString PasswordConfirm = PasswordConfirmInput->GetText().ToString();

	if (Email.IsEmpty() || Password.IsEmpty() || PasswordConfirm.IsEmpty())
	{
		SetStatus(TEXT("모든 필드를 입력해주세요."), AuthStyle::C::Error);
		return FReply::Handled();
	}

	if (!Email.Contains(TEXT("@")) || !Email.Contains(TEXT(".")))
	{
		SetStatus(TEXT("유효한 이메일 형식이 아닙니다."), AuthStyle::C::Error);
		return FReply::Handled();
	}

	if (Password != PasswordConfirm)
	{
		SetStatus(TEXT("비밀번호가 일치하지 않습니다."), AuthStyle::C::Error);
		return FReply::Handled();
	}

	SetStatus(TEXT("회원가입 처리 중..."), AuthStyle::C::Body);
	SetLoading(true);

	const TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("Email"), Email);
	JsonObject->SetStringField(TEXT("Password"), Password);
	JsonObject->SetStringField(TEXT("PasswordConfirm"), PasswordConfirm);

	FString JsonString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(TEXT("https://localhost:5001/api/auth/register"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(JsonString);

	Request->OnProcessRequestComplete().BindRaw(this, &SSignUpPanel::OnSignUpResponseReceived);
	Request->ProcessRequest();

	return FReply::Handled();
}

FReply SSignUpPanel::OnBackLoginClicked()
{
	OnGoToLogin.ExecuteIfBound(TEXT(""), TEXT(""));
	return FReply::Handled();
}

void SSignUpPanel::OnSignUpResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	SetLoading(false);

	if (!bWasSuccessful || !Response.IsValid())
	{
		SetStatus(TEXT("서버와 연결할 수 없습니다."), AuthStyle::C::Error);
		return;
	}

	FString ResEmail = TEXT("");
	FString ResMessage = TEXT("서버 처리 중 오류가 발생했습니다.");

	const FString JsonString = Response->GetContentAsString();
	TSharedPtr<FJsonObject> JsonObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		JsonObject->TryGetStringField(TEXT("email"), ResEmail);
		JsonObject->TryGetStringField(TEXT("message"), ResMessage);
	}

	const int32 ResponseCode = Response->GetResponseCode();
	if (ResponseCode == 200)
	{
		OnGoToEmailVerification.ExecuteIfBound(TEXT("이메일 인증이 필요합니다."), ResEmail);
	}
	else
	{
		SetStatus(ResMessage, AuthStyle::C::Error);
	}
}

void SSignUpPanel::SetStatus(const FString& Text, const FLinearColor& Color)
{
	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(Text));
		StatusText->SetColorAndOpacity(FSlateColor(Color));
	}
}

void SSignUpPanel::SetLoading(const bool bLoading)
{
	if (SignUpButton.IsValid())
	{
		SignUpButton->SetEnabled(!bLoading);
	}
}
