#include "SResetPasswordPanel.h"
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

namespace SResetPasswordPanel_Local
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

void SResetPasswordPanel::Construct(const FArguments& InArgs)
{
	OnGoToLogin = InArgs._OnGoToLogin;

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
						SAssignNew(SendVerifyCodeButton, SButton)
						.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
						.ButtonColorAndOpacity(AuthStyle::C::Secondary)
						.OnClicked(FOnClicked::CreateSP(this, &SResetPasswordPanel::OnSendVerifyCodeClicked))
						.ContentPadding(FMargin(0.0f, 12.0f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("인증 코드 전송")))
							.Font(AuthStyle::SmallFont())
							.ColorAndOpacity(AuthStyle::C::Body)
							.Justification(ETextJustify::Center)
						]
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
						SAssignNew(ResetPasswordButton, SButton)
						.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
						.ButtonColorAndOpacity(AuthStyle::C::Primary)
						.OnClicked(FOnClicked::CreateSP(this, &SResetPasswordPanel::OnPasswordConfirmClicked))
						.ContentPadding(FMargin(0.0f, 12.0f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("비밀번호 변경")))
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
							FOnClicked::CreateSP(this, &SResetPasswordPanel::OnBackToLoginClicked))
					]
				]
			]
		]
	];
}

void SResetPasswordPanel::ResetPanel(const FString& InStatusText, const FString& Email)
{
	SetSendCodeLoading(false);
	SetResetLoading(false);

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
		SetStatus(TEXT("비밀번호 찾기 UI 초기화에 실패했습니다. 다시 실행해주세요."), AuthStyle::C::Error);
		return FReply::Handled();
	}
	if (bSendCodeInFlight)
	{
		return FReply::Handled();
	}

	const FString Email = SResetPasswordPanel_Local::NormalizeEmail(EmailInputText->GetText().ToString());
	if (Email.IsEmpty())
	{
		SetStatus(TEXT("이메일을 입력해주세요."), AuthStyle::C::Error);
		return FReply::Handled();
	}
	if (!SResetPasswordPanel_Local::IsLikelyValidEmail(Email))
	{
		SetStatus(TEXT("유효한 이메일 형식이 아닙니다."), AuthStyle::C::Error);
		return FReply::Handled();
	}

	SetStatus(TEXT("인증 코드 전송 중..."), AuthStyle::C::Body);
	SetSendCodeLoading(true);

	const TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("Email"), Email);

	FString JsonString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(SResetPasswordPanel_Local::ResolveAuthEndpointUrl(TEXT("forgot-password")));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(JsonString);

	const TWeakPtr<SResetPasswordPanel> WeakPanel = StaticCastSharedRef<SResetPasswordPanel>(AsShared());
	Request->OnProcessRequestComplete().BindLambda(
		[WeakPanel](FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bInWasSuccessful)
		{
			if (const TSharedPtr<SResetPasswordPanel> Pinned = WeakPanel.Pin())
			{
				Pinned->OnSendCodeResponseReceived(InRequest, InResponse, bInWasSuccessful);
			}
		});
	if (!Request->ProcessRequest())
	{
		SetSendCodeLoading(false);
		SetStatus(TEXT("인증 코드 전송 요청 전송에 실패했습니다."), AuthStyle::C::Error);
	}

	return FReply::Handled();
}

FReply SResetPasswordPanel::OnPasswordConfirmClicked()
{
	if (!EmailInputText.IsValid() || !VerifyCodeInput.IsValid() || !PasswordInput.IsValid() || !PasswordConfirmInput.IsValid())
	{
		SetStatus(TEXT("비밀번호 찾기 UI 초기화에 실패했습니다. 다시 실행해주세요."), AuthStyle::C::Error);
		return FReply::Handled();
	}
	if (bResetInFlight)
	{
		return FReply::Handled();
	}

	const FString Email = SResetPasswordPanel_Local::NormalizeEmail(EmailInputText->GetText().ToString());
	const FString VerifyCode = VerifyCodeInput->GetText().ToString();
	const FString ResetPassword = PasswordInput->GetText().ToString();
	const FString ResetPasswordConfirm = PasswordConfirmInput->GetText().ToString();

	if (Email.IsEmpty() || VerifyCode.IsEmpty() || ResetPassword.IsEmpty() || ResetPasswordConfirm.IsEmpty())
	{
		SetStatus(TEXT("모든 항목을 입력해주세요."), AuthStyle::C::Error);
		return FReply::Handled();
	}
	if (!SResetPasswordPanel_Local::IsLikelyValidEmail(Email))
	{
		SetStatus(TEXT("유효한 이메일 형식이 아닙니다."), AuthStyle::C::Error);
		return FReply::Handled();
	}
	if (!SResetPasswordPanel_Local::IsSixDigitCode(VerifyCode))
	{
		SetStatus(TEXT("인증번호는 6자리 숫자여야 합니다."), AuthStyle::C::Error);
		return FReply::Handled();
	}
	if (ResetPassword.Len() < 8 || ResetPassword.Len() > 128)
	{
		SetStatus(TEXT("새 비밀번호는 8자 이상 128자 이하여야 합니다."), AuthStyle::C::Error);
		return FReply::Handled();
	}

	if (ResetPassword != ResetPasswordConfirm)
	{
		SetStatus(TEXT("새로운 비밀번호가 일치하지 않습니다."), AuthStyle::C::Error);
		return FReply::Handled();
	}

	SetStatus(TEXT("비밀번호 변경 중..."), AuthStyle::C::Body);
	SetResetLoading(true);

	const TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("Email"), Email);
	JsonObject->SetStringField(TEXT("VerifyCode"), VerifyCode);
	JsonObject->SetStringField(TEXT("ResetPassword"), ResetPassword);

	FString JsonString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(SResetPasswordPanel_Local::ResolveAuthEndpointUrl(TEXT("reset-password")));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(JsonString);

	const TWeakPtr<SResetPasswordPanel> WeakPanel = StaticCastSharedRef<SResetPasswordPanel>(AsShared());
	Request->OnProcessRequestComplete().BindLambda(
		[WeakPanel](FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bInWasSuccessful)
		{
			if (const TSharedPtr<SResetPasswordPanel> Pinned = WeakPanel.Pin())
			{
				Pinned->OnResetPasswordResponseReceived(InRequest, InResponse, bInWasSuccessful);
			}
		});
	if (!Request->ProcessRequest())
	{
		SetResetLoading(false);
		SetStatus(TEXT("비밀번호 변경 요청 전송에 실패했습니다."), AuthStyle::C::Error);
	}

	return FReply::Handled();
}

FReply SResetPasswordPanel::OnBackToLoginClicked()
{
	OnGoToLogin.ExecuteIfBound(TEXT(""), TEXT(""));
	return FReply::Handled();
}

void SResetPasswordPanel::OnSendCodeResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	SetSendCodeLoading(false);

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
		if (ServerMessage.IsEmpty() || ServerMessage == TEXT("서버 처리 중 오류가 발생했습니다."))
		{
			ServerMessage = SResetPasswordPanel_Local::ExtractServerMessage(Response);
		}
		ServerMessage = AuthErrorMapper::ResolveMessage(ResponseCode, TEXT(""), ServerMessage);
		SetStatus(ServerMessage, AuthStyle::C::Error);
	}
}

void SResetPasswordPanel::OnResetPasswordResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	SetResetLoading(false);

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
		if (ResMessage.IsEmpty() || ResMessage == TEXT("서버 처리 중 오류가 발생했습니다."))
		{
			ResMessage = SResetPasswordPanel_Local::ExtractServerMessage(Response);
		}
		ResMessage = AuthErrorMapper::ResolveMessage(ResponseCode, TEXT(""), ResMessage);
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

void SResetPasswordPanel::SetSendCodeLoading(const bool bLoading)
{
	bSendCodeInFlight = bLoading;
	if (SendVerifyCodeButton.IsValid())
	{
		SendVerifyCodeButton->SetEnabled(!bLoading);
	}
}

void SResetPasswordPanel::SetResetLoading(const bool bLoading)
{
	bResetInFlight = bLoading;
	if (ResetPasswordButton.IsValid())
	{
		ResetPasswordButton->SetEnabled(!bLoading);
	}
}
