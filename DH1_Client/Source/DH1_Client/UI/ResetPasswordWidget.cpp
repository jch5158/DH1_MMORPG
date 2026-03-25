#include "ResetPasswordWidget.h"
#include "AuthWidgetStyle.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonSerializer.h"

void UResetPasswordWidget::ResetResetPasswordWidget(const FString& StatusText, const FString& Email) const
{
    if (StatusTextMessage)
    {
        StatusTextMessage->SetText(FText::FromString(StatusText));
    }

    if (EmailInputText)
    {
        EmailInputText->SetText(FText::FromString(Email));
    }

    if (VerifyCodeInput)
    {
        VerifyCodeInput->SetText(FText::GetEmpty());
    }

    if (PasswordInput)
    {
        PasswordInput->SetText(FText::GetEmpty());
    }

    if (PasswordConfirmInput)
    {
        PasswordConfirmInput->SetText(FText::GetEmpty());
    }
}

void UResetPasswordWidget::SetStatusTextMessage(const FString& StatusText) const
{
    if (StatusTextMessage)
    {
        StatusTextMessage->SetText(FText::FromString(StatusText));
    }
}

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

    if (PasswordInput)
    {
        PasswordInput->SetIsPassword(true);
    }

    if (PasswordConfirmInput)
    {
        PasswordConfirmInput->SetIsPassword(true);
    }

    AuthWidgetStyle::ApplySecondaryButtonStyle(SendVerifyCodeButton);
    AuthWidgetStyle::ApplyPrimaryButtonStyle(PasswordConfirmButton);
    AuthWidgetStyle::ApplySecondaryButtonStyle(BackToLoginButton);
    AuthWidgetStyle::ApplyStatusTextStyle(StatusTextMessage);
}

void UResetPasswordWidget::OnSendVerifyCodeButtonClicked()
{
    const FString Email = EmailInputText->GetText().ToString();
    if (Email.IsEmpty())
    {
        SetStatusTextMessage(TEXT("이메일 주소에 문제가 발생되었습니다."));
        return;
    }

    const TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField("Email", Email);

    FString JsonString;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

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
    const FString ResetPasswordConfirm = PasswordConfirmInput->GetText().ToString();

    if (Email.IsEmpty() || VerifyCode.IsEmpty() || ResetPassword.IsEmpty() || ResetPasswordConfirm.IsEmpty())
    {
        SetStatusTextMessage(TEXT("모든 항목을 입력해주세요."));
        return;
    }

    if (ResetPassword != ResetPasswordConfirm)
    {
        SetStatusTextMessage(TEXT("새로운 비밀번호가 일치하지 않습니다."));
        return;
    }

    const TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField("Email", Email);
    JsonObject->SetStringField("VerifyCode", VerifyCode);
    JsonObject->SetStringField("ResetPassword", ResetPassword);

    FString JsonString;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    const FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
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
        OnGoToLogin.Broadcast("", "");
    }
}

void UResetPasswordWidget::OnSendCodeResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        SetStatusTextMessage(TEXT("서버와 연결에 실패했습니다."));

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

void UResetPasswordWidget::OnResetPasswordResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        SetStatusTextMessage(TEXT("서버와의 연결에 실패했습니다."));

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