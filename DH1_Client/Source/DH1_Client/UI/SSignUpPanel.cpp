#include "SSignUpPanel.h"
#include "AuthWidgetStyle.h"
#include "AuthErrorMapper.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Network/Subsystem/ClientNetSubsystem.h"
#include "Serialization/JsonSerializer.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"

namespace SSignUpPanel_Local
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

	FString ExtractServerMessage(const FHttpResponsePtr& Response)
	{
		if (!Response.IsValid())
		{
			return TEXT("서버와 연결할 수 없습니다.");
		}

		FString Message = TEXT("");
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

			const TSharedPtr<FJsonObject>* ErrorsObject = nullptr;
			if (JsonObject->TryGetObjectField(TEXT("errors"), ErrorsObject) && ErrorsObject != nullptr && ErrorsObject->IsValid())
			{
				for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*ErrorsObject)->Values)
				{
					const TArray<TSharedPtr<FJsonValue>>* ErrorsArray = nullptr;
					if (Pair.Value.IsValid() && Pair.Value->TryGetArray(ErrorsArray) && ErrorsArray != nullptr && ErrorsArray->Num() > 0)
					{
						const FString FirstError = (*ErrorsArray)[0].IsValid() ? (*ErrorsArray)[0]->AsString() : TEXT("");
						if (!FirstError.IsEmpty())
						{
							return FirstError;
						}
					}
				}
			}
		}

		return AuthErrorMapper::MessageForStatus(Response->GetResponseCode());
	}
}

void SSignUpPanel::Construct(const FArguments& InArgs)
{
	OnGoToLogin = InArgs._OnGoToLogin;
	OnGoToEmailVerification = InArgs._OnGoToEmailVerification;

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
	SetLoading(false);

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
		SetStatus(TEXT("회원가입 UI 초기화에 실패했습니다. 다시 실행해주세요."), AuthStyle::C::Error);
		return FReply::Handled();
	}
	if (SignUpButton.IsValid() && !SignUpButton->IsEnabled())
	{
		return FReply::Handled();
	}

	const FString Email = SSignUpPanel_Local::NormalizeEmail(EmailInput->GetText().ToString());
	const FString Password = PasswordInput->GetText().ToString();
	const FString PasswordConfirm = PasswordConfirmInput->GetText().ToString();

	if (Email.IsEmpty() || Password.IsEmpty() || PasswordConfirm.IsEmpty())
	{
		SetStatus(TEXT("모든 필드를 입력해주세요."), AuthStyle::C::Error);
		return FReply::Handled();
	}

	if (!SSignUpPanel_Local::IsLikelyValidEmail(Email))
	{
		SetStatus(TEXT("유효한 이메일 형식이 아닙니다."), AuthStyle::C::Error);
		return FReply::Handled();
	}
	if (Password.Len() < 8 || Password.Len() > 128)
	{
		SetStatus(TEXT("비밀번호는 8자 이상 128자 이하여야 합니다."), AuthStyle::C::Error);
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

	FString JsonString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(SSignUpPanel_Local::ResolveAuthEndpointUrl(TEXT("register")));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(JsonString);

	const TWeakPtr<SSignUpPanel> WeakPanel = StaticCastSharedRef<SSignUpPanel>(AsShared());
	Request->OnProcessRequestComplete().BindLambda(
		[WeakPanel](FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bInWasSuccessful)
		{
			if (const TSharedPtr<SSignUpPanel> Pinned = WeakPanel.Pin())
			{
				Pinned->OnSignUpResponseReceived(InRequest, InResponse, bInWasSuccessful);
			}
		});
	if (!Request->ProcessRequest())
	{
		SetLoading(false);
		SetStatus(TEXT("회원가입 요청 전송에 실패했습니다."), AuthStyle::C::Error);
	}

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
	FString ResCode;

	const FString JsonString = Response->GetContentAsString();
	TSharedPtr<FJsonObject> JsonObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		JsonObject->TryGetStringField(TEXT("email"), ResEmail);
		JsonObject->TryGetStringField(TEXT("message"), ResMessage);
		JsonObject->TryGetStringField(TEXT("code"), ResCode);
	}

	const int32 ResponseCode = Response->GetResponseCode();
	AuthErrorMapper::ReportUnknownCodeIfAny(ResCode, TEXT("RegisterResponse"));
	if (ResponseCode == 200)
	{
		// 서버가 회원가입 처리 시 이미 인증 메일을 발송함 — 자동 재전송 시 쿨다운(30초)에 걸릴 수 있음
		OnGoToEmailVerification.ExecuteIfBound(TEXT("이메일 인증이 필요합니다."), ResEmail, true);
	}
	else if (ResponseCode == 403 && AuthErrorMapper::IsEmailUnverifiedCode(ResCode))
	{
		const FString FinalMessage = AuthErrorMapper::ResolveMessage(ResponseCode, ResCode, ResMessage);
		OnGoToEmailVerification.ExecuteIfBound(FinalMessage, ResEmail, false);
	}
	else
	{
		if (ResMessage.IsEmpty() || ResMessage == TEXT("서버 처리 중 오류가 발생했습니다."))
		{
			ResMessage = SSignUpPanel_Local::ExtractServerMessage(Response);
		}
		ResMessage = AuthErrorMapper::ResolveMessage(ResponseCode, ResCode, ResMessage);
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
