#include "ResetPasswordWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonSerializer.h"

void UResetPasswordWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (StatusTextMessage)
    {
        StatusTextMessage->SetText(FText::GetEmpty());
    }

    if (SendVerifyCodeButton)
    {
        SendVerifyCodeButton->OnClicked.AddDynamic(this, &UResetPasswordWidget::OnSendVerifyCodeButtonClicked);
    }
    
	if (PasswordConfirmButton)
    {
        PasswordConfirmButton->OnClicked.AddDynamic(this, &UResetPasswordWidget::OnPasswordConfirmButtonClicked);
    }

    if (BackToLoginButton)
    {
        BackToLoginButton->OnClicked.AddDynamic(this, &UResetPasswordWidget::OnBackToLoginButtonClicked);
    }
}

void UResetPasswordWidget::OnSendVerifyCodeButtonClicked()
{
    const FString Email = EmailInputText->GetText().ToString();
    if (Email.IsEmpty())
    {
        StatusTextMessage->SetText(FText::FromString(TEXT("이메일 주소에 문제가 발생되었습니다.")));
        return;
    }

    StatusTextMessage->SetText(FText::FromString(TEXT("인증번호 발송 중...")));

    // JSON 페이로드 생성
    const TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField("Email", Email);

    FString JsonString;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    // HTTP 요청 세팅
    const FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindUObject(this, &UResetPasswordWidget::OnSendCodeResponseReceived);
    Request->SetURL("https://localhost:5001/api/auth/forgot-password"); // 서버 주소에 맞게 변경
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetContentAsString(JsonString);
    Request->ProcessRequest();
}

void UResetPasswordWidget::OnPasswordConfirmButtonClicked()
{
    const FString Email = EmailInputText->GetText().ToString();
    const FString VerifyCode = VerifyCodeInput->GetText().ToString();
    const FString ResetPassword = PasswordInput->GetText().ToString();

    if (Email.IsEmpty() || VerifyCode.IsEmpty() || ResetPassword.IsEmpty())
    {
        StatusTextMessage->SetText(FText::FromString(TEXT("모든 항목을 입력해주세요.")));
        return;
    }

    const TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField("Email", Email);
    JsonObject->SetStringField("VerifyCode", VerifyCode);
    JsonObject->SetStringField("ResetPassword", ResetPassword);

    FString JsonString;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    // HTTP 요청 세팅
    FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindUObject(this, &UResetPasswordWidget::OnResetPasswordResponseReceived);
    Request->SetURL("https://localhost:5001/api/auth/reset-password"); // 서버 주소에 맞게 변경
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetContentAsString(JsonString);
    Request->ProcessRequest();
}

void UResetPasswordWidget::OnBackToLoginButtonClicked()
{
    if (OnGoToLogin.IsBound())
    {
        OnGoToLogin.Broadcast();
    }
}

void UResetPasswordWidget::OnSendCodeResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        StatusTextMessage->SetText(FText::FromString(TEXT("서버와의 연결에 실패했습니다.")));
        return;
    }

    TSharedPtr<FJsonObject> JsonObject;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

    FString ServerMessage = TEXT("응답을 처리할 수 없습니다.");
    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        JsonObject->TryGetStringField(TEXT("message"), ServerMessage);
    }

    StatusTextMessage->SetText(FText::FromString(ServerMessage));
}

void UResetPasswordWidget::OnResetPasswordResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        StatusTextMessage->SetText(FText::FromString(TEXT("서버와의 연결에 실패했습니다.")));
        return;
    }

    TSharedPtr<FJsonObject> JsonObject;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

    FString ServerMessage = TEXT("응답을 처리할 수 없습니다.");
    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        JsonObject->TryGetStringField(TEXT("message"), ServerMessage);
    }

    StatusTextMessage->SetText(FText::FromString(ServerMessage));

    if (Response->GetResponseCode() == 200)
    {
        if (OnResetPasswordSuccess.IsBound())
        {
            OnResetPasswordSuccess.Broadcast(EmailInputText->GetText().ToString());
        }
    }
}