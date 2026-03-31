#include "SEmailVerificationPanel.h"
#include "AuthWidgetStyle.h"
#include "AuthErrorMapper.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Network/Subsystem/ClientNetSubsystem.h"
#include "Serialization/JsonSerializer.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"

namespace SEmailVerificationPanel_Local
{
	FString ResolveAuthEndpointUrl(const FString& Path)
	{
		if (GEngine && GEngine->GameViewport)
		{
			if (const UWorld* World = GEngine->GameViewport->GetWorld())
			{
				if (const UGameInstance* GI = World->GetGameInstance())
				{
					if (const UClientNetSubsystem* Net = GI->GetSubsystem<UClientNetSubsystem>())
					{
						return FString::Printf(TEXT("%s/%s"), *Net->GetLoginServerApiBaseUrl(), *Path);
					}
				}
			}
		}
		return FString::Printf(TEXT("https://localhost:5001/api/auth/%s"), *Path);
	}

	bool IsSixDigitCode(const FString& Code)
	{
		if (Code.Len() != 6)
		{
			return false;
		}

		for (const TCHAR Ch : Code)
		{
			if (!FChar::IsDigit(Ch))
			{
				return false;
			}
		}
		return true;
	}

	FString ExtractServerMessage(const FHttpResponsePtr& Response)
	{
		if (!Response.IsValid())
		{
			return TEXT("서버와 연결할 수 없습니다.");
		}

		FString Message;
		const FString JsonString = Response->GetContentAsString();
		TSharedPtr<FJsonObject> JsonObject;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			if (JsonObject->TryGetStringField(TEXT("message"), Message) && !Message.IsEmpty())
			{
				return Message;
			}
			if (JsonObject->TryGetStringField(TEXT("detail"), Message) && !Message.IsEmpty())
			{
				return Message;
			}
			if (JsonObject->TryGetStringField(TEXT("title"), Message) && !Message.IsEmpty())
			{
				return Message;
			}
		}

		return AuthErrorMapper::MessageForStatus(Response->GetResponseCode());
	}
}

void SEmailVerificationPanel::Construct(const FArguments& InArgs)
{
	OnGoToLogin = InArgs._OnGoToLogin;
	static const FButtonStyle PrimaryStyle = AuthStyle::PrimaryButtonStyle();
	static const FButtonStyle SecondaryStyle = AuthStyle::SecondaryButtonStyle();

	ChildSlot
	[
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
						.AutoWrapText(true)
						.WrapTextAt(590.0f)
					]

					// Email (read-only display)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
					[
						SAssignNew(EmailText, STextBlock)
						.Font(AuthStyle::BodyFont())
						.ColorAndOpacity(AuthStyle::C::Body)
						.Justification(ETextJustify::Center)
						.AutoWrapText(true)
						.WrapTextAt(590.0f)
					]

					// Verify Code
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 28)
					[
						AuthStyle::MakeInput(VerifyCodeInput, FText::FromString(TEXT("인증 코드")))
					]

					// Verify Button
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
					[
						SAssignNew(VerifyButton, SButton)
						.ButtonStyle(&PrimaryStyle)
						.ButtonColorAndOpacity(AuthStyle::C::Primary)
						.OnClicked(FOnClicked::CreateSP(this, &SEmailVerificationPanel::OnVerifyClicked))
						.ContentPadding(FMargin(0.0f, 12.0f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("인증하기")))
							.Font(AuthStyle::ButtonFont())
							.ColorAndOpacity(AuthStyle::C::BtnText)
							.Justification(ETextJustify::Center)
						]
					]

					// Resend Button
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
					[
						SAssignNew(ResendButton, SButton)
						.ButtonStyle(&SecondaryStyle)
						.ButtonColorAndOpacity(AuthStyle::C::Secondary)
						.OnClicked(FOnClicked::CreateSP(this, &SEmailVerificationPanel::OnResendClicked))
						.ContentPadding(FMargin(0.0f, 12.0f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("인증 코드 재전송")))
							.Font(AuthStyle::SmallFont())
							.ColorAndOpacity(AuthStyle::C::Body)
							.Justification(ETextJustify::Center)
						]
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

void SEmailVerificationPanel::ResetPanel(const FString& InStatusText, const FString& Email, const bool bSkipAutoSendVerifyCode)
{
	SetVerifyLoading(false);
	SetResendLoading(false);

	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(InStatusText));
		StatusText->SetColorAndOpacity(FSlateColor(AuthStyle::C::Dim));
	}

	if (EmailText.IsValid())
	{
		EmailText->SetText(FText::FromString(Email));
	}

	if (VerifyCodeInput.IsValid())
	{
		VerifyCodeInput->SetText(FText::GetEmpty());
	}

	const FString Trimmed = Email.TrimStartAndEnd();
	if (!bSkipAutoSendVerifyCode && !Trimmed.IsEmpty())
	{
		FString PendingStatus;
		if (InStatusText.IsEmpty())
		{
			PendingStatus = TEXT("인증 코드를 이메일로 보내는 중...");
		}
		else
		{
			PendingStatus = FString::Printf(TEXT("%s\n\n%s"), *InStatusText, TEXT("인증 코드를 이메일로 보내는 중..."));
		}
		StartSendVerifyCode(false, PendingStatus);
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
		SetStatus(TEXT("인증 UI 초기화에 실패했습니다. 다시 실행해주세요."), AuthStyle::C::Error);
		return FReply::Handled();
	}
	if (bVerifyRequestInFlight)
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
	if (!SEmailVerificationPanel_Local::IsSixDigitCode(Code))
	{
		SetStatus(TEXT("인증번호는 6자리 숫자여야 합니다."), AuthStyle::C::Error);
		return FReply::Handled();
	}

	SetStatus(TEXT("인증 확인 중..."), AuthStyle::C::Body);
	SetVerifyLoading(true);

	const TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("Email"), Email);
	JsonObject->SetStringField(TEXT("VerifyCode"), Code);

	FString JsonString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(SEmailVerificationPanel_Local::ResolveAuthEndpointUrl(TEXT("verify-code")));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(JsonString);

	const TWeakPtr<SEmailVerificationPanel> WeakPanel = StaticCastSharedRef<SEmailVerificationPanel>(AsShared());
	Request->OnProcessRequestComplete().BindLambda(
		[WeakPanel](FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bInWasSuccessful)
		{
			if (const TSharedPtr<SEmailVerificationPanel> Pinned = WeakPanel.Pin())
			{
				Pinned->OnVerifyResponseReceived(InRequest, InResponse, bInWasSuccessful);
			}
		});
	if (!Request->ProcessRequest())
	{
		SetVerifyLoading(false);
		SetStatus(TEXT("인증 확인 요청 전송에 실패했습니다."), AuthStyle::C::Error);
	}

	return FReply::Handled();
}

void SEmailVerificationPanel::StartSendVerifyCode(const bool bManualResend, const FString& PendingStatus)
{
	if (!EmailText.IsValid())
	{
		return;
	}
	if (bResendRequestInFlight)
	{
		return;
	}

	const FString Email = EmailText->GetText().ToString().TrimStartAndEnd();
	if (Email.IsEmpty())
	{
		SetStatus(TEXT("이메일 인증에 문제가 발생되었습니다."), AuthStyle::C::Error);
		return;
	}

	if (PendingStatus.IsEmpty())
	{
		SetStatus(bManualResend ? TEXT("인증 코드 재전송 중...") : TEXT("인증 코드를 이메일로 보내는 중..."), AuthStyle::C::Body);
	}
	else
	{
		SetStatus(PendingStatus, AuthStyle::C::Body);
	}
	SetResendLoading(true);

	const TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("Email"), Email);

	FString JsonString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(SEmailVerificationPanel_Local::ResolveAuthEndpointUrl(TEXT("send-verify-code")));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(JsonString);

	const TWeakPtr<SEmailVerificationPanel> WeakPanel = StaticCastSharedRef<SEmailVerificationPanel>(AsShared());
	Request->OnProcessRequestComplete().BindLambda(
		[WeakPanel](FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bInWasSuccessful)
		{
			if (const TSharedPtr<SEmailVerificationPanel> Pinned = WeakPanel.Pin())
			{
				Pinned->OnResendResponseReceived(InRequest, InResponse, bInWasSuccessful);
			}
		});
	if (!Request->ProcessRequest())
	{
		SetResendLoading(false);
		SetStatus(
			bManualResend ? TEXT("인증 코드 재전송 요청 전송에 실패했습니다.") : TEXT("인증 코드 발송 요청 전송에 실패했습니다."),
			AuthStyle::C::Error);
	}
}

FReply SEmailVerificationPanel::OnResendClicked()
{
	StartSendVerifyCode(true);
	return FReply::Handled();
}

FReply SEmailVerificationPanel::OnBackToLoginClicked()
{
	OnGoToLogin.ExecuteIfBound(TEXT(""), TEXT(""));
	return FReply::Handled();
}

void SEmailVerificationPanel::OnVerifyResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	SetVerifyLoading(false);

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
		if (ResMessage.IsEmpty() || ResMessage == TEXT("서버 처리 중 오류가 발생했습니다."))
		{
			ResMessage = SEmailVerificationPanel_Local::ExtractServerMessage(Response);
		}
		ResMessage = AuthErrorMapper::ResolveMessage(ResponseCode, TEXT(""), ResMessage);
		SetStatus(ResMessage, AuthStyle::C::Error);
	}
}

void SEmailVerificationPanel::OnResendResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	SetResendLoading(false);

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
		if (ServerMessage.IsEmpty() || ServerMessage == TEXT("서버 처리 중 오류가 발생했습니다."))
		{
			ServerMessage = SEmailVerificationPanel_Local::ExtractServerMessage(Response);
		}
		ServerMessage = AuthErrorMapper::ResolveMessage(ResponseCode, TEXT(""), ServerMessage);
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

void SEmailVerificationPanel::SetVerifyLoading(const bool bLoading)
{
	bVerifyRequestInFlight = bLoading;
	if (VerifyButton.IsValid())
	{
		VerifyButton->SetEnabled(!bLoading);
	}
}

void SEmailVerificationPanel::SetResendLoading(const bool bLoading)
{
	bResendRequestInFlight = bLoading;
	if (ResendButton.IsValid())
	{
		ResendButton->SetEnabled(!bLoading);
	}
}
