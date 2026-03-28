#include "SEmailVerificationPanel.h"
#include "AuthWidgetStyle.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"

void SEmailVerificationPanel::Construct(const FArguments& InArgs)
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
						.Text(FText::FromString(TEXT("이메일 인증")))
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

					// Email (read-only display)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
					[
						SAssignNew(EmailText, STextBlock)
						.Font(AuthStyle::BodyFont())
						.ColorAndOpacity(AuthStyle::C::Body)
						.Justification(ETextJustify::Center)
					]

					// Verify Code
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 28)
					[
						AuthStyle::MakeInput(VerifyCodeInput, FText::FromString(TEXT("인증 코드")))
					]

					// Verify Button
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
					[
						AuthStyle::MakePrimaryButton(
							FText::FromString(TEXT("인증하기")),
							FOnClicked::CreateSP(this, &SEmailVerificationPanel::OnVerifyClicked))
					]

					// Resend Button
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
					[
						AuthStyle::MakeSecondaryButton(
							FText::FromString(TEXT("인증 코드 재전송")),
							FOnClicked::CreateSP(this, &SEmailVerificationPanel::OnResendClicked))
					]

					// Back to Login Button
					+ SVerticalBox::Slot().AutoHeight()
					[
						AuthStyle::MakeSecondaryButton(
							FText::FromString(TEXT("로그인으로 돌아가기")),
							FOnClicked::CreateSP(this, &SEmailVerificationPanel::OnBackToLoginClicked))
					]
				]
			]
		]
	];
}

void SEmailVerificationPanel::ResetPanel(const FString& InStatusText, const FString& Email)
{
	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(InStatusText));
	}

	if (EmailText.IsValid())
	{
		EmailText->SetText(FText::FromString(Email));
	}

	if (VerifyCodeInput.IsValid())
	{
		VerifyCodeInput->SetText(FText::GetEmpty());
	}
}

FReply SEmailVerificationPanel::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Enter)
	{
		if (VerifyCodeInput.IsValid() && VerifyCodeInput->HasKeyboardFocus())
		{
			OnVerifyClicked();
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

FReply SEmailVerificationPanel::OnVerifyClicked()
{
	if (!EmailText.IsValid() || !VerifyCodeInput.IsValid())
	{
		return FReply::Handled();
	}

	const FString Email = EmailText->GetText().ToString();
	if (Email.IsEmpty())
	{
		SetStatus(TEXT("이메일 인증에 문제가 발생되었습니다."), AuthStyle::C::Error);
		return FReply::Handled();
	}

	const FString Code = VerifyCodeInput->GetText().ToString();
	if (Code.IsEmpty())
	{
		SetStatus(TEXT("이메일 인증번호를 입력해주세요."), AuthStyle::C::Error);
		return FReply::Handled();
	}

	SetStatus(TEXT("인증 확인 중..."), AuthStyle::C::Body);

	const TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("Email"), Email);
	JsonObject->SetStringField(TEXT("VerifyCode"), Code);

	FString JsonString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(TEXT("https://localhost:5001/api/auth/verify-code"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(JsonString);

	Request->OnProcessRequestComplete().BindRaw(this, &SEmailVerificationPanel::OnVerifyResponseReceived);
	Request->ProcessRequest();

	return FReply::Handled();
}

FReply SEmailVerificationPanel::OnResendClicked()
{
	if (!EmailText.IsValid())
	{
		return FReply::Handled();
	}

	const FString Email = EmailText->GetText().ToString();
	if (Email.IsEmpty())
	{
		SetStatus(TEXT("이메일 인증에 문제가 발생되었습니다."), AuthStyle::C::Error);
		return FReply::Handled();
	}

	SetStatus(TEXT("인증 코드 재전송 중..."), AuthStyle::C::Body);

	const TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("Email"), Email);

	FString JsonString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(TEXT("https://localhost:5001/api/auth/send-verify-code"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(JsonString);

	Request->OnProcessRequestComplete().BindRaw(this, &SEmailVerificationPanel::OnResendResponseReceived);
	Request->ProcessRequest();

	return FReply::Handled();
}

FReply SEmailVerificationPanel::OnBackToLoginClicked()
{
	OnGoToLogin.ExecuteIfBound(TEXT(""), TEXT(""));
	return FReply::Handled();
}

void SEmailVerificationPanel::OnVerifyResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
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
		OnGoToLogin.ExecuteIfBound(ResMessage, ResEmail);
	}
	else
	{
		SetStatus(ResMessage, AuthStyle::C::Error);
	}
}

void SEmailVerificationPanel::OnResendResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		SetStatus(TEXT("서버와 연결할 수 없습니다."), AuthStyle::C::Error);
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

void SEmailVerificationPanel::SetStatus(const FString& Text, const FLinearColor& Color)
{
	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(Text));
		StatusText->SetColorAndOpacity(FSlateColor(Color));
	}
}
