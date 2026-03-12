#include "DH1LoginWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonSerializer.h"

void UDH1LoginWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (LoginButton)
    {
        LoginButton->OnClicked.AddDynamic(this, &UDH1LoginWidget::OnLoginButtonClicked);
    }

    if (SignUpButton)
    {
        SignUpButton->OnClicked.AddDynamic(this, &UDH1LoginWidget::OnSignUpButtonClicked);
    }
}

void UDH1LoginWidget::OnLoginButtonClicked()
{
    const FString Email = EmailInput->GetText().ToString();
    const FString Password = PasswordInput->GetText().ToString();

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField(TEXT("Email"), Email);
    JsonObject->SetStringField(TEXT("Password"), Password);

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    FHttpModule* Http = &FHttpModule::Get();
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();

    Request->OnProcessRequestComplete().BindUObject(this, &UDH1LoginWidget::OnLoginResponseReceived);
    Request->SetURL("https://localhost:5001/api/Auth/Login"); // C# 서버 주소
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetContentAsString(JsonString);

    Request->ProcessRequest();
}

void UDH1LoginWidget::OnSignUpButtonClicked()
{
    const FString EmailStr = EmailInput->GetText().ToString();
    const FString PasswordStr = PasswordInput->GetText().ToString();

    if (EmailStr.IsEmpty() || PasswordStr.IsEmpty())
    {
        return;
    }

    const TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField(TEXT("Email"), EmailStr);
    JsonObject->SetStringField(TEXT("Password"), PasswordStr);

    FString JsonString;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    FHttpModule* Http = &FHttpModule::Get();
    const TSharedRef<IHttpRequest> Request = Http->CreateRequest();

    Request->SetURL("https://localhost:5001/api/auth/signup");
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetContentAsString(JsonString);

    Request->OnProcessRequestComplete().BindUObject(this, &UDH1LoginWidget::OnSignUpResponseReceived);
    Request->ProcessRequest();
}

void UDH1LoginWidget::OnLoginResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful)
{
    if (bWasSuccessful && Response.IsValid())
    {
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

        if (FJsonSerializer::Deserialize(Reader, JsonObject))
        {
            if (Response->GetResponseCode() == 200)
            {
                const FString Ticket = JsonObject->GetStringField(TEXT("ticket"));
                FString GatewayIp = JsonObject->GetStringField(TEXT("gatewayIp"));
                int32 GatewayPort = JsonObject->GetIntegerField(TEXT("gatewayPort"));

                UE_LOG(LogTemp, Log, TEXT("Login Success! Ticket: %s"), *Ticket);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Login Failed"));
            }
        }
    }
}

void UDH1LoginWidget::OnSignUpResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful)
{
    if (bWasSuccessful && Response.IsValid())
    {
        const int32 ResponseCode = Response->GetResponseCode();
        const FString ResponseContent = Response->GetContentAsString();

        UE_LOG(LogTemp, Log, TEXT("서버 응답 코드: %d"), ResponseCode);
        UE_LOG(LogTemp, Log, TEXT("서버 메시지: %s"), *ResponseContent);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("서버 연결 실패!"));
    }
}
