#include "UI/SignUpWidget.h"

#include "EmailVerificationWidget.h"
#include "HttpModule.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"

#include "Interfaces/IHttpResponse.h"

void USignUpWidget::ResetSignUpWidget() const
{
    if (StatusTextMessage)
    {
        StatusTextMessage->SetText(FText::GetEmpty());
    }

    if (EmailInput)
    {
        EmailInput->SetText(FText::GetEmpty());
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

void USignUpWidget::SetStatusTextMessage(const FString& StatusText) const
{
    if (StatusTextMessage)
    {
        StatusTextMessage->SetText(FText::FromString(StatusText));
    }
}

void USignUpWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SetFocus();

    if (SignUpButton)
    {
        SignUpButton->OnClicked.AddDynamic(this, &USignUpWidget::OnSignUpButtonClicked);
    }

    if (BackLoginButton)
    {
        BackLoginButton->OnClicked.AddDynamic(this, &USignUpWidget::OnBackLoginButtonClicked);
    }
}

void USignUpWidget::OnSignUpButtonClicked()
{
    // 1. UI 텍스트 가져오기
    const FString Email = EmailInput->GetText().ToString();
    const FString Password = PasswordInput->GetText().ToString();
    const FString PasswordConfirm = PasswordConfirmInput->GetText().ToString();

    // 2. 유효성 검사 (빈 값 체크)
    if (Email.IsEmpty() || Password.IsEmpty() || PasswordConfirm.IsEmpty())
    {
        SetStatusTextMessage(TEXT("모든 필드를 입력해주세요."));
        return;
    }

    if (Password != PasswordConfirm)
    {
        SetStatusTextMessage(TEXT("비밀번호가 일치하지 않습니다."));
        return;
    }

    SetStatusTextMessage("");

    const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

    const TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField(TEXT("Email"), Email);
    JsonObject->SetStringField(TEXT("Password"), Password);
    JsonObject->SetStringField(TEXT("PasswordConfirm"), PasswordConfirm);

    FString JsonString;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    Request->SetURL(TEXT("https://localhost:5001/api/auth/register"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(JsonString);

    Request->OnProcessRequestComplete().BindUObject(this, &USignUpWidget::OnSignUpResponseReceived);
    Request->ProcessRequest();
}

void USignUpWidget::OnBackLoginButtonClicked()
{
    if (OnGoToLogin.IsBound())
    {
        OnGoToLogin.Broadcast("", "");
    }
}

void USignUpWidget::OnSignUpResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
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
    if (ResponseCode == 200)
    {
        if (OnGoToEmailVerification.IsBound())
        {
            OnGoToEmailVerification.Broadcast(TEXT("이메일 인증이 필요합니다."), ResEmail);
        }
    }
    else
    {
        SetStatusTextMessage(ResMessage);
    }
}
