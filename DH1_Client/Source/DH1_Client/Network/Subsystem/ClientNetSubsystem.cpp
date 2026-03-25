#include "ClientNetSubsystem.h"

#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Network/CppNetEngine/NetSession.h"
#include "Network/PacketHandler/PacketServiceTypeHandler.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogNetEngine, Log, All);

void UClientNetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	NetEngineLogger::SetLogCallback([](const eNetLogLevel Level, const char* Message)
	{
		const FString Msg = UTF8_TO_TCHAR(Message);
		switch (Level)
		{
		case eNetLogLevel::Trace:
		case eNetLogLevel::Debug:
			UE_LOG(LogNetEngine, Verbose, TEXT("%s"), *Msg);
			break;
		case eNetLogLevel::Info:
			UE_LOG(LogNetEngine, Log, TEXT("%s"), *Msg);
			break;
		case eNetLogLevel::Warn:
			UE_LOG(LogNetEngine, Warning, TEXT("%s"), *Msg);
			break;
		case eNetLogLevel::Error:
			UE_LOG(LogNetEngine, Error, TEXT("%s"), *Msg);
			break;
		case eNetLogLevel::Fatal:
			UE_LOG(LogNetEngine, Fatal, TEXT("%s"), *Msg);
			break;
		}
	});

	NetEngineConfig engineConfig;
	engineConfig.logger.bIsUnrealClient = true;
	EngineInit = MakeUnique<NetEngineInit>(engineConfig);

	PacketServiceTypeHandler::Init();

	ServiceRef = cpp_net_engine::MakeShared<ClientService>(eServiceType::Client);

	if (LoginServerHost.IsEmpty())
	{
		LoginServerHost = TEXT("localhost");
	}
}

void UClientNetSubsystem::Deinitialize()
{
	ServiceRef.reset();
	EngineInit.Reset();
	Super::Deinitialize();
}

void UClientNetSubsystem::Tick(float DeltaTime)
{
	if (!ServiceRef)
	{
		return;
	}

	ServiceRef->Dispatch();
}

TStatId UClientNetSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UClientNetSubsystem, STATGROUP_Tickables);
}

bool UClientNetSubsystem::IsTickable() const
{
	return ServiceRef != nullptr;
}

void UClientNetSubsystem::RequestLogin(const FString& Email, const FString& Password)
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("Email"), Email);
	JsonObject->SetStringField(TEXT("Password"), Password);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	const FString Scheme = bUseHttps ? TEXT("https") : TEXT("http");
	const FString URL = FString::Printf(TEXT("%s://%s:%d/api/auth/login"), *Scheme, *LoginServerHost, LoginServerPort);
	NET_ENGINE_LOG_INFO("[ClientNetSubsystem] RequestLogin URL: {}", TCHAR_TO_UTF8(*URL));

	Request->SetURL(URL);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(JsonString);

	Request->OnProcessRequestComplete().BindUObject(this, &UClientNetSubsystem::OnLoginResponseReceived);
	Request->ProcessRequest();
}

void UClientNetSubsystem::OnLoginResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bWasSuccessful)
{
	NET_ENGINE_LOG_INFO("[ClientNetSubsystem] OnLoginResponseReceived - bWasSuccessful: {}, Response Valid: {}",
		bWasSuccessful, Response.IsValid());

	if (!bWasSuccessful || !Response.IsValid())
	{
		NET_ENGINE_LOG_ERROR("[ClientNetSubsystem] HTTP request failed - URL: {}, Status: {}",
			TCHAR_TO_UTF8(*Request->GetURL()), static_cast<int32>(Request->GetStatus()));
		OnHttpLoginError.Broadcast(0, TEXT("서버와 연결할 수 없습니다."));
		return;
	}

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

		SetAuthData(Ticket, FString::Printf(TEXT("%lld"), AccountId));
		ConnectToServer(GatewayIp, static_cast<int32>(GatewayPort));
	}
	else
	{
		FString Message = TEXT("서버 처리 중 오류가 발생했습니다.");
		FString Code;
		FString Email;

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
			OnEmailVerificationRequired.Broadcast(Message, Email);
		}
		else
		{
			OnHttpLoginError.Broadcast(ResponseCode, Message);
		}
	}
}

bool UClientNetSubsystem::ConnectToServer(const FString& IPAddress, int32 Port)
{
	if (!ServiceRef)
	{
		return false;
	}

	ClientServiceConfig ClientConfig;
	ClientConfig.maxSessionCount = 1;
	ClientConfig.maxConnectionCount = 1;
	ClientConfig.netAddress = NetAddress(TCHAR_TO_UTF8(*IPAddress), static_cast<uint16>(Port));
	ClientConfig.sessionFactory = [argAuthData = ClientAuthData]() -> NetSessionRef
		{
			return	cpp_net_engine::MakeShared<NetSession>(8192, 4096, argAuthData);
		};

	NetworkSchedulerConfig SchedulerConfig;
	SchedulerConfig.waitTimeoutMs = 16;
	SchedulerConfig.tickIntervalMs = 16;
	SchedulerConfig.runningThreadCount = 0;
	ClientConfig.pNetworkScheduler = cpp_net_engine::MakeShared<NetworkScheduler>(SchedulerConfig);

	if (!ServiceRef->Initialize(ClientConfig) || !ServiceRef->Start())
	{
		return false;
	}

	return true;
}

void UClientNetSubsystem::Disconnect()
{
}

void UClientNetSubsystem::SendPacket(const uint8* PacketData, int32 Size)
{
	if (ServiceRef == nullptr)
	{
		return;
	}

	const SessionRef pSession = ServiceRef->GetFirstSessionRef();
	if (pSession == nullptr)
	{
		return;
	}

	const NetSendBufferRef pSendBuffer = cpp_net_engine::MakeShared<NetSendBuffer>(Size);
	byte* pBuffer = pSendBuffer->Reserve(Size);
	if (pBuffer != nullptr)
	{
		std::copy_n(PacketData, Size, pBuffer);
		pSendBuffer->Commit(Size);
		pSession->Send(pSendBuffer);
	}
}

void UClientNetSubsystem::SetAuthData(const FString& ArgTicket, const FString& ArgAccountId)
{
	ClientAuthData.Ticket = ArgTicket;
	ClientAuthData.AccountId = ArgAccountId;
}

AuthData UClientNetSubsystem::GetAuthData() const
{
	return ClientAuthData;
}

void UClientNetSubsystem::NotifyLoginResult(const int32 Result)
{
	OnGatewayLoginResult.Broadcast(Result);
}
