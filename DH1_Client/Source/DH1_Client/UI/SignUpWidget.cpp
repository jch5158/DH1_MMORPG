#include "UI/SignUpWidget.h"

#include "EmailVerificationWidget.h"
#include "HttpModule.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"

#include "Interfaces/IHttpResponse.h"

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
        if (ErrorTextMessage)
        {
            ErrorTextMessage->SetText(FText::FromString(TEXT("모든 필드를 입력해주세요.")));
        }
        return;
    }

    // 3. 유효성 검사 (비밀번호 일치 체크 - Compare 대신 != 연산자 사용)
    if (Password != PasswordConfirm)
    {
        if (ErrorTextMessage)
        {
            ErrorTextMessage->SetText(FText::FromString(TEXT("비밀번호가 일치하지 않습니다.")));
        }
        return;
    }

    // 4. 에러 텍스트 초기화 및 HTTP 요청 시작
    if (ErrorTextMessage)
    {
        ErrorTextMessage->SetText(FText::GetEmpty());
    }

    const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

    // JSON 페이로드 생성
    const TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField(TEXT("Email"), Email);
    JsonObject->SetStringField(TEXT("Password"), Password);
    JsonObject->SetStringField(TEXT("PasswordConfirm"), PasswordConfirm);

    FString JsonString;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    // 이전에 통일하기로 한 AuthController의 엔드포인트 사용
    Request->SetURL(TEXT("https://localhost:5001/api/auth/register"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(JsonString);

    // 응답 콜백 바인딩 및 전송
    Request->OnProcessRequestComplete().BindUObject(this, &USignUpWidget::OnSignUpResponseReceived);
    Request->ProcessRequest();
}

void USignUpWidget::OnBackLoginButtonClicked()
{
    // 마스터 위젯으로 뒤로가기(로그인 화면 이동) 이벤트 전달
    if (OnGoToLogin.IsBound())
    {
        OnGoToLogin.Broadcast();
    }
}

void USignUpWidget::OnSignUpResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    // 1. 서버 통신 실패 처리
    if (!bWasSuccessful || !Response.IsValid())
    {
        if (ErrorTextMessage)
        {
            ErrorTextMessage->SetText(FText::FromString(TEXT("서버와 연결할 수 없습니다.")));
        }
        return;
    }

    const int32 ResponseCode = Response->GetResponseCode();

    // 2. 회원가입 성공 처리 (200 OK)
    if (ResponseCode == 200)
    {
        const FString TargetEmail = EmailInput->GetText().ToString();

        // 화면 전환은 마스터 위젯이 하도록 이벤트만 발생시킴
        if (OnSignUpSuccess.IsBound())
        {
            OnSignUpSuccess.Broadcast(TargetEmail);
        }
    }
    // 3. 중복 이메일 (409 Conflict)
    else if (ResponseCode == 409)
    {
        if (ErrorTextMessage)
        {
            ErrorTextMessage->SetText(FText::FromString(TEXT("이미 사용중인 이메일입니다.")));
        }
    }
    // 4. 기타 에러
    else
    {
        if (ErrorTextMessage)
        {
            ErrorTextMessage->SetText(FText::FromString(TEXT("알 수 없는 오류가 발생했습니다. 다시 시도해 주세요.")));
        }
    }
}
