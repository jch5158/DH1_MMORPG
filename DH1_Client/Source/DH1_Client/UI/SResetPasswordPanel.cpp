#include "SResetPasswordPanel.h"
#include "AuthWidgetStyle.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"

void SResetPasswordPanel::Construct(const FArguments& InArgs)
{
	OnGoToLogin = InArgs._OnGoToLogin;

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
						.Text(FText::FromString(TEXT("비밀번호 찾기")))
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
						AuthStyle::MakeInput(EmailInputText, FText::FromString(TEXT("이메일")))
					]

					// Send Verify Code Button
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
					[
						AuthStyle::MakeSecondaryButton(
							FText::FromString(TEXT("인증 코드 전송")),
							FOnClicked::CreateSP(this, &SResetPasswordPanel::OnSendVerifyCodeClicked))
					]

					// Verify Code
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
					[
						AuthStyle::MakeInput(VerifyCodeInput, FText::FromString(TEXT("인증 코드")))
					]

					// New Password
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
					[
						AuthStyle::MakeInput(PasswordInput, FText::FromString(TEXT("새 비밀번호")), true)
					]

					// New Password Confirm
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 28)
					[
						AuthStyle::MakeInput(PasswordConfirmInput, FText::FromString(TEXT("새 비밀번호 확인")), true)
					]

					// Password Confirm Button
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
					[
						AuthStyle::MakePrimaryButton(
							FText::FromString(TEXT("비밀번호 변경")),
							FOnClicked::CreateSP(this, &SResetPasswordPanel::OnPasswordConfirmClicked))
					]

					// Back to Login Button
					+ SVerticalBox::Slot().AutoHeight()
					[
						AuthStyle::MakeSecondaryButton(
							FText::FromString(TEXT("로그인으로 돌아가기")),
							FOnClicked::CreateSP(this, &SResetPasswordPanel::OnBackToLoginClicked))
					]
				]
			]
		]
	];
}

void SResetPasswordPanel::ResetPanel(const FString& InStatusText, const FString& Email)
{
	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(InStatusText));
	}

	if (EmailInputText.IsValid())
	{
		EmailInputText->SetText(FText::FromString(Email));
	}

	if (VerifyCodeInput.IsValid())
	{
		VerifyCodeInput->SetText(FText::GetEmpty());
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

FReply SResetPasswordPanel::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Enter)
	{
		if (PasswordConfirmInput.IsValid() && PasswordConfirmInput->HasKeyboardFocus())
		{
			OnPasswordConfirmClicked();
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

FReply SResetPasswordPanel::OnSendVerifyCodeClicked()
{
	if (!EmailInputText.IsValid())
	{
		return FReply::Handled();
	}

	const FString Email = EmailInputText->GetText().ToString();
	if (Email.IsEmpty())
	{
		SetStatus(TEXT("이메일을 입력해주세요."), AuthStyle::C::Error);
		return FReply::Handled();
	}

	SetStatus(TEXT("인증 코드 전송 중..."), AuthStyle::C::Body);

	const TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("Email"), Email);

	FString JsonString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(TEXT("https://localhost:5001/api/auth/forgot-password"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(JsonString);

	Request->OnProcessRequestComplete().BindRaw(this, &SResetPasswordPanel::OnSendCodeResponseReceived);
	Request->ProcessRequest();

	return FReply::Handled();
}

FReply SResetPasswordPanel::OnPasswordConfirmClicked()
{
	if (!EmailInputText.IsValid() || !VerifyCodeInput.IsValid() || !PasswordInput.IsValid() || !PasswordConfirmInput.IsValid())
	{
		return FReply::Handled();
	}

	const FString Email = EmailInputText->GetText().ToString();
	const FString VerifyCode = VerifyCodeInput->GetText().ToString();
	const FString ResetPassword = PasswordInput->GetText().ToString();
	const FString ResetPasswordConfirm = PasswordConfirmInput->GetText().ToString();

	if (Email.IsEmpty() || VerifyCode.IsEmpty() || ResetPassword.IsEmpty() || ResetPasswordConfirm.IsEmpty())
	{
		SetStatus(TEXT("모든 항목을 입력해주세요."), AuthStyle::C::Error);
		return FReply::Handled();
	}

	if (ResetPassword != ResetPasswordConfirm)
	{
		SetStatus(TEXT("새로운 비밀번호가 일치하지 않습니다."), AuthStyle::C::Error);
		return FReply::Handled();
	}

	SetStatus(TEXT("비밀번호 변경 중..."), AuthStyle::C::Body);

	const TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("Email"), Email);
	JsonObject->SetStringField(TEXT("VerifyCode"), VerifyCode);
	JsonObject->SetStringField(TEXT("ResetPassword"), ResetPassword);

	FString JsonString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(TEXT("https://localhost:5001/api/auth/reset-password"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(JsonString);

	Request->OnProcessRequestComplete().BindRaw(this, &SResetPasswordPanel::OnResetPasswordResponseReceived);
	Request->ProcessRequest();

	return FReply::Handled();
}

FReply SResetPasswordPanel::OnBackToLoginClicked()
{
	OnGoToLogin.ExecuteIfBound(TEXT(""), TEXT(""));
	return FReply::Handled();
}

void SResetPasswordPanel::OnSendCodeResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		SetStatus(TEXT("서버와 연결에 실패했습니다."), AuthStyle::C::Error);
		return;
	}

	FString ServerMessage = TEXT("서버 처리 중 오류가 발생했습니다.");

	const FString JsonString = Response->GetContentAsString();
	TSharedPtr<FJsonObject> JsonObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		JsonObject->TryGetStringField(TEXT("message"), ServerMessage);
	}

	const int32 ResponseCode = Response->GetResponseCode();
	if (ResponseCode == 200)
	{
		SetStatus(ServerMessage, AuthStyle::C::Success);
	}
	else
	{
		SetStatus(ServerMessage, AuthStyle::C::Error);
	}
}

void SResetPasswordPanel::OnResetPasswordResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		SetStatus(TEXT("서버와의 연결에 실패했습니다."), AuthStyle::C::Error);
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
		OnGoToLogin.ExecuteIfBound(ResMessage, ResEmail);
	}
	else
	{
		SetStatus(ResMessage, AuthStyle::C::Error);
	}
}

void SResetPasswordPanel::SetStatus(const FString& Text, const FLinearColor& Color)
{
	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(Text));
		StatusText->SetColorAndOpacity(FSlateColor(Color));
	}
}
