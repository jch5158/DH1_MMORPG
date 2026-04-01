#include "ClientNetSubsystem.h"

#include "Async/Async.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Styling/SlateTypes.h"
#include "Dom/JsonValue.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Engine/GameViewportClient.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Network/CppNetEngine/NetSession.h"
#include "Network/Dh1StringConv.h"
#include "Network/PacketHandler/ChatPacketHandler.h"
#include "Network/PacketHandler/MovementPacketHandler.h"
#include "Network/PacketHandler/PacketServiceTypeHandler.h"
#include "Network/PacketHandler/RealmPacketHandler.h"
#include "Enum.pb.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Styling/CoreStyle.h"
#include "UI/AuthErrorMapper.h"
#include "UI/AuthWidgetStyle.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Types/SlateEnums.h"

DEFINE_LOG_CATEGORY_STATIC(LogNetEngine, Log, All);

namespace
{
	void ApplyDarkChatComboDropdownStyle(UComboBoxString* Combo)
	{
		if (Combo == nullptr)
		{
			return;
		}

		// 채팅 패널(회색 톤)과 구분: 드롭다운은 남청 계열 + 테두리, 행은 짝/홀 미세 대비
		FComboBoxStyle WidgetStyle = Combo->GetWidgetStyle();

		// 펼친 목록 외곽(메뉴 배경 느낌 + 가장자리 구분)
		WidgetStyle.ComboButtonStyle.MenuBorderBrush.TintColor =
			FSlateColor(FLinearColor(0.06f, 0.09f, 0.16f, 0.99f));
		WidgetStyle.ComboButtonStyle.MenuBorderPadding = FMargin(2.f, 2.f, 2.f, 2.f);

		// 닫힌 상태 채널 버튼 — 로그 영역보다 살짝 푸른 톤
		FButtonStyle& Btn = WidgetStyle.ComboButtonStyle.ButtonStyle;
		const FSlateColor BtnIdle(FLinearColor(0.10f, 0.13f, 0.20f, 0.95f));
		const FSlateColor BtnHover(FLinearColor(0.14f, 0.18f, 0.28f, 0.98f));
		const FSlateColor BtnPress(FLinearColor(0.12f, 0.16f, 0.26f, 1.f));
		Btn.Normal.TintColor = BtnIdle;
		Btn.Hovered.TintColor = BtnHover;
		Btn.Pressed.TintColor = BtnPress;
		Btn.Disabled.TintColor = FSlateColor(FLinearColor(0.08f, 0.08f, 0.10f, 0.7f));

		Combo->SetWidgetStyle(WidgetStyle);

		FTableRowStyle ItemStyle = Combo->GetItemStyle();
		const FSlateColor RowEven(FLinearColor(0.11f, 0.15f, 0.24f, 1.f));
		const FSlateColor RowOdd(FLinearColor(0.09f, 0.13f, 0.21f, 1.f));
		const FSlateColor RowHoverEven(FLinearColor(0.18f, 0.24f, 0.36f, 1.f));
		const FSlateColor RowHoverOdd(FLinearColor(0.16f, 0.22f, 0.34f, 1.f));
		const FSlateColor RowSelected(FLinearColor(0.22f, 0.32f, 0.48f, 1.f));
		ItemStyle.EvenRowBackgroundBrush.TintColor = RowEven;
		ItemStyle.OddRowBackgroundBrush.TintColor = RowOdd;
		ItemStyle.EvenRowBackgroundHoveredBrush.TintColor = RowHoverEven;
		ItemStyle.OddRowBackgroundHoveredBrush.TintColor = RowHoverOdd;
		ItemStyle.ActiveBrush.TintColor = RowSelected;
		ItemStyle.ActiveHoveredBrush.TintColor = RowSelected;
		ItemStyle.InactiveBrush.TintColor = RowSelected;
		ItemStyle.InactiveHoveredBrush.TintColor = RowSelected;
		ItemStyle.TextColor = FSlateColor(FLinearColor::White);
		Combo->SetItemStyle(ItemStyle);
	}
}

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

	constexpr int32 kMaxChatLogLines = 120;

	const TCHAR* ChatChannelTag(const int32 Ch)
	{
		using namespace Protocol;
		switch (Ch)
		{
		case CHAT_CHANNEL_WORLD:
			return TEXT("월드");
		case CHAT_CHANNEL_REALM:
			return TEXT("렐름");
		default:
			return TEXT("일반");
		}
	}

	int32 ChatChannelFromComboIndex(const int32 Idx)
	{
		using namespace Protocol;
		switch (Idx)
		{
		case 1:
			return CHAT_CHANNEL_WORLD;
		case 2:
			return CHAT_CHANNEL_REALM;
		default:
			return CHAT_CHANNEL_LOCAL;
		}
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
	UnregisterChatUi();
	ClearNetworkSpawnedEntities();
	RemoveEnterWorldLoadingFromViewport();
	bClientWorldChatAllowed = false;
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

	for (auto It = NetworkEntityMoveStates.CreateIterator(); It; ++It)
	{
		const TWeakObjectPtr<AActor>* const ActorPtr = NetworkEntityActors.Find(It.Key());
		if (ActorPtr == nullptr || !ActorPtr->IsValid())
		{
			It.RemoveCurrent();
			continue;
		}

		ACharacter* const Ch = Cast<ACharacter>(ActorPtr->Get());
		if (Ch == nullptr)
		{
			continue;
		}

		const FVector Current = Ch->GetActorLocation();
		const FVector Target = It.Value().TargetPosition;
		const FVector Delta = Target - Current;
		const float Dist2D = Delta.Size2D();

		UCharacterMovementComponent* const Mv = Ch->GetCharacterMovement();

		if (Dist2D > 5.f && Mv != nullptr)
		{
			const FVector Dir = Delta.GetSafeNormal2D();
			Ch->AddMovementInput(Dir, 1.f);

			const float DesiredYaw = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
			const float NewYaw = FMath::FInterpTo(Ch->GetActorRotation().Yaw, DesiredYaw, DeltaTime, 10.f);
			Ch->SetActorRotation(FRotator(0.f, NewYaw, 0.f));

			if (Dist2D > 300.f)
			{
				Ch->SetActorLocation(FMath::VInterpTo(Current, Target, DeltaTime, 4.f));
			}
		}
		else if (Mv != nullptr)
		{
			Mv->StopMovementImmediately();
		}
	}
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

	ClearNetworkSpawnedEntities();
	bClientWorldChatAllowed = false;
	return true;
}

void UClientNetSubsystem::Disconnect()
{
	ClearNetworkSpawnedEntities();
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

	bClientWorldChatAllowed = false;

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
			bClientWorldChatAllowed = true;
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
			bClientWorldChatAllowed = true;
			PendingSpawnDisplayName = DisplayName;
			PendingSpawnLevel = Level;
			PendingSpawnCurrentHP = CurrentHP;
			PendingSpawnMaxHP = MaxHP;
			bHasPendingSpawnCharacterSheet = true;
			CachedLocalChatDisplayName = DisplayName;
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

void UClientNetSubsystem::RegisterChatUi(UUserWidget* ChatRoot)
{
	UnregisterChatUi();
	if (ChatRoot == nullptr)
	{
		return;
	}

	UButton* Btn = Cast<UButton>(ChatRoot->GetWidgetFromName(TEXT("Btn_Send")));
	UComboBoxString* Combo = Cast<UComboBoxString>(ChatRoot->GetWidgetFromName(TEXT("Combo_Channel")));
	UEditableTextBox* Edit = Cast<UEditableTextBox>(ChatRoot->GetWidgetFromName(TEXT("EditableText_Message")));
	UScrollBox* Scroll = Cast<UScrollBox>(ChatRoot->GetWidgetFromName(TEXT("Scroll_Log")));
	UVerticalBox* Box = Cast<UVerticalBox>(ChatRoot->GetWidgetFromName(TEXT("VBox_LogLines")));

	if (Btn == nullptr || Combo == nullptr || Edit == nullptr || Scroll == nullptr || Box == nullptr)
	{
		UE_LOG(LogNetEngine, Warning, TEXT("RegisterChatUi: missing named widgets (Btn_Send, Combo_Channel, EditableText_Message, Scroll_Log, VBox_LogLines)"));
		return;
	}

	ChatRootWidget = ChatRoot;
	ChatSendButton = Btn;
	ChatChannelCombo = Combo;
	ChatMessageEdit = Edit;
	ChatScrollLog = Scroll;
	ChatLogLinesBox = Box;

	Combo->ClearOptions();
	Combo->AddOption(TEXT("일반"));
	Combo->AddOption(TEXT("월드"));
	Combo->AddOption(TEXT("렐름"));
	Combo->SetSelectedIndex(0);

	Combo->SetContentPadding(FMargin(6.f, 3.f, 6.f, 3.f));

	{
		FComboBoxStyle WS = Combo->GetWidgetStyle();
		WS.ComboButtonStyle.ButtonStyle.NormalForeground = FSlateColor(FLinearColor::White);
		WS.ComboButtonStyle.ButtonStyle.HoveredForeground = FSlateColor(FLinearColor::White);
		WS.ComboButtonStyle.ButtonStyle.PressedForeground = FSlateColor(FLinearColor::White);
		WS.ComboButtonStyle.ButtonStyle.DisabledForeground = FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.5f));
		Combo->SetWidgetStyle(WS);
	}

	ApplyDarkChatComboDropdownStyle(Combo);

	Combo->OnGenerateWidgetEvent.BindDynamic(this, &UClientNetSubsystem::HandleChatComboGenerateItem);
	Combo->OnOpening.AddDynamic(this, &UClientNetSubsystem::HandleChatChannelComboOpening);

	if (UVerticalBoxSlot* const ScrollSlot = Cast<UVerticalBoxSlot>(Scroll->Slot))
	{
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	Btn->OnClicked.AddDynamic(this, &UClientNetSubsystem::HandleChatSendClicked);

	Edit->OnTextCommitted.AddDynamic(this, &UClientNetSubsystem::HandleChatTextCommitted);
}

void UClientNetSubsystem::UnregisterChatUi()
{
	if (ChatSendButton.IsValid())
	{
		ChatSendButton->OnClicked.RemoveDynamic(this, &UClientNetSubsystem::HandleChatSendClicked);
	}
	if (ChatChannelCombo.IsValid())
	{
		ChatChannelCombo->OnGenerateWidgetEvent.Clear();
		ChatChannelCombo->OnOpening.RemoveDynamic(this, &UClientNetSubsystem::HandleChatChannelComboOpening);
	}
	if (ChatMessageEdit.IsValid())
	{
		ChatMessageEdit->OnTextCommitted.RemoveDynamic(this, &UClientNetSubsystem::HandleChatTextCommitted);
	}

	ChatSendButton = nullptr;
	ChatChannelCombo = nullptr;
	ChatMessageEdit = nullptr;
	ChatScrollLog = nullptr;
	ChatLogLinesBox = nullptr;
	ChatRootWidget = nullptr;
}

bool UClientNetSubsystem::SendChatRequest(const int32 ChannelEnumValue, const FString& Message)
{
	if (!bClientWorldChatAllowed)
	{
		return false;
	}

	if (!ServiceRef)
	{
		return false;
	}

	const SessionRef Session = ServiceRef->GetFirstSessionRef();
	if (Session == nullptr)
	{
		return false;
	}

	FString Trimmed = Message;
	Trimmed.TrimStartAndEndInline();
	if (Trimmed.IsEmpty())
	{
		return false;
	}

	const int32 Ch = FMath::Clamp(ChannelEnumValue, static_cast<int32>(Protocol::CHAT_CHANNEL_LOCAL),
		static_cast<int32>(Protocol::CHAT_CHANNEL_REALM));

	Protocol::C2S_CHAT_REQ Packet;
	Packet.set_channel(static_cast<Protocol::eChatChannel>(Ch));
	Packet.set_message(std::string(TCHAR_TO_UTF8(*Trimmed)));

	Session->Send(ChatPacketHandler::MakeSendBuffer(Packet));
	return true;
}

uint64 UClientNetSubsystem::GetParsedLocalAccountId() const
{
	if (ClientAuthData.AccountId.IsEmpty())
	{
		return 0;
	}
	return FCString::Strtoui64(*ClientAuthData.AccountId, nullptr, 10);
}

void UClientNetSubsystem::NotifyChatMessageReceived(const int32 ChannelEnumValue, const uint64 SenderAccountId,
	const FString& SenderDisplayName, const FString& Message, const int64 ServerTimestampMs)
{
	// 호출부(ChatPacketHandler)에서 게임 스레드로 디스패치함.
	// 로컬 전송 에코와 동일한 본인 S2C 알림은 한 줄만 남기기 위해 제외
	const uint64 LocalAccountId = GetParsedLocalAccountId();
	if (LocalAccountId != 0 && SenderAccountId == LocalAccountId)
	{
		return;
	}
	AppendChatLine(ChannelEnumValue, SenderAccountId, SenderDisplayName, Message, ServerTimestampMs);
}

void UClientNetSubsystem::HandleChatSendClicked()
{
	TrySendChatFromInput();
}

void UClientNetSubsystem::HandleChatChannelComboOpening()
{
	UComboBoxString* Combo = ChatChannelCombo.Get();
	if (Combo == nullptr)
	{
		return;
	}
	const TSharedPtr<SWidget> Cached = Combo->GetCachedWidget();
	if (!Cached.IsValid())
	{
		return;
	}
	const TSharedPtr<SComboBox<TSharedPtr<FString>>> SComboWidget = StaticCastSharedPtr<SComboBox<TSharedPtr<FString>>>(Cached);
	if (SComboWidget.IsValid())
	{
		SComboWidget->SetMenuPlacement(EMenuPlacement::MenuPlacement_CenteredAboveAnchor);
	}
}

UWidget* UClientNetSubsystem::HandleChatComboGenerateItem(FString Item)
{
	UObject* const Outer = ChatChannelCombo.Get() ? static_cast<UObject*>(ChatChannelCombo.Get()) : static_cast<UObject*>(this);
	UTextBlock* const Text = NewObject<UTextBlock>(Outer);
	Text->SetText(FText::FromString(Item));
	FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 12);
	Text->SetFont(FontInfo);
	Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Text->SetMargin(FMargin(6.f, 4.f, 6.f, 4.f));
	return Text;
}

UWorld* UClientNetSubsystem::ResolveGameWorldForSpawning() const
{
	if (UWorld* const W = GetWorld())
	{
		return W;
	}
	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UWorld* const W = GI->GetWorld())
		{
			return W;
		}
	}
	if (GEngine == nullptr)
	{
		return nullptr;
	}
	UGameInstance* const MyGI = GetGameInstance();
	if (MyGI == nullptr)
	{
		return nullptr;
	}
	for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
	{
		if (Ctx.OwningGameInstance != MyGI)
		{
			continue;
		}
		UWorld* const W = Ctx.World();
		if (W == nullptr || !W->IsGameWorld())
		{
			continue;
		}
		const EWorldType::Type WT = Ctx.WorldType;
		if (WT != EWorldType::PIE && WT != EWorldType::Game)
		{
			continue;
		}
		return W;
	}
	return nullptr;
}

void UClientNetSubsystem::ApplyNetworkEntitiesEntered(const TArray<FNetworkEntitySpawnData>& Entities)
{
	UWorld* const PrimaryWorld = GetWorld();
	UWorld* const World = PrimaryWorld != nullptr ? PrimaryWorld : ResolveGameWorldForSpawning();
	if (World == nullptr)
	{
		return;
	}

	static USkeletalMesh* MannequinMesh = nullptr;
	static UClass* AnimBPClass = nullptr;
	if (MannequinMesh == nullptr)
	{
		MannequinMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
		AnimBPClass = LoadObject<UClass>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"));
	}

	for (const FNetworkEntitySpawnData& E : Entities)
	{
		if (E.EntityId == 0)
		{
			continue;
		}

		if (TWeakObjectPtr<AActor>* const Found = NetworkEntityActors.Find(E.EntityId))
		{
			if (Found->IsValid())
			{
				FNetworkEntityMoveState& Ms = NetworkEntityMoveStates.FindOrAdd(E.EntityId);
				Ms.TargetPosition = E.Position;
				Ms.TargetYaw = E.YawDegrees;
				continue;
			}
			NetworkEntityActors.Remove(E.EntityId);
			NetworkEntityMoveStates.Remove(E.EntityId);
		}

		FActorSpawnParameters Sp;
		Sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		ACharacter* const ProxyChar = World->SpawnActor<ACharacter>(ACharacter::StaticClass(), E.Position,
			FRotator(0.f, E.YawDegrees, 0.f), Sp);
		if (ProxyChar == nullptr)
		{
			continue;
		}

#if WITH_EDITOR
		ProxyChar->SetActorLabel(FString::Printf(TEXT("NetEntity_%llu"), E.EntityId));
#endif
		if (USkeletalMeshComponent* const Mc = ProxyChar->GetMesh())
		{
			if (MannequinMesh != nullptr)
			{
				Mc->SetSkeletalMesh(MannequinMesh);
				Mc->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
				Mc->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
			}
			if (AnimBPClass != nullptr)
			{
				Mc->SetAnimInstanceClass(AnimBPClass);
			}
			Mc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		if (UCharacterMovementComponent* const Mv = ProxyChar->GetCharacterMovement())
		{
			Mv->GravityScale = 1.f;
			Mv->SetMovementMode(EMovementMode::MOVE_Walking);
			Mv->bOrientRotationToMovement = false;
			Mv->MaxWalkSpeed = 600.f;
		}

		NetworkEntityActors.Add(E.EntityId, ProxyChar);
		FNetworkEntityMoveState Ms;
		Ms.TargetPosition = E.Position;
		Ms.TargetYaw = E.YawDegrees;
		NetworkEntityMoveStates.Add(E.EntityId, Ms);
	}
}

void UClientNetSubsystem::ApplyNetworkEntitiesLeft(const TArray<uint64>& EntityIds)
{
	for (const uint64 Id : EntityIds)
	{
		if (TWeakObjectPtr<AActor>* const Found = NetworkEntityActors.Find(Id))
		{
			if (Found->IsValid())
			{
				Found->Get()->Destroy();
			}
			NetworkEntityActors.Remove(Id);
		}
		NetworkEntityMoveStates.Remove(Id);
	}
}

void UClientNetSubsystem::ClearNetworkSpawnedEntities()
{
	for (auto It = NetworkEntityActors.CreateIterator(); It; ++It)
	{
		if (It.Value().IsValid())
		{
			It.Value()->Destroy();
		}
	}
	NetworkEntityActors.Empty();
	NetworkEntityMoveStates.Empty();
}

void UClientNetSubsystem::HandleChatTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	(void)Text;
	if (CommitMethod == ETextCommit::OnEnter)
	{
		TrySendChatFromInput();
	}
}

void UClientNetSubsystem::TrySendChatFromInput()
{
	if (!ChatMessageEdit.IsValid() || !ChatChannelCombo.IsValid())
	{
		return;
	}

	FString Msg = ChatMessageEdit->GetText().ToString();
	Msg.TrimStartAndEndInline();
	if (Msg.IsEmpty())
	{
		return;
	}

	if (!bClientWorldChatAllowed)
	{
		UE_LOG(LogNetEngine, Warning, TEXT("채팅은 월드(캐릭터 스폰) 완료 후 전송할 수 있습니다."));
		return;
	}

	const int32 Ch = ChatChannelFromComboIndex(ChatChannelCombo->GetSelectedIndex());
	if (!SendChatRequest(Ch, Msg))
	{
		return;
	}

	const uint64 LocalAccountId = GetParsedLocalAccountId();
	const FString SenderName = CachedLocalChatDisplayName.IsEmpty() ? TEXT("나") : CachedLocalChatDisplayName;
	const int64 TsMs = static_cast<int64>(FDateTime::UtcNow().GetTicks() / ETimespan::TicksPerMillisecond);
	AppendChatLine(Ch, LocalAccountId, SenderName, Msg, TsMs);

	ChatMessageEdit->SetText(FText::GetEmpty());
}

void UClientNetSubsystem::AppendChatLine(const int32 ChannelEnumValue, const uint64 SenderAccountId,
	const FString& SenderDisplayName, const FString& Message, const int64 ServerTimestampMs)
{
	(void)SenderAccountId;
	(void)ServerTimestampMs;

	if (!ChatLogLinesBox.IsValid() || !ChatScrollLog.IsValid() || !ChatRootWidget.IsValid())
	{
		return;
	}

	UVerticalBox* const Box = ChatLogLinesBox.Get();
	UScrollBox* const Scroll = ChatScrollLog.Get();
	UUserWidget* const Root = ChatRootWidget.Get();

	while (Box->GetChildrenCount() >= kMaxChatLogLines)
	{
		if (UWidget* Oldest = Box->GetChildAt(0))
		{
			Box->RemoveChild(Oldest);
		}
		else
		{
			break;
		}
	}

	UTextBlock* Line = NewObject<UTextBlock>(Root);
	const FString DisplayName = SenderDisplayName.IsEmpty() ? TEXT("?") : SenderDisplayName;
	Line->SetText(FText::FromString(FString::Printf(TEXT("[%s] %s: %s"), ChatChannelTag(ChannelEnumValue), *DisplayName,
		*Message)));

	FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 11);
	Line->SetFont(FontInfo);
	Line->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.92f, 1.0f, 1.0f)));
	Line->SetAutoWrapText(true);

	Box->AddChildToVerticalBox(Line);

	Scroll->InvalidateLayoutAndVolatility();
	Scroll->ScrollToEnd();
}
