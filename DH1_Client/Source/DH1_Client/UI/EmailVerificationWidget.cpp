#include "EmailVerificationWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

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
}

void UEmailVerificationWidget::SetVerificationEmail(const FString& Email)
{
    // 현재 인증을 진행할 이메일 저장
    CurrentEmail = Email;

    // UI 텍스트에 이메일 표시 (예: "user@test.com으로 전송된 인증번호를 입력하세요")
    if (TargetEmailText)
    {
        TargetEmailText->SetText(FText::FromString(CurrentEmail));
    }
}

void UEmailVerificationWidget::OnVerifyButtonClicked()
{
    const FString Code = VerificationCodeInput->GetText().ToString();

    // 1. 클라이언트 측 빈 값 검사
    if (Code.IsEmpty())
    {
        if (ErrorTextMessage)
        {
            ErrorTextMessage->SetText(FText::FromString(TEXT("인증번호를 입력해주세요.")));
        }
        return;
    }

    if (ErrorTextMessage)
    {
        ErrorTextMessage->SetText(FText::GetEmpty());
    }

    // 2. HTTP 요청 생성
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField(TEXT("Email"), CurrentEmail);
    JsonObject->SetStringField(TEXT("VerifyCode"), Code); // C# 서버의 DTO 프로퍼티 이름과 일치해야 합니다.

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    Request->SetURL(TEXT("https://localhost:5001/api/auth/verify-code"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(JsonString);

    Request->OnProcessRequestComplete().BindUObject(this, &UEmailVerificationWidget::OnVerifyResponseReceived);
    Request->ProcessRequest();
}

void UEmailVerificationWidget::OnVerifyResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
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

    if (ResponseCode == 200) // 인증 성공
    {
        const FString TargetEmail = TargetEmailText->GetText().ToString();

        // 마스터 위젯에 인증 성공 이벤트를 날려 로그인 화면으로 이동하게 함
        if (OnVerificationSuccess.IsBound())
        {
            OnVerificationSuccess.Broadcast(TargetEmail);
        }
    }
    else if (ResponseCode == 400 || ResponseCode == 401) // BadRequest 또는 Unauthorized
    {
        if (ErrorTextMessage)
        {
            ErrorTextMessage->SetText(FText::FromString(TEXT("인증번호가 올바르지 않거나 만료되었습니다.")));
        }
    }
    else
    {
        if (ErrorTextMessage)
        {
            ErrorTextMessage->SetText(FText::FromString(TEXT("인증 처리 중 오류가 발생했습니다.")));
        }
    }
}

void UEmailVerificationWidget::OnResendButtonClicked()
{
    if (CurrentEmail.IsEmpty())
    {
	    return;
    }

    if (ErrorTextMessage)
    {
        ErrorTextMessage->SetText(FText::FromString(TEXT("인증번호를 재전송 중입니다...")));
    }

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField(TEXT("Email"), CurrentEmail);

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    // 재전송 API 엔드포인트
    Request->SetURL(TEXT("https://localhost:5001/api/auth/send-verify-code"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(JsonString);

    Request->OnProcessRequestComplete().BindUObject(this, &UEmailVerificationWidget::OnResendResponseReceived);
    Request->ProcessRequest();
}

void UEmailVerificationWidget::OnResendResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        if (ErrorTextMessage)
        {
            ErrorTextMessage->SetText(FText::FromString(TEXT("서버와 연결할 수 없습니다.")));
        }
        return;
    }

    if (Response->GetResponseCode() == 200)
    {
        if (ErrorTextMessage)
        {
            ErrorTextMessage->SetText(FText::FromString(TEXT("인증번호가 재전송되었습니다. 이메일을 확인해주세요.")));
        }
    }
    else
    {
        if (ErrorTextMessage)
        {
            ErrorTextMessage->SetText(FText::FromString(TEXT("재전송에 실패했습니다. 잠시 후 다시 시도해주세요.")));
        }
    }
}