#include "LoginWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/Widget.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"

#include "Network/Subsystem/ClientNetSubsystem.h"

#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

void ULoginWidget::ResetLoginWidget(const FString& StatusText, const FString& Email) const
{
	if (StatusTextMessage)
	{
		StatusTextMessage->SetText(FText::FromString(StatusText));
	}

	if (EmailInput)
	{
		EmailInput->SetText(FText::FromString(Email));
	}

	if (PasswordInput)
	{
		PasswordInput->SetText(FText::GetEmpty());
	}
}

void ULoginWidget::SetStatusTextMessage(const FString& StatusText) const
{
	if (StatusTextMessage)
	{
		StatusTextMessage->SetText(FText::FromString(StatusText));
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

	if (ResetPasswordButton)
	{
		ResetPasswordButton->OnClicked.AddDynamic(this, &ULoginWidget::OnResetPasswordButtonClicked);
	}

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (UClientNetSubsystem* NetSubsystem = GameInstance->GetSubsystem<UClientNetSubsystem>())
		{
			NetSubsystem->OnGatewayLoginResult.AddUObject(this, &ULoginWidget::HandleGatewayLoginResult);
		}
	}

	SetLoading(false);
}

void ULoginWidget::NativeDestruct()
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (UClientNetSubsystem* NetSubsystem = GameInstance->GetSubsystem<UClientNetSubsystem>())
		{
			NetSubsystem->OnGatewayLoginResult.RemoveAll(this);
		}
	}

	Super::NativeDestruct();
}

void ULoginWidget::OnLoginButtonClicked()
{
	const FString Email = EmailInput->GetText().ToString();
	const FString Password = PasswordInput->GetText().ToString();

	if (Email.IsEmpty() || Password.IsEmpty())
	{
		SetStatusTextMessage(TEXT("이메일과 비밀번호를 모두 입력해주세요."));
		return;
	}

	if (!Email.Contains(TEXT("@")) || !Email.Contains(TEXT(".")))
	{
		SetStatusTextMessage(TEXT("유효한 이메일 형식이 아닙니다."));
		return;
	}

	SetStatusTextMessage(TEXT(""));
	SetLoading(true);

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

void ULoginWidget::OnSignUpButtonClicked()
{
	// 뷰포트에 직접 위젯을 생성하지 않고 마스터 위젯으로 이벤트만 전달
	if (OnGoToSignUp.IsBound())
	{
		OnGoToSignUp.Broadcast();
	}
}

void ULoginWidget::OnResetPasswordButtonClicked()
{
	if (OnGoToResetPassword.IsBound())
	{
		OnGoToResetPassword.Broadcast();
	}
}

void ULoginWidget::OnLoginResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		SetStatusTextMessage(TEXT("서버와 연결할 수 없습니다."));
		SetLoading(false);
		return;
	}

	FString Message = TEXT("서버 처리 중 오류가 발생했습니다.");
	const int32 ResponseCode = Response->GetResponseCode();
	if (ResponseCode == 200)
	{
		FString Ticket;
		int64 AccountId = 0;
		FString GatewayIp;
		int64 GatewayPort = 0;

		const FString JsonString = Response->GetContentAsString();
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
		TSharedPtr<FJsonObject> JsonObject;
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			JsonObject->TryGetStringField(TEXT("ticket"), Ticket);
			JsonObject->TryGetNumberField(TEXT("accountId"), AccountId);
			JsonObject->TryGetStringField(TEXT("gatewayIp"), GatewayIp);
			JsonObject->TryGetNumberField(TEXT("gatewayPort"), GatewayPort);
		}

		if (const UGameInstance* GameInstance = GetGameInstance())
		{
			if (UClientNetSubsystem* NetSubsystem = GameInstance->GetSubsystem<UClientNetSubsystem>())
			{
				NetSubsystem->SetAuthData(Ticket, FString::Printf(TEXT("%lld"), AccountId));
				NetSubsystem->ConnectToServer(GatewayIp, static_cast<int32>(GatewayPort));
			}
		}
		// 로딩 유지: 게이트웨이 인증(S2C_LOGIN_RES) 완료 후 HandleGatewayLoginResult에서 처리
	}
	else
	{
		FString Code = TEXT("");
		FString Email = TEXT("");

		const FString JsonString = Response->GetContentAsString();
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
		TSharedPtr<FJsonObject> JsonObject;
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			JsonObject->TryGetStringField(TEXT("message"), Message);
			JsonObject->TryGetStringField(TEXT("code"), Code);
			JsonObject->TryGetStringField(TEXT("email"), Email);
		}

		if (ResponseCode == 403 && Code == TEXT("EMAIL_UNVERIFIED"))
		{
			SetLoading(false);
			if (OnGoToEmailVerification.IsBound())
			{
				OnGoToEmailVerification.Broadcast(Message, Email);
			}
		}
		else
		{
			SetLoading(false);
			SetStatusTextMessage(Message);
		}
	}
}

void ULoginWidget::HandleGatewayLoginResult(const int32 Result)
{
	// eLoginResult: 0=SUCCESS, 1=INVALID_TICKET, 2=SERVER_FULL, 3=MAINTENANCE, 4=INTERNAL_ERROR
	if (Result == 0)
	{
		SetLoading(false);
		if (OnLoginSuccess.IsBound())
		{
			OnLoginSuccess.Broadcast();
		}
	}
	else
	{
		SetLoading(false);

		switch (Result)
		{
		case 1:
			SetStatusTextMessage(TEXT("인증 정보가 만료되었습니다. 다시 로그인해주세요."));
			break;
		case 2:
			SetStatusTextMessage(TEXT("서버가 가득 찼습니다. 잠시 후 다시 시도해주세요."));
			break;
		case 3:
			SetStatusTextMessage(TEXT("서버 점검 중입니다. 잠시 후 다시 시도해주세요."));
			break;
		default:
			SetStatusTextMessage(TEXT("서버 오류가 발생했습니다. 잠시 후 다시 시도해주세요."));
			break;
		}
	}
}

void ULoginWidget::SetLoading(const bool bLoading)
{
	if (LoginButton)
	{
		LoginButton->SetIsEnabled(!bLoading);
	}

	if (LoadingOverlay)
	{
		LoadingOverlay->SetVisibility(bLoading ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}
