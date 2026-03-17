#include "EmailVerificationWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

void UEmailVerificationWidget::ResetVerificationWidget(const FString& StatusText, const FString& Email) const
{
	if (StatusTextMessage)
	{
		StatusTextMessage->SetText(FText::FromString(StatusText));
	}

	if (EmailText)
	{
		EmailText->SetText(FText::FromString(Email));
	}

	if (VerifyCodeInput)
	{
		VerifyCodeInput->SetText(FText::GetEmpty());
	}
}

void UEmailVerificationWidget::SetStatusTextMessage(const FString& StatusText) const
{
	if (StatusTextMessage)
	{
		StatusTextMessage->SetText(FText::FromString(StatusText));
	}
}

void UEmailVerificationWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (VerifyButton)
	{
		VerifyButton->OnClicked.AddDynamic(this, &UEmailVerificationWidget::OnVerifyButtonClicked);
	}

	if (ResendButton)
	{
		ResendButton->OnClicked.AddDynamic(this, &UEmailVerificationWidget::OnResendButtonClicked);
	}

	if (BackToLoginButton)
	{
		BackToLoginButton->OnClicked.AddDynamic(this, &UEmailVerificationWidget::OnBackToLoginButtonClicked);
	}
}

void UEmailVerificationWidget::OnVerifyButtonClicked()
{
	const FString Email = EmailText->GetText().ToString();
	if (Email.IsEmpty())
	{
		SetStatusTextMessage(TEXT("이메일 인증에 문제가 발생되었습니다."));
		return;
	}

	const FString Code = VerifyCodeInput->GetText().ToString();
	if (Code.IsEmpty())
	{
		SetStatusTextMessage(TEXT("이메일 인증번호를 입력해주세요."));
		return;
	}

	if (StatusTextMessage)
	{
		SetStatusTextMessage("");
	}

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

	Request->OnProcessRequestComplete().BindUObject(this, &UEmailVerificationWidget::OnVerifyResponseReceived);
	Request->ProcessRequest();
}

void UEmailVerificationWidget::OnResendButtonClicked()
{
	const FString Email = EmailText->GetText().ToString();
	if (Email.IsEmpty())
	{
		SetStatusTextMessage(TEXT("이메일 인증에 문제가 발생되었습니다."));
		return;
	}

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

	Request->OnProcessRequestComplete().BindUObject(this, &UEmailVerificationWidget::OnResendResponseReceived);
	Request->ProcessRequest();
}

void UEmailVerificationWidget::OnBackToLoginButtonClicked()
{
	if (OnGoToLogin.IsBound())
	{
		OnGoToLogin.Broadcast("", "");
	}
}

void UEmailVerificationWidget::OnVerifyResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		SetStatusTextMessage(TEXT("서버와 연결할 수 없습니다."));
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
	if (ResponseCode == 200) // 인증 성공
	{
		if (OnGoToLogin.IsBound())
		{
			OnGoToLogin.Broadcast(ResMessage, ResEmail);
		}
	}
	else
	{
		SetStatusTextMessage(ResMessage);
	}
}

void UEmailVerificationWidget::OnResendResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		SetStatusTextMessage(TEXT("서버와 연결할 수 없습니다."));
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

	SetStatusTextMessage(ServerMessage);
}
