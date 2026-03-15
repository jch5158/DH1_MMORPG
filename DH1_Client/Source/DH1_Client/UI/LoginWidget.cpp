#include "LoginWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

void ULoginWidget::SetEmail(const FString& EmailToSet)
{
    if (EmailInput)
    {
        EmailInput->SetText(FText::FromString(EmailToSet));
    }
}

void ULoginWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (LoginButton)
    {
        LoginButton->OnClicked.AddDynamic(this, &ULoginWidget::OnLoginButtonClicked);
    }

    if (SignUpButton)
    {
        SignUpButton->OnClicked.AddDynamic(this, &ULoginWidget::OnSignUpButtonClicked);
    }
}

void ULoginWidget::OnSignUpButtonClicked()
{
    // 뷰포트에 직접 위젯을 생성하지 않고 마스터 위젯으로 이벤트만 전달
    if (OnGoToSignUp.IsBound())
    {
        OnGoToSignUp.Broadcast();
    }
}

void ULoginWidget::OnLoginButtonClicked()
{
    const FString Email = EmailInput->GetText().ToString();
    const FString Password = PasswordInput->GetText().ToString();

    // 유효성 검사
    if (Email.IsEmpty() || Password.IsEmpty())
    {
        if (ErrorTextMessage)
        {
            ErrorTextMessage->SetText(FText::FromString(TEXT("이메일과 비밀번호를 모두 입력해주세요.")));
        }
        return;
    }

    if (ErrorTextMessage)
    {
        ErrorTextMessage->SetText(FText::GetEmpty());
    }

    // HTTP 요청 생성
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField(TEXT("Email"), Email);
    JsonObject->SetStringField(TEXT("Password"), Password);

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    // AuthController 엔드포인트 적용
    Request->SetURL(TEXT("https://localhost:5001/api/auth/login"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(JsonString);

    Request->OnProcessRequestComplete().BindUObject(this, &ULoginWidget::OnLoginResponseReceived);
    Request->ProcessRequest();
}

void ULoginWidget::OnLoginResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        if (ErrorTextMessage)
        {
            ErrorTextMessage->SetText(FText::FromString(TEXT("서버와 연결할 수 없습니다.")));
        }
        return;
    }

    const int32 ResponseCode = Response->GetResponseCode();

    if (ResponseCode == 200)
    {
        ErrorTextMessage->SetText(FText::FromString(TEXT("로그인 성공!")));

        if (OnLoginSuccess.IsBound())
        {
            OnLoginSuccess.Broadcast();
        }
    }
    else if (ResponseCode == 401 || ResponseCode == 404) // Unauthorized or Not Found
    {
        if (ErrorTextMessage)
        {
            ErrorTextMessage->SetText(FText::FromString(TEXT("이메일 또는 비밀번호가 잘못되었습니다.")));
        }
    }
    else
    {
        if (ErrorTextMessage)
        {
            ErrorTextMessage->SetText(FText::FromString(TEXT("로그인 중 오류가 발생했습니다.")));
        }
    }
}