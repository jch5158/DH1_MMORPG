#include "ClientNetSubsystem.h"

#include "Async/Async.h"
#include "Dom/JsonValue.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Network/CppNetEngine/NetSession.h"
#include "Network/Dh1StringConv.h"
#include "Network/PacketHandler/MovementPacketHandler.h"
#include "Network/PacketHandler/PacketServiceTypeHandler.h"
#include "Network/PacketHandler/RealmPacketHandler.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Styling/CoreStyle.h"
#include "UI/AuthErrorMapper.h"
#include "UI/AuthWidgetStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(LogNetEngine, Log, All);

namespace
{
	FString NormalizeEmail(const FString& InEmail)
	{
		return InEmail.TrimStartAndEnd().ToLower();
	}

	bool IsLikelyValidEmail(const FString& Email)
	{
		const FString Normalized = NormalizeEmail(Email);
		int32 AtIndex = INDEX_NONE;
		if (!Normalized.FindChar(TEXT('@'), AtIndex))
		{
			return false;
		}

		if (AtIndex <= 0 || AtIndex >= Normalized.Len() - 3)
		{
			return false;
		}

		const int32 DotIndex = Normalized.Find(TEXT("."), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		return DotIndex > AtIndex + 1 && DotIndex < Normalized.Len() - 1;
	}

	bool IsValidPasswordLength(const FString& Password)
	{
		const int32 Len = Password.Len();
		return Len >= 8 && Len <= 128;
	}

	FString ExtractMessageFromJson(const TSharedPtr<FJsonObject>& JsonObject)
	{
		if (!JsonObject.IsValid())
		{
			return TEXT("");
		}

		FString Message;
		if (JsonObject->TryGetStringField(TEXT("message"), Message) && !Message.IsEmpty())
		{
			return Message;
		}

		if (JsonObject->TryGetStringField(TEXT("detail"), Message) && !Message.IsEmpty())
		{
			return Message;
		}

		if (JsonObject->TryGetStringField(TEXT("title"), Message) && !Message.IsEmpty())
		{
			return Message;
		}

		const TSharedPtr<FJsonObject>* ErrorsObject = nullptr;
		if (JsonObject->TryGetObjectField(TEXT("errors"), ErrorsObject) && ErrorsObject != nullptr && ErrorsObject->IsValid())
		{
			for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*ErrorsObject)->Values)
			{
				const TArray<TSharedPtr<FJsonValue>>* ErrorsArray = nullptr;
				if (Pair.Value.IsValid() && Pair.Value->TryGetArray(ErrorsArray) && ErrorsArray != nullptr && ErrorsArray->Num() > 0)
				{
					const FString FirstError = (*ErrorsArray)[0].IsValid() ? (*ErrorsArray)[0]->AsString() : TEXT("");
					if (!FirstError.IsEmpty())
					{
						return FirstError;
					}
				}
			}
		}

		return TEXT("");
	}

}

namespace
{
	TSharedPtr<SWidget> GEnterWorldLoadingViewportWidget;

	static void RemoveEnterWorldLoadingFromViewport()
	{
		if (GEnterWorldLoadingViewportWidget.IsValid() && GEngine && GEngine->GameViewport)
		{
			GEngine->GameViewport->RemoveViewportWidgetContent(GEnterWorldLoadingViewportWidget.ToSharedRef());
		}
		GEnterWorldLoadingViewportWidget.Reset();
	}

	static void AddEnterWorldLoadingToViewport()
	{
		if (!GEngine || !GEngine->GameViewport || GEnterWorldLoadingViewportWidget.IsValid())
		{
			return;
		}

		// 로그인과 같은 팔레트만 사용. T_LoginBg 텍스처는 PNG 디코드·CreateTransient·UpdateResource를
		// Slate/뷰포트 초기화와 겹치면 크래시할 수 있어 여기서는 쓰지 않음.
		GEnterWorldLoadingViewportWidget = SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.Padding(FMargin(0))
				.BorderImage(AuthStyle::FlatBrush())
				.BorderBackgroundColor(AuthStyle::C::ScreenBg)
				[
					SNew(SBox)
				]
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("DH1", "EnterWorldLoadingOverlay", "캐릭터 정보를 불러오는 중..."))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 22))
				.ColorAndOpacity(AuthStyle::C::Title)
				.Justification(ETextJustify::Center)
			];
		GEngine->GameViewport->AddViewportWidgetContent(GEnterWorldLoadingViewportWidget.ToSharedRef(), 2147483000);
	}
}

void UClientNetSubsystem::ForEachPlayClientNetSubsystem(TFunction<void(UClientNetSubsystem*)> Fn)
{
	if (!GEngine || !Fn)
	{
		return;
	}

	TSet<UGameInstance*> Seen;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UWorld* World = Context.World();
		if (World == nullptr)
		{
			continue;
		}

		const EWorldType::Type WT = Context.WorldType;
		if (WT != EWorldType::PIE && WT != EWorldType::Game)
		{
			continue;
		}

		if (!World->IsGameWorld())
		{
			continue;
		}

		UGameInstance* GI = Context.OwningGameInstance;
		if (GI == nullptr || Seen.Contains(GI))
		{
			continue;
		}

		if (UClientNetSubsystem* Net = GI->GetSubsystem<UClientNetSubsystem>())
		{
			Seen.Add(GI);
			Fn(Net);
		}
	}
}

void UClientNetSubsystem::ShowEnterWorldLoadingOverlay()
{
	const auto Add = []() { AddEnterWorldLoadingToViewport(); };
	if (!IsInGameThread())
	{
		AsyncTask(ENamedThreads::GameThread, Add);
		return;
	}
	Add();
}

void UClientNetSubsystem::HideEnterWorldLoadingOverlay()
{
	const auto Remove = []() { RemoveEnterWorldLoadingFromViewport(); };
	if (!IsInGameThread())
	{
		AsyncTask(ENamedThreads::GameThread, Remove);
		return;
	}
	Remove();
}

void UClientNetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	NetEngineLogger::SetLogCallback([](const eNetLogLevel Level, const char* Message)
	{
		const FString Msg = Dh1Utf8CStringToFString(Message);
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
	RemoveEnterWorldLoadingFromViewport();
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
	const FString NormalizedEmail = NormalizeEmail(Email);
	if (!IsLikelyValidEmail(NormalizedEmail))
	{
		OnHttpLoginError.Broadcast(400, TEXT("유효한 이메일 형식이 아닙니다."));
		return;
	}
	if (!IsValidPasswordLength(Password))
	{
		OnHttpLoginError.Broadcast(400, TEXT("비밀번호는 8자 이상 128자 이하여야 합니다."));
		return;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("Email"), NormalizedEmail);
	JsonObject->SetStringField(TEXT("Password"), Password);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	const FString URL = FString::Printf(TEXT("%s/login"), *GetLoginServerApiBaseUrl());
	NET_ENGINE_LOG_INFO("[ClientNetSubsystem] RequestLogin URL: {}", TCHAR_TO_UTF8(*URL));

	Request->SetURL(URL);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(JsonString);

	Request->OnProcessRequestComplete().BindUObject(this, &UClientNetSubsystem::OnLoginResponseReceived);
	Request->ProcessRequest();
}

FString UClientNetSubsystem::GetLoginServerApiBaseUrl() const
{
	const FString Scheme = bUseHttps ? TEXT("https") : TEXT("http");
	const FString Host = LoginServerHost.IsEmpty() ? TEXT("localhost") : LoginServerHost;
	return FString::Printf(TEXT("%s://%s:%d/api/auth"), *Scheme, *Host, LoginServerPort);
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
		FString Message = TEXT("");
		FString Code;
		FString Email;

		const FString JsonString = Response->GetContentAsString();
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
		TSharedPtr<FJsonObject> JsonObject;
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			Message = ExtractMessageFromJson(JsonObject);
			JsonObject->TryGetStringField(TEXT("code"), Code);
			JsonObject->TryGetStringField(TEXT("email"), Email);
		}

		AuthErrorMapper::ReportUnknownCodeIfAny(Code, TEXT("LoginResponse"));
		Message = AuthErrorMapper::ResolveMessage(ResponseCode, Code, Message);

		if (ResponseCode == 403 && AuthErrorMapper::IsEmailUnverifiedCode(Code))
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
	UE_LOG(LogNetEngine, Log, TEXT("[ClientNetSubsystem] NotifyLoginResult: %d"), Result);
	AsyncTask(ENamedThreads::GameThread, [this, Result]()
		{
			UE_LOG(LogNetEngine, Log, TEXT("[ClientNetSubsystem] Broadcasting LoginResult: %d, Bound: %d"), Result, OnGatewayLoginResult.IsBound());
			OnGatewayLoginResult.Broadcast(Result);
		});
}

void UClientNetSubsystem::NotifyRealmList(const TArray<FRealmServerInfo>& RealmList)
{
	UE_LOG(LogNetEngine, Log, TEXT("[ClientNetSubsystem] NotifyRealmList: count=%d"), RealmList.Num());
	AsyncTask(ENamedThreads::GameThread, [this, RealmList]()
		{
			UE_LOG(LogNetEngine, Log, TEXT("[ClientNetSubsystem] Broadcasting RealmList: count=%d, Bound: %d"), RealmList.Num(), OnRealmListReceived.IsBound());
			OnRealmListReceived.Broadcast(RealmList);
		});
}

void UClientNetSubsystem::NotifyRealmSelectResult(const int32 Result)
{
	UE_LOG(LogNetEngine, Log, TEXT("[ClientNetSubsystem] NotifyRealmSelectResult: %d"), Result);
	AsyncTask(ENamedThreads::GameThread, [this, Result]()
		{
			if (Result == 0)
			{
				AddEnterWorldLoadingToViewport();
			}
			UE_LOG(LogNetEngine, Log, TEXT("[ClientNetSubsystem] Broadcasting RealmSelectResult: %d, Bound: %d"), Result, OnRealmSelectResult.IsBound());
			OnRealmSelectResult.Broadcast(Result);
		});
}

void UClientNetSubsystem::RequestRealmList()
{
	if (ServiceRef == nullptr)
	{
		return;
	}

	const auto pSession = ServiceRef->GetFirstSessionRef();
	if (pSession == nullptr)
	{
		return;
	}

	Protocol::C2S_REALM_LIST_REQ packet;
	pSession->Send(RealmPacketHandler::MakeSendBuffer(packet));
}

void UClientNetSubsystem::RequestRealmSelect(const int32 RealmId)
{
	if (ServiceRef == nullptr)
	{
		return;
	}

	const auto pSession = ServiceRef->GetFirstSessionRef();
	if (pSession == nullptr)
	{
		return;
	}

	Protocol::C2S_REALM_SELECT_REQ packet;
	packet.set_realmid(RealmId);
	pSession->Send(RealmPacketHandler::MakeSendBuffer(packet));
}


void UClientNetSubsystem::NotifySpawnPosition(const FVector& Position, const float Yaw)
{
	UE_LOG(LogNetEngine, Log, TEXT("[ClientNetSubsystem] NotifySpawnPosition: pos=%s, yaw=%.1f"), *Position.ToString(), Yaw);
	AsyncTask(ENamedThreads::GameThread, [this, Position, Yaw]()
		{
			bHasPendingSpawnCharacterSheet = false;
			UE_LOG(LogNetEngine, Log, TEXT("[ClientNetSubsystem] Broadcasting SpawnPosition, Bound: %d"), OnSpawnPositionReceived.IsBound());
			OnSpawnPositionReceived.Broadcast(Position, Yaw);
		});
}

void UClientNetSubsystem::NotifySpawnPositionWithCharacterSheet(
	const FVector& Position,
	const float Yaw,
	const FString& DisplayName,
	const int32 Level,
	const float CurrentHP,
	const float MaxHP)
{
	UE_LOG(LogNetEngine, Log, TEXT("[ClientNetSubsystem] NotifySpawnPositionWithCharacterSheet: pos=%s, yaw=%.1f, name=%s, Lv=%d, HP=%.1f/%.1f"),
		*Position.ToString(), Yaw, *DisplayName, Level, CurrentHP, MaxHP);
	AsyncTask(ENamedThreads::GameThread, [this, Position, Yaw, DisplayName, Level, CurrentHP, MaxHP]()
		{
			RemoveEnterWorldLoadingFromViewport();
			PendingSpawnDisplayName = DisplayName;
			PendingSpawnLevel = Level;
			PendingSpawnCurrentHP = CurrentHP;
			PendingSpawnMaxHP = MaxHP;
			bHasPendingSpawnCharacterSheet = true;
			OnCharacterOverheadData.Broadcast(DisplayName, Level, CurrentHP, MaxHP);
			OnSpawnPositionReceived.Broadcast(Position, Yaw);
		});
}

bool UClientNetSubsystem::ConsumePendingSpawnCharacterSheet(
	FString& OutDisplayName,
	int32& OutLevel,
	float& OutCurrentHP,
	float& OutMaxHP)
{
	if (!bHasPendingSpawnCharacterSheet)
	{
		return false;
	}

	bHasPendingSpawnCharacterSheet = false;
	OutDisplayName = PendingSpawnDisplayName;
	OutLevel = PendingSpawnLevel;
	OutCurrentHP = PendingSpawnCurrentHP;
	OutMaxHP = PendingSpawnMaxHP;
	return true;
}

void UClientNetSubsystem::RequestSpawnPosition()
{
	if (!ServiceRef) return;

	const auto pSession = ServiceRef->GetFirstSessionRef();
	if (pSession == nullptr) return;

	// BeginPlay 직후에는 GameViewport·Slate 상태가 아직 안정적이지 않을 수 있어 오버레이는 다음 게임 스레드 틱으로 미룸.
	AsyncTask(ENamedThreads::GameThread, []()
	{
		AddEnterWorldLoadingToViewport();
	});

	Protocol::C2S_SPAWN_POSITION_REQ packet;
	pSession->Send(MovementPacketHandler::MakeSendBuffer(packet));

	UE_LOG(LogNetEngine, Warning, TEXT("RequestSpawnPosition sent"));
}

void UClientNetSubsystem::SendMoveToPosition(const FVector& CurrentPosition, const FVector& Destination)
{
	if (!ServiceRef) return;

	const auto pSession = ServiceRef->GetFirstSessionRef();
	if (pSession == nullptr) return;

	++MoveSequenceId;

	Protocol::C2S_MOVE_TO_POSITION_REQ packet;
	packet.set_sequenceid(MoveSequenceId);
	packet.mutable_destination()->set_x(static_cast<float>(Destination.X));
	packet.mutable_destination()->set_y(static_cast<float>(Destination.Y));
	packet.mutable_destination()->set_z(static_cast<float>(Destination.Z));
	packet.set_clienttimestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count());
	packet.mutable_currentposition()->set_x(static_cast<float>(CurrentPosition.X));
	packet.mutable_currentposition()->set_y(static_cast<float>(CurrentPosition.Y));
	packet.mutable_currentposition()->set_z(static_cast<float>(CurrentPosition.Z));

	pSession->Send(MovementPacketHandler::MakeSendBuffer(packet));

	UE_LOG(LogNetEngine, Log, TEXT("SendMoveToPosition - seq: %u, from: %s, dest: %s"),
		MoveSequenceId, *CurrentPosition.ToString(), *Destination.ToString());
}

void UClientNetSubsystem::NotifyMovePath(const uint32 SeqId, const TArray<FVector>& Waypoints, const float MoveSpeed)
{
	UE_LOG(LogNetEngine, Log, TEXT("[ClientNetSubsystem] NotifyMovePath: seq=%u, waypoints=%d"), SeqId, Waypoints.Num());
	AsyncTask(ENamedThreads::GameThread, [this, SeqId, Waypoints, MoveSpeed]()
		{
			OnMovePathReceived.Broadcast(SeqId, Waypoints, MoveSpeed);
		});
}

void UClientNetSubsystem::NotifyPositionCorrection(const FVector& CorrectedPosition)
{
	UE_LOG(LogNetEngine, Log, TEXT("[ClientNetSubsystem] NotifyPositionCorrection: %s"), *CorrectedPosition.ToString());
	AsyncTask(ENamedThreads::GameThread, [this, CorrectedPosition]()
		{
			OnPositionCorrection.Broadcast(CorrectedPosition);
		});
}

void UClientNetSubsystem::NotifyCharacterOverheadData(const FString& DisplayName, const int32 Level, const float CurrentHP, const float MaxHP)
{
	AsyncTask(ENamedThreads::GameThread, [this, DisplayName, Level, CurrentHP, MaxHP]()
		{
			OnCharacterOverheadData.Broadcast(DisplayName, Level, CurrentHP, MaxHP);
		});
}
